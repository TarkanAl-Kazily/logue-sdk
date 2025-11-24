/*
 * copyright 2025 tarkan al-kazily
 */

#include "osc.hpp"

///////////////////////////////////////////////
/// PRIVATE FUNCTIONS
///////////////////////////////////////////////

void Osc::setPitch(float w0) {
  w0_ = w0;  // use this as the phase increment for oscillator
}

void Osc::setShapeLfo(float lfo) { lfo_ = lfo; }

///////////////////////////////////////////////
/// PROCESSOR FUNCTIONS
///////////////////////////////////////////////

void Osc::init(float* buffer) {
  (void)buffer;
  params_.reset();
  phasor_ = 0.f;
}

void Osc::process(const float* __restrict in, float* __restrict out,
                  uint32_t frames) {
  // caching current parameter values. consider smoothing sensitive parameters
  // in audio loop
  for (const float* out_end = out + frames; out != out_end; in += 2, out += 1) {
    // process/generate samples here

    // phasor update
    phasor_ = fmodf(phasor_ + w0_, 1.f);

    // read sine wave table
    out[0] = osc_sinf(phasor_);
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
      params_.shape = param_10bit_to_f32(value);  // 0 .. 1023 -> 0.0 .. 1.0
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
