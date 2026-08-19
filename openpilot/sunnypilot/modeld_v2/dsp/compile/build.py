"""Builds the megakernel: int8 ONNX -> a Hexagon .so plus the op list, weight blob and layout it runs.

megakernel.c is fixed and model-independent; everything model-specific is data produced here. The pipeline is
parse_onnx (graph and fusions) -> codegen (op records, weights, arena) -> this module, which cross-compiles the
kernel and packages the result. build() is pure codegen and never runs the model.

    HOST:   python3 build.py build <onnx> <out.pkl>   ONNX -> op list, weights and compiled .so, pickled
    DEVICE: python3 build.py verify <onnx|pkl>        Runs it and checks cosine against the OnnxRunner golden

`build()` is what SConscript/compile_modeld.py call, and it is PURE CODEGEN -- it never runs the model and never
computes a golden. `verify` is the accuracy check, and for a never-seen ONNX it is the only one there is: the
kernel registry's fast-or-fail covers SHAPE support (no kernel matches => build error), but a fusion rule that
matches a subgraph it shouldn't, a residency plan that hands a producer and consumer different layouts, or a
quant scale wired to the wrong tensor all build cleanly and return a wrong number. Run it on anything new.

Note that the Hexagon clang is not deterministic: identical source produces differing instruction scheduling
between runs. Compare kernel changes by output (cosine against the golden), never by hashing the .so.
"""
import hashlib
import os
import sys
import pathlib
import platform
import tempfile
import numpy as np

from tinygrad.helpers import Context, diskcache_get, diskcache_put, getenv, system
from openpilot.common.basedir import BASEDIR
from openpilot.sunnypilot.modeld_v2.dsp.megakernel import MegakernelLayout
from openpilot.sunnypilot.modeld_v2.dsp.compile import lower, parse

HERE = pathlib.Path(__file__).parent

_ASM_FILES = (
  "gvconv2dbbb_circ_d32_v65_h.S", "gvconv2dbbb_circ_d64_v65_h.S", "repstream2_h.S",
  "to_d32_h.S", "from_d32_h.S",
  "dwconv2dbbb_s1_3x3_h.S", "dwconv2dbbb_s1_5xN_h.S", "dwconv2dbbb_s1_7xN_h.S",
  "dwconv2dbbb_s2_3x3_h.S", "dwconv2dbbb_s2_5xN_h.S")

def compile_megakernel(layout: MegakernelLayout, tune_defines: list[str] | None = None) -> tuple[str, bytes]:
  """Cross-compile megakernel.c (+ the vendored HVX asm) to a Hexagon .so; returns (src_with_defines, lib_bytes).
  Runs on host or device -- clang targeting hexagon, no DSP needed. HAP_power.h comes from tinygrad's stock HAP
  headers rather than a vendored copy; ld.lld is vendored in lib/ because the comma device ships clang but no lld."""
  src = (HERE / "megakernel.c").read_text()
  inc = os.path.join(BASEDIR, "tinygrad_repo/extra/dsp/include")
  ld = HERE / "lib" / "ld.lld"
  link = f"--ld-path={ld}" if (ld.exists() and platform.machine() == "aarch64") else "-fuse-ld=lld"
  flags = f"-shared --target=hexagon -mcpu=hexagonv65 {link} -nostdlib -mhvx=v65 -mhvx-length=128b -O2"
  args = f"{flags} -fno-stack-protector -fPIC -ffreestanding -I{inc} -I{HERE} " + " ".join(
    layout.defines() + lower.opcode_defines() + (tune_defines or []))
  if getenv("PROF_OPS"):
    args += " -DPROF_OPS=1"
  if os.getenv("EXTRA_DEFS"):
    args += " " + os.getenv("EXTRA_DEFS")
  _asm_paths = [HERE / "lib" / f for f in _ASM_FILES]
  _h = hashlib.sha256((HERE / "megakernel.c").read_bytes())
  for _a in _asm_paths: _h.update(pathlib.Path(_a).read_bytes())
  _h.update(args.encode())
  _ck = _h.hexdigest()[:32]
  _hit = diskcache_get("dsp_megakernel_so", _ck)
  if _hit is not None:
    return src, _hit
  secs = ['text', 'rela.plt', 'rela.dyn', 'plt', 'rodata', 'data', 'bss', 'hash', 'dynamic', 'got', 'got.plt',
          'dynsym', 'dynstr', 'symtab', 'shstrtab', 'strtab']
  ls = ("SECTIONS { . = 0x0; " + "\n".join(f".{n} : ALIGN(4096) {{ *(.{n}) }}" for n in secs) +
        "\n /DISCARD/ : { *(.note .note.* .gnu.hash .comment) } }")
  with tempfile.NamedTemporaryFile(suffix=".c", delete=False) as cf:
    cf.write(src.encode())
  with tempfile.NamedTemporaryFile(suffix=".ld", delete=False) as lf:
    lf.write(ls.encode())
  with tempfile.NamedTemporaryFile(suffix=".so", delete=False) as of:
    pass
  asm = " ".join(str(a) for a in _asm_paths)
  system(f"{getenv('CC', 'clang')} {args} -T{lf.name} {cf.name} {asm} -o {of.name}")
  lib = pathlib.Path(of.name).read_bytes()
  diskcache_put("dsp_megakernel_so", _ck, lib)
  return src, lib

def _ru(x, m): return (x + m - 1) // m * m

def scratch_layout(ops):
  """128-aligned offsets of the 3 conv-scratch regions packed into one buffer, sized to the max over every conv
  (they are reused op to op). The SIZES come from the op records -- codegen emitted them from geometry.conv_geom --
  rather than being re-derived here from Cin/Ho/Win/Hp, which is what used to make this a third copy of the conv
  geometry. The kernel reads the offsets back from the op-stream header.

  Was SIX regions until the V60 conv path was deleted (2026-07-24): `suma` (the per-pixel activation-zp correction)
  and the `ds`/`II` integral images behind it exist only for V60's gvconv. V65 folds that correction into biasbuf
  via gemsumb, so it needs none of them.

  ★ `d32in` is not only conv's: setail_d32 (gqtab), se_gate (atab/acc/xqu/fc1u) and the head (gap/mcp/acc/qu) all
  carve out of it, because it is dead between ops. Every op DECLARES what it carves in `lower.SCRATCH` -- a field
  every record has at the same index -- so this is one max over the op list rather than a per-kind table that has
  now twice fallen out of sync with the kernel and corrupted memory."""
  d32in = max((int(r[lower.SCRATCH]) for r in ops), default=0)
  d32out = max((int(r[lower.F("CV", "sz_d32out")]) for r in ops if r[0] == lower.OP_CONV), default=0)
  sizes = dict(d32in=int(d32in), d32out=int(d32out), minmax=64 * 4)
  off, cur = {}, 0
  for name in ("d32in", "d32out", "minmax"):
    off[name] = cur
    cur = _ru(cur + sizes[name] + 8192, 128)
  return off, cur

def _lifetime_offsets(p, prog):
  """Assign arena offsets by TENSOR LIFETIME instead of bump-allocating, so a tensor's space is reused once its
  last reader has run. The bump allocator never reused anything -- MobileNetV2's arena was the SUM of all 69
  tensors (13.47MB). Uses tinygrad's TLSFAllocator (runtime/support/memory.py), the same suballocator its own
  memory planner uses, driven by first/last-use indices from the op list.

  Scope, honestly: this is a FOOTPRINT win, not a speed one -- we are not DDR-capacity-bound. Buffers that must
  outlive the schedule (the seed, the output, the shared scratch regions) are pinned by never being freed."""
  from tinygrad.runtime.support.memory import TLSFAllocator
  ops, sizes = p["op_list"], p["alloc_sizes"]
  live = {n: [len(ops), -1] for n in sizes}
  for i, o in enumerate(ops):
    for key in lower._ROLES + ("out",):
      nm = getattr(o, key, None)
      if isinstance(nm, str) and nm in live:
        live[nm][0] = min(live[nm][0], i)
        live[nm][1] = max(live[nm][1], i)
  for n in sizes:
    if live[n][1] < 0: live[n] = [0, len(ops)]
  al = TLSFAllocator(sum(sizes.values()) * 2, block_size=128)
  offs, freeing = {}, {}
  for i in range(len(ops) + 1):
    for n in freeing.pop(i, []): al.free(offs[n])
    for n in sizes:
      if live[n][0] == i and n not in offs:
        offs[n] = al.alloc(sizes[n], 128)
        freeing.setdefault(live[n][1] + 1, []).append(n)
  return offs

def ensure_mult32_seed(onnx_path, mult=32):
  """★ A GRAPH INPUT WHOSE CHANNEL COUNT IS NOT A MULTIPLE OF 32 IS SILENTLY WRONG. Zero-pad it here.

  Measured 2026-08-16 (compile/probe_stem_zp.py): Cin=32 reads cosine 1.0000, but Cin=24/16/12 read
  0.7998 / 0.7603 / 0.8194 -- and the error is periodic with period 32 in the output WIDTH (the HVX vector
  width), uniform across rows and channels, i.e. lanes gathering from the wrong input columns.

  CAUSE: `ConvGeom` (lower.py:296) and the weight packer (lower.py:506) both round the input channel count
  up with `_ru(Cin, 32)` UNCONDITIONALLY, so the kernel always reads round_up(Cin,32) channels -- while the
  seed buffer is sized from the real ONNX shape (`seed_bytes = np.prod(sshape)` below). The conv then reads
  a 32-channel layout out of a 12-channel buffer.
  ⚠ Fixing `_pad32` does NOT work and was tried: it changed the op records but left the device output
  bit-identical, because those two `_ru` sites are downstream of it.

  THE FIX, applied to the ONNX so the GOLDEN AND THE DEVICE SEE THE SAME MODEL: zero-extend the first
  conv's weights over the new channels and widen the graph input. Mathematically an identity -- the new
  channels carry zero weights, so they contribute 0 whatever they hold, and the activation-zp correction
  `bias - zp*Sigma_w` sums those zeros unchanged. Validated: Cin=12 probe 0.8194 -> 1.0000000, the v21 dual
  stem 0.9623 -> 0.9999920, shared stem 0.9663 -> 0.9999927.

  ⚠ THE CALLER MUST NOW FEED mult-32 CHANNELS. That is a real contract change and it costs input bandwidth
  (12->32 is 2.67x the bytes over FastRPC), which is exactly why OP_INCONV exists for small Cin. Padding is
  the CORRECT GENERAL fix; INCONV is the fast path.
  """
  from tinygrad.nn.onnx import OnnxPBParser
  _g = OnnxPBParser(pathlib.Path(onnx_path)).parse()["graph"]
  _produced = {o for n in _g["node"] for o in n["output"]}
  _shape = next(i for i in _g["input"] if i["name"] not in _produced)["parsed_type"].shape
  if len(_shape) < 2 or not isinstance(_shape[1], int) or _shape[1] <= 0:
    return onnx_path
  Cin = _shape[1]
  Cp = (Cin + mult - 1) // mult * mult
  if Cp == Cin:
    return onnx_path

  import onnx as _onnx
  from onnx import numpy_helper as _nh
  m = _onnx.load(onnx_path)
  g = m.graph
  dims = g.input[0].type.tensor_type.shape.dim
  gin = g.input[0].name
  dq = next((n for n in g.node if n.op_type == "DequantizeLinear" and gin in n.input), None)
  conv = next((n for n in g.node if dq is not None and dq.output[0] in n.input and n.op_type == "Conv"), None)
  if conv is None:
    return onnx_path
  wdq = next((n for n in g.node if n.output and n.output[0] == conv.input[1]), None)
  wname = wdq.input[0] if wdq is not None else conv.input[1]
  init = {i.name: i for i in g.initializer}
  if wname not in init:
    return onnx_path
  w = _nh.to_array(init[wname])
  if w.shape[1] != Cin:
    return onnx_path
  attr = {a.name: a for a in conv.attribute}
  ks = list(attr["kernel_shape"].ints) if "kernel_shape" in attr else []
  st = list(attr["strides"].ints) if "strides" in attr else [1, 1]
  grp = attr["group"].i if "group" in attr else 1
  if grp == 1 and Cin <= 4 and ks == [3, 3] and st == [2, 2] and w.shape[0] <= 32:
    return onnx_path
  wp = np.zeros((w.shape[0], Cp, w.shape[2], w.shape[3]), w.dtype)
  wp[:, :Cin] = w
  init[wname].CopyFrom(_nh.from_array(wp, wname))
  dims[1].dim_value = Cp
  out = str(pathlib.Path(tempfile.gettempdir()) / (pathlib.Path(onnx_path).stem + f"_seed{Cp}.onnx"))
  _onnx.save(m, out)
  print(f"seed channels {Cin} -> {Cp} (zero-padded; the runner reads round_up(Cin,32) either way) -> {out}",
        flush=True)
  return out

def build(onnx):
  """HOST or DEVICE: ONNX -> (lib, src, layout, data). The backbone takes the u8 seed directly (the stem's graph
  quantizes it), so nothing here has to describe the seed quant to the caller. Pure codegen -- the model is never
  run: the arena comes from lower.arena_shapes. For the golden, see reference.golden (test only)."""
  onnx = ensure_mult32_seed(onnx)
  prog = parse.extract(onnx)
  p = lower.build_program(prog)
  if not getenv("NO_ARENA_REUSE"):
    p = lower.build_program(prog, _preassigned=_lifetime_offsets(p, prog))
  ops = np.asarray(p["ops"]).reshape(-1, lower.INTS_PER_OP)
  seed_off, sshape = p["seed"]
  out_off, out_sz = p["out"]
  soff, scratch_bytes = scratch_layout(ops)
  out_bytes = int(out_sz) * 4 if p["out_f32"] else (int(out_sz) + 3) // 4 * 4
  layout = MegakernelLayout(seed_off=int(seed_off), seed_bytes=int(np.prod(sshape)),
                            out_off=int(out_off), out_bytes=out_bytes, nops=len(ops),
                            d32in=soff["d32in"], d32out=soff["d32out"],
                            minmax_off=soff["minmax"], arena_size=int(p["arena_size"]),
                            scratch_bytes=scratch_bytes)
  src, lib = compile_megakernel(layout)
  hdr = np.zeros((1, lower.INTS_PER_OP), np.int32)
  hdr[0, :len(layout.header())] = layout.header()
  data = dict(wts=np.frombuffer(p["wts"], np.uint8).copy(),
              ops=np.concatenate([hdr, ops]).reshape(-1).astype(np.int32))
  return lib, src, layout, data

def _golden_cached(onnx: str, prog, seed_u8):
  """The oracle is BY FAR the slowest thing here -- MEASURED 209.6s for distill (817 nodes) -- and it depends on
  NOTHING in compile/: only the model, the seed, and tinygrad's OnnxRunner.

  ★ WHAT IS CACHED IS EVERY OP OUTPUT, NOT THE ONE GOLDEN, and the key deliberately does NOT include MK_TRUNC.
  Truncating the op list changes WHICH TENSOR the golden is taken from, not the arena it comes out of -- so
  keying on MK_TRUNC made every new truncation pay the full 209.6s, which is what made per-op debugging
  unusable in practice (a five-point bisection cost 17 minutes of pure oracle). Caching the op outputs instead
  costs a few MB and makes every truncation after the first one free.

  (An earlier version of this docstring claimed OnnxRunner takes `limit=N` to stop at node N, which would be the
  cheaper fix. It does not, in the pinned tinygrad: the signature is __call__(inputs, debug=0).)

  The key hashes the ONNX bytes, the seed, AND tinygrad's onnx.py, so a submodule bump correctly invalidates a
  cached oracle answer instead of letting a stale golden silently outlive the change. That last term is the whole
  reason this is safe to cache: the golden is the GATE, and a gate that goes stale without saying so is worse
  than a slow one."""
  import inspect
  import tinygrad.nn.onnx as _tgonnx
  h = hashlib.sha256(pathlib.Path(onnx).read_bytes())
  h.update(seed_u8.tobytes())
  h.update(pathlib.Path(_tgonnx.__file__).read_bytes())
  h.update(inspect.getsource(golden).encode())
  k = h.hexdigest()[:32]
  outs = diskcache_get("dsp_golden_ops", k)
  if outs is None:
    arena = onnx_arena(prog, seed_u8)
    outs = {o.out: arena[o.out] for o in parse.emit(prog)[0] if isinstance(o.out, str) and o.out in arena}
    diskcache_put("dsp_golden_ops", k, outs)
  v = outs[_oplist(prog)[-1].out]
  return nhwc(v[0] if v.ndim == 4 else v).reshape(-1).astype(np.float32)

def build_to_pkl(onnx, path):
  """Build + bake a random seed and its numpy golden, so `run`/`jit` below can check the device against numpy.
  The golden is for THIS CLI only -- a scons build calls build() and never runs the replica."""
  import pickle
  import dataclasses
  onnx = ensure_mult32_seed(onnx)
  lib, src, layout, data = build(onnx)
  prog = parse.extract(onnx)
  seed = np.random.default_rng(42).integers(0, 256, prog["seed_shape"], dtype=np.uint8)
  emb = _golden_cached(onnx, prog, seed)
  data = dict(data, seed=nhwc(seed).astype(np.uint8))
  with open(path, "wb") as f:
    pickle.dump(dict(lib=lib, src=src, layout=dataclasses.asdict(layout), data=data, emb=emb), f)
  print(f"built {path}: lib {len(lib)} B, {layout.nops} ops, arena {layout.arena_size/1e6:.2f}MB, scratch {layout.scratch_bytes/1e6:.2f}MB")

def verify_pkl(path, iters=30):
  """DEVICE: run the backbone and check it against the golden baked into the pkl by `build`.

  Merges what were two entry points, `run` (one invoke + cosine) and `jit` (30 invokes + a wall-clock median).
  There was never a reason for both: the JIT'd replay is the only honest per-frame number -- a single realize
  includes schedule and compile -- and it produces the cosine anyway. Timing is the ON-DSP median
  (Device["DSP"].last_kernel_us), NOT the wall clock the old `jit` printed: the wall carries host-side scheduling
  noise that swamps a 0.1 ms effect, which is exactly the kind of difference this is used to judge.

  A ~0.9999 cosine on a real model is EXPECTED and is not a failure -- for a QDQ int8 model there is no single
  true float answer (literal Q/DQ vs fused int8 kernels, accumulation order and requant rounding are all
  spec-legal). What matters is that it does not MOVE. Synthetic single-kernel graphs read 1.0000000, which is
  what validates a kernel that has no recorded history."""
  import pickle, statistics
  from tinygrad import Tensor, Device
  from tinygrad.engine.jit import TinyJit
  from openpilot.sunnypilot.modeld_v2.dsp.megakernel import MegakernelLayout, megakernel
  d = pickle.load(open(path, "rb"))
  layout, data, emb = MegakernelLayout(**d["layout"]), d["data"], np.asarray(d["emb"], np.float64)
  dev = lambda a, dt: Tensor(a.astype(dt), device="DSP").realize()
  wts, oplist = dev(data["wts"], np.uint8), dev(data["ops"], np.int32)
  seed = dev(data["seed"].reshape(-1), np.uint8)

  @TinyJit
  def f(s):
    return megakernel(s, wts, oplist, d["lib"], d["src"], layout).realize()

  for _ in range(4):
    f(seed)
    Device["DSP"].synchronize()
  us = []
  for _ in range(iters):
    out = f(seed)
    Device["DSP"].synchronize()
    us.append(Device["DSP"].last_kernel_us)
  raw = out.numpy()
  got = np.asarray(raw.view(np.uint8) if len(emb) > raw.size else raw, np.float64).ravel()[:len(emb)]
  cos = float(got @ emb / (np.linalg.norm(got) * np.linalg.norm(emb) + 1e-12))
  print(f"cosine vs OnnxRunner golden {cos:.7f} | on-DSP median {statistics.median(us) / 1e3:.3f} ms (N={iters})")
  return cos

"""The golden the device is checked against: the ORIGINAL QDQ ONNX, run by tinygrad's OnnxRunner on CPU.

    golden(prog, seed_u8)      The expected feature for a seed. build.py bakes it into the pkl; verify.py
                               compares the device against it.
    onnx_arena(prog, seed_u8)  Every intermediate tensor by name -- independent of our IR, fusions, layouts and
                               quantization code, because it reads the model rather than our op list.

WHY THIS AND NOT A NUMPY REPLICA OF OUR OWN KERNELS. There was one (`reference_numpy.py`, 156 lines
re-implementing every op with our exact fixed-point arithmetic), kept because it was "exact by construction" --
probes read 1.0000000, so any deviation was a real bug, and that tightness caught the 5x5 depthwise bug.

MEASURED 2026-07-25, and it retired the replica: on ALL SIX synthetic probes the replica and this oracle agree to
**1.000000000**, and the device reads 1.0000000 against either. The probes are what validate a NEW kernel, where
there is no recorded baseline -- exactly the case the replica was being kept for -- and it turns out this oracle
is just as exact there. The two only diverge on deep models (MNv2 0.99984 between them), where accumulated
fixed-point-vs-float differences show; and those have baselines, so the gate is "cosine unchanged", which works
against either golden.

The replica was also LESS independent, not more: it read the same op dataclasses lower.py reads, so an emitter
bug -- a wrong scale wired into an op record -- was invisible to it. This oracle reads the ONNX and shares
nothing with our pipeline.

Note for a QDQ int8 model there is still no single "true" float answer (literal Q/DQ vs fused int8 kernels,
accumulation order and requant rounding are all spec-legal), so a full-model cosine of ~0.9999 is EXPECTED and is
not a bug. The gate is that it does not MOVE."""

def _oplist(prog):
  """The op list exactly as build_program sees it, including MK_TRUNC truncation. The last op's `out` names the
  tensor the device will produce, which is what the golden must be taken from."""
  ops = parse.emit(prog)[0]
  if os.environ.get("MK_TRUNC"):
    ops = ops[:int(os.environ["MK_TRUNC"])]
  return ops

_ORACLE_CACHE: dict = {}

def onnx_arena(prog, seed_u8):
  """The ORIGINAL QDQ ONNX under tinygrad's OnnxRunner on CPU -> {tensor name: numpy}, every intermediate.

  Every op we emit is named for the ONNX tensor it produces, so `arena[op["out"]]` works for truncated builds too;
  OnnxRunner also takes `limit=N` to stop at node N, which is MK_TRUNC's semantics if we ever want it. CPU rather
  than the host default (NV) so the result is deterministic and driver-independent."""
  st = pathlib.Path(prog["path"]).stat()
  key = (prog["path"], st.st_mtime_ns, st.st_size, seed_u8.tobytes())
  if key not in _ORACLE_CACHE:
    from tinygrad import Tensor
    from tinygrad.helpers import Context
    from tinygrad.nn.onnx import OnnxRunner
    with Context(DEV="CPU"):
      r = OnnxRunner(prog["path"])
      name, spec = next(iter(r.graph_inputs.items()))
      if [d for d in spec.shape if isinstance(d, int) and d <= 0]:
        raise ValueError(f"{prog['path']}: graph input {name} has a non-positive dim {spec.shape} -- a dynamic "
                         f"batch exported as dim_value 0. Fix the model (set it to 1); do not paper over it.")
      r({name: Tensor(seed_u8.reshape([1] + list(prog["seed_shape"])))})
    _ORACLE_CACHE.clear()
    _ORACLE_CACHE[key] = {k: v.numpy() for k, v in r.graph_values.items() if v is not None and hasattr(v, "numpy")}
  return _ORACLE_CACHE[key]

def golden(prog, seed_u8):
  """The expected feature for a given u8 seed. Uncached -- callers want _golden_cached."""
  v = onnx_arena(prog, seed_u8)[_oplist(prog)[-1].out]
  return nhwc(v[0] if v.ndim == 4 else v).reshape(-1).astype(np.float32)

def nhwc(v):
  """spatial [C,H,W] -> [H,W,C] (the on-device arena layout); scalars/vectors pass through."""
  return v.transpose(1, 2, 0).copy() if v.ndim == 3 else v

if __name__ == "__main__":
  cmd = sys.argv[1] if len(sys.argv) > 1 else ""
  if cmd == "build" and len(sys.argv) >= 4:
    build_to_pkl(os.path.abspath(sys.argv[2]), sys.argv[3])
  elif cmd == "verify" and len(sys.argv) >= 3:
    src = sys.argv[2]
    if src.endswith(".onnx"):
      import tempfile
      pkl = tempfile.NamedTemporaryFile(suffix=".pkl", delete=False).name
      build_to_pkl(os.path.abspath(src), pkl)
      src = pkl
    verify_pkl(src)
  else:
    print("usage: build.py build <onnx> <out.pkl>   |   build.py verify <onnx|pkl>   (verify runs ON THE DEVICE)")
