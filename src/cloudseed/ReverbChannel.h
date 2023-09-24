
#ifndef REVERBCHANNEL
#define REVERBCHANNEL

#include <cmath>
#include <map>
#include <memory>
#include <vector>

#include "../constants.h"
#include "AllpassDiffuser.h"
#include "DelayLine.h"
#include "Filters/atone.h"
#include "Filters/tone.h"
#include "ModulatedDelay.h"
#include "MultitapDiffuser.h"
#include "Parameter.h"
#include "ReverbChannel.h"
#include "Utils.h"
#include "audiolib/sharandom.h"

#define PRE_DELAY_BUFFER_LENGTH 1 // Max 1 second of delay
#define MULTITAP_BUFFER_LENGTH 1  // Max 1 second of multitap delay
// 150ms buffer, to allow for 100ms + modulation time
#define DIFFUSER_BUFFER_LENGTH 150

// IMPORTANT: CHANGE "TotalLineCount" FOR DAISY SEED HARDWARE
//            Original CloudSeed plugin uses 8 Delay Lines, or 12 delay lines?
//            DaisyCloudSeed adjusted to 2 to use with Stereo on DaisyPatch
//            hardware (otherwise causes buffer underruns for most presets
//            (except ChorusDelay) 4/26/2023 GuitarML fork of DaisyCloudSeed
//            uses 4, able to increase for Mono Only Terrarium platform (mono
//            guitar pedal using Daisy Seed)
#define MAX_DELAY_LINES 5

static float DEFAULT_HIGH_PASS_FREQ = 20.0f;
static float DEFAULT_LOW_PASS_FREQ = 20000.0f;

namespace cloudSeed {
class ReverbChannel {
  private:
  std::map<Parameter, float> _parameters;
  int _MCU_CLOCK_RATE;

  ModulatedDelay _pre_delay;
  MultitapDiffuser _multitap;
  AllpassDiffuser _diffuser;
  DelayLine* _lines[MAX_DELAY_LINES];
  float* _delay_line_seeds;
  daisysp::ATone _high_pass;
  daisysp::Tone _low_pass;
  float* _temp_buffer;
  float* _line_out_buffer;
  float* _out_buffer;

  int kdelay_line_seed;
  int kpost_diffusion_seed;
  size_t kline_count;
  bool khigh_pass_enabled;
  bool klow_pass_enabled;
  bool kdiffuser_enabled;
  float kdry_out_gain;
  float kpredelay_out_gain;
  float kearly_out_gain;
  float kline_out_gain;

  public:
  ReverbChannel()
    : _pre_delay(PRE_DELAY_BUFFER_LENGTH),
      _multitap(MULTITAP_BUFFER_LENGTH),
      _diffuser(DIFFUSER_BUFFER_LENGTH) {
    for (int i = 0; i < MAX_DELAY_LINES; i++)
      _lines[i] = new DelayLine();

    for (auto value = 0; value < (int)Parameter::Count; value++)
      _parameters[static_cast<Parameter>(value)] = 0.0;

    kline_count = MAX_DELAY_LINES;
    _diffuser.setInterpolationEnabled(true);
    _high_pass.Init(MCU_CLOCK_RATE);
    _high_pass.SetFreq(DEFAULT_HIGH_PASS_FREQ);
    _low_pass.Init(MCU_CLOCK_RATE);
    _low_pass.SetFreq(DEFAULT_LOW_PASS_FREQ);

    _temp_buffer = new float[BATCH_SIZE];
    _line_out_buffer = new float[BATCH_SIZE];
    _out_buffer = new float[BATCH_SIZE];
    _delay_line_seeds = sdramAllocate<float>(MAX_DELAY_LINES * 3);
  }

  ~ReverbChannel() {
    for (auto line : _lines)
      delete line;

    delete _temp_buffer;
    delete _line_out_buffer;
    delete _out_buffer;
  }

  float* getOutput() {
    return _out_buffer;
  }

  float* _getLineOutput() {
    return _line_out_buffer;
  }

  void setParameter(Parameter para, float value) {
    _parameters[para] = value;

    switch (para) {
    case Parameter::PreDelay:
      _pre_delay.ksample_delay = (int)_ms2Samples(value);
      break;
    case Parameter::HighPass:
      _high_pass.SetFreq(value);
      break;
    case Parameter::LowPass:
      _low_pass.SetFreq(value);
      break;

    case Parameter::TapCount:
      _multitap.setTapCount((int)value);
      break;
    case Parameter::TapLength:
      _multitap.setTapLength((int)_ms2Samples(value));
      break;
    case Parameter::TapGain:
      _multitap.setTapGain(value);
      break;
    case Parameter::TapDecay:
      _multitap.setTapDecay(value);
      break;

    case Parameter::DiffusionEnabled: {
      auto newVal = value >= 0.5;
      if (newVal != kdiffuser_enabled)
        _diffuser.clearBuffers();
      kdiffuser_enabled = newVal;
      break;
    }
    case Parameter::DiffusionStages:
      _diffuser.setStages((int)value);
      break;
    case Parameter::DiffusionDelay:
      _diffuser.setDelay((int)_ms2Samples(value));
      break;
    case Parameter::DiffusionFeedback:
      _diffuser.setFeedback(value);
      break;

    case Parameter::LineCount:
      kline_count = (int)value; // Originally commented out
      // per_line_gain = GetPerLineGain();  // In original Cloud Seed
      break;
    case Parameter::LineDelay:
      _updateLines();
      break;
    case Parameter::LineDecay:
      _updateLines();
      break;

    case Parameter::LateDiffusionEnabled:
      for (auto line : _lines) {
        auto newVal = value >= 0.5;
        if (newVal != line->kdiffuser_enabled)
          line->clearDiffuserBuffer();
        line->kdiffuser_enabled = newVal;
      }
      break;
    case Parameter::LateDiffusionStages:
      for (auto line : _lines)
        line->setDiffuserStages((int)value);
      break;
    case Parameter::LateDiffusionDelay:
      for (auto line : _lines)
        line->setDiffuserDelay((int)_ms2Samples(value));
      break;
    case Parameter::LateDiffusionFeedback:
      for (auto line : _lines)
        line->setDiffuserFeedback(value);
      break;

    case Parameter::PostLowShelfGain:
      for (auto line : _lines)
        line->setLowShelfGain(value);
      break;
    case Parameter::PostLowShelfFrequency:
      for (auto line : _lines)
        line->setLowShelfFrequency(value);
      break;
    case Parameter::PostHighShelfGain:
      for (auto line : _lines)
        line->setHighShelfGain(value);
      break;
    case Parameter::PostHighShelfFrequency:
      for (auto line : _lines)
        line->setHighShelfFrequency(value);
      break;
    case Parameter::PostCutoffFrequency:
      for (auto line : _lines)
        line->setCutoffFrequency(value);
      break;

    case Parameter::EarlyDiffusionModAmount:
      _diffuser.setModulationEnabled(value > 0.0);
      _diffuser.setModAmount(_ms2Samples(value));
      break;
    case Parameter::EarlyDiffusionModRate:
      _diffuser.setModRate(value);
      break;
    case Parameter::LineModAmount:
      _updateLines();
      break;
    case Parameter::LineModRate:
      _updateLines();
      break;
    case Parameter::LateDiffusionModAmount:
      _updateLines();
      break;
    case Parameter::LateDiffusionModRate:
      _updateLines();
      break;

    case Parameter::TapSeed:
      _multitap.setSeed((int)value);
      break;
    case Parameter::DiffusionSeed:
      _diffuser.setSeed((int)value);
      break;
    case Parameter::DelaySeed:
      kdelay_line_seed = (int)value;
      _updateLines();
      break;
    case Parameter::PostDiffusionSeed:
      kpost_diffusion_seed = (int)value;
      _updatePostDiffusion();
      break;

    case Parameter::DryOut:
      kdry_out_gain = value;
      break;
    case Parameter::PredelayOut:
      kpredelay_out_gain = value;
      break;
    case Parameter::EarlyOut:
      kearly_out_gain = value;
      break;
    case Parameter::MainOut:
      kline_out_gain = value;
      break;

    case Parameter::HiPassEnabled:
      khigh_pass_enabled = value >= 0.5;
      break;
    case Parameter::LowPassEnabled:
      klow_pass_enabled = value >= 0.5;
      break;
    case Parameter::LowShelfEnabled:
      for (auto line : _lines)
        line->klow_shelf_enabled = value >= 0.5;
      break;
    case Parameter::HighShelfEnabled:
      for (auto line : _lines)
        line->khigh_shelf_enabled = value >= 0.5;
      break;
    case Parameter::CutoffEnabled:
      for (auto line : _lines)
        line->kcutoff_enabled = value >= 0.5;
      break;
    case Parameter::LateStageTap:
      for (auto line : _lines)
        line->klate_stage_tap = value >= 0.5;
      break;

    case Parameter::Interpolation:
      for (auto line : _lines)
        line->setInterpolationEnabled(value >= 0.5);
      break;
    default:
      break;
    }
  }

  void tick(float* input) {
    auto predelayOutput = _pre_delay.getOutput();

    if (!klow_pass_enabled && !khigh_pass_enabled) {
      memcpy(_temp_buffer, input, BATCH_SIZE);
    } else {
      for (size_t i = 0; i < BATCH_SIZE; i++) {
        if (khigh_pass_enabled) {
          _temp_buffer[i] = _high_pass.Process(input[i]);
        }
        if (klow_pass_enabled) {
          auto lowPassInput = khigh_pass_enabled ? _temp_buffer[i] : input[i];
          _temp_buffer[i] = _low_pass.Process(lowPassInput);
        }
      }
    }

    // completely zero if no input present
    // Previously, the very small values were causing some really strange CPU
    // spikes
    for (size_t i = 0; i < BATCH_SIZE; i++) {
      auto n = _temp_buffer[i];
      if (n * n < 0.000000001)
        _temp_buffer[i] = 0;
    }

    auto pre_delay_output = _pre_delay.tick(_temp_buffer);
    auto multitap_output = _multitap.tick(pre_delay_output);

    if (kdiffuser_enabled) {
      auto diffuser_output = _diffuser.tick(multitap_output);
      memcpy(_temp_buffer, diffuser_output, BATCH_SIZE);
    } else {
      memcpy(_temp_buffer, multitap_output, BATCH_SIZE);
    }

    auto earlyOutStage = _temp_buffer;

    // mix in the feedback from the other channel
    // for (int i = 0; i < len; i++)
    //	tempBuffer[i] += crossMix[i];

    for (size_t i = 0; i < kline_count; i++)
      _lines[i]->tick(_temp_buffer);

    for (size_t i = 0; i < kline_count; i++) {
      auto buf = _lines[i]->getOutput();

      if (i == 0) {
        for (int j = 0; j < BATCH_SIZE; j++)
          _temp_buffer[j] = buf[j];
      } else {
        for (int j = 0; j < BATCH_SIZE; j++)
          _temp_buffer[j] += buf[j];
      }
    }

    auto per_line_gain = _getPerLineGain();
    utils::gain(_temp_buffer, per_line_gain, BATCH_SIZE);
    utils::copy(_line_out_buffer, _temp_buffer, BATCH_SIZE);

    for (size_t i = 0; i < BATCH_SIZE; i++) {
      _out_buffer[i] = kdry_out_gain * input[i] +               //
                       kpredelay_out_gain * predelayOutput[i] + //
                       kearly_out_gain * earlyOutStage[i] +     //
                       kline_out_gain * _temp_buffer[i];
    }
  }

  void clearBuffers() {
    for (int i = 0; i < BATCH_SIZE; i++) {
      _temp_buffer[i] = 0.0;
      _line_out_buffer[i] = 0.0;
      _out_buffer[i] = 0.0;
    }

    _pre_delay.clearBuffers();
    _multitap.clearBuffers();
    _diffuser.clearBuffers();
    for (auto line : _lines)
      line->clearBuffers();
  }

  private:
  float _getPerLineGain() {
    return 1.0 / std::sqrt(kline_count);
  }

  void _updateLines() {
    auto lineDelaySamples = (int)_ms2Samples(_parameters[Parameter::LineDelay]);
    auto lineDecayMillis = _parameters[Parameter::LineDecay] * 1000;
    auto lineDecaySamples = _ms2Samples(lineDecayMillis);

    auto lineModAmount = _ms2Samples(_parameters[Parameter::LineModAmount]);
    auto lineModRate = _parameters[Parameter::LineModRate];

    auto lateDiffusionModAmount =
      _ms2Samples(_parameters[Parameter::LateDiffusionModAmount]);
    auto lateDiffusionModRate = _parameters[Parameter::LateDiffusionModRate];

    audioLib::sharandom::generate(
      kdelay_line_seed, MAX_DELAY_LINES * 3, _delay_line_seeds);
    for (size_t i = 0; i < MAX_DELAY_LINES; i++) {
      auto modAmount =
        lineModAmount * (0.7 + 0.3 * _delay_line_seeds[i + MAX_DELAY_LINES]);
      auto modRate = lineModRate *
                     (0.7 + 0.3 * _delay_line_seeds[i + 2 * MAX_DELAY_LINES]) /
                     MCU_CLOCK_RATE;

      auto delaySamples = (0.5 + 1.0 * _delay_line_seeds[i]) * lineDelaySamples;
      // when the delay is set really short,
      // and the modulation is very high
      if (delaySamples < modAmount + 2) {
        // the mod could actually take the delay time negative, prevent
        // that! -- provide 2 extra sample as margin of safety
        delaySamples = modAmount + 2;
      }

      auto dbAfter1Iteration =
        delaySamples / lineDecaySamples *
        (-60); // lineDecay is the time it takes to reach T60
      auto gainAfter1Iteration = utils::DB2gain(dbAfter1Iteration);

      _lines[i]->setDelay((int)delaySamples);
      _lines[i]->setFeedback(gainAfter1Iteration);
      _lines[i]->setLineModAmount(modAmount);
      _lines[i]->setLineModRate(modRate);
      _lines[i]->setDiffuserModAmount(lateDiffusionModAmount);
      _lines[i]->setDiffuserModRate(lateDiffusionModRate);
    }
  }

  void _updatePostDiffusion() {
    for (int i = 0; i < MAX_DELAY_LINES; i++)
      _lines[i]->setDiffuserSeed(((long long)kpost_diffusion_seed) * (i + 1));
  }

  float _ms2Samples(float value) {
    return value / 1000.0 * _MCU_CLOCK_RATE;
  }
};

} // namespace cloudSeed

#endif
