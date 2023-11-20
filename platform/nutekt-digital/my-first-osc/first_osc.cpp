/*
 * Copyright 2023 Tarkan Al-Kazily
 *
 * @file first_osc.cpp
 *
 * Saw and Triangle wave oscillator
 *
 */

#include "userosc.h"
#include "first_osc.hpp"

#define N_WAVS 7
typedef MyOsc<N_WAVS> MyOscN;
static MyOscN s_osc;

void OSC_INIT(uint32_t platform, uint32_t api)
{
  (void)platform;
  (void)api;
}

void OSC_CYCLE(const user_osc_param_t * const params,
               int32_t *yn,
               const uint32_t frames)
{
  
  MyOscN::State &s = s_osc.state;
  const MyOscN::Params &p = s_osc.params;

  // Handle events.
  {
    const uint32_t flags = s.flags;
    s.flags = MyOscN::k_flags_none;
    
    // Update the maximum detune range
    if (flags & MyOscN::k_flag_max_detune) {
        s.max_detune = linintf(p.max_detune, MyOscN::kMinDetune, MyOscN::kMaxDetune);
    }

    // Handle new note / pitch value
    s_osc.updatePitch(osc_w0f_for_note((params->pitch)>>8, params->pitch & 0xFF));
    
    // Note reset
    if (flags & MyOscN::k_flag_reset) {
      s.reset();
    }
  }

  q31_t * __restrict y = (q31_t *)yn;
  const q31_t * y_e = y + frames;
  
  for (; y != y_e; ) {
    *(y++) = f32_to_q31(s_osc.nextSample());
  }
}

void OSC_NOTEON(const user_osc_param_t * const params)
{
  s_osc.state.flags |= MyOscN::k_flag_reset;
}

void OSC_NOTEOFF(const user_osc_param_t * const params)
{
  (void)params;
}

void OSC_PARAM(uint16_t index, uint16_t value)
{ 
  MyOscN::Params &p = s_osc.params;
  MyOscN::State &s = s_osc.state;
  
  switch (index) {
  case k_user_osc_param_id1:
    // wave 0
    // select parameter
    break;
    
  case k_user_osc_param_id2:
    // wave 1
    // select parameter
    break;
    
  case k_user_osc_param_id3:
    // sub wave
    // select parameter
    break;
    
  case k_user_osc_param_id4:
    // sub mix
    // percent parameter
    break;
    
  case k_user_osc_param_id5:
    // ring mix
    // percent parameter
    break;
    
  case k_user_osc_param_id6:
    // bit crush
    // percent parameter
    break;
    
  case k_user_osc_param_shape:
    // 10bit parameter
    p.max_detune = param_val_to_f32(value);
    s.flags |= MyOscN::k_flag_max_detune;
    break;
    
  case k_user_osc_param_shiftshape:
    // 10bit parameter
    p.detune = param_val_to_f32(value);
    break;
    
  default:
    break;
  }
}

