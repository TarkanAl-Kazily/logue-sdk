/**
 * square.cpp
 *
 * Square wave oscillator
 *
 * Annotated from checkin 82faa9d
 */

#include "userosc.h"

typedef struct State {
    // ???
    float w0;
    // ???
    float phase;
    // Duty cycle for the square wave
    float duty;
    // ???
    float angle;
    // ???
    float lfo;
    // ???
    float lfoz;
    // Communicate basic state info through a bit field.
    // See following enum for details
    uint8_t flags;
} State;

// Bitfield for State flags
enum {
    k_flag_none = 0,
    k_flag_reset = 1<<0,
};

// Global oscillator state
static State s_state;

void OSC_INIT(uint32_t platform, uint32_t api)
{
    (void)platform;
    (void)api;

    s_state.w0 = 0.f;
    s_state.phase = 0.f;
    s_state.duty = 0.1f;
    s_state.angle = 0.f;
    s_state.lfo = 0.f;
    s_state.lfoz = 0.f;
    s_state.flags = k_flag_none;
}

void OSC_CYCLE(const user_osc_param_t * const params,
        int32_t *yn,
        const uint32_t frames)
{
    // Get flags (if a note pressed event happened)
    const uint8_t flags = s_state.flags;
    s_state.flags = k_flag_none;

    // TODO: Figure out what w0 is
    const float w0 = osc_w0f_for_note((params->pitch)>>8, params->pitch & 0xFF);
    s_state.w0 = w0;
    // Reset phase if note on event happened
    float phase = (flags & k_flag_reset) ? 0.f : s_state.phase;

    const float duty = s_state.duty;
    const float angle = s_state.angle;

    // Get value of LFO that is being applied to the shape parameter
    const float lfo = q31_to_f32(params->shape_lfo);
    s_state.lfo = lfo;
    float lfoz = (flags & k_flag_reset) ? lfo : s_state.lfoz;
    // Delta to modify LFO by after each sample
    const float lfo_inc = (lfo - lfoz) / frames;

    // Current pointer
    q31_t * __restrict y = (q31_t *)yn;
    // End pointer
    const q31_t * y_e = y + frames;

    while (y != y_e) {
        const float pwm = clipminmaxf(0.1f, duty + lfoz, 0.9f);

        float sig = (phase - pwm <= 0.f) ? 1.f : -1.f;

        sig *= 1.f - (angle * phase);

        *(y++) = f32_to_q31(sig);

        phase += w0;
        phase -= (uint32_t)phase;

        lfoz += lfo_inc;
    }

    s_state.phase = phase;
    s_state.lfoz = lfoz;
}

void OSC_NOTEON(const user_osc_param_t * const params)
{
    (void)params;
    s_state.flags |= k_flag_reset;
}

void OSC_NOTEOFF(const user_osc_param_t * const params)
{
    (void)params;
}

void OSC_PARAM(uint16_t index, uint16_t value)
{ 
    const float valf = param_val_to_f32(value);

    switch (index) {
        case k_user_osc_param_id1:
        case k_user_osc_param_id2:
        case k_user_osc_param_id3:
        case k_user_osc_param_id4:
        case k_user_osc_param_id5:
        case k_user_osc_param_id6:
            break;
        case k_user_osc_param_shape:
            s_state.duty = 0.1f + valf * 0.8f;
            break;
        case k_user_osc_param_shiftshape:
            s_state.angle = 0.8f * valf;
            break;
        default:
            break;
    }
}
