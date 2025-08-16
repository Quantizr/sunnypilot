#!/usr/bin/env python3
import socket, struct, time, pickle, numpy as np
import onnxruntime as ort
from pathlib import Path
from openpilot.selfdrive.modeld.constants import ModelConstants
from openpilot.selfdrive.modeld.parse_model_outputs import Parser

HOST = "0.0.0.0"      # listen on all interfaces
PORT = 55001
BUF_SIZE = 1 << 20    # 1 MiB per recv() call

# paths to model files (now using .onnx instead of .pkl)
VISION_ONNX_PATH = Path(__file__).parent / 'models/driving_vision.onnx'
VISION_METADATA_PATH = Path(__file__).parent / 'models/driving_vision_metadata.pkl'
POLICY_ONNX_PATH = Path(__file__).parent / 'models/driving_policy.onnx'
POLICY_METADATA_PATH = Path(__file__).parent / 'models/driving_policy_metadata.pkl'

# load metadata and models once at startup
with open(VISION_METADATA_PATH, 'rb') as f:
  vision_metadata = pickle.load(f)
  vision_input_shapes = vision_metadata['input_shapes']
  vision_input_names = list(vision_input_shapes.keys())  # order matters for reconstruction!
  vision_output_size = vision_metadata['output_shapes']['outputs'][1]

with open(POLICY_METADATA_PATH, 'rb') as f:
  policy_metadata = pickle.load(f)
  policy_output_size = policy_metadata['output_shapes']['outputs'][1]

# Initialize ONNX Runtime sessions with GPU support
print("Loading ONNX models...")
providers = ['CUDAExecutionProvider', 'CPUExecutionProvider']
sess_options = ort.SessionOptions()
sess_options.graph_optimization_level = ort.GraphOptimizationLevel.ORT_ENABLE_ALL

# Load vision model
vision_session = ort.InferenceSession(str(VISION_ONNX_PATH), sess_options, providers=providers)
vision_input_names_ort = [inp.name for inp in vision_session.get_inputs()]
vision_output_names_ort = [out.name for out in vision_session.get_outputs()]

# Load policy model
policy_session = ort.InferenceSession(str(POLICY_ONNX_PATH), sess_options, providers=providers)
policy_input_names_ort = [inp.name for inp in policy_session.get_inputs()]
policy_output_names_ort = [out.name for out in policy_session.get_outputs()]

parser = Parser()

# rolling history buffers (match your local shapes)
# note: now reset per connection
def reset_buffers():
  full_features_buffer = np.zeros((1, ModelConstants.FULL_HISTORY_BUFFER_LEN, ModelConstants.FEATURE_LEN), dtype=np.float32)
  full_desire = np.zeros((1, ModelConstants.FULL_HISTORY_BUFFER_LEN, ModelConstants.DESIRE_LEN), dtype=np.float32)
  return full_features_buffer, full_desire

temporal_idxs = slice(-1-(ModelConstants.TEMPORAL_SKIP*(ModelConstants.INPUT_HISTORY_BUFFER_LEN-1)), None, ModelConstants.TEMPORAL_SKIP)

print(f"remote-vision-policy-server ready on {HOST}:{PORT} (vout size: {vision_output_size}, pout size: {policy_output_size})")

with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as srv:
  srv.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
  srv.bind((HOST, PORT))
  srv.listen()

  while True:
    conn, addr = srv.accept()
    print(f"Connection from {addr}")
    conn.settimeout(5)  # 5s timeout for reads
    conn.setsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY, 1)

    # reset histories on new connection
    full_features_buffer, full_desire = reset_buffers()

    # ---- MODIFICATION: Initialize variables for statistics ----
    last_stat_time = time.perf_counter()
    request_count = 0
    total_compute_us = 0
    total_bytes_in = 0
    total_bytes_out = 0
    # ---- END MODIFICATION ----

    while True:
      try:
        # 1) read 8-byte header: 4-byte length + 4-byte seq
        raw_header = conn.recv(8)
        if len(raw_header) < 8:
          if len(raw_header) == 0:
            print("Client disconnected cleanly.")
            break  # clean disconnect
          raise ValueError("short header")

        (length, seq) = struct.unpack("!II", raw_header)

        # 2) read full payload
        data = bytearray(length)
        view = memoryview(data)
        to_recv = length
        while to_recv:
          n = conn.recv_into(view[length - to_recv:], min(BUF_SIZE, to_recv))
          if n == 0:
            raise ConnectionError("peer closed early")
          to_recv -= n

        # 3) reconstruct vision_inputs as numpy arrays (for ONNX Runtime)
        vision_inputs = {}
        offset = 0
        for name in vision_input_names:
          shape = vision_input_shapes[name]
          size = np.prod(shape)
          buf = data[offset:offset + size]
          arr = np.frombuffer(buf, dtype=np.uint8).reshape(shape)
          vision_inputs[name] = arr  # Keep as uint8 for ONNX
          offset += size

        # 4) parse policy extras (48 B total)
        new_desire = np.frombuffer(data[offset:offset + 32], dtype=np.float32).reshape(ModelConstants.DESIRE_LEN)
        offset += 32
        traffic_convention = np.frombuffer(data[offset:offset + 8], dtype=np.float32).reshape((1, ModelConstants.TRAFFIC_CONVENTION_LEN))
        offset += 8
        lateral_control_params = np.frombuffer(data[offset:offset + 8], dtype=np.float32).reshape((1, ModelConstants.LATERAL_CONTROL_PARAMS_LEN))

        # 5) run vision model + update features history
        ts_compute_start = time.perf_counter_ns()

        # Run ONNX vision model
        vision_outputs = vision_session.run(vision_output_names_ort, vision_inputs)
        vision_output = vision_outputs[0].astype(np.float32)  # Convert to float32

        # FIXED: Use correct slicing syntax
        vision_outputs_dict = parser.parse_vision_outputs({k: vision_output[:, v] for k, v in vision_metadata['output_slices'].items()})

        full_features_buffer[0, :-1] = full_features_buffer[0, 1:]
        full_features_buffer[0, -1] = vision_outputs_dict['hidden_state'][0, :]

        # 6) update desire history + compute max-pooled input
        full_desire[0, :-1] = full_desire[0, 1:]
        full_desire[0, -1] = new_desire
        desire_input = full_desire.reshape((1, ModelConstants.INPUT_HISTORY_BUFFER_LEN, ModelConstants.TEMPORAL_SKIP, -1)).max(axis=2)

        # 7) assemble policy inputs (zero curv as in your code)
        policy_inputs_dict = {
          'desire': desire_input.astype(np.float16),
          'traffic_convention': traffic_convention.astype(np.float16),
          'lateral_control_params': lateral_control_params.astype(np.float16),
          'prev_desired_curv': np.zeros((1, ModelConstants.INPUT_HISTORY_BUFFER_LEN, ModelConstants.PREV_DESIRED_CURV_LEN), dtype=np.float16),
          'features_buffer': full_features_buffer[0, temporal_idxs][np.newaxis].astype(np.float16),
        }

        # 8) run policy model
        policy_outputs = policy_session.run(policy_output_names_ort, policy_inputs_dict)
        policy_output = policy_outputs[0].astype(np.float32)  # Convert to float32

        ts_compute_end = time.perf_counter_ns()
        compute_us = (ts_compute_end - ts_compute_start) / 1_000  # in µs

        # 9) send back: 4-byte len + 4-byte seq + vout bytes + pout bytes + 8-byte float time
        out_bytes = vision_output.tobytes() + policy_output.tobytes()
        out_prefix = struct.pack("!II", len(out_bytes), seq)
        time_bytes = struct.pack("!d", compute_us)  # double-precision float

        conn.sendall(out_prefix)
        conn.sendall(out_bytes)
        conn.sendall(time_bytes)

        request_count += 1
        total_compute_us += compute_us
        total_bytes_in += length + 8  # payload + header
        total_bytes_out += len(out_bytes) + len(out_prefix) + len(time_bytes)

        current_time = time.perf_counter()
        elapsed = current_time - last_stat_time
        if elapsed >= 2.0:
          fps = request_count / elapsed
          avg_compute_ms = (total_compute_us / request_count) / 1000 if request_count > 0 else 0
          in_mib_s = (total_bytes_in / elapsed) / (1 << 20)
          out_mib_s = (total_bytes_out / elapsed) / (1 << 20)

          print(f"stats: {fps:5.1f} fps | avg_compute: {avg_compute_ms:5.1f} ms | in: {in_mib_s:4.2f} MiB/s | out: {out_mib_s:4.2f} MiB/s")

          # Reset stats for the next interval
          last_stat_time = current_time
          request_count = 0
          total_compute_us = 0
          total_bytes_in = 0
          total_bytes_out = 0

      except (OSError, socket.timeout, ValueError, ConnectionError) as e:
        print(f"Connection error: {e}")
        break  # close and wait for new accept
