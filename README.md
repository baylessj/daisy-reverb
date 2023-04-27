# DaisyCloudSeed (GuitarML fork for Terrarium)
Cloud Seed is an open source algorithmic reverb plugin under the MIT license, which can be found at [ValdemarOrn/CloudSeed](https://github.com/ValdemarOrn/CloudSeed).
DaisyCloudSeed is a port to the Daisy environment for running on a Daisy Patch unit. This code (GuitarML's fork) further modifies DaisyCloudSeed
for use on the Terrarium guitar pedal. The processing has been changed to mono (from stereo), which allows up to 5 delay lines,
and fills out all of the Terrarium's controls. 

This repo also includes a modified version of CloudyReverb for Terrarium. It is a lighter reverb than CloudSeed (in terms of memory/processing requirements), and uses
the reverb algorithm from [eurorack](https://github.com/pichenettes/eurorack/tree/master).

## Getting started
The new code for Terrarium has been added to ```DaisyCloudSeed/petal```.
Build the daisy libraries and CloudSeed with (after installing the Daisy Toolchain):
```
./rebuild_libs.sh
cd petal/CloudSeed
make
```

Then flash your terrarium with the following commands (or use the [Electrosmith Web Programmer](https://electro-smith.github.io/Programmer/))
```
cd your_pedal
# using USB (after entering bootloader mode)
make program-dfu
# using JTAG/SWD adaptor (like STLink)
make program
```

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
