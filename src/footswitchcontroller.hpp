/**
 * Footswitch utilities.
 *
 * The two Terrarium footswitches are used to:
 * - bypass the pedal (permanently or momentarily)
 * - set the current preset
 * - save the current preset
 * - determine the control selector mode
 */
#pragma once

#include "daisy_petal.h"
#include "terrarium.h"

// How long the left switch can be held before it is considered a Momentary
// Bypass, in milliseconds
#define MOMENTARY_BYPASS_TIME 1000

// How long the right switch can be held before it is considered a hold for
// the Control Selection, and not a tap. In milliseconds.
#define CONTROL_SELECTION_TIME 1000

// How long both buttons should be held for the saving process, in milliseconds.
#define SAVING_TIME 4000

struct FootswitchControllerInfo {
  bool bypassed;
  bool controlSelectionModeActive;
  bool advancePreset;
  bool saving;
  bool save;
};

class FootswitchController {
  public:
  FootswitchController(daisy::DaisyPetal* hw)
    : _left(hw->switches[terrarium::Terrarium::FOOTSWITCH_1]),
      _right(hw->switches[terrarium::Terrarium::FOOTSWITCH_2]) {}

  /**
   * The hardware controller update **MUST** be called prior to this in the
   * event loop.
   */
  FootswitchControllerInfo tick() {
    float both_held_time =
      daisysp::fmin(_left.TimeHeldMs(), _right.TimeHeldMs());

    if (!_bypassed && !_saving && _left.Pressed() && _right.Pressed() &&
        both_held_time < SAVING_TIME) {
      // check that we haven't exceeded the saving time so it doesn't start
      // saving again right after finishing
      _saving = true;
    }

    if (_saving) {
      if (!_left.Pressed() || !_right.Pressed()) {
        // Footswitch released, saving process ended
        _saving = false;
        return FootswitchControllerInfo{
          _bypassed, false, false, _saving, false};
      }

      if (both_held_time > SAVING_TIME) {
        // Completed saving process
        _saving = false;
        return FootswitchControllerInfo{_bypassed, false, false, _saving, true};
      }
    }

    _processBypass();
    auto advancePreset = _processPreset();

    return FootswitchControllerInfo{
      _bypassed, _controlSelection, advancePreset, _saving, false};
  }

  private:
  void _processBypass() {
    if (_left.RisingEdge()) {
      _bypassed = !_bypassed;
    }

    if (!_bypassed && _left.FallingEdge() &&
        _left.TimeHeldMs() > MOMENTARY_BYPASS_TIME) {
      // the footswitch has been held down for longer than our cutoff, consider
      // the effect to have been momentarily turned on and now turn it off.
      _bypassed = true;
    }
  }

  /**
   * Returns: True if the preset should be advanced.
   */
  bool _processPreset() {
    if (_right.RisingEdge()) {
      _controlSelection = true;
    }

    if (_controlSelection && _right.FallingEdge()) {
      _controlSelection = false;

      if (_right.TimeHeldMs() < CONTROL_SELECTION_TIME) {
        // The user didn't hold the switch down long enough, consider it a tap
        // and advance the preset.
        return true;
      }
    }

    return false;
  }

  daisy::Switch _left, _right;

  bool _saving = false;

  bool _bypassed = true;
  bool _controlSelection = false;
};