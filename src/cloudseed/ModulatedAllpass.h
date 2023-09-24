#pragma once

#include <cstddef>
#include <cstring>

#include "../allocator.hpp"
#include "../constants.h"

// Number of samples before updating the modulation
#define ALLPASS_MODULATION_UPDATE_RATE 8

namespace cloudSeed {
class ModulatedAllpass {
  public:
  // const int _delay_bufferSamples = 19200; // 100ms at 192Khz

  private:
  float* _delay_buffer;
  float* _output;
  size_t _index;
  unsigned int _samples_processed;
  size_t _delay_buffer_samples;

  float _mod_phase;
  int _delay_a;
  int _delay_b;
  float _gain_a;
  float _gain_b;

  public:
  int ksample_delay;
  float kfeedback;
  float kmod_amount;
  float kmod_rate;

  bool kinterpolation_enabled;
  bool kmodulation_enabled;

  /**
   * Params:
   * sample_delay: in ms
   * max_sample_delay: in ms
   */
  ModulatedAllpass(int sample_delay, size_t max_sample_delay)
    : ksample_delay(sample_delay) {
    (void)max_sample_delay;
    kinterpolation_enabled = true;
    _delay_buffer_samples = (MCU_CLOCK_RATE / 1000.0) * max_sample_delay;
    _delay_buffer = sdramAllocate<float>(_delay_buffer_samples);
    _output = sdramAllocate<float>(BATCH_SIZE);

    _index = _delay_buffer_samples - 1;
    _mod_phase = 0.01 + 0.98 * std::rand() / (float)RAND_MAX;
    kmod_rate = 0.0;
    kmod_amount = 0.0;
    modulate();
  }

  inline float* getOutput() {
    return _output;
  }

  void clearBuffers() {
    memset(_delay_buffer, 0.0f, _delay_buffer_samples);
    memset(_output, 0.0f, BATCH_SIZE);
  }

  void tick(float* input) {
    if (kmodulation_enabled)
      processWithMod(input);
    else
      processNoMod(input);
  }

  private:
  void processNoMod(float* input) {
    size_t delayed_index = _index - ksample_delay;
    if (delayed_index < 0)
      delayed_index += _delay_buffer_samples;

    for (int i = 0; i < BATCH_SIZE; i++) {
      auto bufOut = _delay_buffer[delayed_index];
      auto inVal = input[i] + bufOut * kfeedback;

      _delay_buffer[_index] = inVal;
      _output[i] = bufOut - inVal * kfeedback;

      _index++;
      delayed_index++;
      if (_index >= _delay_buffer_samples)
        _index -= _delay_buffer_samples;
      if (delayed_index >= _delay_buffer_samples)
        delayed_index -= _delay_buffer_samples;
      _samples_processed++;
    }
  }

  void processWithMod(float* input) {
    for (int i = 0; i < BATCH_SIZE; i++) {
      if (_samples_processed >= ALLPASS_MODULATION_UPDATE_RATE)
        modulate();

      float buf_out;

      if (kinterpolation_enabled) {
        int idxA = _index - _delay_a;
        int idxB = _index - _delay_b;
        idxA += _delay_buffer_samples * (idxA < 0); // modulo
        idxB += _delay_buffer_samples * (idxB < 0); // modulo

        buf_out = _delay_buffer[idxA] * _gain_a + _delay_buffer[idxB] * _gain_b;
      } else {
        int idxA = _index - _delay_a;
        idxA += _delay_buffer_samples * (idxA < 0); // modulo
        buf_out = _delay_buffer[idxA];
      }

      auto inVal = input[i] + buf_out * kfeedback;
      _delay_buffer[_index] = inVal;
      _output[i] = buf_out - inVal * kfeedback;

      _index++;
      if (_index >= _delay_buffer_samples)
        _index -= _delay_buffer_samples;
      _samples_processed++;
    }
  }

  inline float get(int delay) {
    int idx = _index - delay;
    if (idx < 0)
      idx += _delay_buffer_samples;

    return _delay_buffer[idx];
  }

  void modulate() {
    _mod_phase += kmod_rate * ALLPASS_MODULATION_UPDATE_RATE;
    if (_mod_phase > 1)
      _mod_phase = std::fmod(_mod_phase, 1.0);

    auto mod = sinf(_mod_phase);
    auto total_delay = ksample_delay + kmod_amount * mod;

    // truncate the delay amount to get these two sample locations
    _delay_a = (int)total_delay;
    _delay_b = (int)total_delay + 1;

    auto partial = total_delay - _delay_a;

    _gain_a = 1 - partial;
    _gain_b = partial;

    _samples_processed = 0;
  }
};
} // namespace cloudSeed
