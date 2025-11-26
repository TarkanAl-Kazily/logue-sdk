/**
 * Copyright 2025 Tarkan Al-Kazily
 */
#pragma once

#include <math.h>
#include <stdint.h>

#ifdef __cplusplus__
extern "C" {
#endif

#define BITS(x) (((uint64_t)1 << (x)) - 1)

typedef uint64_t q48_16_t;
typedef uint64_t q32_32_t;

static inline q32_32_t float_to_q32_32(float f) {
    uint64_t integer = (uint32_t)trunc(f);
    uint64_t r = (uint32_t)((f - trunc(f)) * ((uint64_t)1 << 32));

    q32_32_t result = (integer << 32) + r;
    return result;
}

static inline float q32_32_to_float(q32_32_t q) {
    float integer = (uint32_t)(q >> 32);
    float r = ((float)(q & BITS(32))) / ((float)BITS(32));

    float result = integer + r;
    return result;
}

static inline q48_16_t float_to_q48_16(float f) {
    uint64_t integer = trunc(f);
    uint64_t r = ((f - trunc(f)) * ((uint64_t)1 << 16));

    q48_16_t result = (integer << 16) + r;
    return result;
}

static inline float q48_16_to_float(q48_16_t q) {
    float integer = (q >> 16);
    float r = ((float)(q & BITS(16))) / ((float)BITS(16));

    float result = integer + r;
    return result;
}

static inline q32_32_t q32_32_add(q32_32_t a, q32_32_t b) { return a + b; }

static inline q48_16_t q48_16_add(q48_16_t a, q48_16_t b) { return a + b; }

static inline q32_32_t q32_32_mul(q32_32_t a, q32_32_t b) {
    // Lose precision by pre-shifting in multiplying decimals but avoids overlow
    return (a >> 16) * (b >> 16);
}

static inline q48_16_t q48_16_mul(q48_16_t a, q48_16_t b) {
    return (a * b) >> 16;
}

#ifdef __cplusplus__
}
#endif
