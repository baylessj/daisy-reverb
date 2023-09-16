/**
 * Knob (potentiometer) Controller.
 *
 * The Terrarium has 6 knobs, and we've defined 4 banks of controls to be
 * affected by these knobs for a total of 24 parameters.
 */
#pragma once

#include "Parameter.h"
#include "daisy_petal.h"
#include "terrarium.h"

#define NUM_KNOBS 6

#define CONTROLS_BASICS 0
#define CONTROLS_DIFFUSION 1
#define CONTROLS_MODULATION 2
#define CONTROLS_EQ 3

static const float UNUSED_VALUE = std::numeric_limits<float>::max();

typedef std::array<float, NUM_KNOBS> KnobVals;
typedef std::array<std::pair<std::uint8_t, float>, NUM_KNOBS> ParamVals;

#define INPUT_MIX ((std::uint8_t)Parameter::Count + 1)
#define EARLY_LATE_MIX ((std::uint8_t)Parameter::Count + 2)
#define UNUSED_PARAM ((std::uint8_t)Parameter::Count + 3)

const std::array<std::uint8_t, NUM_KNOBS> TERRARIUM_KNOBS = {
  terrarium::Terrarium::KNOB_1,
  terrarium::Terrarium::KNOB_2,
  terrarium::Terrarium::KNOB_3,
  terrarium::Terrarium::KNOB_4,
  terrarium::Terrarium::KNOB_5,
  terrarium::Terrarium::KNOB_6,
};

class KnobController {
  public:
  KnobController(daisy::DaisyPetal* hw) {
    for (std::uint8_t i = 0; i < NUM_KNOBS; i++) {
      _knobs[i].Init(
        hw->knob[TERRARIUM_KNOBS[i]], 0.0f, 1.0f, daisy::Parameter::LINEAR);
    }
  }

  ParamVals tick(bool control_selection_toggle, bool control_selection_fsw) {
    std::uint8_t controls_mode =
      (control_selection_toggle << 1) | control_selection_fsw;

    if (controls_mode != _prev_controls_mode) {
      _fill_reset_vals();
      _prev_controls_mode = controls_mode;
      return _empty_params();
    }

    auto changed_vals = _fetch_changed_values();
    switch (controls_mode) {
    case CONTROLS_BASICS:
      return {{
        {INPUT_MIX, changed_vals[0]},
        {(std::uint8_t)Parameter::TapDecay, changed_vals[1]},
        {(std::uint8_t)Parameter::TapCount, _map_value(changed_vals[2], 5.0f)},
        {EARLY_LATE_MIX, changed_vals[3]},
        {(std::uint8_t)Parameter::LineDecay, changed_vals[4]},
        {(std::uint8_t)Parameter::LineCount, _map_value(changed_vals[5], 5.0f)},
      }};
    case CONTROLS_DIFFUSION:
      return {{
        {(std::uint8_t)Parameter::DiffusionDelay, changed_vals[0]},
        {(std::uint8_t)Parameter::DiffusionFeedback, changed_vals[1]},
        {(std::uint8_t)Parameter::DiffusionStages,
         _map_value(changed_vals[2], 5.0f)},
        {(std::uint8_t)Parameter::LateDiffusionDelay, changed_vals[3]},
        {(std::uint8_t)Parameter::LateDiffusionFeedback, changed_vals[4]},
        {(std::uint8_t)Parameter::LateDiffusionStages,
         _map_value(changed_vals[5], 5.0f)},
      }};
    case CONTROLS_MODULATION:
      return {{
        {(std::uint8_t)Parameter::EarlyDiffusionModAmount, changed_vals[0]},
        {(std::uint8_t)Parameter::LineModAmount, changed_vals[1]},
        {(std::uint8_t)Parameter::LateDiffusionModAmount, changed_vals[2]},
        {(std::uint8_t)Parameter::EarlyDiffusionModRate, changed_vals[3]},
        {(std::uint8_t)Parameter::LineModRate, changed_vals[4]},
        {(std::uint8_t)Parameter::LateDiffusionModRate, changed_vals[5]},
      }};
    default: // CONTROLS_EQ
      return {{
        {(std::uint8_t)Parameter::PreDelay, changed_vals[0]},
        {(std::uint8_t)Parameter::PostLowShelfGain, changed_vals[1]},
        {(std::uint8_t)Parameter::PostHighShelfGain, changed_vals[2]},
        {(std::uint8_t)Parameter::HighPass, changed_vals[3]},
        {(std::uint8_t)Parameter::PostLowShelfFrequency, changed_vals[4]},
        {(std::uint8_t)Parameter::PostHighShelfFrequency, changed_vals[5]},
      }};
    }
  }

  private:
  std::array<daisy::Parameter, NUM_KNOBS> _knobs;

  std::uint8_t _prev_controls_mode = CONTROLS_BASICS;
  KnobVals _reset_vals;

  void _fill_reset_vals() {
    for (std::uint8_t i = 0; i < NUM_KNOBS; i++) {
      _reset_vals[i] = _knobs[i].Process();
    }
  }

  bool _float_nearly_equal(float a, float b) {
    return std::abs(a - b) < 0.01;
  }

  float _map_value(float value, float new_max) {
    return value == UNUSED_VALUE ? UNUSED_VALUE : value * new_max;
  }

  ParamVals _empty_params() {
    ParamVals out;
    out.fill({UNUSED_PARAM, UNUSED_VALUE});
    return out;
  }

  KnobVals _fetch_changed_values() {
    KnobVals out;
    out.fill(UNUSED_PARAM);

    for (std::uint8_t i = 0; i < NUM_KNOBS; i++) {
      auto cur_val = _knobs[i].Process();
      if (!_float_nearly_equal(cur_val, _reset_vals[i])) {
        out[i] = cur_val;
      }
    }
    return out;
  }
};