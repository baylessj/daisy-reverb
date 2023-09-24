#pragma once

#include <array>
#include <cstddef>

#define DSY_SDRAM_BSS

namespace daisy {

namespace AudioHandle {
/** Non-Interleaving input buffer
 * Buffer arranged by float[chn][sample]
 * const so that the user can't modify the input
 */
typedef const float* const* InputBuffer;

/** Non-Interleaving output buffer
 * Arranged by float[chn][sample]
 */
typedef float** OutputBuffer;

/** Type for a Non-Interleaving audio callback
 * Non-Interleaving audio callbacks in daisy will be of this type
 */
typedef void (*AudioCallback)(InputBuffer in, OutputBuffer out, size_t size);
} // namespace AudioHandle

struct AnalogControl {};

struct Parameter {
  enum Curve { LINEAR };

  void Init(AnalogControl input, float min, float max, Curve curve) {
    (void)input;
    (void)min;
    (void)max;
    (void)curve;
  }

  float Process() {
    return 0.0f;
  }
};

struct Switch {
  float TimeHeldMs() {
    return 0.0f;
  }

  bool Pressed() {
    return false;
  }

  bool RisingEdge() {
    return false;
  }

  bool FallingEdge() {
    return false;
  }
};

struct Led {
  void Init(int pin, bool state) {
    (void)pin;
    (void)state;
  }

  void Set(bool on) {
    (void)on;
  }

  void Update() {}
};

struct DaisySeed {
  int GetPin(int pin) {
    return pin;
  }
};

struct DaisyPetal {
  std::array<Switch, 100> switches;

  std::array<AnalogControl, 100> knob;

  DaisySeed seed;

  void Init() {}

  void ProcessAnalogControls() {}

  void ProcessDigitalControls() {}

  void SetAudioBlockSize(int size) {
    (void)size;
  }

  void StartAdc() {}

  void StartAudio(AudioHandle::AudioCallback cb) {
    (void)cb;
  }
};

namespace System {
static void Delay(int time) {
  (void)time;
}
} // namespace System
} // namespace daisy