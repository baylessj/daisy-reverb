/**
 * Controls saving/recalling all parameters for presets.
 */
#pragma once

#include "daisy.h"
#include "daisy_petal.h"

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
  PresetController(daisy::DaisyPetal* hw)
    : _nvm(daisy::PersistentStorage<
           std::array<std::array<float, NUM_PRESETS>, PARAMETERS_LENGTH>>(
        hw->seed.qspi)) {}

  void save(std::uint8_t preset_number, float* parameters) {
    auto cur_settings = _nvm.GetSettings();
    std::copy(parameters,
              parameters + PARAMETERS_LENGTH,
              std::begin(cur_settings[preset_number]));
    _nvm.Save();
  }

  float* recall(std::uint8_t preset_number) {
    return _nvm.GetSettings()[preset_number].begin();
  }

  private:
  daisy::PersistentStorage<
    std::array<std::array<float, NUM_PRESETS>, PARAMETERS_LENGTH>>
    _nvm;
};