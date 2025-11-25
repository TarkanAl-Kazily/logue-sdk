/*
 * copyright 2025 tarkan al-kazily
 */

#include "osc.hpp"

#include "macros.h"
#include "math.h"
#include "osc_api.h"

#if defined(WASM_ENABLED)
#define PI 3.14159265358979f

#include <emscripten/emscripten.h>

#define LOG(...)                                     \
    {                                                \
        emscripten_log(EM_LOG_CONSOLE, __VA_ARGS__); \
    }
#else  // defined(WASM_ENABLED)
#define LOG(x)
#endif  // !defined(WASM_ENABLED)

///////////////////////////////////////////////
/// PROCESSOR STATE FUNCTIONS
///////////////////////////////////////////////

void Osc::State::reset() {
    w0 = 440.0f;
    fs = 48000.0f;
    phasor = 0;
    last_edge = 0.0f;
    last_output = 0.0f;
    polarity = 1.0f;
    buf_index = 0;
}

///////////////////////////////////////////////
/// PRIVATE FUNCTIONS
///////////////////////////////////////////////

static inline bool in_range(float val, float start, float end) {
    return (val >= start) && (val <= end);
}

void Osc::setPitch(float w0) {
    state_.reset();
    state_.w0 = w0;
}

void Osc::setShapeLfo(float lfo) { (void)lfo; }

void Osc::setupBlits() {
    for (uint16_t i = 0; i < kBlits; i++) {
        auto& blit = blits_[i];
        float sample_offset = ((float)i) / ((float)kBlits);
        for (uint16_t j = 0; j < kBlitSamples; j++) {
            float pi_x = sample_offset + j - (kBlitSamples >> 1);

            if (pi_x == 0) {
                blit[j] = 1.0f;
                continue;
            }

            pi_x *= PI;  // TODO: Multiply by state_.fs ?
            blit[j] = sinf(pi_x) / pi_x;
        }
        LOG("setupBlits i %d %f {%f %f %f %f %f %f %f %f}", i, sample_offset,
            blit[0], blit[1], blit[2], blit[3], blit[4], blit[5], blit[6],
            blit[7]);
    }
}

float Osc::getNextPeriod() const {
    float osc_period_samples = state_.fs / state_.w0;
    return state_.last_edge + osc_period_samples;
}

void Osc::fillBlitBuffer(const State& s) {
    float next_period = getNextPeriod();
    float remainder = next_period - ((int)next_period);
    uint16_t which_blit = remainder * kBlits;
    const auto& blit = blits_[which_blit];

    for (uint8_t i = 0; i < kBlitSamples; i++) {
        buf_[i] = blit[i] * s.polarity;
        LOG("blit %d %f", i, buf_[i]);
    }
}

///////////////////////////////////////////////
/// PROCESSOR FUNCTIONS
///////////////////////////////////////////////

void Osc::init(float* buffer) {
    (void)buffer;
    LOG("INIT");
    params_.reset();
    state_.reset();

    state_.fs = getSampleRate();

    setupBlits();
    LOG("DONE INIT");
}

void Osc::process(const float* __restrict in, float* __restrict out,
                  uint32_t frames) {
    (void)in;
    for (const float* out_end = out + frames; out != out_end;
         in += 2, out += 1) {
        const State s = state_;

        float next_output = s.last_output * 0.9999f;
        float next_period = getNextPeriod();
        if (s.buf_index == 0 &&
            (next_period - (kBlitSamples >> 1) - s.phasor < 1.0f) &&
            (next_period - (kBlitSamples >> 1) - s.phasor >= 0.0f)) {
            state_.polarity = -s.polarity;
            state_.buf_index = 1;
            state_.last_edge = next_period;
            fillBlitBuffer(s);
            next_output += buf_[0];
            // TODO: Handle large floats
        }

        if (s.buf_index > 0) {
            next_output += buf_[s.buf_index];
            if (s.buf_index == kBlitSamples - 1) {
                state_.buf_index = 0;
            } else {
                state_.buf_index = s.buf_index + 1;
            }
        }

        *out = next_output * 2.0f - 1.0f;

        state_.phasor += 1;
        state_.last_output = next_output;
    }
}

const char* Osc::getParameterStrValue(uint8_t index, int32_t value) const {
    // Note: String memory must be accessible even after function returned.
    //       It can be assumed that caller will have copied or used the string
    //       before the next call to getParameterStrValue
    static const char* param3_strings[NUM_PARAM3_VALUES] = {
        "VAL 0",
        "VAL 1",
        "VAL 2",
        "VAL 3",
    };

    switch (index) {
        case PARAM3:
            if (value >= PARAM3_VALUE0 && value < NUM_PARAM3_VALUES)
                return param3_strings[value];
            break;
        default:
            break;
    }

    return nullptr;
}

void Osc::setParameter(uint8_t index, int32_t value) {
    switch (index) {
        case SHAPE:
            params_.shape =
                param_10bit_to_f32(value);  // 0 .. 1023 -> 0.0 .. 1.0
            break;

        case ALT:
            params_.alt = param_10bit_to_f32(value);  // 0 .. 1023 -> 0.0 .. 1.0
            break;

        case PARAM3:
            params_.param3 = value;  // string type, receiving index
            break;

        default:
            break;
    }
}
