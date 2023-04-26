#include "daisy_petal.h"
#include "daisysp.h"
#include "terrarium.h"

#include "../../CloudSeed/Default.h"
#include "../../CloudSeed/ReverbController.h"
#include "../../CloudSeed/FastSin.h"
#include "../../CloudSeed/AudioLib/ValueTables.h"
#include "../../CloudSeed/AudioLib/MathDefs.h"


using namespace daisy;
using namespace daisysp;
using namespace terrarium;  // This is important for mapping the correct controls to the Daisy Seed on Terrarium PCB

// Declare a local daisy_petal for hardware access
DaisyPetal hw;
::daisy::Parameter wetDryMix, inLevel, time, diffusion;
bool      bypass;
int       c;
Led led1, led2;
float pwet_dry_mix_value, ptime_value, pdiffusion_value;

CloudSeed::ReverbController* reverb = 0;

  

#define CUSTOM_POOL_SIZE (48*1024*1024)
DSY_SDRAM_BSS char custom_pool[CUSTOM_POOL_SIZE];
size_t pool_index = 0;
int allocation_count = 0;
void* custom_pool_allocate(size_t size)
{
        if (pool_index + size >= CUSTOM_POOL_SIZE)
        {
                return 0;
        }
        void* ptr = &custom_pool[pool_index];
        pool_index += size;
        return ptr;
}

void cyclePreset()
{
    c += 1;
    if ( c > 8 ) {
        c = 0;
    }

    reverb->ClearBuffers();
        
    if ( c == 0 ) {
            reverb->initFactoryChorus();
    } else if ( c == 1 ) {
            reverb->initFactoryDullEchos();
    } else if ( c == 2 ) {
            reverb->initFactoryHyperplane();
    } else if ( c == 3 ) {
            reverb->initFactoryMediumSpace();
    } else if ( c == 4 ) {
            reverb->initFactoryNoiseInTheHallway();
    } else if ( c == 5 ) {
            reverb->initFactoryRubiKaFields();
    } else if ( c == 6 ) {
            reverb->initFactorySmallRoom();
    } else if ( c == 7 ) {
            reverb->initFactory90sAreBack();
    } else if ( c == 8 ) {
            reverb->initFactoryThroughTheLookingGlass(); // Only preset that sounds scratchy (using 4 delay lines, mono) causes buffer underruns
                                                           //   TODO Try slight modifications to this preset to allow to work
    }
}


// This runs at a fixed rate, to prepare audio samples
static void AudioCallback(AudioHandle::InputBuffer  in,
                          AudioHandle::OutputBuffer out,
                          size_t                    size)
{
    //hw.ProcessAllControls();
    hw.ProcessAnalogControls();
    hw.ProcessDigitalControls();
    led1.Update();
    led2.Update();

    float drylevel = inLevel.Process();
    float wet_dry_mix_value = wetDryMix.Process();
    float time_value = time.Process();
    float diffusion_value = diffusion.Process();

    if ((pwet_dry_mix_value < wet_dry_mix_value) || ( pwet_dry_mix_value > wet_dry_mix_value))
    {
      reverb->SetParameter(::Parameter::MainOut, wet_dry_mix_value);
      pwet_dry_mix_value = wet_dry_mix_value;
    }

    if ((ptime_value < time_value) || ( ptime_value > time_value))
    {
      reverb->SetParameter(::Parameter::LineDecay, time_value);
      ptime_value = time_value;
    }
    if ((pdiffusion_value < diffusion_value) || ( pdiffusion_value > diffusion_value))
    {
      reverb->SetParameter(::Parameter::LateDiffusionFeedback, diffusion_value);
      pdiffusion_value = diffusion_value;
    }
    
    //float ins[2*48];
    //float outs[2*48];
    float ins[48];
    float outs[48];
    for (size_t i = 0; i < size; i++)
    {
        // Read Inputs (only left channel used)
        ins[i] = in[0][i]*drylevel;

        //ins[2*i] = in[0][i]*drylevel;
        //ins[2*i+1]= in[1][i]*drylevel;  // Right Channel
    }

    // (De-)Activate bypass and toggle LED when left footswitch is pressed
    if(hw.switches[Terrarium::FOOTSWITCH_1].RisingEdge())
    {
        bypass = !bypass;
        led1.Set(bypass ? 0.0f : 1.0f);
    }

    // Cycle available models
    if(hw.switches[Terrarium::FOOTSWITCH_2].RisingEdge())
    {  
        cyclePreset();
    }

    if(!bypass) {
        reverb->Process(ins, outs, 48);
        for (size_t i = 0; i < size; i++)
        {  
            out[0][i] = outs[i];
            //out[1][i] = outs[i*2+1];
        }
    } else {
        for (size_t i = 0; i < size; i++)
        {  
            out[0][i] = in[0][i];
        }
    }
}

int main(void)
{
    float samplerate;

    hw.Init();
    samplerate = hw.AudioSampleRate();
    c = 0;

    AudioLib::ValueTables::Init();
    CloudSeed::FastSin::Init();
    
    reverb = new CloudSeed::ReverbController(samplerate);
    reverb->ClearBuffers();
    reverb->initFactoryChorus();

    //hw.SetAudioBlockSize(4);

    wetDryMix.Init(hw.knob[Terrarium::KNOB_1], 0.0f, 1.0f, ::daisy::Parameter::LINEAR);
    inLevel.Init(hw.knob[Terrarium::KNOB_2], 0.0f, 1.0f, ::daisy::Parameter::LINEAR);
    time.Init(hw.knob[Terrarium::KNOB_3], 0.0f, 1.0f, ::daisy::Parameter::LINEAR); 
    diffusion.Init(hw.knob[Terrarium::KNOB_4], 0.0f, 1.0f, ::daisy::Parameter::LINEAR); 
    pwet_dry_mix_value = 0.0;
    ptime_value = 0.0;
    pdiffusion_value = 0.0;

    // Init the LEDs and set activate bypass
    led1.Init(hw.seed.GetPin(Terrarium::LED_1),false);
    led1.Update();
    bypass = true;

    led2.Init(hw.seed.GetPin(Terrarium::LED_2),false);
    led2.Update();

    hw.StartAdc();
    hw.StartAudio(AudioCallback);
    while(1)
    {
        // Do Stuff Infinitely Here
        System::Delay(10);
    }
}
