# Description
CloudSeed Mono Reverb algorithm.

# Control

| Control | Description | Comment |
| --- | --- | --- |
| Ctrl 1 | Dry Level | Adjusts the Dry level out |
| Ctrl 2 | Early Reverberation Level | Adjusts the Early Reverb stage output.  |
| Ctrl 3 | Late Reverberation Level | Adjusts the Late Reverb stage output |
| Ctrl 4 | Late Reverberation Feedback | Adjusts amount of signal fed back through the delay line. |
| Ctrl 5 | Early Reverberation Dampening | Controls amount of dampening for the early reverb stage. Actual parameter name is "TapDecay" |
| Ctrl 6 | Late Reverberation Decay | Adjust the decay time of the late reverberation stage. |
| SW 1 - 4 | Selectable Delay Lines | Turn on or off to engage from 1 to 5 delay lines (1 delay line is always on) The order doesn't matter, just the total number that are on. (i.e., 1st and 4th switch on is the same as 2nd and 3rd switch on)|
| FS 1 | Bypass/Active | Bypass / effect engaged |
| FS 2 | Cycle Preset | Loads the next available Preset, starts at beginning after the last in the list. These are the same as the original Cloud Seed plugin presets, except for "Through the Looking Glass" |
| LED 1 | Bypass/Active Indicator |Illuminated when effect is set to Active |
| LED 2 | Not used | N/A |
| Audio In 1 | Audio input | Mono only for Terrarium |
| Audio Out 1 | Mix Out | Mono only for Terrarium |

# To Test

- Does the taps number or the feedback affect the delay lines more? Does changing them both simultaneously work?


# Controls

## Knobs

4 batches:
- FSW 2 open, Ctrl Selector off
- FSW 2 closed, Ctrl Selector on
- FSW 2 open, Ctrl Selector on
- FSW 2 closed, Ctrl Selector on

### Set 1 (Basics)

1. Mix
2. Early Decay
3. Early Density (number of taps + )
4. Early/Late mix
5. Late Decay
6. Late Density (number of taps)

### Set 2 (Diffusion Tweaking)

1. Early Decay Time
2. Early Feedback
3. Early Taps
4. Late Decay Time
5. Late Feedback
6. Late Dampening

### Set 3 (Modulation Tweaking)

1. Early diffusion mod amount
2. Early diffusion mod rate
3. Late mod amount
4. Late mod rate
5. Late diffusion mod amount
6. Late diffusion mod rate

### Set 4 (EQ)

1. Pre-Delay
2. Low Shelf Gain
3. High Shelf Gain
4. High Pass Freq
5. Low Shelf Freq
6. Low Shelf Freq

## Switches

1. Early diffusion on/off
2. Late diffusion on/off
3. Late diffusion pre/post
4. Control Selection

## Footswitches

1. Tap: On/Off | Hold: Momentary On/Off
2. Tap: Next preset | Hold: Control Selector

1 + 2 Hold: Save Preset (flash both LEDs three times first, then set)

## LEDs

1. On/Off
2. Blink once then off for preset one, twice for preset two, etc. Don't blink if the effect is off.


# All Controls

## Early Reflections Controls

- Number of taps (1-50) [1.3]
- Decay time [1.2]
- Gain (essentially just a mix of dry and tapped signal, can probably be 100%?)
- Feedback (called decay) [1.3]

### All-pass params for extra variance

Note: the diffusion seed value will change the tone of this, play around with some different options there
- Delay time [2.1]
- Feedback [2.2]
- Enable/Disable [SW1]
- Number of Delay taps (1-8) [2.3]
- Modulation Amount [3.1]
- Modulation Rate [3.2]

## Late Stage Controls

- Decay time [1.5]
- Feedback [1.6]
- Number of taps [1.6]
- Modulation amount [3.3]
- Modulation Rate [3.4]

- Toggle all-pass before/after the main decay
- Toggle all-pass entirely

### All-pass params

- Decay time [2.4]
- Feedback [2.5]
- Number of Delay taps [2.6]
- Modulation amount [3.5]
- Modulation rate [3.6]

### Feedback Filters

- Low Shelf gain, freq, toggle
- High Shelf gain, freq, toggle
- High Pass freq, toggle


