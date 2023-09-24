#pragma once

#include <array>

#include "../allocator.hpp"
#include "../constants.h"
#include "ModulatedAllpass.h"
#include "Utility/dsp.h"
#include "audiolib/sharandom.h"

#define MAX_DIFFUSER_STAGE_COUNT 2

#define ALLPASS_DELAY ((int)100) // delay length in ms

namespace cloudSeed {
class AllpassDiffuser {
  public:
  /**
   * Params:
   * delay_buffer_length: the maximum delay time, in milliseconds
   */
  AllpassDiffuser(size_t delay_buffer_length) {
    _seed_values = sdramAllocate<float>(MAX_DIFFUSER_STAGE_COUNT * 3);
    for (int i = 0; i < MAX_DIFFUSER_STAGE_COUNT; i++) {
      (void)delay_buffer_length;
      _filters[i] = new ModulatedAllpass(ALLPASS_DELAY, delay_buffer_length);
    }

    _seed = 23456;
    updateSeeds();
    _stages = 1;
  }

  ~AllpassDiffuser() {
    for (auto filter : _filters)
      delete filter;
  }

  void setSeed(int seed) {
    _seed = seed;
    updateSeeds();
  }

  void setCrossSeed(float cross_seed) {
    _cross_seed = cross_seed;
    updateSeeds();
  }

  bool getModulationEnabled() {
    return _filters[0]->kmodulation_enabled;
  }

  void setModulationEnabled(bool value) {
    for (auto filter : _filters)
      filter->kmodulation_enabled = value;
  }

  void setInterpolationEnabled(bool enabled) {
    for (auto filter : _filters)
      filter->kinterpolation_enabled = enabled;
  }

  float* getOutput() {
    return _filters[_stages - 1]->getOutput();
  }

  void setDelay(int delay_samples) {
    _delay = delay_samples;
    update();
  }

  void setFeedback(float feedback) {
    for (auto filter : _filters)
      filter->kfeedback = feedback;
  }

  void setModAmount(float amount) {
    for (size_t i = 0; i < _filters.size(); i++) {
      _filters[i]->kmod_amount =
        amount * (0.85 + 0.3 * _seed_values[MAX_DIFFUSER_STAGE_COUNT + i]);
    }
  }

  void setModRate(float rate) {
    _mod_rate = rate;

    for (size_t i = 0; i < _filters.size(); i++)
      _filters[i]->kmod_rate =
        rate * (0.85 + 0.3 * _seed_values[MAX_DIFFUSER_STAGE_COUNT * 2 + i]) /
        _samplerate;
  }

  void setStages(size_t stages) {
    _stages = stages;
  }

  float* tick(float* input) {
    _filters[0]->tick(input);
    for (size_t i = 1; i < _stages; i++) {
      _filters[i]->tick(_filters[i - 1]->getOutput());
    }
    return getOutput();
  }

  void clearBuffers() {
    for (auto filter : _filters)
      filter->clearBuffers();
  }

  private:
  void update() {
    for (size_t i = 0; i < _filters.size(); i++) {
      auto r = _seed_values[i];
      auto d = daisysp::pow10f(r) * 0.1; // 0.1 ... 1.0
      _filters[i]->ksample_delay = (int)(_delay * d);
    }
  }

  void updateSeeds() {
    audioLib::sharandom::generate(
      _seed, MAX_DIFFUSER_STAGE_COUNT * 3, _seed_values);
    update();
  }

  private:
  size_t _samplerate;
  std::array<ModulatedAllpass*, MAX_DIFFUSER_STAGE_COUNT> _filters;
  int _delay;
  float _mod_rate;

  float* _seed_values;
  int _seed;
  float _cross_seed;

  size_t _stages;
};
} // namespace cloudSeed
