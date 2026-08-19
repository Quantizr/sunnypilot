"""The op list -> what the device actually receives: int32 op records, a resident weight blob, and the arena.

    arena_shapes(ops, ...)   every tensor's shape/dtype in CLOSED FORM, so a build never runs the model
    build_program(prog)      records + weight blob + arena/scratch sizes
    MK_RESIDENCY_DUMP=1      per-op layout decisions (the diagnostic, not a gate)

Everything between the op list and the device lives here, because none of it has any other consumer:

    the RECORD ABI     one field list per op kind -> `pack()` and megakernel.c's -D field offsets
    geometry           shapes and buffer sizes, derived EXACTLY ONCE (see the section header)
    the kernel REGISTRY  which HVX kernel runs an op, or a build error naming why none does
    quantization       float ONNX tensors -> the exact bytes each kernel reads
    residency          which tensors stay in d32 layout, and which depthwises get a padded buffer
    lowering           arena offsets, the weight blob, the op records

They were six files. Each was imported by exactly one other module -- this one -- which is the definition of
"part of it" rather than "a dependency of it".
"""
from __future__ import annotations

import os
import dataclasses
from dataclasses import dataclass, field

import numpy as np

from openpilot.sunnypilot.modeld_v2.dsp.compile.parse import emit, seed_quant

INTS_PER_OP = 48
OP_CONV, OP_SE_GATE, OP_SETAIL, OP_HEAD = 1, 2, 3, 4
OP_DWCONV, OP_ADD, OP_INCONV = 5, 6, 8
OP_PACK = 11

FIELDS: dict[str, list[str]] = {
  "DW": ["op", "out", "src", "filt", "bias", "d32in", "d32out", "C", "H", "W", "kh", "kw", "fz", "recip", "rsh",
         "aux", "s", "ind", "outd", "ind_bord", "in_lpad", "xzp", "_r22", "_r23",
         "Cp", "pad", "ofw", "padL", "Wp", "Hp", "oH", "oW", "oLp", "ils", "owt", "src_wop", "tile", "threads", "kern"],
  "ADD": ["op", "out", "a", "b", "ra", "rb", "S", "za", "zb", "zo", "qmax", "n"],
  "SE":  ["op", "gate", "expand", "blob", "fc1w", "fc2w", "lut", "Cexp", "HW", "Csq", "d32", "eH", "eW", "eWop"],
  "ST":  ["op", "out", "conv", "res", "gate", "blob", "C", "HW", "zc", "zr", "zo", "relu", "hasres", "has_gate",
          "d32", "H", "W", "Wop", "outd32"],
  "HD":  ["op", "out", "conv", "res", "gate", "blob", "gw", "C", "HW", "zcc", "zr", "hasres", "O", "has_gate",
          "d32", "H", "W", "Wop", "relu"],
  "PK":  ["op", "out", "src", "W", "H", "Cigp", "Wop"],
  "IC":  ["op", "out", "src", "wvec", "bias", "recip", "Cin", "H", "W", "Cout", "k", "stride", "pad", "zsh",
          "Ho", "Wo", "cp4", "xzp"],
  "CV":  ["op", "out", "src", "wt", "bias", "recip",
          "Cin", "H", "W", "Cout", "kh", "kw", "sh", "sw", "ph", "pw", "groups", "xzp", "zshift",
          "Ho", "Wo", "Wop", "Win", "Hp", "w_gcstride",
          "in_d32", "out_d32", "circ", "bord_Wp", "bord_base", "in_left_skip",
          "Cigp", "Cogp", "tot", "sz_d32in", "sz_d32out"],
}
CV_P0 = 6

SCRATCH = INTS_PER_OP - 1

def field_defines() -> list[str]:
  d = [f"-D{pfx}_{nm}={i}" for pfx, names in FIELDS.items() for i, nm in enumerate(names) if not nm.startswith("_")]
  d += [f"-DCV_P0={CV_P0}"]
  d += [f"-DP_{nm}={i - CV_P0}" for i, nm in enumerate(FIELDS["CV"]) if i >= CV_P0 and not nm.startswith("_")]
  return d

def scr(*sizes: int) -> int:
  """Total bytes of the shared scratch a kernel carves, given its sub-buffer sizes IN THE ORDER IT TAKES THEM.
  megakernel.c carves with SCR_TAKE, which 128-aligns each one (HVX aligned loads), so this rounds identically."""
  return sum(_ru(int(n), 128) for n in sizes)

def pack(pfx: str, *geoms, scratch: int = 0, **fields) -> list[int]:
  """One op record, built from NAMED fields -- the schema above is the only thing that knows an index.

  `geoms` are dataclasses (DwGeom / ConvGeom / DwSched) whose field names are deliberately the SAME as the
  schema's, so the derived-geometry tail of a record needs no per-field list at all; anything they carry that the
  schema does not name (nbytes_in, circ, ...) is simply not a record field and is dropped. Explicit `fields` win
  over `geoms`, which is what lets the bordered-project case override Win.

  This replaces a hybrid of `rec[:6] = [...]`, `rec[6:6+len(params)] = params` and `rec[F(pfx,name)] = ...` -- and
  with it the positional writes that were the last of P2: `params[16] = bp[0]` (index 16 of an unnamed list) is now
  `Win=bp[0]`, and build.py's `r[35]`/`r[37]` are `r[F("CV","sz_d32in")]`."""
  names = FIELDS[pfx]
  assert len(names) <= SCRATCH, f"{pfx}: schema reaches the reserved SCRATCH slot {SCRATCH}"
  d: dict = {}
  for g in geoms:
    d.update({k: v for k, v in dataclasses.asdict(g).items() if k in names})
  d.update(fields)
  if unknown := set(d) - set(names):
    raise KeyError(f"{pfx}: no such record field(s) {sorted(unknown)} -- schema is {names}")
  rec = [0] * INTS_PER_OP
  for k, v in d.items():
    rec[names.index(k)] = int(v)
  rec[SCRATCH] = int(scratch)
  return rec

def F(pfx: str, name: str) -> int:
  """Index of a named field, for the emitter. The C side reads the same index via the -D above."""
  return FIELDS[pfx].index(name)

def opcode_defines() -> list[str]:
  """The op-record contract, handed to megakernel.c as -D so the C and this encoder cannot drift."""
  return [f"-DINTS_PER_OP={INTS_PER_OP}", f"-DOP_CONV={OP_CONV}", f"-DOP_SE_GATE={OP_SE_GATE}",
          f"-DOP_SETAIL={OP_SETAIL}", f"-DOP_HEAD={OP_HEAD}", f"-DOP_DWCONV={OP_DWCONV}", f"-DOP_ADD={OP_ADD}",
          f"-DOP_INCONV={OP_INCONV}",
          f"-DOP_PACK={OP_PACK}", f"-DWP_PIL={WP_PIL}"] + field_defines()

def _ru(x, m):
  return (x + m - 1) // m * m

WP_PIL = 448
STEM_MAX_W = WP_PIL - 64

def _pad32(ops):
  """Pad every conv's Cin/Cout to mult-32 (zero-filled) so ALL activation tensors are mult-32 -> full d32 residency
  + fast pack/unpack. Pad channels carry zero weights -> contribute 0; the real output (final Gemm, unpadded) is
  unchanged. Build-side only.

  ★ MEASURED 2026-07-26, because this pass BUYS speed by ADDING WORK and that trade had never been quantified.
  On MobileNetV2 it pads 9 of 51 ops (16->32, 24->32, 144->160) for **+15.5% MACs** (288.7M -> 333.4M), and it is
  worth it by a mile:

      with _pad32     5.49 ms          without    22.55 ms   (+308.7%, 4.1x)   cosine identical

  because the padding is what makes d32 RESIDENCY reachable. Unpadded, 6 convs and 2 depthwises fall back to NHWC
  and the whole pack/from_d32 roundtrip returns. distill is unaffected (already mult-32 everywhere).

  ⇒ This is also why megakernel.c has no scalar d32->NHWC unpack any more. The alternative to a non-mult-32
  shape is not "unpack it slowly", it is "pad it and use from_d32_asm", and that alternative is ALWAYS available
  since zero weights are always legal. Two cases this pass does NOT yet cover are asserted at emit rather than
  handled slowly -- see the conv and dwconv branches of build_program."""
  for o in ops:
    if o.t not in ("CONV", "DWCONV", "INCONV"):
      continue
    w = o.w_q
    Cout, Cin = w.shape[0], w.shape[1]
    Coutp = _ru(Cout, 32)
    if o.t == "INCONV":
      if Coutp == Cout:
        continue
      nw = np.zeros((Coutp, Cin, w.shape[2], w.shape[3]), w.dtype)
      nw[:Cout] = w
      o.w_q = nw
    elif o.t == "DWCONV":
      if Coutp == Cout:
        continue
      nw = np.zeros((Coutp, 1, w.shape[2], w.shape[3]), w.dtype)
      nw[:Cout] = w
      o.w_q, o.groups = nw, Coutp
    else:
      Cinp = _ru(Cin, 32) if o.groups == 1 else Cin
      if Coutp == Cout and Cinp == Cin:
        continue
      nw = np.zeros((Coutp, Cinp, w.shape[2], w.shape[3]), w.dtype)
      nw[:Cout, :Cin] = w
      o.w_q = nw
    ws = np.asarray(o.w_scale, np.float64).reshape(-1)
    if ws.size > 1:
      o.w_scale = np.concatenate([ws, np.full(Coutp - Cout, ws[-1])])
    if o.bias_f is not None:
      o.bias_f = np.concatenate([np.asarray(o.bias_f, np.float64).reshape(-1), np.zeros(Coutp - Cout)])

"""Buffer geometry -- derived EXACTLY ONCE, here, and read by everyone else.

The rule this file exists to enforce:

    Python owns every formula that encodes a DECISION -- the `&~3` roundings, the `+8` slack, the padL choice,
    the ofw filter stride, the oLp/ils phase. C may only do arithmetic on values it was handed.

Before this, the depthwise formulas lived in THREE places (megakernel.c's dwconv_op, codegen's shared-scratch
sizing, and codegen's _dw_geom for the bordered path) and had to be kept in agreement by hand. Generalizing
stride-2 `padL` from a hardcoded 9 to `4*s+pad` on 2026-07-24 required editing two of them; missing one is a
silent wrong answer or an out-of-bounds write. Emitting every intermediate would not have helped -- `niw =
D*Wp*32` cannot disagree with itself -- so the line is drawn at policy, not arithmetic.

Consumers: the op-record emitter (codegen), the arena/scratch sizer (codegen, build.scratch_layout), the bordered
T1a planner, and -- via the record -- the kernel itself."""

def out_hw(H, W, kh, kw, sh, sw, ph, pw):
  """THE convolution output size. Trivial, and repeated in four places before this: conv_geom, dw_geom,
  codegen's arena_shapes and its INCONV branch. Repeating a formula is how the padL bug in this file's header
  happened -- one copy got the fix and the others did not."""
  return (H + 2 * ph - kh) // sh + 1, (W + 2 * pw - kw) // sw + 1

@dataclass(frozen=True)
class DwGeom:
  """Depthwise buffer  `padL` is the only input that varies between the two call sites: the RESIDENT
  (T1a bordered) path forces 4 so the producing conv can write the valid region with a 128-byte-aligned store,
  while the non-resident path uses the natural pad. Everything else follows from it -- which is precisely why
  there must be one function and not two."""
  Cp: int; pad: int; ofw: int; padL: int
  Wp: int; Hp: int
  oH: int; oW: int
  oLp: int; ils: int; owt: int
  nbytes_in: int; nbytes_out: int
  base_off: int

  @property
  def D(self): return self.Cp // 32

def dw_geom(C, H, W, kh, kw, s, padL_override=0) -> DwGeom:
  """The one definition. `padL_override` = the resident path's in_left_pad (4); 0 means derive it.

  Stride-2 uses `padL = 4*s + pad`, which makes out_left_pad exactly 4 -- a multiple of 4, so the from_d32 read
  pointer (to + oLp*32) stays 128-byte aligned -- AND drives the in_left_skip term to zero identically for any k,
  which is what lets s2_5xN (no in_left_skip arg) work at all. k=3 gives 9, the value this was hardcoded to."""
  Cp, pad, ofw = _ru(C, 32), kh // 2, _ru(kw, 4)
  padL = padL_override or (4 * s + pad if s == 2 else pad)
  Wp = (_ru(W, 4) + ofw + 2 * padL + 8) & ~3
  Hp = H + 2 * pad + 2
  oH, oW = out_hw(H, W, kh, kw, s, s, pad, pad)
  oLp = (padL - pad) // s
  ils = ((padL - (oLp * s + pad)) & 1) * 8 if s == 2 else 0
  owt = _ru(oW + oLp, 4)
  D = Cp // 32
  return DwGeom(Cp=Cp, pad=pad, ofw=ofw, padL=padL, Wp=Wp, Hp=Hp, oH=oH, oW=oW, oLp=oLp, ils=ils, owt=owt,
                nbytes_in=D * Hp * Wp * 32, nbytes_out=oH * D * owt * 32, base_off=pad * D * Wp * 32 + padL * 32)

@dataclass(frozen=True)
class ConvGeom:
  """Conv buffer  `Ho/Wo/Wop/Win/Hp` were already emitted in the op record; what was duplicated is
  everything derived AROUND them -- the channel-group padding in conv_op, the six scratch region sizes in
  build.scratch_layout, and the circular-buffer size in _v65_pick, each re-deriving from the others' outputs."""
  Cigp: int; Cogp: int; tot: int
  Ho: int; Wo: int; Wop: int; Win: int; Hp: int
  sz_d32in: int; sz_d32out: int
  circ: int
  padL: int = 0
  ils: int = 0

CONV_NT = 2

def conv_geom(Cin, Cout, H, W, kh, kw, sh, sw, ph, pw, groups, in_w_override=0) -> ConvGeom:
  """The one definition. `in_w_override` is the bordered-project case: the input is the dw's d32 output, so the
  circular buffer must be sized from that buffer's width (owt-oLp) rather than from this conv's own Win."""
  Cig, Cog = Cin // groups, Cout // groups
  Cigp, Cogp = _ru(Cig, 32), _ru(Cog, 32)
  Ho, Wo = out_hw(H, W, kh, kw, sh, sw, ph, pw)
  Wop = _ru(Wo, 4)
  Win = (Wop - 1) * sw + kw
  padL = 4 if pw > 0 else 0
  ils = padL - pw if pw > 0 else 0
  Win += ils
  circ_win = Win
  Win = _ru(Win, 4)
  Hp = H + 2 * ph
  tot = groups * (Cogp // 32)
  in_width_pad = ((in_w_override or circ_win) + 3 + 8 * sw) & ~3
  circ = ((in_width_pad * 2 * Cigp * max(kh, sh) + 127) & ~127) * CONV_NT + 256
  return ConvGeom(Cigp=Cigp, Cogp=Cogp, tot=tot, Ho=Ho, Wo=Wo, Wop=Wop, Win=Win, Hp=Hp, padL=padL, ils=ils,
                  sz_d32in=groups * Hp * (Cigp // 32) * Win * 32, sz_d32out=Ho * Wop * 32 * tot, circ=circ)

@dataclass(frozen=True)
class DwSched:
  """The Halide split: WHAT the depthwise computes is geometry, HOW it is executed is schedule. These were `if`s
  and `#define`s inside dwconv_op, so a new architecture silently inherited distill/MNv2's tuning. Deciding them
  host-side is also what lets C9 autotune them per model instead of hand-sweeping one global constant."""
  tile: int
  threads: int

def dw_sched(g: DwGeom, kh: int, s: int) -> DwSched:
  """Measured gates, moved verbatim from megakernel.c so this commit is behaviour-preserving.
  tile=4 swept best: -35% on mem-bound tall shapes (d32in > L2), neutral where d32in already fits L2.
  Threading is SIZE-GATED: 2 threads give ~1.9x DDR bandwidth but add barrier overhead, so it only pays on the
  big high-res stride-1 3x3 dws (measured: op1 C=32 112x112 0.362->0.322 WINS; the 56x56 and stride-2 ones LOSE,
  op7 +0.05, op4 +0.08). dw is memory-bound, so this is a bandwidth trade, not a compute one."""
  tile = int(os.getenv("DW_TH", "4"))
  thr = 2 if (kh == 3 and s == 1 and g.oH * g.oW >= int(os.getenv("DW_THREAD_MIN", "8192"))) else 1
  return DwSched(tile=tile, threads=thr)

"""Which kernel runs an op -- one declarative table, and a loud failure when nothing matches.

Kernel selection used to be scattered: `_v65_pick`'s env-gated levels in codegen, `have_asm`'s
`kh==3||kh==5||kh==7` conditions in megakernel.c, and a handful of build-time asserts bolted on as each
unsupported shape was discovered. Adding a kernel meant editing all three and remembering the asserts.

FAST-OR-FAIL (user decision 2026-07-24): there is no generic slow fallback. If no kernel matches, the BUILD fails
with a message naming the op and why -- we would rather refuse than silently ship a 13x-slower path. verify.py
found exactly that on its first run: stride-2 7x7 depthwise had no asm and quietly fell to the scalar
dw_mxn_cn at 5.0ms against 0.39ms for the stride-2 5x5. Genericity comes from the ewise VM (new activations are
chain data at full speed), not from slow C fallbacks.

Adding a kernel is now one row here plus the extern + dispatch arm in megakernel.c."""

DW_KERNELS = [
  (1, "dwconv2dbbb_s1_3x3", lambda kh, kw, s: s == 1 and kh == 3 and kw == 3),
  (2, "dwconv2dbbb_s1_5xN", lambda kh, kw, s: s == 1 and kh == 5 and kw == 5),
  (3, "dwconv2dbbb_s1_7xN", lambda kh, kw, s: s == 1 and kh == 7 and kw == 7),
  (4, "dwconv2dbbb_s2_3x3", lambda kh, kw, s: s == 2 and kh == 3 and kw == 3),
  (5, "dwconv2dbbb_s2_5xN", lambda kh, kw, s: s == 2 and kh == 5 and kw == 5),
]

def pick_stem(Cin: int, Cout: int, kh: int, s: int, W: int) -> None:
  """Check the low-depth stem (OP_INCONV) shape, or raise. There is exactly ONE stem kernel -- the PIXELS-IN-LANES
  one (32 output pixels in lanes, i8 scalar-Rt weights, no vsplat: 269 GMAC/s, MobileNetV2's stem 0.995 -> ~0.45ms)
  -- so this is a validity check, not a choice. WP_PIL bounds the gather, hence W <= STEM_MAX_W (both above).

  ★ This found a real latent bug (2026-07-24). The emitter still had an `else` branch building a SECOND, differently
  laid out INCONV record for the old nnlib `inconv2dbbb332` kernel -- which was deleted from megakernel.c earlier in
  the campaign. `stem_worker` unconditionally calls stem_conv_pil, so any model whose stem missed the predicate
  would have had a PIL stem read that record's fields from the wrong slots: a SILENT WRONG ANSWER, exactly what
  fast-or-fail exists to stop. The dead branch and its five unused scratch allocations are gone; the shape check
  now fails the build."""
  if not (Cin <= 4 and Cout == 32 and kh == 3 and s == 2 and W <= STEM_MAX_W):
    raise AssertionError(
      f"no stem kernel for Cin={Cin} Cout={Cout} {kh}x{kh} stride {s} width {W}. stem_conv_pil handles Cin<=4, "
      f"Cout==32, 3x3 stride 2, W<={STEM_MAX_W}. Raise WP_PIL (one constant, both consumers follow) or let the "
      "shape fall to the ordinary conv path in parse.py -- but do NOT emit an OP_INCONV record it cannot read.")

def pick_dw(kh: int, kw: int, s: int) -> int:
  """The depthwise kernel id for this shape, or a build error. Also subsumes the asymmetric/even-k asserts that
  used to be written out separately: neither can match a row, so both fail here with the same message."""
  for kid, _name, pred in DW_KERNELS:
    if pred(kh, kw, s):
      return kid
  raise AssertionError(
    f"no depthwise kernel for {kh}x{kw} stride {s}. Available: " +
    ", ".join(f"{n.split('_', 1)[1]}" for _i, n, _p in DW_KERNELS) +
    ". Add the nnlib .S + a row in DW_KERNELS + a dispatch arm in dwconv_op, or change the model. "
    "(There is deliberately no scalar fallback -- it would run ~13x slower with no warning.)")

"""Weights and scales: float ONNX tensors -> the exact bytes each HVX kernel reads.

    requant(...)        THE fixed-point requant every conv path shares (see its docstring)
    conv_params(...)    the V65 d32 weight layout + biasbuf/recip + the op record's conv params
    dw_params(...)      the depthwise-asm filter layout + per-channel bias/recip
    inconv_params(...)  the pixels-in-lanes stem's [go][ky][kx][32][4] weight layout

Split out of codegen.py, which had grown into four unrelated jobs (this, the ONNX->op-list emitter, the
op-list->record lowering, and the record ABI). Nothing here knows about arenas, layouts or op records -- it is
pure "what bytes does this kernel want", which is why it is separable at all."""

FILT_ZERO = 128

def requant(x_scale, w_scale, out_scale, out_zp, bias_f, wsum, x_zp, n):
  """THE fixed-point requant, shared by every conv path (conv / depthwise / stem). Was written out three times.

  Folds the float multiplier M = x_scale*w_scale/out_scale into an int32 `recip` at shift `rsh`, and folds BOTH
  the output zero point and the ACTIVATION zero-point correction into the integer bias, so the kernel does one
  integer multiply-shift and no per-pixel zp pass. `wsum` is the signed weight sum per output channel (the
  caller supplies it -- the V65 conv gets it from the weight rearrangement's gemsumb, the others sum directly).

  Per-TENSOR and per-CHANNEL w_scale are the same code: a scalar is broadcast, and `rsh` comes from M.max() so a
  per-channel recip cannot overflow int32. That equivalence is why there is one function here and not three.

  This is also the ONE place the device's requant rounding is defined -- worth knowing, because the residual
  ~1e-4 cosine against the float ONNX is this fixed point, not a bug (measured 2026-07-24, see the memory)."""
  ws = np.asarray(w_scale, np.float64).reshape(-1)
  if ws.size == 1:
    ws = np.full(n, ws[0])
  M = x_scale * ws / out_scale
  rsh = max(0, int(np.ceil(np.log2(max(float(M.max()), 1e-12)))) + 1)
  recip = np.round(M * (2.0 ** (31 - rsh))).astype(np.int64)
  bias_q = np.round((bias_f if bias_f is not None else np.zeros(n)) / (x_scale * ws)).astype(np.int64)
  if out_zp:
    bias_q = bias_q + np.round(out_zp / M).astype(np.int64)
  return bias_q - x_zp * np.asarray(wsum, np.int64), recip, rsh

def inconv_params(w_q, w_scale, bias_f, x_scale, x_zp, out_scale, out_zp, kh, kw):
  """Stem conv Cin<=4, 3x3, s2 -> (weights, bias_q[Cout], recip, rsh). PER-TENSOR w_scale. Weight layout
  [go][ky][kx][32][4] = raw int8, read as a scalar Rt pair by stem_conv_pil's vrmpy. ic>=Cin -> 0; bias folds
  bias_q - x_zp*Sigma_w so the xzp-filled borders cancel (activation-zp corr).

  The `pil` flag is gone with the nnlib inconv2dbbb332 kernel it selected (deleted earlier in the campaign, and
  registry.pick_stem already fails the build for anything the PIL stem cannot take) -- one stem, one layout."""
  Cout, Cin = w_q.shape[0], w_q.shape[1]
  wsum = w_q.reshape(Cout, -1).astype(np.int64).sum(1)
  bias_q, recip, rsh = requant(x_scale, w_scale, out_scale, out_zp, bias_f, wsum, x_zp, Cout)
  bias_q, recip = bias_q.astype(np.int32), int(recip[0])
  wp = np.zeros((Cout, 4, kh, kw), np.int8)
  wp[:, :Cin] = w_q
  return wp.reshape(Cout // 32, 32, 4, kh, kw).transpose(0, 3, 4, 1, 2).copy(), bias_q, recip, rsh

def dw_pack(w_q, kh, kw):
  """w_q int [C,1,kh,kw] SIGNED -> uint8 filt in the nnlib dwconv2dbbb layout (bit-exact, from dspbench dw_ref).
  filt[32*d*kh*ofw + ofw*fy*32 + z*4 + 128*(fx//4) + (fx%4)], ofw=(kw+3)&~3; real taps fx<kw = w+128, pad taps = 0.
  C padded to mult-32: pad channels get w=0 (-> stored 128 for real taps -> zum cancels -> outputs 0).

  That index is AFFINE in (d, fy, fx//4, z, fx%4) with strides (kh*ofw*32, ofw*32, 128, 4, 1) -- i.e. the layout is
  just [C/32][kh][ofw/4][32][4] viewed row-major, so this is a transpose, not a loop. Written out as five nested
  Python loops it was also the slowest thing in a build after the oracle."""
  Cp, ofw = _ru(w_q.shape[0], 32), (kw + 3) & ~3
  w = np.zeros((Cp, kh, kw), np.int64)
  w[: w_q.shape[0]] = w_q.reshape(w_q.shape[0], kh, kw)
  filt = np.zeros((Cp, kh, ofw), np.uint8)
  filt[:, :, :kw] = (w + FILT_ZERO) & 0xFF
  return filt.reshape(Cp // 32, 32, kh, ofw // 4, 4).transpose(0, 2, 3, 1, 4).ravel()

def dw_params(w_q, w_scale, bias_f, x_scale, x_zp, out_scale, out_zp, kh, kw):
  """Depthwise conv -> (filt, bias_sum[Cp], recip, rsh) for the dw-asm. PER-TENSOR w_scale. C padded to mult-32."""
  C = w_q.shape[0]
  Cp = _ru(C, 32)
  wsum = w_q.reshape(C, -1).astype(np.int64).sum(1)
  bias_q, recip, rsh = requant(x_scale, w_scale, out_scale, out_zp, bias_f, wsum, x_zp, C)
  recip = int(recip[0])
  bias = np.zeros(Cp, np.int32)
  bias[:C] = bias_q.astype(np.int32)
  return dw_pack(w_q, kh, kw), bias, recip, rsh, Cp

def rearrange_d32_v65(w_q):
  """w_q[Cout,Cin,kh,kw] int8 SIGNED -> (flat repacked signed-as-u8 blob, gemsumb[Coutp]). SAME tile layout as
  rearrange_d32 but the stored coeff is the SIGNED weight (two's-complement byte, V65 `vrmpy .b` reads it as int8)
  and padding channels are 0. gemsumb = Σ signed weight per out-channel (pad -> 0), which folds into the V65 biasbuf
  as the activation-zero-point correction (no per-pixel suma). Matches supernode_procweights.c signed_mode_sel=1."""
  Cout, Cin, kh, kw = w_q.shape
  Coutp, Cinp = _ru(Cout, 32), _ru(Cin, 32)
  wp = np.zeros((kh, kw, Cinp, Coutp), np.int16)
  wp[:, :, :Cin, :Cout] = w_q.transpose(2, 3, 1, 0).astype(np.int16)
  gemsumb = wp.astype(np.int32).sum(axis=(0, 1, 2))
  out = (wp.reshape(kh, kw, Cinp // 32, 8, 4, Coutp // 32, 32).transpose(5, 0, 2, 1, 3, 6, 4).ravel() & 0xFF)
  return out.astype(np.uint8), gemsumb

def conv_params(x_q_shape, x_scale, x_zp, w_q, w_scale, bias_f, kh, kw, sh, sw, ph, pw, groups, out_scale, out_zp):
  Cin, H, W = x_q_shape
  Cout = w_q.shape[0]
  g = conv_geom(Cin, Cout, H, W, kh, kw, sh, sw, ph, pw, groups)
  Cig, Cog = Cin // groups, Cout // groups
  Cogp, totchunks = g.Cogp, g.tot
  Ho, Wo, Wop, Win, Hp = g.Ho, g.Wo, g.Wop, g.Win, g.Hp
  reps, sumbs = zip(*(rearrange_d32_v65(w_q[gi * Cog : (gi + 1) * Cog].reshape(Cog, Cig, kh, kw).astype(np.int32))
                      for gi in range(groups)))
  wsum = np.concatenate([sb[:Cog] for sb in sumbs])
  bias_q, recip_pc, zsh = requant(x_scale, w_scale, out_scale, out_zp, bias_f, wsum, x_zp, Cout)
  biasbuf, recip = np.zeros(groups * Cogp, np.int32), np.zeros(groups * Cogp, np.int32)
  for gi in range(groups):
    biasbuf[gi * Cogp : gi * Cogp + Cog] = bias_q[gi * Cog : (gi + 1) * Cog]
    recip[gi * Cogp : gi * Cogp + Cog] = recip_pc[gi * Cog : (gi + 1) * Cog]
  wrep = np.concatenate(reps)
  w_gcstride = wrep.size // totchunks
  params = dict(Cin=Cin, H=H, W=W, Cout=Cout, kh=kh, kw=kw, sh=sh, sw=sw, ph=ph, pw=(g.padL or pw), groups=groups,
                xzp=x_zp, zshift=zsh, w_gcstride=w_gcstride, in_left_skip=g.ils)
  return wrep, biasbuf, recip, params, (Cout, Ho, Wo), g

"""Which tensors stay in d32 layout, and which depthwises get a persistent padded buffer.

    R = plan(ops, shp, seed_name, alloc, off)     annotates each op in place; returns what is not per-op
    MK_RESIDENCY_DUMP=1                           per-op decision table (the diagnostic, not a gate)

d32 is [H][C/32][W][32]. Every op boundary that changes layout costs a pack or a from_d32 transpose -- measured
at 22ms/43% of MobileNetV2 before any of this existed, and 3.75ms of distill. So the planner's job is to keep a
whole stage resident and pay the transpose only at stage boundaries.

★★★ THE FIXPOINT IS A **GREATEST** FIXPOINT, AND THAT IS NOT AN IMPLEMENTATION DETAIL.
Every candidate tensor starts `d32=True` and the iteration only ever RETRACTS:

    ok = len(c) >= 1 and _prod_std(nm) and all(_reads_d32(x, nm) for x in c)

Write it the other way round -- start False and add -- and you get a LEAST fixpoint, which produces correct
output with silently worse performance: fewer tensors resident, more transposes, and no test failure anywhere.
That is also why this pass is NOT written with tinygrad's graph_rewrite (which computes a least fixpoint) even
though C6's fusions are: the direction is wrong, and the edges here are ROLE-NAMED dict fields (`src`, `conv`,
`res`, `a`, `b`, `expand`) rather than positional UOp src -- `_reads_d32` genuinely needs to know whether a name
arrived as a SETAIL's `conv` or its `res`.

★ `len(c) >= 1` IS A VACUOUS-TRUTH GUARD, NOT A FORMALITY. `all()` over an empty consumer list is True, so a
tensor with NO consumers would be declared d32 and its producer would write a layout nobody reads. This bites
under MK_TRUNC, where the truncated terminal op has zero consumers.

★ THE MUTUAL DEPENDENCY IS REAL. `_reads_d32` consults the in-progress `d32t` for SETAIL/HEAD (setail_d32 cannot
mix layouts, so `conv` and `res` must BOTH be d32 -- each one's answer depends on the other's) and for ADD. A
predicate reading the fixpoint state mid-iteration is exactly what makes this a fixpoint and not a single pass."""

_ROLES = ("src", "expand", "conv", "res", "gate", "a", "b")

ANNOTATIONS = ("in_d32", "out_d32", "d32")

@dataclass
class Residency:
  """What is left of the layout plan once the per-op booleans live on the ops: the BORDERED buffers, which carry
  real geometry (an arena offset, a DwGeom) rather than a flag, plus the seed decision and the settled tensor
  set for the dump. Those legitimately stay a side table -- they are not properties of a single op."""
  seed_d32: bool = False
  bord_exp: dict = field(default_factory=dict)
  bord_dw: dict = field(default_factory=dict)
  bord_proj: dict = field(default_factory=dict)
  d32t: dict = field(default_factory=dict)

def plan(ops, shp, seed_name, alloc, off) -> Residency:
  """`alloc`/`off` are passed in because the T1a planner has to ALLOCATE the dw's bordered buffer and re-point
  the expand's output at it -- it is a layout decision with an allocation consequence, not a pure analysis."""
  R = Residency()
  for o in ops:
    for a in ANNOTATIONS:
      if hasattr(o, a):
        setattr(o, a, 0)
  cons: dict[str, list] = {}
  for o in ops:
    for key in _ROLES:
      nm = getattr(o, key, None)
      if isinstance(nm, str):
        cons.setdefault(nm, []).append(o)
  prod = {o.out: o for o in ops if isinstance(o.out, str)}

  def _set(op, ind=None, outd=None):
    if ind is not None:
      op.in_d32 = ind
    if outd is not None:
      op.out_d32 = outd

  def _conv_std(o):
    """Does this op write STANDARD d32 that a d32 consumer can read directly?"""
    if o.t == "CONV":
      return o.w_q.shape[0] % 32 == 0 and (o.w_q.shape[0] // o.groups) % 32 == 0
    if o.t == "DWCONV":
      return o.w_q.shape[0] % 32 == 0 and o.sh == 1
    if o.t == "INCONV":
      Cout, _Ho, Wo = shp[o.out][0]
      return Cout == 32 and Wo % 4 == 0
    return False

  def _reads_d32(c, nm):
    """Can consumer op `c` read tensor `nm` as d32? Dispatches on the ROLE `nm` plays for `c`, which is why this
    is not a structural pattern match."""
    if c.t == "SE_GATE" and c.expand == nm:
      return True
    if c.t in ("SETAIL", "HEAD") and (c.conv == nm or c.res == nm):
      if not c.res:
        return c.t == "SETAIL"
      return bool(R.d32t.get(c.res if c.conv == nm else c.conv, False))
    if c.t in ("CONV", "DWCONV") and c.src == nm:
      return True
    if c.t == "ADD" and (c.a == nm or c.b == nm):
      return R.d32t.get(c.out, False)
    return False

  seed_d32_ok = shp[seed_name][0][0] % 32 == 0

  def _prod_std(nm):
    if nm == seed_name:
      return seed_d32_ok
    if (p := prod.get(nm)) is None:
      return False
    if _conv_std(p):
      return True
    if p.t == "ADD":
      return R.d32t.get(p.a, False) and R.d32t.get(p.b, False)
    if p.t == "SETAIL":
      return R.d32t.get(p.conv, False) and (not p.res or R.d32t.get(p.res, False))
    return False

  cand = [o.out for o in ops if isinstance(o.out, str) and (_conv_std(o) or o.t in ("ADD", "SETAIL"))]
  if seed_d32_ok:
    cand.append(seed_name)
  R.d32t = {nm: True for nm in cand}
  changed = True
  while changed:
    changed = False
    for nm in cand:
      c = cons.get(nm, [])
      ok = len(c) >= 1 and _prod_std(nm) and all(_reads_d32(x, nm) for x in c)
      if R.d32t.get(nm) != ok:
        R.d32t[nm] = ok
        changed = True

  for o in ops:
    nm = o.out
    if isinstance(nm, str) and R.d32t.get(nm):
      if o.t == "ADD":
        o.d32 = 1
      elif o.t != "SETAIL":
        _set(o, outd=1)
    if o.t in ("CONV", "DWCONV") and R.d32t.get(o.src):
      _set(o, ind=1)
    if o.t == "SE_GATE" and R.d32t.get(o.expand):
      o.d32 = 1
    if o.t == "SETAIL" and R.d32t.get(o.conv) and (not o.res or R.d32t.get(o.res)):
      o.d32 = 1
      if R.d32t.get(o.out):
        o.out_d32 = 1
    if o.t == "HEAD" and o.res and R.d32t.get(o.conv) and R.d32t.get(o.res):
      o.d32 = 1
  R.seed_d32 = bool(R.d32t.get(seed_name, False))

  for dw in ops:
    if dw.t != "DWCONV" or dw.kh < 3 or dw.w_q.shape[0] % 32:
      continue
    exp = prod.get(dw.src)
    if (exp is None or exp.t != "CONV" or exp.kh != 1 or exp.kw != 1 or exp.groups != 1
        or exp.w_q.shape[0] % 32 or cons.get(exp.out) != [dw]):
      continue
    dc = cons.get(dw.out, [])
    if len(dc) != 1:
      continue
    proj = dc[0]
    if proj.t != "CONV" or proj.kh != 1 or proj.kw != 1 or proj.groups != 1 or proj.src != dw.out:
      continue
    C, (H, W) = dw.w_q.shape[0], shp[dw.src][0][1:]
    g = dw_geom(C, H, W, dw.kh, dw.kh, dw.sh, padL_override=4)
    boff = alloc(exp.out + "_bord", ((g.nbytes_in,), np.uint8))
    off[exp.out] = boff
    R.bord_exp[id(exp)] = (boff, g.Wp, g.base_off)
    R.bord_dw[id(dw)] = (boff, g)
    R.bord_proj[id(proj)] = (g.owt, g.oLp)
    _set(exp, outd=1)
    _set(dw, ind=1, outd=1)
    _set(proj, ind=1)

  if os.getenv("MK_RESIDENCY_DUMP"):
    _dump(ops, R)
  return R

def _dump(ops, R: Residency) -> None:
  """Per-op decision table. Residency changes otherwise surface only as a timing delta or a cosine break, with no
  way to see WHICH op flipped -- which is what made this pass frightening to touch."""
  print(f"{'#':>3} {'op':8} {'in_d32':>6} {'out_d32':>7} {'bordered':>8}  out")
  for i, o in enumerate(ops):
    ind, outd = getattr(o, "in_d32", ""), getattr(o, "out_d32", "")
    bord = ("exp" if id(o) in R.bord_exp else "dw" if id(o) in R.bord_dw else
            "proj" if id(o) in R.bord_proj else "")
    extra = ("d32" if getattr(o, "d32", 0) else "")
    print(f"{i:>3} {getattr(o, 't', type(o).__name__):8} {ind:>6} {outd:>7} {bord:>8}  {o.out} {extra}")

LUTN = 4096
LUT_LO, LUT_HI = -16.0, 16.0
LUT_SCALE = LUTN / (LUT_HI - LUT_LO)

def sigmoid_lut():
  u = LUT_LO + (np.arange(LUTN) + 0.5) / LUT_SCALE
  return (1.0 / (1.0 + np.exp(-u))).astype(np.float32)

_LUT = sigmoid_lut()

def _circ_bytes(o, src_shape, in_w_override=0):
  """V65 circular-datapath scratch for a conv. EVERY conv runs V65 -- k==1 g==1, 3x3-s1, grouped and 3x3-s2 are all
  proven bit-exact on it, and it is what took distill_vision 19.3->15.7ms. The V60 gvconv path it replaced (a
  per-pixel `suma` correction plus its ds/II integral images) was deleted 2026-07-24 along with three of the six
  conv scratch regions; there is no longer a slow twin to fall back to, which is the fast-or-fail policy from C5."""
  Cin, H, W = src_shape
  return conv_geom(Cin, o.w_q.shape[0], H, W, o.kh, o.kw, o.sh, o.sw, o.ph, o.pw, o.groups, in_w_override).circ

def arena_shapes(ops, seed_name, seed_shape):
  """name -> (shape, dtype) for every arena tensor, in closed form -- so a build never runs the model.
  Verified against reference.run_arena (shape, dtype AND nbytes) for all 92 ops of distill_vision."""
  shp = {seed_name: (tuple(seed_shape), np.uint8)}
  for o in ops:
    if o.t in ("CONV", "DWCONV", "INCONV"):
      shp[o.out] = ((o.w_q.shape[0], *out_hw(*shp[o.src][0][1:], o.kh, o.kw,
                                                            o.sh, o.sw, o.ph, o.pw)), np.uint8)
    elif o.t == "SE_GATE":
      shp[o.gate] = ((shp[o.expand][0][0],), np.float32)
    elif o.t == "ADD":
      shp[o.out] = (shp[o.a][0], np.uint8)
    elif o.t == "SETAIL":
      shp[o.out] = (shp[o.conv][0], np.uint8)
    elif o.t == "HEAD":
      shp[o.out] = ((o.gw.shape[0],), np.float32)
  return shp

def build_program(prog, _preassigned=None):
  """Encode the op list for the C interpreter: arena offsets, weight blob, op records.
  Returns dict with ops(int32 [nops,INTS_PER_OP]), wts(bytes), arena_size, seed(off,shape), out(off,size),
  op_outoff (per-op output arena offset), op_list."""
  ops = emit(prog)[0]
  _pad32(ops)
  if os.getenv("MK_TRUNC"):
    ops = ops[:int(os.getenv("MK_TRUNC"))]
  _s, _z, seed_name = seed_quant(prog)
  shp = arena_shapes(ops, seed_name, prog["seed_shape"])
  def AL(n):
    return (n + 127) // 128 * 128
  off, sizes, order = {}, {}, []
  cur = 0

  def alloc_out(o, d32_elems=None):
    """This op's output slice: the d32 byte count when it writes d32 (which is >= the NHWC size, so the NHWC
    shape would under-reserve), else the plain NHWC shape. Every op kind needs this and each used to spell out
    its own two-branch version."""
    return alloc(o.out, ((d32_elems,), np.uint8) if d32_elems is not None else shp[o.out])

  def alloc(name, sd):
    nonlocal cur
    if name in off:
      return off[name]
    shape, dt = sd
    nbytes = AL(int(np.prod(shape)) * np.dtype(dt).itemsize)
    sizes[name] = nbytes
    order.append(name)
    if _preassigned is not None and name in _preassigned:
      off[name] = _preassigned[name]
      cur = max(cur, off[name] + nbytes)
    else:
      off[name] = cur
      cur += nbytes
    return off[name]

  alloc(seed_name, shp[seed_name])
  R = plan(ops, shp, seed_name, alloc, off)
  seed_is_d32 = R.seed_d32
  max_circ = 0
  for o in ops:
    if o.t != "CONV":
      continue
    src_shape = (o.w_q.shape[1] * o.groups,) + shp[o.src][0][1:]
    bp = R.bord_proj.get(id(o))
    max_circ = max(max_circ, _circ_bytes(o, src_shape, (bp[0] - bp[1]) if bp else 0))
  circ_off = alloc("__v65_circ__", ((max_circ,), np.uint8)) if max_circ else 0
  dw_in_max = dw_out_max = dw_dmax = 0
  for o in ops:
    if o.t != "DWCONV":
      continue
    C = o.w_q.shape[0]
    g = dw_geom(C, shp[o.src][0][1], shp[o.src][0][2], o.kh, o.kw, o.sh)
    dw_dmax = max(dw_dmax, g.D)
    dw_in_max, dw_out_max = max(dw_in_max, g.nbytes_in), max(dw_out_max, g.nbytes_out)
  dw_d32in_off = alloc("__dw_d32in__", ((dw_in_max,), np.uint8)) if dw_in_max else 0
  dw_d32out_off = alloc("__dw_d32out__", ((dw_out_max,), np.uint8)) if dw_out_max else 0
  dw_aux_off = alloc("__dw_aux__", ((max(64 * 128, dw_dmax * 128 + 4096),), np.uint8)) if dw_in_max else 0
  blob = bytearray()

  def put(arr, dt):
    if len(blob) % 128:
      blob.extend(b"\0" * (128 - len(blob) % 128))
    o = len(blob)
    blob.extend(np.ascontiguousarray(arr, dt).tobytes())
    return o

  lut_off = put(_LUT, np.float32)
  seed_d32_off = None
  if seed_is_d32:
    sC, sH, sW = shp[seed_name][0]
    seed_d32_off = alloc("__seed_d32__", ((_ru(sC, 32) // 32 * sH * _ru(sW, 4) * 32,), np.uint8))
  def _rd(name, is_d32):
    return seed_d32_off if (seed_is_d32 and name == seed_name and is_d32) else off[name]

  recs = []
  op_outoff = []

  def _pack_before(o, src_name):
    """Insert an OP_PACK converting `src_name` from NHWC to plain d32, and return the packed buffer's offset.

    ★ LAYOUT CONVERSION IS AN OP, NOT A CASE INSIDE EVERY KERNEL. This is what lets every compute kernel read d32
    and ONLY d32. Without it each kernel needs an NHWC twin, which is how this file used to carry a scalar pack()
    at 2.33 ns/B plus NHWC copies of setail and the SE GAP -- three implementations of "the other layout".

    It exists because some producers CANNOT write standard d32: a stride-2 depthwise (its d32 output has oLp junk
    left columns no plain consumer can skip), a grouped conv with Cog%32, and the conv feeding a res-less HEAD
    (measured: NHWC is 2.4x faster there). Their consumers still want d32, so the mismatch has to be paid
    somewhere -- and paying it once, here, with the vectorized to_d32_asm, beats paying it in every kernel.

    ★ MEASURED REGRESSION THIS FIXES (2026-07-27). Deleting the NHWC paths earlier today was justified by "no gate
    model executes this", which is the wrong test: the bar is the mnv2/distill op set in any REASONABLE order, not
    the two graphs we happen to own. `dw_s2 -> conv3x3` and `dw_s2 -> dw_s1` are both reasonable and both stopped
    building. They build again, now without a scalar path anywhere."""
    nonlocal recs, op_outoff
    C, H, W = shp[src_name][0]
    Cigp, Wop = _ru(C, 32), _ru(W, 4)
    d32_off = alloc(src_name + "_d32", ((Cigp // 32 * H * Wop * 32,), np.uint8))
    if src_name not in packed:
      recs.append(pack("PK", op=OP_PACK, out=d32_off, src=off[src_name], W=W, H=H, Cigp=Cigp, Wop=Wop))
      op_outoff.append(d32_off)
      packed.add(src_name)
    return d32_off

  packed: set = set()
  for o in ops:
    if o.t == "CONV":
      in_d32, out_d32 = o.in_d32, o.out_d32
      in_off = _rd(o.src, in_d32)
      wrep, bb, rc, params, _, cg = conv_params(
        (o.w_q.shape[1] * o.groups,) + shp[o.src][0][1:], o.x_scale, o.x_zp, o.w_q, o.w_scale, o.bias_f,
        o.kh, o.kw, o.sh, o.sw, o.ph, o.pw, o.groups, o.out_scale, o.out_zp)
      if not out_d32 and (o.w_q.shape[0] // o.groups) % 32:
        raise NotImplementedError(
          f"conv {o.out}: groups={o.groups} gives Cog={o.w_q.shape[0] // o.groups}, not a multiple of 32, and this "
          f"conv must emit NHWC. from_d32_asm cannot unpack it. Fix: pad Cout to a multiple of 32*groups in _pad32.")
      if not in_d32 and not (o.groups == 1 and o.kh == o.kw == 1 and o.ph == o.pw == 0
                             and o.sh == o.sw == 1 and o.w_q.shape[1] % 32 == 0):
        in_off, in_d32 = _pack_before(o, o.src), 1
      be, bp = R.bord_exp.get(id(o)), R.bord_proj.get(id(o))
      out_off = alloc_out(o, cg.tot * cg.Ho * cg.Wop * 32 if out_d32 else None)
      g2 = cg if not bp else conv_geom(
        o.w_q.shape[1] * o.groups, o.w_q.shape[0], *shp[o.src][0][1:], o.kh, o.kw, o.sh,
        o.sw, o.ph, o.pw, o.groups, in_w_override=bp[0] - bp[1])
      f = dict(op=OP_CONV, out=out_off, src=in_off, wt=put(wrep, np.uint8), bias=put(bb, np.int32),
               recip=put(rc, np.int32), in_d32=int(in_d32), out_d32=int(out_d32), circ=circ_off, **params)
      if be:
        f |= dict(bord_Wp=be[1], bord_base=be[2])
      if bp:
        f |= dict(Win=bp[0], in_left_skip=bp[1])
      rec = pack("CV", g2, scratch=g2.sz_d32in, **f)
      op_outoff.append(out_off)
    elif o.t == "DWCONV":
      in_off = _rd(o.src, o.in_d32)
      C = o.w_q.shape[0]
      s = o.sh
      kern = pick_dw(o.kh, o.kw, s)
      in_d32, out_d32 = o.in_d32, o.out_d32
      filt, bias, recip, rsh, Cp = dw_params(o.w_q, o.w_scale, o.bias_f, o.x_scale, o.x_zp,
                                             o.out_scale, o.out_zp, o.kh, o.kw)
      H, W = shp[o.src][0][1:]
      bd = R.bord_dw.get(id(o))
      d32in_off, ind_bord, in_lpad = dw_d32in_off, 0, 0
      if bd:
        boff, g = bd
        d32in_off, ind_bord, in_lpad = boff, 1, 4
        out_off = alloc_out(o, g.nbytes_out)
      else:
        out_off = alloc_out(o, (Cp // 32) * H * ((W + 3) & ~3) * 32 if out_d32 else None)
      if not out_d32 and (C % 32 or C // 32 < 2):
        raise NotImplementedError(
          f"dwconv {o.out}: C={C} must emit NHWC, but from_d32_asm needs C mult-32 with >=2 depth chunks (C>=64). "
          f"Fix: keep this dw's output d32, or pad C to 64 in _pad32 (doubles its work -- measure first).")
      if not in_d32 and not ind_bord:
        in_off, in_d32 = _pack_before(o, o.src), 1
      gg = dw_geom(C, H, W, o.kh, o.kw, s, padL_override=in_lpad)
      sc = dw_sched(gg, o.kh, s)
      rec = pack("DW", gg, sc, op=OP_DWCONV, out=out_off, src=in_off, filt=put(filt, np.uint8),
                 bias=put(bias, np.int32), d32in=d32in_off, d32out=dw_d32out_off, C=C, H=H, W=W,
                 kh=o.kh, kw=o.kw, fz=FILT_ZERO, recip=recip, rsh=rsh, aux=dw_aux_off, s=s,
                 ind=int(in_d32), outd=int(out_d32), src_wop=_ru(W, 4), kern=kern,
                 ind_bord=ind_bord,
                 in_lpad=in_lpad,
                 xzp=o.x_zp)
      op_outoff.append(out_off)
    elif o.t == "INCONV":
      out_off = alloc(o.out, shp[o.out])
      in_off = off[o.src]
      Cout, Cin = o.w_q.shape[0], o.w_q.shape[1]
      H, W = shp[o.src][0][1:]
      pad = o.kh // 2
      Ho, Wo = out_hw(H, W, o.kh, o.kw, o.sh, o.sw, pad, pad)
      pick_stem(Cin, Cout, o.kh, o.sh, W)
      wv, bias, recip, rsh = inconv_params(o.w_q, o.w_scale, o.bias_f, o.x_scale, o.x_zp,
                                           o.out_scale, o.out_zp, o.kh, o.kw)
      recipv = np.full(Cout, recip, np.int32)
      rec = pack("IC", op=OP_INCONV, out=out_off, src=in_off, wvec=put(wv, np.int8), bias=put(bias, np.int32),
                 recip=put(recipv, np.int32), Cin=Cin, H=H, W=W, Cout=Cout, k=o.kh, stride=o.sh, pad=pad,
                 zsh=rsh, Ho=Ho, Wo=Wo, xzp=o.x_zp,
                 cp4=alloc(o.out + "_pilcp4", ((2 * 3 * WP_PIL * 4,), np.uint8)))
      op_outoff.append(out_off)
    elif o.t == "SE_GATE":
      Cexp = shp[o.expand][0][0]
      HW = int(np.prod(shp[o.expand][0][1:]))
      Csq = o.fc1_w.shape[0]
      _f1 = o.fc1_w.reshape(Csq, -1)
      if _f1.shape[1] < Cexp:
        o.fc1_w = np.concatenate([_f1, np.zeros((Csq, Cexp - _f1.shape[1]), _f1.dtype)], axis=1)
      _n = o.fc2_w.shape[0] if o.fc2_w.ndim > 1 else o.fc2_w.size // Csq
      if _n < Cexp:
        o.fc2_w = np.concatenate([o.fc2_w.reshape(_n, Csq), np.zeros((Cexp - _n, Csq), o.fc2_w.dtype)])
        if o.fc2_b is not None:
          o.fc2_b = np.concatenate([np.asarray(o.fc2_b, np.float64).reshape(-1), np.zeros(Cexp - _n)])
        _ws = np.asarray(o.fc2_ws, np.float64).reshape(-1)
        if _ws.size > 1:
          o.fc2_ws = np.concatenate([_ws, np.full(Cexp - _n, _ws[-1])])
      Cexpp = _ru(Cexp, 128)
      if Cexp % 32:
        raise NotImplementedError(f"SE_GATE expand width Cexp={Cexp} (op {o.out}): must be a multiple of 32 "
                                  f"(_pad32 guarantees this); the d32 GAP chunks by 32.")
      gate_off = alloc(o.gate, shp[o.gate])
      exp_off = _pack_before(o, o.expand) if not o.d32 else off[o.expand]
      g0 = o.sc_exp / HW
      g1 = o.sc_exp * o.zc_exp
      M1 = np.broadcast_to((o.s_rm * o.fc1_ws / o.fc1_outs).astype(np.float32), (Csq,))
      b1q = np.round((o.fc1_b if o.fc1_b is not None else np.zeros(Csq)) / (o.s_rm * o.fc1_ws)).astype(np.float32)

      A2 = (o.fc2_ins * o.fc2_ws).astype(np.float32)
      B2 = (o.fc2_b if o.fc2_b is not None else np.zeros(Cexp)).astype(np.float32)
      wsum1 = o.fc1_w.reshape(Csq, -1).astype(np.float64).sum(axis=1).astype(np.float32)
      for _nm, _a in (("M1", M1), ("b1q", b1q), ("wsum1", wsum1)):
        if len(_a) != Csq:
          raise AssertionError(f"SE_GATE {o.gate}: blob field {_nm} has length {len(_a)}, expected Csq={Csq}")
      mA = 128.0 * A2.astype(np.float64)
      mB = 128.0 * (B2.astype(np.float64) + 16.0)
      accbound = 255.0 * np.abs(o.fc2_w.reshape(Cexp, Csq).astype(np.float64)).sum(axis=1).max()
      denom = np.abs(mA).max() * accbound + np.abs(mB).max() + 1.0
      S2 = int(np.clip(np.floor(np.log2((2.0**31 - 1) / denom)) - 1, 0, 30))
      mAi = np.clip(np.round(mA * (2.0**S2)), -2**31, 2**31 - 1).astype(np.int32)
      mBi = np.clip(np.round(mB * (2.0**S2)), -2**31, 2**31 - 1).astype(np.int32)

      c = np.arange(Cexp)
      lane = c & 127
      rr = lane & 3
      gg = np.where(rr == 1, 2, np.where(rr == 2, 1, rr))
      di = (c & ~127) + (gg << 5) + (lane >> 2)
      mAi_d, mBi_d = np.zeros(Cexpp, np.int32), np.zeros(Cexpp, np.int32)
      mAi_d[di], mBi_d[di] = mAi, mBi
      sig = np.concatenate([mAi_d.view(np.float32), mBi_d.view(np.float32), np.array([S2], np.int32).view(np.float32)])
      blob_off = put(np.concatenate([[g0, g1, 1.0 / o.s_rm, float(o.z_rm)], M1, b1q, wsum1, sig]), np.float32)
      def _padrows(w):
        z = np.zeros((Csq, Cexpp), np.int8); z[:, :Cexp] = w; return z
      fc1w_off = put(_padrows(o.fc1_w.reshape(Csq, -1)), np.int8)
      fc2w_off = put(_padrows(o.fc2_w.reshape(Cexp, Csq).T), np.int8)
      d32 = o.d32
      eH, eW = shp[o.expand][0][1:]
      rec = pack("SE", op=OP_SE_GATE, gate=gate_off, expand=exp_off, blob=blob_off, fc1w=fc1w_off, fc2w=fc2w_off,
                 lut=lut_off, Cexp=Cexp, HW=HW, Csq=Csq,
                 scratch=scr(Cexp // 32 * 128 * 4, Cexpp * 4, Cexp, Csq * 4),

                 **(dict(d32=1, eH=eH, eW=eW, eWop=_ru(eW, 4)) if d32 else {}))
      op_outoff.append(gate_off)
    elif o.t == "ADD":
      C, H, W = shp[o.a][0]
      d32 = o.d32
      n = ((C + 31) // 32) * H * ((W + 3) & ~3) * 32 if d32 else C * H * W
      out_off = alloc_out(o, n if d32 else None)
      ma, mb = o.sa / o.so, o.sb / o.so
      smx, S = max(ma, mb, 1.0), 15
      while smx * (1 << S) >= 32760.0:
        S -= 1
      ra, rb = int(round(ma * (1 << S))), int(round(mb * (1 << S)))
      rec = pack("ADD", op=OP_ADD, out=out_off, a=off[o.a], b=off[o.b], ra=ra, rb=rb, S=S,
                 za=o.za, zb=o.zb, zo=o.zo, qmax=255, n=n)
      op_outoff.append(out_off)
    elif o.t == "SETAIL":
      C, H, W = shp[o.conv][0]
      HW = H * W
      assert C % 32 == 0, f"SETAIL at {o.out}: C={C} is not a multiple of 32 (nchunk=C/32 truncates)"
      d32 = o.d32
      if not d32:
        conv_off, res_off, d32 = _pack_before(o, o.conv), (_pack_before(o, o.res) if o.res else 0), 1
      else:
        conv_off, res_off = _rd(o.conv, d32), (_rd(o.res, d32) if o.res else 0)
      outd32 = o.out_d32
      out_off = alloc_out(o, _ru(C, 32) // 32 * H * _ru(W, 4) * 32 if outd32 else None)
      gate_off = off[o.gate] if o.gate else 0
      hasres = 1 if o.res else 0
      blob_off = put([o.sc / o.so, o.sr / o.so], np.float32)
      rec = pack("ST", op=OP_SETAIL, out=out_off, conv=conv_off, res=res_off, gate=gate_off, blob=blob_off,
                 C=C, HW=HW, zc=o.zc, zr=o.zr, zo=o.zo, relu=int(o.relu), hasres=hasres,
                 has_gate=int(o.has_gate), scratch=scr(C * 8))
      if d32:
        rec[F("ST", "d32")], rec[F("ST", "H")], rec[F("ST", "W")] = 1, H, W
        rec[F("ST", "Wop")], rec[F("ST", "outd32")] = _ru(W, 4), int(outd32)
      op_outoff.append(out_off)
    elif o.t == "HEAD":
      Cc, Hh, Ww = shp[o.conv][0]
      C = Cc
      HW = Hh * Ww
      O = o.gw.shape[0]
      hd32 = o.d32
      out_off = alloc(o.out, shp[o.out])
      conv_off = _rd(o.conv, hd32)
      gate_off = off[o.gate] if o.gate else 0
      res_off = _rd(o.res, hd32) if o.res else 0
      hasres = 1 if o.res else 0
      P = (o.bn_scale / HW).astype(np.float32)
      Q = o.bn_shift.astype(np.float32)
      gws = o.gws.astype(np.float32)
      gb = (o.gb if o.gb is not None else np.zeros(O)).astype(np.float32)
      gwsum = o.gw.reshape(O, C).astype(np.float64).sum(axis=1).astype(np.float32)
      blob_off = put(np.concatenate([[o.scc / 1.0, o.sr, 1.0 / o.s_bn, float(o.z_bn), o.s_bn], P, Q, gws, gb, gwsum]), np.float32)
      gwp = np.zeros((O, _ru(C, 128)), np.int8); gwp[:, :C] = o.gw.reshape(O, C)
      gw_off = put(gwp, np.int8)
      rec = pack("HD", op=OP_HEAD, out=out_off, conv=conv_off, res=res_off, gate=gate_off, blob=blob_off,
                 gw=gw_off, C=C, HW=HW, zcc=o.zcc, zr=o.zr, hasres=hasres, O=O, relu=int(o.relu),
                 has_gate=int(o.has_gate),
                 scratch=scr(C * 4) + (scr(C * 4) * 2 if not hd32 else scr(C)))
      if hd32:
        rec[F("HD", "d32")], rec[F("HD", "H")], rec[F("HD", "W")], rec[F("HD", "Wop")] = 1, Hh, Ww, _ru(Ww, 4)
      op_outoff.append(out_off)
    recs.append(rec)
  out_name = ops[-1].out
  if seed_is_d32:
    sC, sH, sW = shp[seed_name][0]
    prec = pack("PK", op=OP_PACK, out=seed_d32_off, src=off[seed_name], W=sW, H=sH, Cigp=_ru(sC, 32), Wop=_ru(sW, 4))
    recs.insert(0, prec)
    op_outoff.insert(0, seed_d32_off)
  return dict(
    ops=np.array(recs, np.int32),
    wts=bytes(blob),
    arena_size=cur,
    alloc_sizes=sizes,
    alloc_order=order,
    seed=(off[seed_name], shp[seed_name][0]),
    out=(off[out_name], int(np.prod(shp[out_name][0]))),
    out_f32=ops[-1].t == "HEAD",

    op_outoff=op_outoff,
    op_list=ops,
  )
