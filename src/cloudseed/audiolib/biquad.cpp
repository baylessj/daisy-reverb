#include <array>
#include <cmath>

#include "biquad.hpp"

namespace audioLib {
Biquad::Biquad() {
  clearBuffers();
}

Biquad::Biquad(FilterType filterType, float samplerate) {
  type = filterType;
  _samplerate = samplerate;

  setGainDb(0.0);
  kfrequency = samplerate / 4;
  setQ(0.5);
  clearBuffers();
}

float Biquad::getGainDb() {
  return std::log10(kgain) * 20;
}

void Biquad::setGainDb(float value) {
  setGain(std::pow(10, value / 20));
}

float Biquad::getGain() {
  return kgain;
}

void Biquad::setGain(float value) {
  if (value == 0)
    value = 0.001; // -60dB

  kgain = value;
  update();
}

float Biquad::getQ() {
  return _q;
}

void Biquad::setQ(float value) {
  if (value == 0)
    value = 1e-12;
  _q = value;
}

std::array<float, BIQUAD_CONSTANT_LENGTH> Biquad::getA() {
  return std::array<float, BIQUAD_CONSTANT_LENGTH>{1.0f, a1, a2};
}

std::array<float, BIQUAD_CONSTANT_LENGTH> Biquad::getB() {
  return {b0, b1, b2};
}

void Biquad::update() {
  float omega = 2 * M_PI * kfrequency / _samplerate;
  float sinOmega = std::sin(omega);
  float cosOmega = std::cos(omega);

  float sqrtGain = 0.0;
  float alpha = 0.0;

  if (type == FilterType::LowShelf || type == FilterType::HighShelf) {
    alpha =
      sinOmega / 2 * std::sqrt((kgain + 1 / kgain) * (1 / kslope - 1) + 2);
    sqrtGain = std::sqrt(kgain);
  } else {
    alpha = sinOmega / (2 * _q);
  }

  switch (type) {
  case FilterType::LowPass:
    b0 = (1 - cosOmega) / 2;
    b1 = 1 - cosOmega;
    b2 = (1 - cosOmega) / 2;
    a0 = 1 + alpha;
    a1 = -2 * cosOmega;
    a2 = 1 - alpha;
    break;
  case FilterType::HighPass:
    b0 = (1 + cosOmega) / 2;
    b1 = -(1 + cosOmega);
    b2 = (1 + cosOmega) / 2;
    a0 = 1 + alpha;
    a1 = -2 * cosOmega;
    a2 = 1 - alpha;
    break;
  case FilterType::BandPass:
    b0 = alpha;
    b1 = 0;
    b2 = -alpha;
    a0 = 1 + alpha;
    a1 = -2 * cosOmega;
    a2 = 1 - alpha;
    break;
  case FilterType::Notch:
    b0 = 1;
    b1 = -2 * cosOmega;
    b2 = 1;
    a0 = 1 + alpha;
    a1 = -2 * cosOmega;
    a2 = 1 - alpha;
    break;
  case FilterType::Peak:
    b0 = 1 + (alpha * kgain);
    b1 = -2 * cosOmega;
    b2 = 1 - (alpha * kgain);
    a0 = 1 + (alpha / kgain);
    a1 = -2 * cosOmega;
    a2 = 1 - (alpha / kgain);
    break;
  case FilterType::LowShelf:
    b0 = kgain * ((kgain + 1) - (kgain - 1) * cosOmega + 2 * sqrtGain * alpha);
    b1 = 2 * kgain * ((kgain - 1) - (kgain + 1) * cosOmega);
    b2 = kgain * ((kgain + 1) - (kgain - 1) * cosOmega - 2 * sqrtGain * alpha);
    a0 = (kgain + 1) + (kgain - 1) * cosOmega + 2 * sqrtGain * alpha;
    a1 = -2 * ((kgain - 1) + (kgain + 1) * cosOmega);
    a2 = (kgain + 1) + (kgain - 1) * cosOmega - 2 * sqrtGain * alpha;
    break;
  case FilterType::HighShelf:
    b0 = kgain * ((kgain + 1) + (kgain - 1) * cosOmega + 2 * sqrtGain * alpha);
    b1 = -2 * kgain * ((kgain - 1) + (kgain + 1) * cosOmega);
    b2 = kgain * ((kgain + 1) + (kgain - 1) * cosOmega - 2 * sqrtGain * alpha);
    a0 = (kgain + 1) - (kgain - 1) * cosOmega + 2 * sqrtGain * alpha;
    a1 = 2 * ((kgain - 1) - (kgain + 1) * cosOmega);
    a2 = (kgain + 1) - (kgain - 1) * cosOmega - 2 * sqrtGain * alpha;
    break;
  }

  float g = 1 / a0;

  b0 = b0 * g;
  b1 = b1 * g;
  b2 = b2 * g;
  a1 = a1 * g;
  a2 = a2 * g;
}

float Biquad::getResponse(float freq) {
  float phi = std::pow((std::sin(2 * M_PI * freq / (2.0 * _samplerate))), 2);
  return (std::pow(b0 + b1 + b2, 2.0) -
          4.0 * (b0 * b1 + 4.0 * b0 * b2 + b1 * b2) * phi +
          16.0 * b0 * b2 * phi * phi) /
         (std::pow(1.0 + a1 + a2, 2.0) - 4.0 * (a1 + 4.0 * a2 + a1 * a2) * phi +
          16.0 * a2 * phi * phi);
}

void Biquad::clearBuffers() {
  y = 0;
  x2 = 0;
  y2 = 0;
  x1 = 0;
  y1 = 0;
}

} // namespace audioLib