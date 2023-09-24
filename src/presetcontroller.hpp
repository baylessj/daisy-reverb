/**
 * Controls saving/recalling all parameters for presets.
 */
#pragma once

#include "cloudseed/Parameter.h"
#include <cstdint>
#include <cstring>

#define NUM_PRESETS 3

#define PARAMETERS_LENGTH ((std::uint16_t)cloudSeed::Parameter::Count + 3)

/**
 * TODO: this is currently just storing in RAM, the presets will be lost on
 * power off. Figure out how to save this to flash.
 *
 * https://forum.electro-smith.com/t/persisting-data-to-from-flash/502/10
 * https://github.com/electro-smith/DaisyExamples/blob/master/seed/QSPI/QSPI.cpp
 */
class PresetController {
  public:
  void save(std::uint8_t preset_number, float* parameters) {
    std::memcpy(presets[preset_number], parameters, PARAMETERS_LENGTH);
  }

  float* recall(std::uint8_t preset_number) {
    return presets[preset_number];
  }

  private:
  float presets[NUM_PRESETS][PARAMETERS_LENGTH];
};