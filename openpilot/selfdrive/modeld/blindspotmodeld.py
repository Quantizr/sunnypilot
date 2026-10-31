#!/usr/bin/env python3
"""Blind spot detection from the driver (cabin) camera.

The cabin camera sees the side windows and the rear window, so a car in either blind spot is visible to it. A small model
(MobileNetV2 on a 448x128 band across the cabin + a GRU, ~17 ms per step on a comma 3X) runs at 5 Hz:
  - each step warps the newest camera frame to the model band (nearest-neighbour, yuv420 planes, like modeld's warp)
  - the input is the last three warped frames (t - 0.4 s, t - 0.2 s, t), oldest first
  - a GRU state is carried from step to step (it starts from zero, as in training)
Publishes blindSpotModelSP with per-vehicle-side probabilities and an on/off detection per side (see Detector). Left is the
vehicle's left: the cabin camera faces backwards, so that side is on the image's right, but the model was trained with
labels from the cars' own blind spot radars, so its outputs are already per vehicle side.
Runs onroad while "Show Blind Spot Warnings" (BlindSpot param) is on.
"""
import time
import pickle
import numpy as np
from tinygrad.tensor import Tensor

from openpilot.cereal import messaging
from openpilot.cereal.messaging import PubMaster
from openpilot.cereal.visionipc import VisionStreamType
from msgq.visionipc import VisionIpcClient, VisionBuf
from openpilot.common.swaglog import cloudlog
from openpilot.selfdrive.modeld.helpers import MODELS_DIR, load_oob
from openpilot.system.camerad.cameras.nv12_info import get_nv12_info

MODEL_PKL_PATH = MODELS_DIR / 'bsm_model_tinygrad.pkl'
MODEL_W, MODEL_H = 448, 128
N_FRAMES = 3
FRAME_STEP = 4  # cabin camera runs at 20 fps, the model at 5 Hz
DT_FRAME = 0.05
# binary detection: (probability threshold, minimum on-time after the last step at or above it), longest hold wins
DETECT_HOLDS = ((0.5, 1.0), (0.3, 0.5))

# training geometry: a 964x604 canvas (the 1928x1208 f=567 cabin image at half resolution; the 1344x760 f=425.25 image is
# scaled to the same focal length and centred), cropped to columns 34-930 and canvas rows 144-400 (the model band)
CANVAS_W, CANVAS_H = 964, 604
BAND_X0, BAND_Y0, BAND_SCALE = 34, 144, 2  # canvas pixels per model pixel


def canvas_geometry(cam_w: int, cam_h: int) -> tuple[int, int, int, int]:
  """size and offset of the camera image inside the canvas (bsm_exp bsm/nvdec.py canvas_geometry)"""
  s = CANVAS_W / 1928 if (cam_w, cam_h) == (1928, 1208) else (567.0 / 425.25) * CANVAS_W / 1928
  tw, th = min(2 * round(cam_w * s / 2), CANVAS_W), min(2 * round(cam_h * s / 2), CANVAS_H)
  return tw, th, 2 * ((CANVAS_W - tw) // 4), 2 * ((CANVAS_H - th) // 4)


def model_to_camera(cam_w: int, cam_h: int) -> np.ndarray:
  """3x3 transform from model-band pixel (x, y) to camera pixel (the warp's M_inv), pixel-centre aligned"""
  tw, th, x0, y0 = canvas_geometry(cam_w, cam_h)
  sx, sy = cam_w / tw, cam_h / th
  # model pixel centre -> canvas: BAND + SCALE * (x + 0.5) - 0.5; canvas -> camera: (c - offset + 0.5) * s - 0.5
  ax, bx = BAND_SCALE * sx, (BAND_X0 + 0.5 * BAND_SCALE - x0) * sx - 0.5
  ay, by = BAND_SCALE * sy, (BAND_Y0 + 0.5 * BAND_SCALE - y0) * sy - 0.5
  return np.array([[ax, 0, bx], [0, ay, by], [0, 0, 1]], dtype=np.float32)


class Detector:
  """per-side on/off signal: on at p >= 0.3, held 0.5 s after the last step >= 0.3 and 1 s after the last step >= 0.5.
  Holds don't stack: the one ending latest wins."""
  def __init__(self):
    self.on_until = -1.0

  def update(self, p: float, t: float) -> bool:
    for thresh, hold in DETECT_HOLDS:
      if p >= thresh:
        self.on_until = max(self.on_until, t + hold)
    return t < self.on_until - 1e-6


class ModelState:
  """warp + model, both precompiled: no tinygrad kernels are built at runtime. The frame history and the GRU state live on
  the host (uploads per step, like modeld's packed inputs): the compiled model's GRU state output can have a different dtype
  than its input (f32 out, f16 in on QCOM), so it's cast in numpy."""
  def __init__(self, cam_w: int, cam_h: int):
    jits = load_oob(MODEL_PKL_PATH)
    self.DEV = jits['input_specs']['frames'][2]
    self.model_run = jits['run']
    self.outputs = {name: Tensor(np.zeros(shape, dtype=dtype), device=device).realize()
                    for name, (shape, dtype, device) in jits['output_specs'].items()}
    h_shape, h_dtype, _ = jits['input_specs']['h_gru']
    self.h_np = np.zeros(h_shape, dtype=h_dtype)
    self.h_host = Tensor(self.h_np, device='NPY')._buffer()
    self.h_gru = Tensor(self.h_np, device=self.DEV).realize()
    with open(MODELS_DIR / f'bsm_warp_{cam_w}x{cam_h}_tinygrad.pkl', "rb") as f:
      self.image_warp = pickle.load(f)['run']
    self.frame_size = get_nv12_info(cam_w, cam_h)[3]
    self.transform = Tensor(model_to_camera(cam_w, cam_h), device='NPY')
    self._blob_cache: dict[int, Tensor] = {}
    # model input: the last N_FRAMES warped frames (6 yuv420 planes of 64 x 224 each), oldest first
    self.frames_np = np.zeros((1, 6 * N_FRAMES, MODEL_H // 2, MODEL_W // 2), dtype=np.uint8)
    self.frames_host = Tensor(self.frames_np, device='NPY')._buffer()
    self.frames = Tensor(self.frames_np, device=self.DEV).realize()
    self.n_seen = 0

  def reset(self):
    self.n_seen = 0
    self.h_np[:] = 0

  def run(self, buf: VisionBuf) -> tuple[np.ndarray, float]:
    t1 = time.perf_counter()
    ptr = np.frombuffer(buf.data, dtype=np.uint8).ctypes.data
    # camerad uses a ring of buffers: cache one tensor per buffer
    if ptr not in self._blob_cache:
      self._blob_cache[ptr] = Tensor.from_blob(ptr, (self.frame_size,), dtype='uint8', device=self.DEV)
    warped = self.image_warp(input_frame=self._blob_cache[ptr], M_inv=self.transform).numpy().reshape(6, MODEL_H // 2, MODEL_W // 2)
    if self.n_seen == 0:  # at the start, repeat the first frame (as in training)
      self.frames_np[0] = np.tile(warped, (N_FRAMES, 1, 1))
    else:
      self.frames_np[0, :-6] = self.frames_np[0, 6:]
      self.frames_np[0, -6:] = warped
    self.n_seen += 1
    self.frames._buffer().copy_from(self.frames_host)
    self.h_gru._buffer().copy_from(self.h_host)
    self.model_run(output_buffers=self.outputs, frames=self.frames, h_gru=self.h_gru)
    probs = self.outputs['probs'].numpy().astype(np.float32).reshape(-1)
    self.h_np[:] = self.outputs['h_gru_out'].numpy()
    return probs, time.perf_counter() - t1


def main():
  cloudlog.warning("blindspotmodeld: connecting to cabin stream")
  vipc_client = VisionIpcClient("camerad", VisionStreamType.VISION_STREAM_CABIN, True)
  while not vipc_client.connect(False):
    time.sleep(0.1)
  cloudlog.warning(f"blindspotmodeld: connected, {vipc_client.width}x{vipc_client.height}")

  model = ModelState(vipc_client.width, vipc_client.height)
  pm = PubMaster(["blindSpotModelSP"])
  last_frame_id = None
  detectors = [Detector(), Detector()]

  while True:
    buf = vipc_client.recv()
    if buf is None:
      continue
    frame_id = vipc_client.frame_id
    if frame_id % FRAME_STEP != 0:
      continue
    # a gap in the stream (camera restart, dropped frames): the frame stack and GRU state are stale
    if last_frame_id is not None and frame_id - last_frame_id != FRAME_STEP:
      model.reset()
      detectors = [Detector(), Detector()]
    last_frame_id = frame_id

    t1 = time.perf_counter()
    probs, gpu_time = model.run(buf)
    msg = messaging.new_message('blindSpotModelSP', valid=True)
    bs = msg.blindSpotModelSP
    bs.frameId = frame_id
    # model outputs: [vehicle left, vehicle right, adjacent L, adjacent R, approaching L, approaching R]
    bs.leftProb, bs.rightProb = float(probs[0]), float(probs[1])
    bs.leftAdjacentProb, bs.rightAdjacentProb = float(probs[2]), float(probs[3])
    bs.leftApproachingProb, bs.rightApproachingProb = float(probs[4]), float(probs[5])
    t = frame_id * DT_FRAME
    bs.leftDetected = detectors[0].update(bs.leftProb, t)
    bs.rightDetected = detectors[1].update(bs.rightProb, t)
    bs.modelExecutionTime = time.perf_counter() - t1
    bs.gpuExecutionTime = gpu_time
    pm.send("blindSpotModelSP", msg)


if __name__ == "__main__":
  try:
    main()
  except KeyboardInterrupt:
    cloudlog.warning("got SIGINT")
