// see https://github.com/ValdemarOrn/CloudSeed
// port to Daisy and App_Dekrispator by Erwin Coumans

#pragma once

#include "Parameter.h"
#include "ReverbChannel.h"

#include "../constants.h"
#include "AllpassDiffuser.h"
#include "MultitapDiffuser.h"
#include "ReverbController.h"
#include "Utils.h"
#include "audiolib/valuetables.h"

namespace cloudSeed {
using namespace audioLib;

class ReverbController {
  private:
  ReverbChannel _channel;
  float _left_channel_in[BATCH_SIZE];
  // TODO (baylessj): we have two places where parameters are stored currently,
  // leave these to be just stored in the channel?
  float _parameters[(int)Parameter::Count];

  public:
  float* getAllParameters() {
    return _parameters;
  }

  float getScaledParameter(Parameter param) {
    switch (param) {
    // Input
    case Parameter::InputMix:
      return P(Parameter::InputMix);
    case Parameter::PreDelay:
      return (int)(P(Parameter::PreDelay) * 1000);

    case Parameter::HighPass:
      return 20 + ValueTables::Get(P(Parameter::HighPass),
                                   ValueTables::Response4Oct) *
                    980;
    case Parameter::LowPass:
      return 400 + ValueTables::Get(P(Parameter::LowPass),
                                    ValueTables::Response4Oct) *
                     19600;

    // Early
    case Parameter::TapCount:
      return 1 + (int)(P(Parameter::TapCount) * (MAX_DIFFUSER_TAPS - 1));
    case Parameter::TapLength:
      return (int)(P(Parameter::TapLength) * 500);
    case Parameter::TapGain:
      return ValueTables::Get(P(Parameter::TapGain), ValueTables::Response2Dec);
    case Parameter::TapDecay:
      return P(Parameter::TapDecay);

    case Parameter::DiffusionEnabled:
      return P(Parameter::DiffusionEnabled) < 0.5 ? 0.0 : 1.0;
    case Parameter::DiffusionStages:
      return 1 + (int)(P(Parameter::DiffusionStages) *
                       (MAX_DIFFUSER_STAGE_COUNT - 0.001));
    case Parameter::DiffusionDelay:
      return (int)(10 + P(Parameter::DiffusionDelay) * 90);
    case Parameter::DiffusionFeedback:
      return P(Parameter::DiffusionFeedback);

    // Late
    case Parameter::LineCount:
      return 1 + (int)(P(Parameter::LineCount) * 11.999);
    case Parameter::LineDelay:
      return (int)(20.0 + ValueTables::Get(P(Parameter::LineDelay),
                                           ValueTables::Response2Dec) *
                            980);
    case Parameter::LineDecay:
      return 0.05 + ValueTables::Get(P(Parameter::LineDecay),
                                     ValueTables::Response3Dec) *
                      59.95;

    case Parameter::LateDiffusionEnabled:
      return P(Parameter::LateDiffusionEnabled) < 0.5 ? 0.0 : 1.0;
    case Parameter::LateDiffusionStages:
      return 1 + (int)(P(Parameter::LateDiffusionStages) *
                       (MAX_DIFFUSER_STAGE_COUNT - 0.001));
    case Parameter::LateDiffusionDelay:
      return (int)(10 + P(Parameter::LateDiffusionDelay) * 90);
    case Parameter::LateDiffusionFeedback:
      return P(Parameter::LateDiffusionFeedback);

    // Frequency Response
    case Parameter::PostLowShelfGain:
      return ValueTables::Get(P(Parameter::PostLowShelfGain),
                              ValueTables::Response2Dec);
    case Parameter::PostLowShelfFrequency:
      return 20 + ValueTables::Get(P(Parameter::PostLowShelfFrequency),
                                   ValueTables::Response4Oct) *
                    980;
    case Parameter::PostHighShelfGain:
      return ValueTables::Get(P(Parameter::PostHighShelfGain),
                              ValueTables::Response2Dec);
    case Parameter::PostHighShelfFrequency:
      return 400 + ValueTables::Get(P(Parameter::PostHighShelfFrequency),
                                    ValueTables::Response4Oct) *
                     19600;
    case Parameter::PostCutoffFrequency:
      return 400 + ValueTables::Get(P(Parameter::PostCutoffFrequency),
                                    ValueTables::Response4Oct) *
                     19600;

    // Modulation
    case Parameter::EarlyDiffusionModAmount:
      return P(Parameter::EarlyDiffusionModAmount) * 2.5;
    case Parameter::EarlyDiffusionModRate:
      return ValueTables::Get(P(Parameter::EarlyDiffusionModRate),
                              ValueTables::Response2Dec) *
             5;
    case Parameter::LineModAmount:
      return P(Parameter::LineModAmount) * 2.5;
    case Parameter::LineModRate:
      return ValueTables::Get(P(Parameter::LineModRate),
                              ValueTables::Response2Dec) *
             5;
    case Parameter::LateDiffusionModAmount:
      return P(Parameter::LateDiffusionModAmount) * 2.5;
    case Parameter::LateDiffusionModRate:
      return ValueTables::Get(P(Parameter::LateDiffusionModRate),
                              ValueTables::Response2Dec) *
             5;

    // Seeds
    case Parameter::TapSeed:
      return (int)std::floor(P(Parameter::TapSeed) * 1000000 + 0.001);
    case Parameter::DiffusionSeed:
      return (int)std::floor(P(Parameter::DiffusionSeed) * 1000000 + 0.001);
    case Parameter::DelaySeed:
      return (int)std::floor(P(Parameter::DelaySeed) * 1000000 + 0.001);
    case Parameter::PostDiffusionSeed:
      return (int)std::floor(P(Parameter::PostDiffusionSeed) * 1000000 + 0.001);

    // Output
    case Parameter::CrossSeed:
      return P(Parameter::CrossSeed);

    case Parameter::DryOut:
      return ValueTables::Get(P(Parameter::DryOut), ValueTables::Response2Dec);
    case Parameter::PredelayOut:
      return ValueTables::Get(P(Parameter::PredelayOut),
                              ValueTables::Response2Dec);
    case Parameter::EarlyOut:
      return ValueTables::Get(P(Parameter::EarlyOut),
                              ValueTables::Response2Dec);
    case Parameter::MainOut:
      return ValueTables::Get(P(Parameter::MainOut), ValueTables::Response2Dec);

    // Switches
    case Parameter::HiPassEnabled:
      return P(Parameter::HiPassEnabled) < 0.5 ? 0.0 : 1.0;
    case Parameter::LowPassEnabled:
      return P(Parameter::LowPassEnabled) < 0.5 ? 0.0 : 1.0;
    case Parameter::LowShelfEnabled:
      return P(Parameter::LowShelfEnabled) < 0.5 ? 0.0 : 1.0;
    case Parameter::HighShelfEnabled:
      return P(Parameter::HighShelfEnabled) < 0.5 ? 0.0 : 1.0;
    case Parameter::CutoffEnabled:
      return P(Parameter::CutoffEnabled) < 0.5 ? 0.0 : 1.0;
    case Parameter::LateStageTap:
      return P(Parameter::LateStageTap) < 0.5 ? 0.0 : 1.0;

    // Effects
    case Parameter::Interpolation:
      return P(Parameter::Interpolation) < 0.5 ? 0.0 : 1.0;

    default:
      return 0.0;
    }

    return 0.0;
  }

  void setParameter(Parameter param, float value) {
    _parameters[(int)param] = value;
    auto scaled = getScaledParameter(param);

    _channel.setParameter(param, scaled);
  }

  void clearBuffers() {
    _channel.clearBuffers();
  }

  void tick(float* input, float* output) {
    memcpy(_left_channel_in, input, BATCH_SIZE);

    _channel.tick(_left_channel_in);
    auto left_out = _channel.getOutput();

    memcpy(output, left_out, BATCH_SIZE);
  }

  private:
  float P(Parameter para) {
    auto idx = (int)para;
    return idx >= 0 && idx < (int)Parameter::Count ? _parameters[idx] : 0.0;
  }
};
} // namespace cloudSeed
