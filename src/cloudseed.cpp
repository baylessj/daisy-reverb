// Modified version of DaisyCloudSeed by Keith Bloemer.
// Intended for the Terrarrium guitar pedal hardware.
// Code has been modified for mono processing (originally stereo) to
// allow up to 5 delay lines on the Daisy Seed.

#include "daisy_petal.h"
#include "daisysp.h"

#include "ReverbController.h"
#include "footswitchcontroller.hpp"
#include "knobcontroller.hpp"
#include "ledcontroller.hpp"
#include "presetcontroller.hpp"
#include "toggleswitchcontroller.hpp"

#define MCU_CLOCK_RATE 48000 // Clock rate in Hz
#define BATCH_SIZE 4

daisy::DaisyPetal hw;

std::uint8_t preset_number = 0;

FootswitchController footswitch_controller(&hw);
LedController led_controller(&hw, MCU_CLOCK_RATE / BATCH_SIZE);
ToggleSwitchController toggleswitch_controller(&hw);
KnobController knob_controller(&hw);
PresetController preset_controller;

daisysp::CrossFade input_mix;
float early_late_mix;

CloudSeed::ReverbController reverb =
  CloudSeed::ReverbController(MCU_CLOCK_RATE);

// This is used in the modified CloudSeed code for allocating
// delay line memory to SDRAM (64MB available on Daisy)
#define CUSTOM_POOL_SIZE (48 * 1024 * 1024)
DSY_SDRAM_BSS char custom_pool[CUSTOM_POOL_SIZE];
size_t pool_index = 0;
int allocation_count = 0;
void* custom_pool_allocate(size_t size) {
  if (pool_index + size >= CUSTOM_POOL_SIZE) {
    return 0;
  }
  void* ptr = &custom_pool[pool_index];
  pool_index += size;
  return ptr;
}

/**
 * Constant power crossfade between two gains
 *
 * TODO: this might need to be scaled up * 2 so that the middle position has
 * both of them on full?
 */
static std::pair<float, float> gainsFromMix(float mix) {
  return {
    sinf(mix * HALFPI_F),
    sinf((1.0f - mix) * HALFPI_F),
  };
}

static void setParameter(std::uint8_t param, float val) {
  switch (param) {
  case UNUSED_PARAM:
    break;
  case INPUT_MIX: {
    input_mix.SetPos(val);
    break;
  }
  case EARLY_LATE_MIX: {
    early_late_mix = val;
    auto verb_gains = gainsFromMix(val);
    reverb.SetParameter(Parameter::EarlyOut, verb_gains.first);
    reverb.SetParameter(Parameter::MainOut, verb_gains.second);
    break;
  }
  default: {
    reverb.SetParameter((Parameter)param, val);
    break;
  }
  }
}

static void writeMixedOutput(float* out,
                             float* dry_in,
                             float* reverb_out,
                             size_t batch_size) {
  for (size_t i = 0; i < batch_size; i++) {
    out[i] = input_mix.Process(dry_in[i], reverb_out[i]);
  }
}

static void recallAllPresets() {
  auto preset_params = preset_controller.recall(preset_number);
  for (std::uint16_t i = 0; i < (std::uint16_t)Parameter::Count; i++) {
    setParameter(i, preset_params[i]);
  }
  setParameter(INPUT_MIX, preset_params[INPUT_MIX]);
  setParameter(EARLY_LATE_MIX, preset_params[EARLY_LATE_MIX]);
}

// This runs at a fixed rate to prepare audio samples
static void audioCallback(daisy::AudioHandle::InputBuffer in,
                          daisy::AudioHandle::OutputBuffer out,
                          size_t batch_size) {
  hw.ProcessAnalogControls();
  hw.ProcessDigitalControls();

  auto fsw_info = footswitch_controller.tick();

  if (fsw_info.advancePreset) {
    if (preset_number == NUM_PRESETS) {
      preset_number = 0;
    } else {
      preset_number++;
    }

    recallAllPresets();
  }

  if (fsw_info.save) {
    float current_params[PARAMETERS_LENGTH];
    memcpy(current_params, reverb.GetAllParameters(), (size_t)Parameter::Count);
    current_params[INPUT_MIX] = input_mix.GetPos(0.0); // parameter is unused?
    current_params[EARLY_LATE_MIX] = early_late_mix;

    preset_controller.save(preset_number, current_params);
  }

  led_controller.tick(fsw_info.bypassed, preset_number, fsw_info.saving);

  auto toggle_info = toggleswitch_controller.tick();

  auto knob_info = knob_controller.tick(toggle_info.control_selector_mode,
                                        fsw_info.controlSelectionModeActive);

  for (auto ki : knob_info) {
    setParameter(ki.first, ki.second);
  }

  float mono_input[batch_size];
  memcpy(mono_input, in[0], batch_size);

  float mono_reverb_output[batch_size];
  if (!fsw_info.bypassed) {
    reverb.Process(mono_input, mono_reverb_output, batch_size);
    writeMixedOutput(out[0], mono_input, mono_reverb_output, batch_size);

  } else {
    memcpy(out[0], mono_input, batch_size);
  }
}

int main(void) {
  hw.Init();

  AudioLib::ValueTables::Init();
  CloudSeed::FastSin::Init();

  reverb.ClearBuffers();
  reverb.initFactoryChorus();

  hw.SetAudioBlockSize(BATCH_SIZE);

  input_mix.SetCurve(daisysp::CROSSFADE_CPOW);

  hw.StartAdc();
  hw.StartAudio(audioCallback);
  while (1) {
    daisy::System::Delay(10);
  }
}