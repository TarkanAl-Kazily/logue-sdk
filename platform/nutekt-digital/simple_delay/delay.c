/**
 * Copyright 2023 Tarkan Al-Kazily
 *
 * @file delay.cpp
 * @brief Basic delay effect with controllable time, feedback, and mix parameters
 */

#include <userdelfx.h> // DELFX_* macros
#include <osc_api.h> // k_samplerate

#define kDelayBufferSize (k_samplerate * 2) // 2 seconds worth of samples

typedef struct my_delay_s {
    // Buffer for all samples
    float *buffer;
    size_t buffer_len;
    size_t index;

    float wet_mix;
    float dry_mix; // calculated once to be 1.0 - wet_mix
    float feedback;
    uint32_t time_samples; // half a second worth of samples
} my_delay_t;

// Efficient SDRAM memory
float g_my_buffer[kDelayBufferSize] __sdram;

static my_delay_t delay_s;

/*
 * Private Functions
 */

__fast_inline void init(float *buf, const size_t buf_len) {
    memset(buf, 0, buf_len * sizeof(float));

    delay_s.buffer = buf;
    delay_s.buffer_len = buf_len;

    delay_s.index = 0;

    delay_s.wet_mix = 0.5;
    delay_s.dry_mix = 0.5;
    delay_s.feedback = 0.2;
    delay_s.time_samples = 1.0 * k_samplerate;
}

__fast_inline void process(float *xn, uint32_t frames) {
    float *sample_p = xn;
    // There are two samples per frame
    const float *sample_end = xn + frames * 2;
    for (; sample_p != sample_end; sample_p++) {
        // Compute new output signal from mix value
        float dry = *sample_p;
        float wet = delay_s.buffer[delay_s.index];
        float yn = dry * delay_s.dry_mix + wet * delay_s.wet_mix;

        // Compute new buffer signal
        delay_s.buffer[delay_s.index] = delay_s.feedback * (wet + dry);

        // Assign output (making mono)
        *sample_p = yn;
        sample_p++;
        *(sample_p) = yn;
        delay_s.index = (delay_s.index + 1) % delay_s.time_samples;
    }
}

/*
 * DELFX_ API
 */

void DELFX_INIT(uint32_t platform, uint32_t api) {
    (void) platform;
    (void) api;
    init(g_my_buffer, kDelayBufferSize);
}
 
void DELFX_PROCESS(float *xn, uint32_t frames) {
    process(xn, frames);
}
 
void DELFX_SUSPEND(void) {
}
 
void DELFX_RESUME(void) {
}
 
void DELFX_PARAM(uint8_t index, int32_t value) {
    const float percent = q31_to_f32(value);
    switch (index) {
        case (k_user_delfx_param_time):
            // Time setting
            delay_s.time_samples = k_samplerate * linintf(percent, 0.001f, 1.9f); 
            delay_s.index = delay_s.index % delay_s.time_samples;
            memset(delay_s.buffer + delay_s.time_samples, 0, delay_s.buffer_len - delay_s.time_samples);
            break;
        case (k_user_delfx_param_depth):
            delay_s.feedback = percent;
            break;
        case (k_user_delfx_param_shift_depth):
            delay_s.wet_mix = percent;
            delay_s.dry_mix = 1.0f - percent;
            break;
        default:
            break;
    }
}
