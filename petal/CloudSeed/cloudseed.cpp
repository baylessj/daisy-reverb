// Modified version of DaisyCloudSeed by Keith Bloemer.
// Intended for the Terrarrium guitar pedal hardware.
// Code has been modified for mono processing (originally stereo) to
// allow up to 5 delay lines on the Daisy Seed.

#include "daisy_petal.h"
#include "daisysp.h"

#include "../../CloudSeed/AudioLib/MathDefs.h"
#include "../../CloudSeed/AudioLib/ValueTables.h"
#include "../../CloudSeed/Default.h"
#include "../../CloudSeed/FastSin.h"
#include "../../CloudSeed/ReverbController.h"
#include "footswitchcontroller.hpp"
#include "ledcontroller.hpp"
#include "toggleswitchcontroller.hpp"

using namespace daisy;
using namespace daisysp;
using namespace terrarium;

#define MCU_CLOCK_RATE 48000 // Clock rate in Hz
#define BATCH_SIZE 4

DaisyPetal hw;
FootswitchController footswitch_controller(&hw);
LedController led_controller(MCU_CLOCK_RATE / BATCH_SIZE);
ToggleSwitchController toggleswitch_controller(&hw);
std::uint8_t preset_number = 0;

::daisy::Parameter dryOut, earlyOut, mainOut, time, diffusion, tapDecay;
int c;

// Initialize "previous" p values
float pdryout_value, pearlyout_value, pmainout_value, ptime_value,
  pdiffusion_value, pnumDelayLines, ptap_decay_value;

CloudSeed::ReverbController* reverb = NULL;

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

void cyclePreset() {
  c += 1;
  if (c > 7) {
    c = 0;
  }

  reverb->ClearBuffers();

  if (c == 0) {
    reverb->initFactoryChorus();
  } else if (c == 1) {
    reverb->initFactoryDullEchos();
  } else if (c == 2) {
    reverb->initFactoryHyperplane();
  } else if (c == 3) {
    reverb->initFactoryMediumSpace();
  } else if (c == 4) {
    reverb->initFactoryNoiseInTheHallway();
  } else if (c == 5) {
    reverb->initFactoryRubiKaFields();
  } else if (c == 6) {
    reverb->initFactorySmallRoom();
  } else if (c == 7) {
    reverb->initFactory90sAreBack();
    //} else if ( c == 8 ) {
    //        reverb->initFactoryThroughTheLookingGlass(); // Only preset that
    //        sounds scratchy (using 4-5 delay lines, mono) causes buffer
    //        underruns
    //                                                       //   TODO Try
    //                                                       slight
    //                                                       modifications to
    //                                                       this preset to
    //                                                       allow to work
  }
}

// TODO: update the controls here too, don't just read the values
static void processControls() {
  float dryout_value = dryOut.Process();
  float earlyout_value = earlyOut.Process();
  float mainout_value = mainOut.Process();
  float time_value = time.Process();
  float diffusion_value = diffusion.Process();
  float tap_decay_value = tapDecay.Process();

  if ((pdryout_value < dryout_value) || (pdryout_value > dryout_value)) {
    reverb->SetParameter(::Parameter::DryOut, dryout_value);
    pdryout_value = dryout_value;
  }

  if ((pearlyout_value < earlyout_value) ||
      (pearlyout_value > earlyout_value)) {
    reverb->SetParameter(::Parameter::EarlyOut, earlyout_value);
    pearlyout_value = earlyout_value;
  }

  if ((pmainout_value < mainout_value) || (pmainout_value > mainout_value)) {
    reverb->SetParameter(::Parameter::MainOut, mainout_value);
    pmainout_value = mainout_value;
  }

  if ((ptime_value < time_value) || (ptime_value > time_value)) {
    reverb->SetParameter(::Parameter::LineDecay, time_value);
    ptime_value = time_value;
  }
  if ((pdiffusion_value < diffusion_value) ||
      (pdiffusion_value > diffusion_value)) {
    reverb->SetParameter(::Parameter::LateDiffusionFeedback, diffusion_value);
    pdiffusion_value = diffusion_value;
  }

  if ((ptap_decay_value < tap_decay_value) ||
      (ptap_decay_value > tap_decay_value)) {
    reverb->SetParameter(::Parameter::TapDecay, tap_decay_value);
    ptap_decay_value = tap_decay_value;
  }

  // Delay Line Switches
  //     - The .Pressed() function below counts an 'ON' switch as pressed.
  //     - Total number of switches on sets how many delay lines are activated
  //     (1 - 5)
  int switches[4] = {Terrarium::SWITCH_1,
                     Terrarium::SWITCH_2,
                     Terrarium::SWITCH_3,
                     Terrarium::SWITCH_4}; // Can this be moved elsewhere?
  float numDelayLines = 1.0;
  for (int i = 0; i < 4; i++) {
    if (hw.switches[switches[i]].Pressed()) {
      numDelayLines += 1.0;
    }
  }
  if (pnumDelayLines != numDelayLines) {
    // reverb->ClearBuffers();  //TODO is this needed?
    reverb->SetParameter(::Parameter::LineCount, numDelayLines);
    pnumDelayLines = numDelayLines;
  }
}

// This runs at a fixed rate, to prepare audio samples
static void AudioCallback(AudioHandle::InputBuffer in,
                          AudioHandle::OutputBuffer out,
                          size_t batch_size) {
  hw.ProcessAnalogControls();
  hw.ProcessDigitalControls();

  processControls();

  auto fsw_info = footswitch_controller.tick();

  if (fsw_info.advancePreset) {
    // TODO: set a limit to this and have it roll over
    preset_number++;
  }

  led_controller.tick(fsw_info.bypassed, preset_number);

  auto toggle_info = toggleswitch_controller.tick();

  float mono_input[batch_size];
  memcpy(mono_input, in[0], batch_size);

  if (!fsw_info.bypassed) {
    reverb->Process(mono_input, out[0], batch_size);
    // for (size_t i = 0; i < size; i++)
    // {
    //     out[0][i] = outs[i] * 1.2;  // Slight overall volume boost at 1.2
    // }
  } else {
    memcpy(out[0], mono_input, batch_size);
  }
}

int main(void) {
  float samplerate;

  hw.Init();
  samplerate = hw.AudioSampleRate();
  c = 0;

  led_controller.init();

  AudioLib::ValueTables::Init();
  CloudSeed::FastSin::Init();

  reverb = new CloudSeed::ReverbController(samplerate);
  reverb->ClearBuffers();
  reverb->initFactoryChorus();

  // hw.SetAudioBlockSize(4);

  dryOut.Init(
    hw.knob[Terrarium::KNOB_1], 0.0f, 1.0f, ::daisy::Parameter::LINEAR);
  earlyOut.Init(
    hw.knob[Terrarium::KNOB_2], 0.0f, 1.0f, ::daisy::Parameter::LINEAR);
  mainOut.Init(
    hw.knob[Terrarium::KNOB_3], 0.0f, 1.0f, ::daisy::Parameter::LINEAR);
  diffusion.Init(
    hw.knob[Terrarium::KNOB_4], 0.0f, 1.0f, ::daisy::Parameter::LINEAR);
  tapDecay.Init(
    hw.knob[Terrarium::KNOB_5], 0.0f, 1.0f, ::daisy::Parameter::LINEAR);
  time.Init(hw.knob[Terrarium::KNOB_6], 0.0f, 1.0f, ::daisy::Parameter::LINEAR);

  pdryout_value = 0.0;
  pearlyout_value = 0.0;
  pmainout_value = 0.0;
  ptime_value = 0.0;
  pdiffusion_value = 0.0;
  ptap_decay_value = 0.0;
  pnumDelayLines = 5.0; // Set to max number of delay lines initially

  hw.StartAdc();
  hw.StartAudio(AudioCallback);
  while (1) {
    // Do Stuff Infinitely Here
    System::Delay(10);
  }
}
