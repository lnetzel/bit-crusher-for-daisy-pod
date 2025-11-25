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
float sr_knob = 0;   // POT1 = sample-rate / pitch crush
float bd_knob = 0;   // POT2 = bit depth

// --- State toggles ---
bool pitchTrackingOn = true;   // BUTTON 1 toggles this
bool bypassOn = false;         // BUTTON 2 toggles true bypass

// --- Bitcrusher state ---
struct CrushState {
    float heldSample = 0.f;
    int counter = 0;
};
CrushState L_state, R_state;

// --- Pitch detection state ---
float lastSample = 0.f;
float timeSinceZero = 0.f;
float detectedFreq = 110.f;

// --- Parameters ---
const float pre_gain = 1.0f;
const float post_gain = 0.8f;

// --- Soft clip ---
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

    const int levels = 1 << bit_depth;
    v = floorf(v * levels + 0.5f) / levels;

    v *= post_gain;
    v = MySoftClip(v);

    return v;
}

// --- Pitch tracking via zero-crossing ---
inline void UpdatePitch(float s, float sampleRate)
{
    timeSinceZero += 1.0f;

    if(lastSample < 0.f && s >= 0.f)
    {
        if(timeSinceZero > 0)
        {
            float period = timeSinceZero / sampleRate;
            float freq = 1.0f / period;

            if(freq > 20 && freq < 4000)
                detectedFreq = freq;
        }
        timeSinceZero = 0.f;
    }

    lastSample = s;
}

// --- Audio callback ---
void AudioCallback(float **in, float **out, size_t size)
{
    float sr = DAISY.get_samplerate();

    float crushAmount = sr_knob / 65535.f;
    int bit_depth = 2 + (int)((bd_knob / 65535.f) * 14);

    for(size_t i = 0; i < size; i++)
    {
        float L = in[0][i];
        float R = in[1][i];

        int sr_factor;

        if(pitchTrackingOn)
        {
            UpdatePitch(L, sr);

            float pitch = detectedFreq;
            if(pitch < 1.0f) pitch = 1.0f;

            float pitchFactor = (200.f / pitch);
            if(pitchFactor < 1.f) pitchFactor = 1.f;

            float scaled = 1.f + crushAmount * pitchFactor * 30.f;

            sr_factor = (int)scaled;
        }
        else
        {
            sr_factor = 1 + (int)(crushAmount * 99.f);
        }

        if(sr_factor < 1) sr_factor = 1;
        if(sr_factor > 100) sr_factor = 100;

        // True bypass check
        if(bypassOn)
        {
            out[0][i] = L;
            out[1][i] = R;
        }
        else
        {
            out[0][i] = BitcrushSample(L, L_state, sr_factor, bit_depth);
            out[1][i] = BitcrushSample(R, R_state, sr_factor, bit_depth);
        }
    }
}

// --- Setup / Loop ---
void setup()
{
    analogReadResolution(16);

    hw = DAISY.init(DAISY_POD, AUDIO_SR_48K);
    DAISY.begin(AudioCallback);
}

void loop()
{
    hw.DebounceControls();

    // Read knobs
    sr_knob = analogRead(PIN_POD_POT_1);
    bd_knob = analogRead(PIN_POD_POT_2);

    // --- BUTTON 1: toggle pitch tracking ---
    if(hw.buttons[0].RisingEdge())
    {
        pitchTrackingOn = !pitchTrackingOn;
    }

    // LED 0 = pitch tracking
    hw.leds[0].Set(pitchTrackingOn ? 1.0f : 0.0f,
                    pitchTrackingOn ? 1.0f : 0.0f,
                    pitchTrackingOn ? 1.0f : 0.0f);

    // --- BUTTON 2: toggle true bypass ---
    if(hw.buttons[1].RisingEdge())
    {
        bypassOn = !bypassOn;
    }

    // LED 1 = effect active (off = bypass)
    hw.leds[1].Set(bypassOn ? 0.0f : 1.0f,
                    bypassOn ? 0.0f : 1.0f,
                    bypassOn ? 0.0f : 1.0f);
}
