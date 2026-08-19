"""The fused DSP vision backbone, run as one tinygrad Tensor.custom_kernel on Device["DSP"].

Runtime half of the megakernel: the layout contract and the custom_kernel call. `compile/` produces what this
runs -- the Hexagon .so, the op list and the weight blob -- from an int8 ONNX, and holds the kernel C itself.

    lib, src, layout, data = compile.build.build(onnx)                # build time
    feature = megakernel(seed_t, wts_t, oplist_t, lib, src, layout)   # runtime

The kernel is realized from the binary, which carries its own entry point, so tinygrad renders and compiles no
DSP code for it: the whole backbone is one prebuilt .so behind a single FastRPC invoke.
"""
from __future__ import annotations
import functools
from dataclasses import dataclass
from tinygrad import Tensor, dtypes
from tinygrad.uop.ops import UOp, Ops, KernelInfo
from tinygrad.renderer import Estimates

CORNER = 6

@dataclass(frozen=True)
class MegakernelLayout:
  """Per-model sizes. These size the HOST-side buffers; they are NOT baked into the .so -- the kernel reads the
  same values from a header record at the front of the op stream, so one .so runs any model. Only CORNER (a DCVS
  policy, not a model property) is still a compile-time define."""
  seed_off: int
  seed_bytes: int
  out_off: int
  out_bytes: int
  nops: int
  d32in: int
  d32out: int
  minmax_off: int
  arena_size: int
  scratch_bytes: int

  def defines(self) -> list[str]:
    return [f"-DCORNER={CORNER}"]

  def header(self) -> list[int]:
    """The per-model header record prepended to the op stream. Order MUST match megakernel.c's HDR_* enum."""
    return [self.nops, self.seed_off, self.seed_bytes, self.out_off, self.out_bytes,
            self.d32in, self.d32out, self.minmax_off]

@functools.cache
def _scratch(dev:str, layout:MegakernelLayout) -> tuple[Tensor, Tensor, Tensor]:
  """The kernel's own buffers (feature out, activation arena, conv scratch), allocated once per (device, model).
  They hold nothing across frames, but a fresh set per call would leave the old ones as holes in the DSP arena,
  whose high-water mark the entry maps on every invoke.

  +128 on the arena is one HVX vector of slack for usum_u8's vmux remainder, which reads up to 127 bytes past a
  channel group (see megakernel.c) -- the kernel's own requirement, so the DSP allocator stays stock."""
  return (Tensor.empty(layout.out_bytes // 4, dtype=dtypes.float32, device=dev),
          Tensor.empty(layout.arena_size + 128, dtype=dtypes.uint8, device=dev),
          Tensor.empty(layout.scratch_bytes, dtype=dtypes.uint8, device=dev))

def megakernel(seed: Tensor, wts: Tensor, oplist: Tensor, lib: bytes, src: str, layout: MegakernelLayout) -> Tensor:
  """Run the whole backbone as ONE Tensor.custom_kernel on Device["DSP"]. Returns the feature Tensor.
  seed: u8 input (from the GPU stem). wts/oplist: resident u8/int32 buffers from the op-list compiler."""
  out, arena, scratch = _scratch(seed.device, layout)

  def fxn(out_ph, seed_ph, arena_ph, wts_ph, ops_ph, scratch_ph):
    sink = UOp.sink(out_ph.base, seed_ph.base, arena_ph.base, wts_ph.base, ops_ph.base, scratch_ph.base,
                    arg=KernelInfo("interp_kernel", estimates=Estimates()))
    return UOp(Ops.PROGRAM, src=(sink, UOp(Ops.LINEAR, src=(*sink.src, sink)),
                                 UOp(Ops.SOURCE, arg=src), UOp(Ops.BINARY, arg=lib)))

  out, *_ = Tensor.custom_kernel(out, seed, arena, wts, oplist, scratch, fxn=fxn)
  return out
