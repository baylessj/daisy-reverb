/**
 * Toggle Switch Controller.
 *
 * Processes toggle switch inputs and maps them to program states.
 */
#include "daisy_petal.h"
#include "terrarium.h"

#pragma once

struct ToggleSwitchInfo {
  bool early_diffusion_enabled;
  bool late_diffusion_enabled;
  bool late_diffusion_post_delay;
  bool control_selector_mode;
};

class ToggleSwitchController {
  public:
  ToggleSwitchController(daisy::DaisyPetal* hw)
    : _early_diffusion_toggle(hw->switches[terrarium::Terrarium::SWITCH_1]),
      _late_diffusion_toggle(hw->switches[terrarium::Terrarium::SWITCH_2]),
      _late_diffusion_pre_post(hw->switches[terrarium::Terrarium::SWITCH_3]),
      _control_selector_mode_toggle(
        hw->switches[terrarium::Terrarium::SWITCH_4]) {}

  ToggleSwitchInfo tick() {
    return ToggleSwitchInfo{
      _early_diffusion_toggle.Pressed(),
      _late_diffusion_toggle.Pressed(),
      _late_diffusion_pre_post.Pressed(),
      _control_selector_mode_toggle.Pressed(),
    };
  }

  private:
  daisy::Switch _early_diffusion_toggle;
  daisy::Switch _late_diffusion_toggle;
  daisy::Switch _late_diffusion_pre_post;
  daisy::Switch _control_selector_mode_toggle;
};