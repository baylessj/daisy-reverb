#pragma once

#include <array>
#include <memory>

#include "../allocator.hpp"
#include "../constants.h"
#include "Utils.h"
#include "audiolib/sharandom.h"

#define MAX_DIFFUSER_TAPS ((size_t)50)

namespace cloudSeed {
class MultitapDiffuser {
  private:
  float* _buffer;
  float* _output;
  size_t _delay_buffer_size;

  int _buffer_index;

  float* _tap_gains;
  float* _tap_positions;
  float* _tap_lengths;
  float* _seed_values;

  int _seed;

  size_t ktap_count = 1;
  float ktap_length = 1;
  float kgain;
  float kdecay;

  public:
  MultitapDiffuser(int delay_buffer_size)
    : _delay_buffer_size(delay_buffer_size) {
    _buffer = sdramAllocate<float>(delay_buffer_size);
    _output = sdramAllocate<float>(BATCH_SIZE);

    _tap_gains = sdramAllocate<float>(MAX_DIFFUSER_TAPS);
    _tap_positions = sdramAllocate<float>(MAX_DIFFUSER_TAPS);
    _tap_lengths = sdramAllocate<float>(MAX_DIFFUSER_TAPS);
    _seed_values = sdramAllocate<float>(MAX_DIFFUSER_TAPS * 2);

    _buffer_index = 0;
    kgain = 1.0;
    kdecay = 0.0;
    updateSeeds();
  }

  ~MultitapDiffuser() {
    delete _buffer;
    delete _output;
  }

  void setSeed(int seed) {
    _seed = seed;
    updateSeeds();
  }

  float* getOutput() {
    return _output;
  }

  void setTapCount(int tap_count) {
    ktap_count = (tap_count >= 1) ? tap_count : 1;
    updateTaps();
  }

  void setTapLength(int tap_length) {
    ktap_length = tap_length;
    updateTaps();
  }

  void setTapDecay(float tap_decay) {
    kdecay = tap_decay;
    updateTaps();
  }

  void setTapGain(float tap_gain) {
    kgain = tap_gain;
    updateTaps();
  }

  float* tick(float* input) {
    // TODO(baylessj): might be possible to clean up this input buffer
    // situation, not sure if both the input and output buffers are necessary
    for (int i = 0; i < BATCH_SIZE; i++) {
      if (_buffer_index < 0)
        _buffer_index += _delay_buffer_size;
      _buffer[_buffer_index] = input[i];
      _output[i] = 0.0;

      for (size_t j = 0; j < ktap_count; j++) {
        auto tap_index =
          (int)fmod(_buffer_index + _tap_positions[j], _delay_buffer_size);
        _output[i] += _buffer[tap_index] * _tap_gains[j];
      }

      _buffer_index--;
    }
    return _output;
  }

  void clearBuffers() {
    memset(_buffer, 0.0f, _delay_buffer_size);
    memset(_output, 0.0f, _delay_buffer_size);
  }

  private:
  void updateTaps() {
    int s = 0;
    auto rand = [&]() { return _seed_values[s++]; };

    if (ktap_length < ktap_count)
      ktap_length = ktap_count;

    // used to adjust the volume of the overall output as it grows when we add
    // more taps
    float tap_count_factor =
      1.0 / (1 + std::sqrt(ktap_count / MAX_DIFFUSER_TAPS));

    // randomly space out the taps
    auto sum_lengths = 0.0;
    for (size_t i = 0; i < ktap_count; i++) {
      auto val = 0.1 + rand();
      _tap_lengths[i] = val;
      sum_lengths += val;
    }

    auto scale_length = ktap_length / sum_lengths;
    _tap_positions[0] = 0;
    for (size_t i = 1; i < ktap_count; i++) {
      // normalize the randomize tap distance around `ktap_length`
      _tap_positions[i] =
        _tap_positions[i - 1] + (int)(_tap_lengths[i] * scale_length);
    }

    float last_tap_pos = _tap_positions[ktap_count - 1];
    for (size_t i = 0; i < ktap_count; i++) {
      // when decay set to 0, there is no decay, when set to 1, the gain at the
      // last sample is 0.01 = -40dB
      auto g = std::pow(
        10, -kdecay * 2 * _tap_positions[i] / (float)(last_tap_pos + 1));

      auto tap = (2 * rand() - 1) * tap_count_factor;
      _tap_gains[i] = tap * g * kgain;
    }

    // Set the tap vs. clean mix
    _tap_gains[0] = (1 - kgain);
  }

  void updateSeeds() {
    // generate two sets of seeds, one for the tap lengths, one for the tap
    // gains
    audioLib::sharandom::generate(
      (long long)_seed, MAX_DIFFUSER_TAPS * 2, _seed_values);
    updateTaps();
  }
};
} // namespace cloudSeed
