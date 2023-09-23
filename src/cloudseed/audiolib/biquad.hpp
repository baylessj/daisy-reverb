#pragma once

#include <vector>

#define BIQUAD_CONSTANT_LENGTH 3

namespace audioLib {
class Biquad {
  public:
  enum class FilterType {
    LowPass = 0,
    HighPass,
    BandPass,
    Notch,
    Peak,
    LowShelf,
    HighShelf
  };

  private:
  float _samplerate;
  float _gainDb;
  float _q;
  float a0, a1, a2, b0, b1, b2;
  float x1, x2, y, y1, y2;
  float kgain;

  public:
  FilterType type;
  float kfrequency;
  float kslope;

  Biquad();
  Biquad(FilterType filterType, float samplerate);

  float getGainDb();
  void setGainDb(float value);

  float getGain();
  void setGain(float value);

  float getQ();
  void setQ(float value);

  std::array<float, BIQUAD_CONSTANT_LENGTH> getA();
  std::array<float, BIQUAD_CONSTANT_LENGTH> getB();

  void update();
  float getResponse(float freq);

  float inline tick(float x) {
    y = b0 * x + b1 * x1 + b2 * x2 - a1 * y1 - a2 * y2;
    x2 = x1;
    y2 = y1;
    x1 = x;
    y1 = y;

    return y;
  }

  void inline tick(float* input, float* output, int len) {
    for (int i = 0; i < len; i++)
      output[i] = tick(input[i]);
  }

  void clearBuffers();
};
} // namespace audioLib