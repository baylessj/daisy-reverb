/**
 * Controls what LEDs should be turned on, and at what rate.
 *
 * The LEDs are used to display the bypass status of the pedal as well as the
 * currently selected preset.
 */
#include "daisy_petal.h"
#include "terrarium.h"

#pragma once

// The duration that the preset LED will be held, high or low, when flashing.
// In milliseconds.
#define LED_FLASH_DURATION 1000

#define NUM_SAVING_FLASHES 4

class LedController {
  public:
  /**
   * hw: A pointer to the Daisy hardware
   * update_interval: the time in milliseconds between calls of the `tick()`
   *                  function.
   */
  LedController(daisy::DaisyPetal* hw, std::uint16_t update_interval)
    : _update_interval(update_interval) {
    _left.Init(hw->seed.GetPin(terrarium::Terrarium::LED_1), false);
    _right.Init(hw->seed.GetPin(terrarium::Terrarium::LED_2), false);
  }

  void tick(bool bypassed, int preset_number, bool saving) {
    if (saving && !_saving) {
      _led_tick_count = 0;
      _num_flashes_this_cycle = 0;
      _setLeft(true);
      _setRight(true);
    }
    _saving = saving;

    if (saving) {
      _savingFlash();
      return;
    }

    if (bypassed) {
      _setLeft(false);
      _setRight(false);
      _led_tick_count = 0;
      _num_flashes_this_cycle = 0;
      return;
    }

    _setLeft(true);

    _presetFlash(preset_number);
  }

  private:
  void _setLeft(bool on) {
    _left.Set(on ? 1.0f : 0.0f);
    _left.Update();
  }

  void _setRight(bool on) {
    _right.Set(on ? 1.0f : 0.0f);
    _right.Update();
    _isRightOn = on;
  }

  std::uint16_t _thisPresetStateDuration(int preset_number) {
    if (_num_flashes_this_cycle == (preset_number + 1) && !_isRightOn) {
      // Keep the LED off for 4 times the normal duration when between flash
      // cycles to designate the end of the cycle
      return LED_FLASH_DURATION * 4;
    }
    return LED_FLASH_DURATION;
  }

  void _flash(std::uint16_t duration, std::uint8_t count) {
    if (_led_tick_count == 0 && _num_flashes_this_cycle == 0) {
      // starting a new round of flashing the preset number
      _setRight(true);
    }

    _led_tick_count++;

    if (_led_tick_count * _update_interval > duration) {
      // held the LED state for the requested duration, change
      _setRight(!_isRightOn);
      _led_tick_count = 0;

      if (!_isRightOn) {
        _num_flashes_this_cycle++;
      }

      if (_num_flashes_this_cycle > count) {
        // LED has flashed the requested number of times, restarting the cycle
        _num_flashes_this_cycle = 0;
      }
    }
  }

  void _presetFlash(int preset_number) {
    auto state_duration = _thisPresetStateDuration(preset_number);
    _flash(state_duration, preset_number + 1);
  }

  void _savingFlash() {
    // Flash twice as fast for the second pair
    auto state_duration =
      (_led_tick_count < 2) ? LED_FLASH_DURATION : LED_FLASH_DURATION / 2;
    _flash(state_duration, NUM_SAVING_FLASHES);
  }

  daisy::Led _left, _right;
  std::uint16_t _update_interval;
  bool _isRightOn = false;
  std::uint16_t _led_tick_count = 0;
  std::uint8_t _num_flashes_this_cycle = 0;
  bool _saving = false;
};