"""
Copyright (c) 2021-, Haibin Wen, sunnypilot, and a number of other contributors.

This file is part of sunnypilot and is licensed under the MIT License.
See the LICENSE.md file in the root directory for more details.

Vertical bars on both screen edges showing the cabin-camera blind spot model (blindSpotModelSP): each bar shows that side's
detection level (blindspotmodeld's leaky bucket: it fills with the model's evidence for a car and drains linearly, a full bar
in 2 s), with a tick at the detection threshold. The road view faces forward, so the vehicle's left side is the screen's
left. A red glow marks the on/off detection (leftDetected/rightDetected, which also drives the blind spot warning in
turn_signal.py), a thin white outline the car's own blind spot radar (carState), when the car has one. Shown while
"Show Blind Spot Warnings" (BlindSpot param) is on.
"""
import pyray as rl

from openpilot.common.filter_simple import FirstOrderFilter
from openpilot.selfdrive.ui.ui_state import ui_state
from openpilot.system.ui.lib.application import gui_app

# sizes in pixels on the comma 3X screen, scaled by SMALL_SCALE on the comma four (bars 30 px / 10 px wide)
BAR_W = 30
BAR_MARGIN_X = 22         # distance from the screen edge
SMALL_SCALE = 1 / 3
BAR_HEIGHT_FRAC = 0.46    # of the onroad view height
BAR_CENTER_FRAC = 0.515   # vertical centre, as a fraction of the view height (comma 3X: midway between MAX and the DM icon)
THRESHOLD = 0.4          # detection threshold on the level (blindspotmodeld BUCKET_THRESHOLD)
RED_AT = 0.8             # level where the fill reaches full red

TRACK = rl.Color(0, 0, 0, 110)
TICK = rl.Color(255, 255, 255, 170)
LOW = (235, 235, 235)     # level near 0
MID = (255, 176, 32)      # level = threshold
HIGH = (240, 56, 56)      # level >= RED_AT
GLOW = (240, 56, 56)
RADAR = rl.Color(255, 255, 255, 230)


def _lerp(a, b, t):
  return tuple(int(x + (y - x) * t) for x, y in zip(a, b, strict=True))


def _fill_color(level: float, alpha: int) -> rl.Color:
  if level < THRESHOLD:
    c = _lerp(LOW, MID, level / THRESHOLD)
  else:
    c = _lerp(MID, HIGH, min(1.0, (level - THRESHOLD) / (RED_AT - THRESHOLD)))
  return rl.Color(*c, alpha)


def model_blindspot() -> tuple[bool, bool]:
  """the model's (left, right) on/off detection, False when the toggle is off or blindspotmodeld isn't running"""
  sm = ui_state.sm
  if not (ui_state.blindspot and sm.alive['blindSpotModelSP'] and sm.valid['blindSpotModelSP']):
    return False, False
  bs = sm['blindSpotModelSP']
  return bs.leftDetected, bs.rightDetected


class BlindSpotBars:
  def __init__(self):
    dt = 1 / gui_app.target_fps
    self._level = {side: FirstOrderFilter(0.0, 0.1, dt) for side in ('left', 'right')}
    self._detected = {side: FirstOrderFilter(0.0, 0.1, dt) for side in ('left', 'right')}
    self._radar = dict.fromkeys(('left', 'right'), False)
    self._alpha = FirstOrderFilter(0.0, 0.3, dt)

  def update(self) -> None:
    sm = ui_state.sm
    live = ui_state.blindspot and sm.alive['blindSpotModelSP'] and sm.valid['blindSpotModelSP']
    self._alpha.update(1.0 if live else 0.0)
    if live:
      bs = sm['blindSpotModelSP']
      self._level['left'].update(bs.leftLevel)
      self._level['right'].update(bs.rightLevel)
    for side, on in zip(('left', 'right'), model_blindspot(), strict=True):
      self._detected[side].update(1.0 if on else 0.0)
    cs = sm['carState']
    self._radar = {'left': cs.leftBlindspot, 'right': cs.rightBlindspot}

  def render(self, rect: rl.Rectangle) -> None:
    if not ui_state.blindspot or self._alpha.x < 0.02:
      return
    s = 1.0 if gui_app.big_ui() else SMALL_SCALE
    w, margin = BAR_W * s, BAR_MARGIN_X * s
    h = rect.height * BAR_HEIGHT_FRAC
    y = rect.y + rect.height * BAR_CENTER_FRAC - h / 2
    for side, x in (('left', rect.x + margin), ('right', rect.x + rect.width - margin - w)):
      self._draw_bar(rl.Rectangle(x, y, w, h), s, self._level[side].x, self._detected[side].x, self._radar[side])

  def _draw_bar(self, r: rl.Rectangle, s: float, level: float, detected: float, radar: bool) -> None:
    a = self._alpha.x
    level = min(max(level, 0.0), 1.0)
    roundness, seg = 1.0, 12
    if detected > 0.01:  # soft red glow around the whole bar while the detection is on
      for grow, ga in ((14 * s, 40), (8 * s, 70)):
        glow = rl.Rectangle(r.x - grow, r.y - grow, r.width + 2 * grow, r.height + 2 * grow)
        rl.draw_rectangle_rounded(glow, roundness, seg, rl.Color(*GLOW, int(ga * a * detected)))
    rl.draw_rectangle_rounded(r, roundness, seg, rl.Color(TRACK.r, TRACK.g, TRACK.b, int(TRACK.a * a)))
    fill_h = r.height * level
    if fill_h > 1:
      fill = rl.Rectangle(r.x, r.y + r.height - fill_h, r.width, fill_h)
      rl.draw_rectangle_rounded(fill, roundness, seg, _fill_color(level, int(235 * a)))
    ty = int(r.y + r.height * (1 - THRESHOLD))
    rl.draw_line_ex(rl.Vector2(r.x - 4 * s, ty), rl.Vector2(r.x + r.width + 4 * s, ty), max(1.0, 2 * s),
                    rl.Color(TICK.r, TICK.g, TICK.b, int(TICK.a * a)))
    if radar:
      o = 3 * s
      rl.draw_rectangle_rounded_lines_ex(rl.Rectangle(r.x - o, r.y - o, r.width + 2 * o, r.height + 2 * o), roundness, seg, max(1.0, 3 * s),
                                         rl.Color(RADAR.r, RADAR.g, RADAR.b, int(RADAR.a * a)))
