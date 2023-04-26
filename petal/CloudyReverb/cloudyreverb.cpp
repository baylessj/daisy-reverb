#include "daisy_petal.h"
#include "daisysp.h"
#include "terrarium.h"

#include "clouds/dsp/frame.h"
//#include "clouds/dsp/fx/reverb.h"
//using namespace clouds;

#include "rings/dsp/fx/reverb.h"
using namespace rings;

Reverb clouds_reverb;
uint16_t reverb_buffer[65536];

using namespace daisy;
using namespace daisysp;
using namespace terrarium;  // This is important for mapping the correct controls to the Daisy Seed on Terrarium PCB

// Declare a local daisy_petal for hardware access
DaisyPetal hw;
Parameter wetDryMix, inLevel, time, diffusion;
bool      bypass;

Led led1, led2;

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


    float wet_dry_mix_value = wetDryMix.Process();
    float in_level_value = inLevel.Process();
    float time_value = time.Process();
    float diffusion_value = diffusion.Process();
    

    float ins_left[48];
    float ins_right[48];

    // (De-)Activate bypass and toggle LED when left footswitch is pressed
    if(hw.switches[Terrarium::FOOTSWITCH_1].RisingEdge())
    {
        bypass = !bypass;
        led1.Set(bypass ? 0.0f : 1.0f);
    }

    // Cycle available models
    if(hw.switches[Terrarium::FOOTSWITCH_2].RisingEdge())
    {  
        //changeModel();
    }



    for(size_t i = 0; i < size; i++)
    {
        //float input = in[0][i];
        //float wet   = input;

        // Read Inputs (only stereo in are used)
        ins_left[i] = in[0][i];
        ins_right[i]= in[0][i]; // Processing right from left buffer for now, remove stereo for terrarium

        // Process your signal here
        if(bypass)
        {
            out[0][i] = in[0][i];
        }
        else
        {
            // TODO Figure out value ranges expected (0 to 1.0?), call set functions only when value changes
            clouds_reverb.set_amount(wet_dry_mix_value);
            clouds_reverb.set_diffusion(diffusion_value);
            clouds_reverb.set_time(0.5 + (0.49 * time_value));           // 0.5f + (0.49f * patch_position));
            clouds_reverb.set_input_gain(in_level_value);
            clouds_reverb.set_lp(0.3f);                   // : 0.6f);  // TODO add control here

            clouds_reverb.Process(&ins_left[i], &ins_right[i], 1);  // TODO "&" needed?
    
            out[0][i] = ins_left[i];         // Only set the left output (Terrarium is mono)
            //out[1][i] = ins_right[i];
            // Out 3 and 4 are just wet
            //out[2][i] = 0;
            //out[3][i] = 0;

        }
    }
}

int main(void)
{
    float samplerate;

  
    hw.Init();
    samplerate = hw.AudioSampleRate();
    clouds_reverb.Init(reverb_buffer);

    //hw.SetAudioBlockSize(4);

    wetDryMix.Init(hw.knob[Terrarium::KNOB_1], 0.0f, 1.0f, Parameter::LINEAR);
    inLevel.Init(hw.knob[Terrarium::KNOB_2], 0.0f, 1.0f, Parameter::LINEAR);
    time.Init(hw.knob[Terrarium::KNOB_3], 0.0f, 1.0f, Parameter::LINEAR); 
    diffusion.Init(hw.knob[Terrarium::KNOB_4], 0.0f, 1.0f, Parameter::LINEAR); 


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
