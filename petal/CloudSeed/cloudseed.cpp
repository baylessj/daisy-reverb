// Modified version of DaisyCloudSeed by Keith Bloemer.
// Intended for the Terrarrium guitar pedal hardware.
// Code has been modified for mono processing (originally stereo) to
// allow up to 5 delay lines on the Daisy Seed.

#include "daisy_petal.h"
#include "daisysp.h"

// #include "../../CloudSeed/AudioLib/MathDefs.h"
// #include "../../CloudSeed/AudioLib/ValueTables.h"
// #include "../../CloudSeed/Default.h"
// #include "../../CloudSeed/FastSin.h"
#include "../../CloudSeed/ReverbController.h"
#include "footswitchcontroller.hpp"
#include "knobcontroller.hpp"
#include "ledcontroller.hpp"
#include "toggleswitchcontroller.hpp"

#define MCU_CLOCK_RATE 48000 // Clock rate in Hz
#define BATCH_SIZE 4

daisy::DaisyPetal hw;
FootswitchController footswitch_controller(&hw);
LedController led_controller(&hw, MCU_CLOCK_RATE / BATCH_SIZE);
ToggleSwitchController toggleswitch_controller(&hw);
KnobController knob_controller(&hw);
std::uint8_t preset_number = 0;

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

static void setParameter(std::uint8_t param, float val) {
  switch (param) {
  case UNUSED_PARAM:
    break;
  case INPUT_MIX:
    break;
  case EARLY_LATE_MIX:
    break;
  default:
    reverb.SetParameter((Parameter)param, val);
    break;
  }
}

// This runs at a fixed rate, to prepare audio samples
static void audioCallback(daisy::AudioHandle::InputBuffer in,
                          daisy::AudioHandle::OutputBuffer out,
                          size_t batch_size) {
  hw.ProcessAnalogControls();
  hw.ProcessDigitalControls();

  auto fsw_info = footswitch_controller.tick();

  if (fsw_info.advancePreset) {
    // TODO: set a limit to this and have it roll over
    preset_number++;
  }

  led_controller.tick(fsw_info.bypassed, preset_number);

  auto toggle_info = toggleswitch_controller.tick();

  auto knob_info = knob_controller.tick(toggle_info.control_selector_mode,
                                        fsw_info.controlSelectionModeActive);

  for (auto ki : knob_info) {
    setParameter(ki.first, ki.second);
  }

  float mono_input[batch_size];
  memcpy(mono_input, in[0], batch_size);

  if (!fsw_info.bypassed) {
    reverb.Process(mono_input, out[0], batch_size);
  } else {
    memcpy(out[0], mono_input, batch_size);
  }
}

int main(void) {
  float samplerate;

  hw.Init();
  samplerate = hw.AudioSampleRate();

  AudioLib::ValueTables::Init();
  CloudSeed::FastSin::Init();

  reverb.ClearBuffers();
  reverb.initFactoryChorus();

  // hw.SetAudioBlockSize(4);

  hw.StartAdc();
  hw.StartAudio(audioCallback);
  while (1) {
    daisy::System::Delay(10);
  }
}
