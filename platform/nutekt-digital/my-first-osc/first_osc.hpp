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

        Params(void) :
            saw_tri_mix(0.0f)
        {}
    };

    /**
     * @brief State for the oscillator.
     */
    struct State {
        // Current phase for the oscillator (phi)
        float phi;
        // Current frequency for the oscillator (omega)
        float w;
        // Current flag field
        uint8_t flags;

        State(void) :
            phi(0.0f),
            w(0.0f),
            flags(k_flags_none)
        {}

        /**
         * @brief Resets the oscillator state on note on event
         */
        void reset(void) {
            phi = 0.0f;
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
        state.w = w;
    }

    State state;
    Params params;
};

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
