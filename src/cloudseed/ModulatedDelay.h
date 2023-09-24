#pragma once

#include <stdint.h>

#include "../allocator.hpp"
#include "../constants.h"

#include "ModulatedDelay.h"
#include "Utils.h"

// Number of samples before updating the modulation
#define DELAY_MODULATION_UPDATE_RATE 8

namespace cloudSeed {
class ModulatedDelay {
  public:
  int ksample_delay;

  float kmod_amount;
  float kmod_rate;

  /**
   * Params:
   * MCU_CLOCK_RATE: the sample rate of the program in Hz
   * max_sample_delay: the length of the delay buffer in seconds
   */
  ModulatedDelay(size_t max_sample_delay) {
    _delay_buffer_size_samples = MCU_CLOCK_RATE * max_sample_delay;
    _delay_buffer = sdramAllocate<float>(_delay_buffer_size_samples);
    _output = sdramAllocate<float>(BATCH_SIZE);

    _write_index = 0;
    _mod_phase = 0.01 + 0.98 * (std::rand() / (float)RAND_MAX);

    ksample_delay = max_sample_delay;
    kmod_rate = 0.0;
    kmod_amount = 0.0;

    modulate();
  }

  float* getOutput() {
    return _output;
  }

  float* tick(float* input) {
    for (int i = 0; i < BATCH_SIZE; i++) {
      if (_samples_processed == DELAY_MODULATION_UPDATE_RATE) {
        modulate();
        _samples_processed = 0;
      }

      _write(input[i]);

      _output[i] = _read();

      _samples_processed++;
    }
    return _output;
  }

  void clearBuffers() {
    memset(_delay_buffer, 0.0f, _delay_buffer_size_samples);
    memset(_output, 0.0f, BATCH_SIZE);
  }

  private:
  float* _delay_buffer;
  float* _output;
  size_t _write_index;
  size_t _read_index_a;
  size_t _read_index_b;
  int _samples_processed;
  size_t _delay_buffer_size_samples;

  float _mod_phase;
  float _gain_a;
  float _gain_b;

  void _write(float input) {
    _delay_buffer[_write_index] = input;
    _write_index++;
    if (_write_index >= _delay_buffer_size_samples)
      _write_index -= _delay_buffer_size_samples;
  }

  float _read() {
    auto output = _delay_buffer[_read_index_a] * _gain_a +
                  _delay_buffer[_read_index_b] * _gain_a;

    _read_index_a++;
    _read_index_b++;
    if (_read_index_a >= _delay_buffer_size_samples)
      _read_index_a -= _delay_buffer_size_samples;
    if (_read_index_b >= _delay_buffer_size_samples)
      _read_index_b -= _delay_buffer_size_samples;

    return output;
  }

  void modulate() {
    _mod_phase += kmod_rate * DELAY_MODULATION_UPDATE_RATE;
    if (_mod_phase > 1)
      _mod_phase = std::fmod(_mod_phase, 1.0);

    auto mod = sinf(_mod_phase);
    auto total_delay = ksample_delay + kmod_amount * mod;

    // truncate the delay amount to get these two sample locations
    auto delay_a = (int)total_delay;
    auto delay_b = (int)total_delay + 1;

    auto partial = total_delay - delay_a;

    _gain_a = 1 - partial;
    _gain_b = partial;

    _read_index_a = _write_index - delay_a;
    _read_index_b = _write_index - delay_b;
    if (_read_index_a < 0)
      _read_index_a += _delay_buffer_size_samples;
    if (_read_index_b < 0)
      _read_index_b += _delay_buffer_size_samples;

    _samples_processed = 0;
  }
};
} // namespace cloudSeed