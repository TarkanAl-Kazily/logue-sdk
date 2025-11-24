/**
 * Copyright Tarkan Al-Kazily
 *
 *  File: osc.hpp
 *  Implementation for a BLIT Square Wave oscillator
 */
#pragma once

#include <array>

#include "processor.h"

class Osc : public Processor {
   public:
    // audio parameters
    enum { SHAPE = 0U, ALT, PARAM3, NUM_PARAMS };

    // Note: Make sure that default param values correspond to declarations in
    // header.c
    struct Params {
        float shape;
        float alt;
        uint32_t param3;

        void reset() {
            shape = 0.f;
            alt = 0.f;
            param3 = 1;
        }

        Params() { reset(); }
    };

    /**
     * @class State
     * @brief Current state of the oscillator
     */
    struct State {
        /**
         * @brief Note frequency to play at
         */
        float w0;
        /**
         * @brief Sample rate
         */
        float fs;
        /**
         * @brief Current phase sample value
         */
        float phasor;
        /**
         * @brief Last oscillator edge for a BLIT integration
         */
        float last_edge;

        /**
         * @brief last output value.
         */
        float last_output;

        /**
         * @brief positive or negative, BLIT phase.
         */
        float polarity;

        bool use_blit_buffer;

        uint16_t buf_index;

        void reset();

        State() { reset(); }
    };

    enum {
        PARAM3_VALUE0 = 0,
        PARAM3_VALUE1,
        PARAM3_VALUE2,
        PARAM3_VALUE3,
        NUM_PARAM3_VALUES,
    };

    // Overridden Processor methods
    uint32_t getBufferSize() const override final { return 0; }
    void setParameter(uint8_t index, int32_t value) override final;
    const char* getParameterStrValue(uint8_t index,
                                     int32_t value) const override final;
    void init(float* buffer) override final;
    void process(const float* __restrict in, float* __restrict out,
                 uint32_t frames) override final;

    // set frequency in digital w (w = f/samplerate, 0.5 is Nyquist)
    void setPitch(float w0);

    // lfo in (-1.0f, 1.0f)
    void setShapeLfo(float lfo);

   private:
    Params params_;
    State state_;

    static constexpr uint16_t kBlitSamples = 8;
    static constexpr uint16_t kBlits = 128;

    std::array<std::array<float, kBlitSamples>, kBlits> blits_;
    std::array<float, kBlitSamples> buf_;

    /**
     * @brief Helper to initialize the blits_ arrays
     */
    void setupBlits();

    /**
     * @brief Setup buf_ with the next blit
     */
    void fillBlitBuffer(const State& s);

    /**
     * @brief Returns the next sample index (float) to integrate a BLIT at.
     *
     * @return sample index (float)
     */
    float getNextPeriod() const;
};
