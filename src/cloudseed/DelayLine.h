#pragma once

#include "../allocator.hpp"
#include "../constants.h"
#include "Filters/tone.h"

#include "AllpassDiffuser.h"
#include "ModulatedDelay.h"
#include "audiolib/biquad.hpp"

// 2 second buffer, to prevent buffer overflow with modulation and randomness
// added (Which may increase effective delay)
#define MODULATED_DELAY_BUFFER 2

// 150ms buffer, to allow for 100ms + modulation time
#define DIFFUSER_BUFFER_LENGTH 150

static float DEFAULT_DELAY_LINE_LOW_PASS_FREQ = 1000.0f;

namespace cloudSeed {
class DelayLine {
  public:
  bool kdiffuser_enabled;
  bool klow_shelf_enabled;
  bool khigh_shelf_enabled;
  bool kcutoff_enabled;
  bool klate_stage_tap;
  float kfeedback;

  DelayLine()
    : _delay(MODULATED_DELAY_BUFFER),
      _diffuser(DIFFUSER_BUFFER_LENGTH),
      _low_shelf(audioLib::Biquad::FilterType::LowShelf, MCU_CLOCK_RATE),
      _high_shelf(audioLib::Biquad::FilterType::HighShelf, MCU_CLOCK_RATE) {
    _temp_buffer = new float[BATCH_SIZE];
    _mixed_buffer = new float[BATCH_SIZE];
    _filter_output_buffer = new float[BATCH_SIZE];

    _low_shelf.kslope = 1.0;
    _low_shelf.setGainDb(-20);
    _low_shelf.kfrequency = 20;

    _high_shelf.kslope = 1.0;
    _high_shelf.setGainDb(-20);
    _high_shelf.kfrequency = 19000;

    _low_pass.Init(MCU_CLOCK_RATE);
    _low_pass.SetFreq(DEFAULT_DELAY_LINE_LOW_PASS_FREQ);
    _low_shelf.update();
    _high_shelf.update();

    setDiffuserSeed(1);
  }

  ~DelayLine() {
    delete _temp_buffer;
    delete _mixed_buffer;
    delete _filter_output_buffer;
  }

  void setDiffuserSeed(int seed) {
    _diffuser.setSeed(seed);
  }

  void setDelay(int delay_samples) {
    _delay.ksample_delay = delay_samples;
  }

  void setFeedback(float feedback) {
    kfeedback = feedback;
  }

  void setDiffuserDelay(int delaySamples) {
    _diffuser.setDelay(delaySamples);
  }

  void setDiffuserFeedback(float feedb) {
    _diffuser.setFeedback(feedb);
  }

  void setDiffuserStages(int stages) {
    _diffuser.setStages(stages);
  }

  void setLowShelfGain(float gain) {
    _low_shelf.setGain(gain);
  }

  void setLowShelfFrequency(float frequency) {
    _low_shelf.kfrequency = frequency;
    _low_shelf.update();
  }

  void setHighShelfGain(float gain) {
    _high_shelf.setGain(gain);
  }

  void setHighShelfFrequency(float frequency) {
    _high_shelf.kfrequency = frequency;
    _high_shelf.update();
  }

  void setCutoffFrequency(float frequency) {
    _low_pass.SetFreq(frequency);
  }

  void setLineModAmount(float amount) {
    _delay.kmod_amount = amount;
  }

  void setLineModRate(float rate) {
    _delay.kmod_rate = rate;
  }

  void setDiffuserModAmount(float amount) {
    _diffuser.setModulationEnabled(amount > 0.0);
    _diffuser.setModAmount(amount);
  }

  void setDiffuserModRate(float rate) {
    _diffuser.setModRate(rate);
  }

  void setInterpolationEnabled(bool value) {
    _diffuser.setInterpolationEnabled(value);
  }

  float* getOutput() {
    if (klate_stage_tap) {
      if (kdiffuser_enabled)
        return _diffuser.getOutput();
      else
        return _mixed_buffer;
    } else {
      return _delay.getOutput();
    }
  }

  void tick(float* input) {
    for (int i = 0; i < BATCH_SIZE; i++)
      _mixed_buffer[i] = input[i] + _filter_output_buffer[i] * kfeedback;

    if (klate_stage_tap) {
      if (kdiffuser_enabled) {
        auto diffuser_out = _diffuser.tick(_mixed_buffer);
        _delay.tick(diffuser_out);
      } else {
        _delay.tick(_mixed_buffer);
      }

      memcpy(_temp_buffer, _delay.getOutput(), BATCH_SIZE * sizeof(float));
    } else if (kdiffuser_enabled) {
      auto delay_out = _delay.tick(_mixed_buffer);
      auto diffuser_out = _diffuser.tick(delay_out);
      memcpy(_temp_buffer, diffuser_out, BATCH_SIZE * sizeof(float));
    } else {
      auto delay_out = _delay.tick(_mixed_buffer);
      memcpy(_temp_buffer, delay_out, BATCH_SIZE * sizeof(float));
    }

    for (size_t i = 0; i < BATCH_SIZE; i++) {
      if (klow_shelf_enabled)
        _low_shelf.tick(_temp_buffer, _temp_buffer, BATCH_SIZE);
      if (khigh_shelf_enabled)
        _high_shelf.tick(_temp_buffer, _temp_buffer, BATCH_SIZE);
      if (kcutoff_enabled) {
        _temp_buffer[i] = _low_pass.Process(_temp_buffer[i]);
      }
    }

    memcpy(_filter_output_buffer, _temp_buffer, BATCH_SIZE * sizeof(float));
  }

  void clearDiffuserBuffer() {
    _diffuser.clearBuffers();
  }

  void clearBuffers() {
    _delay.clearBuffers();
    _diffuser.clearBuffers();
    _low_shelf.clearBuffers();
    _high_shelf.clearBuffers();

    memset(_temp_buffer, 0.0f, BATCH_SIZE);
    memset(_filter_output_buffer, 0.0f, BATCH_SIZE);
  }

  private:
  ModulatedDelay _delay;
  AllpassDiffuser _diffuser;
  audioLib::Biquad _low_shelf;
  audioLib::Biquad _high_shelf;
  daisysp::Tone _low_pass;
  float* _temp_buffer;
  float* _mixed_buffer;
  float* _filter_output_buffer;
};
} // namespace cloudSeed
