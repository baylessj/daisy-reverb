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

class LedController {
  public:
  /**
   * hw: A pointer to the Daisy hardware
   * update_interval: the time in milliseconds between calls of the `tick()`
   *                  function.
   */
  LedController(std::uint8_t update_interval)
    : _update_interval(update_interval) {}

  void init(daisy::DaisyPetal* hw) {
    _left.Init(hw->seed.GetPin(terrarium::Terrarium::LED_1), false);
    _right.Init(hw->seed.GetPin(terrarium::Terrarium::LED_2), false);
  }

  void tick(bool bypassed, int preset_number) {
    if (bypassed) {
      _setLeft(false);
      _setRight(false);
      _led_tick_count = 0;
      _num_flashes_this_cycle = 0;

      return;
    }

    _setLeft(true);

    if (_led_tick_count == 0 && _num_flashes_this_cycle == 0) {
      // starting a new round of flashing the preset number
      _setRight(true);
    }

    _led_tick_count++;

    auto state_duration = _this_state_duration(preset_number);
    if (_led_tick_count * _update_interval > state_duration) {
      // held the LED state for the requested duration, change
      _setRight(!_isRightOn);
      _led_tick_count = 0;

      if (!_isRightOn) {
        _num_flashes_this_cycle++;
      }

      if (_num_flashes_this_cycle > (preset_number + 1)) {
        // LED has flashed the same number of times as our preset number
        // (1-aligned), starting again
        _num_flashes_this_cycle = 0;
      }
    }
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

  std::uint16_t _this_state_duration(int preset_number) {
    if (_num_flashes_this_cycle == (preset_number + 1) && !_isRightOn) {
      // Keep the LED off for 4 times the normal duration when between flash
      // cycles to designate the end of the cycle
      return LED_FLASH_DURATION * 4;
    }
    return LED_FLASH_DURATION;
  }

  daisy::Led _left, _right;
  std::uint16_t _update_interval;
  bool _isRightOn = false;
  std::uint16_t _led_tick_count = 0;
  std::uint8_t _num_flashes_this_cycle = 0;
};