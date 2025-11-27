/*
 * Copyright 2025 Tarkan Al-Kazily
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
    inverse_w0 = 0;
    phasor = 0;
    last_edge = 0;
    phase_state = HIGH_PHASE;
    buf_index = 0;
    duty_cycle = 0.5f;
}

///////////////////////////////////////////////
/// PRIVATE FUNCTIONS
///////////////////////////////////////////////

static inline bool in_range(float val, float start, float end) {
    return (val >= start) && (val <= end);
}

void Osc::setPitch(float w0) {
    LOG("setPitch %f", w0);
    state_.reset();
    state_.inverse_w0 = float_to_q48_16(1.0f / w0);
}

void Osc::setShapeLfo(float lfo) { (void)lfo; }

void Osc::setupBlits() {
    for (uint16_t i = 0; i < kBlits; i++) {
        auto& blit = blits_[i];
        float sample_offset = ((float)i) / ((float)kBlits);
        for (uint16_t j = 0; j < kBlitSamples; j++) {
            float pi_x = sample_offset + j - (kBlitSamples >> 1);

            if (pi_x == 0) {
                blit[j] = kBlitScale * 1.0f;
                continue;
            }

            pi_x *= PI;
            blit[j] = kBlitScale * sinf(pi_x) / pi_x;
        }
        LOG("setupBlits i %d %f {%f %f %f %f %f %f %f %f}", i, sample_offset,
            blit[0], blit[1], blit[2], blit[3], blit[4], blit[5], blit[6],
            blit[7]);
    }
}

q48_16_t Osc::getPeriodCycles(const State& s) const {
    q48_16_t fs = float_to_q48_16(getSampleRate());
    return q48_16_mul(fs, s.inverse_w0);
}

void Osc::fillBlitBuffer(const State& s) {
    q48_16_t next_edge = s.last_edge;
    if (s.phase_state == State::HIGH_PHASE) {
        // next_edge is determined by duty cycle length
        next_edge +=
            q48_16_mul(getPeriodCycles(s), float_to_q48_16(s.duty_cycle));
    } else {
        // next_edge is determined by full oscillator period
        next_edge += getPeriodCycles(s);
    }
    uint64_t remainder = next_edge & BITS(16);
    uint64_t which_blit = remainder / kBlits;
    const auto& blit = blits_[which_blit];

    for (uint8_t i = 0; i < kBlitSamples; i++) {
        buf_[i] = s.phase_state == State::HIGH_PHASE ? blit[i] : -blit[i];
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
    state_.last_output = 0.0f;

    setupBlits();
    LOG("DONE INIT");
}

void Osc::process(const float* __restrict in, float* __restrict out,
                  uint32_t frames) {
    (void)in;
    const Params p = params_;
    for (const float* out_end = out + frames; out != out_end; out += 1) {
        const State s = state_;

        float next_output = s.last_output * 0.999f;
        q48_16_t next_edge = s.last_edge;
        if (s.phase_state == State::HIGH_PHASE) {
            // next_edge is determined by duty cycle length
            next_edge +=
                q48_16_mul(getPeriodCycles(s), float_to_q48_16(s.duty_cycle));
        } else {
            // next_edge is determined by full oscillator period
            next_edge += getPeriodCycles(s);
        }
        // TODO: If the duty cycle was updated and now the last edge has already
        // passed, force an edge immediately. Otherwise the polarity goes out of
        // sync and we integrate multiple blits with the same polarity, and the
        // sound dies.
        // Because we reset the oscillator phase completely whenever a new note
        // plays, this doesn't happen if the oscillator frequency changes (but
        // would also be necessary to support glide too). Additionally if we've
        // already done an edge this cycle we should not do a second edge for
        // the same reason.
        if (s.buf_index == 0 && (((next_edge >> 16) - s.phasor == 0) ||
                                 (s.phase_state == State::HIGH_PHASE &&
                                  (s.phasor > (next_edge >> 16))))) {
            fillBlitBuffer(s);
            next_output += buf_[0];
            state_.buf_index = 1;

            if (s.phase_state == State::HIGH_PHASE) {
                // Half oscillator cycle
                state_.phase_state = State::LOW_PHASE;
            } else {
                // Full oscillator cycle
                state_.phase_state = State::HIGH_PHASE;
                state_.last_edge = next_edge;
            }

        } else if (s.buf_index > 0) {
            next_output += buf_[s.buf_index];
            if (s.buf_index == kBlitSamples - 1) {
                state_.buf_index = 0;
            } else {
                state_.buf_index = s.buf_index + 1;
            }
        }

        *out = next_output;

        state_.phasor += 1;
        // Rollover both last_edge and phasor at 48 bits to stay in sync.
        state_.phasor = state_.phasor & BITS(48);
        state_.last_output = next_output;

        // Update duty cycle from params
        if (fabsf(s.duty_cycle - p.shape) <= 0.025) {
            state_.duty_cycle = p.shape;
        } else {
            state_.duty_cycle += (s.duty_cycle < p.shape) ? 0.01 : -0.01;
        }
        // This implementation of the oscillator can only integrate a single
        // blit at a time, so the duty cycle must be kept within a range that
        // ensures the edges are at least kBlitSamples apart.
        const float min_duty_cycle =
            ((float)(kBlitSamples)) / ((float)(getPeriodCycles(state_) >> 16));
        state_.duty_cycle = fminf(fmaxf(state_.duty_cycle, min_duty_cycle),
                                  1.0f - min_duty_cycle);
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
