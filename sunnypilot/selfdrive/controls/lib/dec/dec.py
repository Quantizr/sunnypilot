# The MIT License
#
# Copyright (c) 2019-, Rick Lan, dragonpilot community, and a number of other of contributors.
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.
#
# Version = 2025-1-18

import numpy as np

from cereal import messaging
from opendbc.car import structs
from numpy import interp
from openpilot.common.params import Params
from openpilot.common.realtime import DT_MDL
from openpilot.sunnypilot.selfdrive.controls.lib.dec.constants import WMACConstants

# d-e2e, from modeldata.h
TRAJECTORY_SIZE = 33
SET_MODE_TIMEOUT = 10


class WeightedMovingAverageCalculator:
  def __init__(self, window_size):
    self.window_size = window_size
    self.data = []
    self.weights = np.linspace(1, 3, window_size)

  def add_data(self, value: float) -> None:
    if len(self.data) == self.window_size:
      self.data.pop(0)
    self.data.append(value)

  def get_weighted_average(self) -> float | None:
    if len(self.data) == 0:
      return None
    weighted_sum: float = float(np.dot(self.data, self.weights[-len(self.data):]))
    weight_total: float = float(np.sum(self.weights[-len(self.data):]))
    return weighted_sum / weight_total

  def reset_data(self) -> None:
    self.data = []


class DynamicExperimentalController:
  def __init__(self, CP: structs.CarParams, mpc, params=None):
    self._CP = CP
    self._mpc = mpc
    self._params = params or Params()
    self._enabled: bool = self._params.get_bool("DynamicExperimentalControl")
    self._active: bool = False
    self._mode: str = 'acc'
    self._frame: int = 0

    # Use weighted moving average for filtering leads
    self._lead_gmac = WeightedMovingAverageCalculator(window_size=WMACConstants.LEAD_WINDOW_SIZE)
    self._has_lead_filtered = False
    self._has_lead_filtered_prev = False

    self._slow_down_gmac = WeightedMovingAverageCalculator(window_size=WMACConstants.SLOW_DOWN_WINDOW_SIZE)
    self._has_slow_down: bool = False

    self._slowness_gmac = WeightedMovingAverageCalculator(window_size=WMACConstants.SLOWNESS_WINDOW_SIZE)
    self._has_slowness: bool = False

    self._v_ego_kph = 0.
    self._v_cruise_kph = 0.
    self._has_standstill = False
    self._set_mode_timeout = 0

    self._lead_moved_after_stop = False
    self._was_lead_stopped = False

  def _read_params(self) -> None:
    if self._frame % int(1. / DT_MDL) == 0:
      self._enabled = self._params.get_bool("DynamicExperimentalControl")

  def mode(self) -> str:
    return str(self._mode)

  def enabled(self) -> bool:
    return self._enabled

  def active(self) -> bool:
    return self._active

  def _update_calculations(self, sm: messaging.SubMaster) -> None:
    car_state = sm['carState']
    lead_one = sm['radarState'].leadOne
    md = sm['modelV2']

    self._v_ego_kph = car_state.vEgo * 3.6
    self._v_cruise_kph = car_state.vCruise
    self._has_standstill = car_state.standstill

    # lead detection with smoothing
    self._lead_gmac.add_data(lead_one.status)
    self._has_lead_filtered = (self._lead_gmac.get_weighted_average() or -1.) > WMACConstants.LEAD_PROB

    # slow down detection
    slow_down_threshold = float(
      interp(self._v_ego_kph, WMACConstants.SLOW_DOWN_BP, WMACConstants.SLOW_DOWN_DIST)
    )
    slow_down_trigger = len(md.orientation.x) == len(md.position.x) == TRAJECTORY_SIZE and md.position.x[TRAJECTORY_SIZE - 1] < slow_down_threshold
    self._slow_down_gmac.add_data(slow_down_trigger)
    self._has_slow_down = (self._slow_down_gmac.get_weighted_average() or 0) > WMACConstants.SLOW_DOWN_PROB

    # slowness detection
    if not self._has_standstill:
      self._slowness_gmac.add_data(self._v_ego_kph <= (self._v_cruise_kph * WMACConstants.SLOWNESS_CRUISE_OFFSET))
      self._has_slowness = (self._slowness_gmac.get_weighted_average() or 0) > WMACConstants.SLOWNESS_PROB

    # keep prev value for lead filtered
    self._has_lead_filtered_prev = self._has_lead_filtered

    #detect lead movement after standstill
    if lead_one.status:
      lead_v = lead_one.vLead  # lead speed in m/s
      self._lead_moved_after_stop = self._has_standstill and self._was_lead_stopped and lead_v > 0.3
      self._was_lead_stopped = lead_v < 0.1
    else:
      self._lead_moved_after_stop = False
      self._was_lead_stopped = False

  def _radarless_mode(self) -> None:
    # when standstill: blended
    if self._has_standstill:
      self._set_mode('blended')
      return

    # when detecting slow down scenario: blended
    if self._has_slow_down:
      self._set_mode('blended')
      return

    # car driving at speed lower than set speed: acc
    if self._has_slowness:
      self._set_mode('acc')
      return

    self._set_mode('acc')

  def _radar_mode(self) -> None:
    # If there is a filtered lead and the vehicle is not in standstill
    if self._has_lead_filtered and not self._has_standstill:
      self._set_mode('acc')
      return

    # when standstill: blended
    if self._has_standstill:
      self._set_mode('blended')
      return

    # when detecting slow down scenario: blended
    if self._has_slow_down:
      self._set_mode('blended')
      return

    # car driving at speed lower than set speed: acc
    if self._has_slowness:
      self._set_mode('acc')
      return

    self._set_mode('acc')

  def _set_mode(self, mode: str) -> None:
    # override blended if lead moved
    if self._lead_moved_after_stop and mode == 'blended':
      self._mode = 'acc'
      self._set_mode_timeout = 0
      return

    if self._set_mode_timeout == 0:
      self._mode = mode
      if mode == 'blended':
        self._set_mode_timeout = SET_MODE_TIMEOUT

    if self._set_mode_timeout > 0:
      self._set_mode_timeout -= 1

  def update(self, sm: messaging.SubMaster) -> None:
    self._read_params()
    self._update_calculations(sm)

    if self._CP.radarUnavailable:
      self._radarless_mode()
    else:
      self._radar_mode()

    self._active = sm['selfdriveState'].experimentalMode and self._enabled
    self._frame += 1
