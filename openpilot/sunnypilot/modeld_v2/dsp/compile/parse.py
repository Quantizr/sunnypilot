"""A quantized ONNX file -> our typed op list. The ONLY module that knows what ONNX is.

    extract(path)      protobuf -> plain node dicts, initializers, and the u8 seed shape
    analyze(prog)      recognise the fusions the megakernel implements -> Fused
    emit(prog)         -> [Conv | SeGate | Setail | Add | Head], the op list everything downstream consumes

The boundary is deliberate: swap the input format and you rewrite this file and nothing else. `lower` speaks
shapes and bytes, never op names. That is also why fusion recognition lives HERE and not with the kernels it
serves -- the rules are written in ONNX vocabulary ("QuantizeLinear", "Sigmoid") and could only ever match an
ONNX graph, whereas the kernel registry is expressed in format-independent terms (kh, kw, stride).

The cut is at the OP LIST, not inside the ONNX handling, because that is where the seam is cleanest: `analyze`
-> `Fused` -> `emit` is the tightest coupling in the pipeline (emit reaches into six of Fused's fields), while
`emit` -> the op list is five dataclasses that nothing downstream can see behind.
"""
from __future__ import annotations

from dataclasses import dataclass
from typing import Any

import functools, pathlib, numpy as np
from tinygrad.uop.ops import Ops, PatternMatcher, UOp, UPat

"""The op list's element types -- what `emit` produces and everything downstream consumes.

Five classes cover all of it (measured over 161 ops in the four gate models). They were plain dicts, which meant
`o.get("res")` returned None both for "this op has no residual" and for "you misspelled the key", and the shape
of an op could only be learned by grepping every consumer.

MUTABLE, not frozen, on purpose: two passes legitimately annotate ops in place -- `_pad32` rewrites conv channel
counts, and `residency.plan` writes the layout decisions (see ANNOTATION FIELDS below). Frozen would force a
`dataclasses.replace` rebuild at both sites for no benefit; we do not need hashability, because putting the op
list in tinygrad UOps is settled as not worth it (a UOp `arg` must be hashable, and the reason to want UOps --
letting tinygrad own allocation -- died on the DSP's per-kernel FastRPC round-trip cost).

`t` is kept as a FIELD rather than replaced by isinstance dispatch, because CONV/DWCONV/INCONV share one class
(identical data, different kernel) so isinstance could not tell them apart anyway."""

@dataclass
class Conv:
  """CONV, DWCONV and INCONV -- ONE class: all three carry exactly the same fields, and `t` selects which KERNEL
  runs, not what the op holds. registry.pick_dw / pick_stem turn `t` plus the shape into a kernel id."""
  t: str
  out: str
  src: str
  name: str
  wname: str
  w_q: Any
  w_scale: Any
  bias_f: Any
  x_scale: float
  x_zp: int
  out_scale: float
  out_zp: int
  kh: int
  kw: int
  sh: int
  sw: int
  ph: int
  pw: int
  groups: int
  in_d32: int = 0
  out_d32: int = 0

@dataclass
class SeGate:
  """OP_SE_GATE: squeeze (GAP over the expand) -> fc1 -> fc2 -> sigmoid -> a per-channel gate[Cexp].
  Produces `gate`, not `out` -- `out` stays None so the shared "what tensor does this op produce" paths work."""
  expand: str
  gate: str
  sigout: str
  fc1name: str
  fc2name: str
  fc1_w: Any
  fc1_ws: Any
  fc1_b: Any
  fc1_outs: float
  fc2_w: Any
  fc2_ws: Any
  fc2_b: Any
  fc2_ins: float
  s_rm: float
  z_rm: int
  sc_exp: float
  zc_exp: int
  t: str = "SE_GATE"
  out: str | None = None
  d32: int = 0

@dataclass
class Setail:
  """OP_SETAIL: out = clip(round(relu(sc*(conv-zc)*gate + sr*(res-zr))/so) + zo). Also carries the plain residual
  Add (MobileNetV2-style) with has_gate=0. Its INPUT and OUTPUT layouts are independent decisions."""
  out: str
  conv: str
  res: str | None
  gate: str | None
  sc: float
  zc: int
  sr: float
  zr: int
  so: float
  zo: int
  relu: bool
  has_gate: int = 1
  t: str = "SETAIL"
  d32: int = 0
  out_d32: int = 0

@dataclass
class Add:
  """OP_ADD: QLinearAdd, out = clip(round((sa*(a-za)+sb*(b-zb))/so)+zo). Per-tensor scales, so the pixel
  structure is irrelevant and the kernel streams the flat byte range."""
  out: str
  a: str
  b: str
  sa: float
  za: int
  sb: float
  zb: int
  so: float
  zo: int
  t: str = "ADD"
  d32: int = 0

@dataclass
class Head:
  """OP_HEAD: the gated-residual-relu GAP -> BN -> quantize -> Gemm tail. `has_gate` defaulted here rather than
  read with `.get("has_gate", 1)`: it was present on only 3 of the 4 HEAD ops in the gate models, which is
  exactly the kind of inconsistency a dict cannot rule out."""
  out: str
  conv: str
  res: str | None
  gate: str | None
  gw: Any
  gws: Any
  gb: Any
  bn_scale: Any
  bn_shift: Any
  scc: float
  zcc: int
  sr: float
  zr: int
  s_bn: float
  z_bn: float
  has_gate: int = 1
  relu: bool = True
  t: str = "HEAD"
  d32: int = 0

"""The ONNX node graph as UOps, so the megakernel's fusions can be written as PATTERNS instead of walks.

    g = Graph(prog)            nodes -> UOp DAG + the INIT/PROD/CONS lookups every rule needs
    g.node(uop)                a matched UOp -> the node dict it came from
    g.solo(uop)                does this node have exactly ONE consumer? (see below -- rules must ask)

WHY UOp AND NOT A LOCAL MATCHER (decided by prototyping both, 2026-07-24). Every fusion here is "recognise a
small subgraph hanging off this node"; the hard one is the SE tail, which is tree-shaped, has COMMUTATIVE
operands and an OPTIONAL Relu. tinygrad's UPat already does all three -- `src` as a list expands to
itertools.permutations, `allow_any_len` matches a prefix -- so the alternative was writing and maintaining
40-60 lines that reimplement it. The old code unpacked those commutative operands BY HAND, twice.

THE ENCODING, and why it is not obvious:
  * `arg` is the ONNX op NAME. UPat.match compares arg by EQUALITY, so the name has to be the whole arg for
    `UPat(Ops.CUSTOM, arg="Conv")` to work.
  * `tag` is the node index. That is not decoration: UOp dedups structurally, so two `Relu(x)` nodes with the
    same arg and src would be THE SAME OBJECT and the graph would silently collapse. A distinct tag keeps them
    distinct, and a pattern that does not mention `tag` ignores it (both verified against the pinned tinygrad).
  * Initializers and the seed have no producing node -> a leaf UOp tagged with its tensor name.

WHAT UPat CANNOT DO, and must not be faked: **use counts**. `len(consumers) == 1` is a property of the whole
graph, not of the matched subtree, and every fusion here depends on it -- fusing away a node someone else still
reads is a silent wrong answer. So rules call `g.solo(...)` explicitly. A structural match is necessary, never
sufficient.

SCOPE: this UOp view is for MATCHING ONLY. The post-fusion OP LIST is a different graph (it is what the
residency planner annotates), and it deliberately does NOT reuse these UOps -- building a representation to throw it away
is exactly what this campaign keeps deleting."""

class Graph:
  """The parsed ONNX graph, in both forms rules need: a UOp DAG to match against, and the name-keyed lookups
  (initializers, producers, consumers) that the match itself cannot express."""

  def __init__(self, prog):
    self.nodes = prog["nodes"]
    self.INIT = prog["init"]
    self.PROD = {o: n for n in self.nodes for o in n["out"]}
    self.CONS: dict[str, list] = {}
    for n in self.nodes:
      for i in n["in"]:
        self.CONS.setdefault(i, []).append(n)
    self.consts = {n["out"][0]: n["attr"]["value"] for n in self.nodes if n["op"] == "Constant"}
    self.uops: dict[str, UOp] = {}
    self._of: dict[UOp, dict] = {}
    for i, n in enumerate(self.nodes):
      src = tuple(self.uops[t] if t in self.uops else self._leaf(t) for t in n["in"])
      u = UOp(Ops.CUSTOM, arg=n["op"], src=src, tag=i)
      self._of[u] = n
      for t in n["out"]:
        self.uops[t] = u

  def _leaf(self, tensor: str) -> UOp:
    """An initializer, the seed, or an empty optional input -- no producer, so it is a graph leaf. Tagged with
    its NAME so two different initializers do not dedup into one UOp."""
    if tensor not in self.uops:
      self.uops[tensor] = UOp(Ops.CUSTOM, arg="__leaf__", tag=tensor)
    return self.uops[tensor]

  def node(self, u: UOp) -> dict:
    """The node dict behind a matched UOp -- its inputs/outputs/attrs, which the UOp itself does not carry."""
    return self._of[u]

  def out(self, u: UOp) -> str:
    return self._of[u]["out"][0]

  def cons(self, u: UOp) -> list:
    """The consumer NODE DICTS of a matched UOp's output (not UOps -- a rule that wants the tensor name reads
    `c["out"][0]`)."""
    return self.CONS.get(self.out(u), [])

  def solo(self, u: UOp) -> bool:
    """Exactly one consumer -- the precondition for folding a node away. NOT expressible as a pattern, so every
    rule that consumes an intermediate must ask. See the module docstring."""
    return len(self.cons(u)) == 1

  def producer(self, node, i=0):
    """The node producing `node`'s i-th input, or None if that input is an initializer, a graph input, or absent."""
    return self.PROD.get(node["in"][i]) if i < len(node["in"]) else None

  def only_consumer(self, node, want=None):
    """`node`'s SOLE consumer, optionally required to be op type `want`. None if there are 0 or 2+, or the type
    does not match -- the single-consumer test every fold here depends on, made explicit."""
    c = self.CONS.get(node["out"][0], [])
    return c[0] if len(c) == 1 and (want is None or c[0]["op"] == want) else None

  def chain(self, node, *ops):
    """Follow a linear chain of single-consumer nodes of the given op types, returning the LAST one. None if any
    link is missing, has the wrong type, or has more than one consumer."""
    for want in ops:
      if (node := self.only_consumer(node, want)) is None:
        return None
    return node

  def back(self, node, *ops):
    """Same, walking BACKWARD through producers: `back(gap, "DequantizeLinear", "QuantizeLinear", "Conv")`."""
    for want in ops:
      if (node := self.producer(node)) is None or node["op"] != want:
        return None
    return node

  def init(self, node, i):
    """`node`'s i-th input as an initializer array, or None if absent -- the unchecked `INIT[x["in"][i]]` that
    turned a missing optional input into an IndexError."""
    return self.INIT.get(node["in"][i]) if i < len(node["in"]) else None

  def sc(self, name: str) -> float:
    return float(np.asarray(self.INIT[name]).reshape(-1)[0])

  def zp(self, ins: list) -> int:
    """A Q/DQ node's zero point: its 3rd input, or 0 when omitted."""
    return int(np.asarray(self.INIT[ins[2]]).reshape(-1)[0]) if len(ins) > 2 else 0

  def qparams(self, u: UOp) -> tuple[float, int]:
    """(scale, zero_point) of a matched QuantizeLinear/DequantizeLinear."""
    ins = self._of[u]["in"]
    return self.sc(ins[1]), self.zp(ins)

"""The megakernel's ONNX fusions, as declarative patterns.

Each entry is `(UPat, fn)`: the UPat says what the subgraph LOOKS like, `fn` decides whether it is really
foldable (use counts, weight shapes -- things a pattern cannot see) and returns what to record, or None to fall
through to the next rule. `PatternMatcher.rewrite` gives "first non-None wins" for free.

This replaces four hand-written walks that were the same boilerplate over and over:

    cons = CONS.get(co, [])
    if len(cons) != 1 or cons[0]["op"] != "QuantizeLinear": continue
    q1 = cons[0]
    dq1c = CONS.get(q1["out"][0], [])
    if len(dq1c) != 1 or dq1c[0]["op"] != "DequantizeLinear": continue
    ...

Adding an architecture's fusion is now one rule here instead of a walk in the parser, a table in its return
tuple, and an elif in emit. See the GRAPH section for why the UOp encoding looks the way it does."""

Q, DQ = "QuantizeLinear", "DequantizeLinear"

def N(op: str, *src: UPat, name: str | None = None) -> UPat:
  """One ONNX node. `src` constrains a PREFIX of the inputs (allow_any_len), so a Q/DQ pattern can pin its data
  input and stay silent about the scale/zp initializers that follow it."""
  return UPat(Ops.CUSTOM, arg=op, src=src or None, name=name, allow_any_len=True)

def Nc(op: str, src: list[UPat], name: str | None = None) -> UPat:
  """A node with COMMUTATIVE operands -- a list src makes UPat try both orders. This is the whole reason the SE
  tail is 6 lines here and was ~45 before: Mul(DQ(conv), Sigmoid(gate)) and Add(gated, DQ(res)) each had to be
  unpacked by hand, with an if/elif on which operand was which."""
  return UPat(Ops.CUSTOM, arg=op, src=src, name=name, allow_any_len=True)

CONV_RELU_Q = N(Q, N("Relu", N(DQ, N(Q, N("Conv", name="cv"), name="q1"), name="dq1"), name="relu"), name="q2")

def conv_relu_q(ctx, cv, q1, dq1, relu, q2):
  """`ctx` is the Graph -- that name is fixed by PatternMatcher (UPat even forbids a capture called "ctx").

  The chain only collapses if each link has exactly ONE consumer; otherwise something else still reads the
  float intermediate we are about to delete."""
  if not all(ctx.solo(u) for u in (cv, q1, dq1, relu)):
    return None
  s1, z1 = ctx.qparams(q1)
  s2, z2 = ctx.qparams(q2)
  skip = [ctx.out(q1), ctx.out(dq1), ctx.out(relu), ctx.out(q2)]
  if len(c := ctx.cons(q2)) == 1 and c[0]["op"] == DQ:
    nxt = ctx.CONS.get(c[0]["out"][0], [])
    if nxt and all(x["op"] == "Conv" for x in nxt):
      skip.append(c[0]["out"][0])
  return dict(kind="conv_u8", conv=ctx.out(cv), s1=s1, z1=z1, s2=s2, z2=z2, relu=True, store=ctx.out(q2), skip=skip)

_SIGF = N("Sigmoid", name="sig")
_SIGQ = N(DQ, N(Q, N("Sigmoid", name="sig")))
_GATED = Nc("Mul", [N(DQ, N(Q, N("Conv", name="cv"), name="qconv"), name="convdq"), _SIGF], name="mul")
_GATEDQ = Nc("Mul", [N(DQ, N(Q, N("Conv", name="cv"), name="qconv"), name="convdq"), _SIGQ], name="mul")
_ADDED = Nc("Add", [_GATED, N(DQ, name="resdq")], name="add")
SE_TAIL_RELU = N(Q, N("Relu", _ADDED, name="relu"), name="qo")
SE_TAIL = N(Q, _ADDED, name="qo")
SE_PLAIN_RELU = N(Q, N("Relu", _GATED, name="relu"), name="qo")
SE_PLAIN = N(Q, _GATED, name="qo")

def _se_tail(ctx, cv, qconv, convdq, sig, mul, add, resdq, qo, relu=None):
  if not all(ctx.solo(u) for u in (cv, qconv, mul, add)):
    return None
  if relu is not None and not ctx.solo(relu):
    return None
  sc_c, zc_c = ctx.qparams(qconv)
  sr, zr = ctx.qparams(resdq)
  so, zo = ctx.qparams(qo)
  gapq = [r["out"][0] for r in ctx.cons(convdq) if r["op"] == "ReduceMean"]
  return dict(kind="se_tail", mul=ctx.out(mul), conv=ctx.out(qconv), sc=sc_c, zc=zc_c,
              res=ctx.node(resdq)["in"][0], sr=sr, zr=zr, gate=ctx.out(sig), so=so, zo=zo,
              relu=relu is not None, store=ctx.out(qo), cv=ctx.out(cv), gapq=gapq,
              skip=[ctx.out(qconv), ctx.out(convdq), ctx.out(add), ctx.out(qo), ctx.out(resdq)]
                   + ([ctx.out(relu)] if relu is not None else []))

def _se_plain(ctx, cv, qconv, convdq, sig, mul, qo, relu=None):
  """SE gate with no residual. Same emitted op as _se_tail with res=None -> hasres=0; the res scale/zp are unused
  by the kernel in that case, so they are filled with the identity (1.0, 0) rather than left absent."""
  if not all(ctx.solo(u) for u in (cv, qconv, mul)):
    return None
  if relu is not None and not ctx.solo(relu):
    return None
  sc_c, zc_c = ctx.qparams(qconv)
  so, zo = ctx.qparams(qo)
  gapq = [r["out"][0] for r in ctx.cons(convdq) if r["op"] == "ReduceMean"]
  return dict(kind="se_tail", mul=ctx.out(mul), conv=ctx.out(qconv), sc=sc_c, zc=zc_c,
              res=None, sr=0.0, zr=0, gate=ctx.out(sig), so=so, zo=zo,
              relu=relu is not None, store=ctx.out(qo), cv=ctx.out(cv), gapq=gapq,
              skip=[ctx.out(qconv), ctx.out(convdq), ctx.out(qo)] + ([ctx.out(relu)] if relu is not None else []))

RULES = [
  (CONV_RELU_Q, conv_relu_q),
  (SE_TAIL_RELU, _se_tail),
  (SE_TAIL, _se_tail),
  (SE_PLAIN_RELU, _se_plain),
  (SE_PLAIN, _se_plain),
  (N(Q, N("Relu", _GATEDQ, name="relu"), name="qo"), _se_plain),
  (N(Q, _GATEDQ, name="qo"), _se_plain),
]

"""QDQ int8 ONNX -> plain-numpy graph and megakernel op analysis. Build time only.

    extract(onnx_path)   Parses the quantized ONNX into a picklable dict of nodes, initializers and the shape
                         of the u8 seed (the graph input no node produces).
    analyze(prog)        Recognises the megakernel's fusions -- fused conv->u8, the SE gate/tail, dequant
                         elision -- and returns them as a `Fused` for emit's ordered pass.

The recognition itself is declarative: the FUSIONS section holds one (UPat, fn) rule each, over the UOp view of
the graph built above. This part only parses and drives."""

@functools.lru_cache(maxsize=8)
def _extract_cached(onnx_path, _stamp):
  return _extract(onnx_path)

def extract(onnx_path):
  """Memoised on (path, mtime, size): build() parses, and build_to_pkl parses AGAIN for the seed shape -- 4.4s of
  distill's 86s spent re-reading a file we just read. The stamp means an edited ONNX still re-parses."""
  st = pathlib.Path(onnx_path).stat()
  return _extract_cached(str(onnx_path), (st.st_mtime_ns, st.st_size))

def _extract(onnx_path):
  import pathlib
  from tinygrad import Tensor
  from tinygrad.nn.onnx import OnnxPBParser

  def _np(v):
    return v.numpy() if isinstance(v, Tensor) else v

  g = OnnxPBParser(pathlib.Path(onnx_path)).parse()["graph"]
  INIT = {i["name"]: _np(i["parsed_tensor"]) for i in g["initializer"]}
  produced = {o for n in g["node"] for o in n["parsed_node"].outputs} | set(INIT)
  vi = next(i for i in g["input"] if i["name"] not in produced)
  seed_shape = tuple(d if isinstance(d, int) and d > 0 else 1 for d in vi["parsed_type"].shape)
  nodes = []
  for n in g["node"]:
    pn = n["parsed_node"]
    attr = {}
    for name, val in pn.opts.items():
      if isinstance(val, Tensor):
        attr[name] = val.numpy()
      elif isinstance(val, (list, tuple)):
        attr[name] = list(val)
      else:
        attr[name] = val
    nodes.append({"op": pn.op, "in": list(pn.inputs), "out": list(pn.outputs), "attr": attr})
  ident = {n["out"][0]: n["in"][0] for n in nodes if n["op"] == "Identity"}
  if ident:
    for dst, src in ident.items():
      while src in ident:
        src = ident[src]
      if src in INIT:
        INIT[dst] = INIT[src]
      ident[dst] = src
    nodes = [n for n in nodes if n["op"] != "Identity"]
    for n in nodes:
      n["in"] = [ident.get(i, i) if i not in INIT else i for i in n["in"]]
  return {"nodes": nodes, "init": INIT, "output": g["output"][0]["name"], "path": str(onnx_path),
          "seed_shape": seed_shape[1:] if len(seed_shape) == 4 else seed_shape}

@dataclass
class Fused:
  """What the rules recognised, in the form the ordered emit pass asks for it.

  Recognition and emission are necessarily TWO PASSES: a rule fires at the ROOT of its subgraph (the final
  QuantizeLinear), but the ops have to be emitted in node order, and a conv's op is emitted when the CONV is
  reached -- using quant parameters only discovered at the Q downstream of it. So these are not leftover side
  tables; they are the recognition result, indexed the way the second pass looks things up. What did go is the
  six-way tuple that used to be unpacked and membership-tested at four call sites."""
  fuse: dict
  u8q: dict
  skip: set
  tail: dict
  convq: dict
  squeeze: dict
  gate_of: dict

  def outq(self, conv_out):
    """(out_scale, out_zp, store) for a conv whose output a fusion routed to u8, else None. Merges what used to
    be two membership tests in a row. A fused conv's zp is 0 by construction -- its output went through a relu."""
    if conv_out in self.fuse:
      f = self.fuse[conv_out]
      return f["s2"], 0, f["store"]
    return self.convq.get(conv_out)

def analyze(prog) -> Fused:
  """Recognise the megakernel's fusions.

  The recognition itself lives in the FUSIONS section as (UPat, fn) rules; this is only the driver that walks
  nodes in order and files each match. It replaces four hand-written chain walks -- see that section's header.

  ★ A GAP-commute walk (ReduceMean(Conv1x1) -> Conv1x1(GAP)) was DELETED here on 2026-07-24 rather than ported.
  Its `gap` table was returned but read by NOBODY, while the walk still did `skip.update([q, dq, rm])` -- so a
  model that matched it would have had the ReduceMean skipped with nothing emitted in its place: silently
  dropped computation. It stayed harmless only because it never fires on our models (distill's DQ feeds both a
  Mul and the ReduceMean, so its single-consumer test fails). Re-add it as a rule WITH an emitter if the
  optimization is ever wanted."""
    
  g = Graph(prog)
  pm = PatternMatcher(RULES)
  fuse, u8q, skip, setail, convq, gapq = {}, {}, set(), {}, {}, {}
  for n in prog["nodes"]:
    if (r := pm.rewrite(g.uops[n["out"][0]], g)) is None:
      continue
    skip.update(r["skip"])
    if r["kind"] == "conv_u8":
      fuse[r["conv"]] = {k: r[k] for k in ("s1", "z1", "s2", "z2", "relu", "store")}
      u8q[r["store"]] = (r["s2"], r["z2"])
    elif r["kind"] == "se_tail":
      setail[r["mul"]] = {k: r[k] for k in ("conv", "sc", "zc", "res", "sr", "zr", "gate", "so", "zo", "relu", "store")}
      u8q[r["store"]] = (r["so"], r["zo"])
      convq[r["cv"]] = (r["sc"], r["zc"], r["conv"])
      for rm_out in r["gapq"]:
        gapq[rm_out] = (r["conv"], r["sc"], r["zc"])
  for n in prog["nodes"]:
    if n["op"] != "DequantizeLinear" or n["out"][0] in skip:
      continue
    cons = g.CONS.get(n["out"][0], [])
    if cons and all(c["op"] == "Conv" for c in cons) and (n["in"][0] in g.INIT or n["in"][0] in u8q):
      skip.add(n["out"][0])
  gate_of = {u8: (rm, s, z) for rm, (u8, s, z) in gapq.items()}
  return Fused(fuse=fuse, u8q=u8q, skip=skip, tail=setail, convq=convq, squeeze=gapq, gate_of=gate_of)

def conv_meta(node, INIT, PROD):
  """Resolve a Conv's int8 weights + input/bias quant params (mirrors parse_onnx's view of the node)."""
  at = node["attr"]
  sh, sw = at.get("strides", [1, 1])
  pads = at.get("pads", [0, 0, 0, 0])
  groups = at.get("group", 1)
  ph, pw = pads[0], pads[1]
  dqx = PROD[node["in"][0]]
  x_scale = float(INIT[dqx["in"][1]])
  x_zp = int(INIT[dqx["in"][2]]) if len(dqx["in"]) > 2 else 0
  dqw = PROD[node["in"][1]]
  w_q = INIT[dqw["in"][0]]
  w_scale = INIT[dqw["in"][1]].astype(np.float64).reshape(-1)
  bias_f = None
  if len(node["in"]) > 2:
    dqb = PROD.get(node["in"][2])
    bias_f = (
      (INIT[dqb["in"][0]].astype(np.float64) * INIT[dqb["in"][1]].astype(np.float64))
      if dqb and dqb["op"] == "DequantizeLinear"
      else INIT[node["in"][2]].astype(np.float64)
    )
  Cout, Cig, kh, kw = w_q.shape
  return dict(x_scale=x_scale, x_zp=x_zp, w_q=w_q, w_scale=w_scale, bias_f=bias_f, kh=kh, kw=kw, sh=sh, sw=sw,
              ph=ph, pw=pw, groups=groups, src=dqx["in"][0], wname=dqw["in"][0])

def seed_quant(prog):
  """The graph input IS the u8 seed: the backbone is split from the stem on the u8 boundary, so the stem's own
  graph does the QuantizeLinear and hands the seed over quantized. Its (scale, zp) live on the DequantizeLinear
  that consumes it -- returned here for callers that need to interpret the seed's values, not to produce them.
  Returns (scale, zp, seed_tensor_name); the graph input is the tensor produced by no node and not an initializer."""
  g = Graph(prog)
  for nd in g.nodes:
    if nd["op"] == "DequantizeLinear" and nd["in"][0] not in g.PROD and nd["in"][0] not in g.INIT:
      return g.sc(nd["in"][1]), g.zp(nd["in"]), nd["in"][0]
  raise AssertionError("no seed DequantizeLinear on the graph input found (is the backbone split on the u8 boundary?)")

def emit(prog):
  g = Graph(prog)
  nodes, INIT, PROD, CONS, sc, zpf = g.nodes, g.INIT, g.PROD, g.CONS, g.sc, g.zp
  F = analyze(prog)
  ops = []
  emitted_conv = set()
  def conv_out_quant(conv_node):
    return out_quant(conv_node["out"][0])

  def out_quant(co):
    """(out_scale, out_zp, store_name) for a conv output that is Q'd to u8, else None. By TENSOR NAME, because
    _resolve_gate knows fc1 only by the name a fusion recorded."""
    if (q := F.outq(co)) is not None:
      return q
    for c in CONS.get(co, []):
      if c["op"] == "QuantizeLinear":
        return sc(c["in"][1]), zpf(c["in"]), c["out"][0]
    return None

  def emit_conv(conv_node):
    co = conv_node["out"][0]
    if co in emitted_conv:
      return
    m = conv_meta(conv_node, INIT, PROD)
    oq = conv_out_quant(conv_node)
    if oq is None:
      return None
    out_scale, out_zp, store = oq
    is_dw = m["groups"] == m["w_q"].shape[0] and m["w_q"].shape[1] == 1 and m["groups"] > 1 and m["kh"] == m["kw"]
    is_in = (m["groups"] == 1 and m["w_q"].shape[1] <= 4 and m["kh"] == 3 and m["kw"] == 3 and m["sh"] == 2
             and m["w_q"].shape[0] <= 32)
    t = "DWCONV" if is_dw else ("INCONV" if is_in else "CONV")
    ops.append(Conv(t=t, out=store, **m, out_scale=out_scale, out_zp=out_zp, name=co))
    emitted_conv.add(co)
    return store

  done = set()
  for nd in nodes:
    op = nd["op"]
    co = nd["out"][0]
    if co in F.skip or co in done:
      continue
    if op == "Conv":
      if any(co == g.fc1name or co == g.fc2name for g in [o for o in ops if o.t == "SE_GATE"]):
        continue
      emit_conv(nd)
    elif op == "ReduceMean" and _is_se_squeeze(nd, g):
      if co in F.squeeze:
        conv_u8, scq, zcq = F.squeeze[co]
      else:
        dqe = PROD[nd["in"][0]]
        conv_u8, scq, zcq = dqe["in"][0], sc(dqe["in"][1]), zpf(dqe["in"])
      if (gp := _resolve_gate(co, g, out_quant)) is None:
        raise AssertionError(f"SE gate at {co}: squeeze->fc1->fc2->activation chain did not match "
                             f"(supported activations: Sigmoid, HardSigmoid)")
      ops.append(SeGate(expand=conv_u8, sc_exp=scq, zc_exp=zcq, gate=gp["sigout"], **gp))
    elif op == "Mul" and co in F.tail:
      se = F.tail[co]
      ops.append(Setail(out=se["store"], conv=se["conv"], sc=se["sc"], zc=se["zc"], res=se["res"],
                           sr=se["sr"], zr=se["zr"], gate=se["gate"], so=se["so"], zo=se["zo"],
                           relu=se["relu"], has_gate=1))
    elif op == "Add":
      a = _emit_add(nd, PROD, CONS, sc, zpf)
      if a is not None:
        ops.append(a)
    elif op == "GlobalAveragePool":
      h = _emit_head(nd, g)
      if h is None:
        raise AssertionError(f"head at {co}: the GAP -> [BN] -> Q -> DQ -> Gemm tail did not match. A residual "
                             f"head needs Mul(DQ(conv), act) -> Add(DQ(res)) -> Relu in front of the GAP.")
      ops.append(h)
  return ops, F

def _is_se_squeeze(rm, g):
  """ReduceMean -> Q -> DQ -> Conv(fc1) marks an SE squeeze (the gate path), vs the head's GAP."""
  return g.chain(rm, "QuantizeLinear", "DequantizeLinear", "Conv") is not None

def _resolve_gate(rmout, g, outq):
  """The SE gate path: squeeze -> Q -> DQ -> fc1 -> (requant) -> DQ -> fc2 -> ... -> activation.

  Every step is checked, so a graph that does not have this shape returns None instead of raising from six
  frames down. `outq(conv_out)` supplies fc1's output quant -- it used to index `fuse[fc1]` directly, which
  assumed fc1 was a fused conv->relu->Q and KeyError'd on any export where the relu had been folded into the Q
  range instead (onnxruntime does that routinely). The activation is MATCHED rather than spelled "Sigmoid"."""
  ACTS = ("Sigmoid", "HardSigmoid")
  q1 = g.only_consumer({"out": [rmout]}, "QuantizeLinear")
  if q1 is None or (fc1 := g.chain(q1, "DequantizeLinear", "Conv")) is None:
    return None
  if (oq1 := outq(fc1["out"][0])) is None:
    return None
  fc1_outs, _fc1_zp, fc1_store = oq1
  if (fc2 := g.chain({"out": [fc1_store]}, "DequantizeLinear", "Conv")) is None:
    return None
  cur, sig = fc2, None
  for _ in range(4):
    if (cur := g.only_consumer(cur)) is None:
      break
    if cur["op"] in ACTS:
      sig = cur["out"][0]
      break
  if sig is None:
    return None
  m1, m2 = conv_meta(fc1, g.INIT, g.PROD), conv_meta(fc2, g.INIT, g.PROD)
  return dict(s_rm=g.sc(q1["in"][1]), z_rm=g.zp(q1["in"]), fc1name=fc1["out"][0], fc2name=fc2["out"][0],
              sigout=sig, fc1_w=m1["w_q"], fc1_ws=m1["w_scale"], fc1_b=m1["bias_f"], fc1_outs=fc1_outs,
              fc2_w=m2["w_q"], fc2_ws=m2["w_scale"], fc2_b=m2["bias_f"], fc2_ins=fc1_outs)

def _emit_add(add_node, PROD, CONS, sc, zpf):
  """Plain residual Add: DQ(a_u8) + DQ(b_u8) -> Q -> u8. Emitted as OP_SETAIL with gate=1 (has_gate=0), no relu."""
  ins = add_node["in"]
  if len(ins) != 2:
    return None
  pa, pb = PROD.get(ins[0]), PROD.get(ins[1])
  if not (pa and pa["op"] == "DequantizeLinear" and pb and pb["op"] == "DequantizeLinear"):
    return None
  a_u8, sa, za = pa["in"][0], sc(pa["in"][1]), zpf(pa["in"])
  b_u8, sb, zb = pb["in"][0], sc(pb["in"][1]), zpf(pb["in"])
  q = next((c for c in CONS.get(add_node["out"][0], []) if c["op"] == "QuantizeLinear"), None)
  if q is None:
    return None
  return Add(out=q["out"][0], a=a_u8, sa=sa, za=za, b=b_u8, sb=sb, zb=zb, so=sc(q["in"][1]), zo=zpf(q["in"]))

def _emit_head(gapnode, g):
  """The classifier head, in both shapes it arrives in. Returns None if the chain does not match, rather than
  raising from six frames down.

      PLAIN     (MobileNetV2)  conv_u8 -> DQ -> GAP -> Q -> DQ -> Reshape -> Q -> DQ -> Gemm
      RESIDUAL  (distill)      Mul(DQ(conv_u8), act) -> Add(DQ(res)) -> Relu -> GAP -> Flatten -> BN -> Q -> DQ -> Gemm

  These were TWO emitters until 2026-07-26, and merging them was not only about the duplicated Gemm tail. The
  plain one was the last function here never ported to the Graph API: it read raw `PROD[...]`/`CONS[...]` and
  found its Gemm with `next(n for n in nodes if n["op"] == "Gemm")` -- the FIRST Gemm in the whole graph, not the
  one this GAP feeds -- so a two-Gemm model would have silently bound the wrong weights, and any mismatch raised
  KeyError/StopIteration from inside instead of returning None. That is the exact failure mode the total-navigation
  helpers exist to prevent (see the GRAPH section; it is how MobileNetV3 died).

  Both the Flatten/Reshape and the BatchNormalization are OPTIONAL, and so is a second Q/DQ pair before the Gemm
  (MobileNetV2 has one around its Reshape, with the same params -- a Reshape does not change values). So the tail
  walks single consumers through the shape/quant no-ops rather than spelling out one exporter's chain."""
  if (p := g.producer(gapnode)) is None:
    return None
  res, gate, sr, zr = None, None, 0.0, 0
  if p["op"] == "DequantizeLinear":
    conv_u8, scc, zcc = p["in"][0], g.sc(p["in"][1]), g.zp(p["in"])
  elif p["op"] == "Relu":
    if (add := g.producer(p)) is None or add["op"] != "Add":
      return None
    mul = None
    for i in add["in"]:
      if (q := g.PROD.get(i)) is None:
        continue
      if q["op"] == "Mul":
        mul = q
      elif q["op"] == "DequantizeLinear":
        res, sr, zr = q["in"][0], g.sc(q["in"][1]), g.zp(q["in"])
    if mul is None:
      return None
    conv_u8 = scc = zcc = None
    for i in mul["in"]:
      if (q := g.PROD.get(i)) is None:
        continue
      if q["op"] == "DequantizeLinear":
        conv_u8, scc, zcc = q["in"][0], g.sc(q["in"][1]), g.zp(q["in"])
      elif q["op"] in ("Sigmoid", "HardSigmoid"):
        gate = q["out"][0]
    if conv_u8 is None or gate is None:
      return None
  else:
    return None
  cur, bn, qn = gapnode, None, None
  for _ in range(4):
    if (nxt := g.only_consumer(cur)) is None:
      return None
    if nxt["op"] == "QuantizeLinear":
      qn = nxt
      break
    if nxt["op"] == "BatchNormalization":
      bn = nxt
    elif nxt["op"] not in ("Flatten", "Reshape"):
      return None
    cur = nxt
  if qn is None:
    return None
  gemm = qn
  for _ in range(6):
    if (gemm := g.only_consumer(gemm)) is None:
      return None
    if gemm["op"] == "Gemm":
      break
    if gemm["op"] not in ("DequantizeLinear", "QuantizeLinear", "Reshape", "Flatten"):
      return None
  if gemm["op"] != "Gemm":
    return None
  if (wq := g.producer(gemm, 1)) is None or (gw := g.init(wq, 0)) is None:
    return None
  gws = np.asarray(g.init(wq, 1), np.float64).reshape(-1)
  if gws.size == 1:
    gws = np.full(gw.shape[0], gws[0])
  gb = g.init(gemm, 2)
  if gb is not None:
    gb = gb.astype(np.float64)
  elif (dqb := g.producer(gemm, 2)) is not None and (b0 := g.init(dqb, 0)) is not None:
    gb = b0.astype(np.float64) * g.init(dqb, 1).astype(np.float64)
  if bn is not None:
    bn_w, bn_b, bn_mean, bn_var = (g.init(bn, i) for i in (1, 2, 3, 4))
    if any(x is None for x in (bn_w, bn_b, bn_mean, bn_var)):
      return None
    invstd = 1.0 / np.sqrt(bn_var + bn["attr"].get("epsilon", 1e-5))
    bn_scale, bn_shift = bn_w * invstd, bn_b - bn_mean * bn_w * invstd
  else:
    bn_scale, bn_shift = np.ones(gw.shape[1]), np.zeros(gw.shape[1])
  return Head(conv=conv_u8, scc=scc, zcc=zcc, res=res, sr=sr, zr=zr, gate=gate, has_gate=1 if gate else 0,
              relu=bool(gate) or res is not None,
              bn_scale=bn_scale, bn_shift=bn_shift,
              s_bn=g.sc(qn["in"][1]), z_bn=g.zp(qn["in"]), gw=gw, gws=gws, gb=gb, out=gemm["out"][0])

