/*
 
 Sample Rate (Decimator) and Bit Crush (Reducer) for Daisy Pod (STM32 Cuber Programmer DFU)
 Date: 2025-11-24
 By: Lars Netzel
 Written in Arduino 1.8.19 (Windows Store 1.8.57.0) with some help from ChatGPT5.
  
Chronological summary per input sample
1. Audio sample arrives in in[0][i] / in[1][i].
2. Read POTs → map to sr_factor (sample hold) and bit_depth.
3. Increment counter, check if held sample updates.
4. Hold the sample for sr_factor frames → decimation effect.
5. Pre-gain applied.
6. Quantize to bit_depth → discrete levels.
7. Post-gain + soft clip → smooth the output.
8. Write to output buffer (out[0][i] / out[1][i]).
9. DAC converts to analog output.
10. Repeat for next sample.
 */

#include "DaisyDuino.h"

DaisyHardware hw;

// --- Pot values ---
float sr_knob = 0;   // POT1 = sample-rate reduction
float bd_knob = 0;   // POT2 = bit depth

// --- Bitcrusher state ---
struct CrushState {
    float heldSample = 0.f;
    int counter = 0;
};
CrushState L_state, R_state;

// --- Parameters ---
const float pre_gain = 1.0f;   // Boost input to maximize bit depth effect
const float post_gain = 0.8f;  // Reduce after crush to avoid clipping

// --- Soft clip to smooth harsh digital steps ---
inline float MySoftClip(float x)
{
    if(x > 1.f) return 2.f/3.f;
    if(x < -1.f) return -2.f/3.f;
    return x - (x*x*x)/3.f;
}

// --- Bitcrusher processing ---
inline float BitcrushSample(float in, CrushState &st, int sr_factor, int bit_depth)
{
    st.counter++;
    if(st.counter >= sr_factor)
    {
        st.counter = 0;
        st.heldSample = in;
    }

    float v = st.heldSample * pre_gain;

    // Bit depth reduction
    const int levels = 1 << bit_depth;
    v = floorf(v * levels + 0.5f) / levels;

    // Post gain + soft clip
    v *= post_gain;
    v = MySoftClip(v);

    return v;
}

// --- Audio callback ---
void AudioCallback(float **in, float **out, size_t size)
{
    // Map knobs to ranges
    int sr_factor = 1 + (int)((sr_knob / 65535.f) * 99);    // 1..100 samples hold
    int bit_depth = 2 + (int)((bd_knob / 65535.f) * 14);    // 2..16 bits

    for(size_t i = 0; i < size; i++)
    {
        float L = in[0][i];
        float R = in[1][i];

        out[0][i] = BitcrushSample(L, L_state, sr_factor, bit_depth);
        out[1][i] = BitcrushSample(R, R_state, sr_factor, bit_depth);
    }
}

void setup()
{
    analogReadResolution(16);

    hw = DAISY.init(DAISY_POD, AUDIO_SR_48K);

    DAISY.begin(AudioCallback);
}

void loop()
{
    hw.DebounceControls();

    // Read knobs exactly like your Phaser example
    sr_knob = analogRead(PIN_POD_POT_1); // POT1 = sample-rate reduction
    bd_knob = analogRead(PIN_POD_POT_2); // POT2 = bit depth
}
