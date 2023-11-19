/*
 * Copyright 2023 Tarkan Al-Kazily
 *
 * @file first_osc.hpp
 *
 * Saw and Triangle wave oscillator
 *
 */

#include "userosc.h"

/**
 * @brief Lookup for a triangle wave oscillator
 *
 * @param x phase ratio
 * @return Resulting value for the triangle wave
 */
__fast_inline float osc_trif(float x) {
    if (x < 0.25f) {
        return x * 4.0f;
    }

    if (x < 0.75f) {
        return (0.5f - x) * 4.0f;
    }

    // x < 1.0f
    return (x - 1.0f) * 4.0f;
}

/**
 * @brief My custom oscillator. Morphs between a saw wave and a triangle wave.
 *
 * @details The design of this oscillator is based on the Waves project from Korg.
 *   * MyOsc::Params contains user-specified state for the settings of the oscillator,
 *     which are updated in the OSC_PARAM callback.
 *   * MyOsc::State contains the active oscillator state for determining the output
 *     audio. This is modified over time in the OSC_CYCLE callback. The state.flags
 *     parameter is the one exception, which is a bit field to indicate specific
 *     changes in the parameters / user control that need to be handled in OSC_CYCLE.
 *
 *     The OSC_CYCLE callback is then implemented in two stages:
 *     1. Handle any state changes from sources:
 *        - User params noted by flags
 *        - Note on / off events
 *        - Pitch info
 *        - LFO shape parameter
 */
template <int N>
struct MyOsc {

    // Types
    enum {
        k_flags_none    = 0,
        k_flag_reset    = 1<<1
    };

    /**
     * @brief Parameters for the oscillator.
     */
    struct Params {
        // Ratio of saw to triangle wave. 0 == Saw, 1 == Triangle
        float saw_tri_mix;
        // Detune spread
        float detune;

        Params(void) :
            saw_tri_mix(0.0f),
            detune(0.0f)
        {}
    };

    /**
     * @brief State for the oscillator.
     */
    struct State {
        // Current phase for the oscillator (phi)
        float phi[N] = {0};
        // Current frequency for the oscillator (omega)
        float w[N] = {0};
        // Current flag field
        uint8_t flags;

        State(void) :
            flags(k_flags_none)
        {}

        /**
         * @brief Resets the oscillator state on note on event
         */
        void reset(void) {
            memset(phi, 0, sizeof(phi));
        }
    };

    // Functions

    /**
     * @brief Constructor
     */
    MyOsc(void) : state(), params()
    {}

    /**
     * @brief Updates the pitch with the new note frequency
     *
     * @param w Frequency of the new note
     */
    void updatePitch(const float w) {
        if (N == 1) {
            state.w[0] = w;
            return;
        }

        const float detune_spread = clipminmaxf(0.000f, params.detune, 1.0f);
        // Between 0 and 4 octaves wide
        const float max_detune = linintf(detune_spread, 0.0f, 8.0f);
        const float detune_delta = max_detune / (N - 1);
        float detune_i = - max_detune / 2.0f;
        for (int i = 0; i < N; i++) {
            state.w[i] = w * fasterpow2f(detune_i);
            detune_i += detune_delta;
        }
    }

    /**
     * @brief Gets next sample as float
     *
     * @return float value to convert to q31 with f32_to_q31 for output
     */
    float nextSample() {
        float sig = 0.0f;

        const float wavemix = clipminmaxf(0.005f, params.saw_tri_mix, 0.995f);

        for (int i = 0; i < N; i++) {
            sig += (1.f - wavemix) * osc_sawf(state.phi[i]);
            sig += wavemix * osc_trif(state.phi[i]);

            // Increment phase based on frequency, wrapping in [0, 1)
            state.phi[i] += state.w[i];
            state.phi[i] -= (uint32_t) state.phi[i];
        }

        sig *= kFactor;

        return sig;
    }

    State state;
    Params params;

    static constexpr float kFactor = 1.0f / N;
};
