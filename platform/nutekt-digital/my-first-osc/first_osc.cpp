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

static MyOsc s_osc;

void OSC_INIT(uint32_t platform, uint32_t api)
{
  (void)platform;
  (void)api;
}

void OSC_CYCLE(const user_osc_param_t * const params,
               int32_t *yn,
               const uint32_t frames)
{
  
  MyOsc::State &s = s_osc.state;
  const MyOsc::Params &p = s_osc.params;

  // Handle events.
  {
    const uint32_t flags = s.flags;
    s.flags = MyOsc::k_flags_none;
    
    // Handle new note / pitch value
    s_osc.updatePitch(osc_w0f_for_note((params->pitch)>>8, params->pitch & 0xFF));
    
    // Note reset
    if (flags & MyOsc::k_flag_reset) {
      s.reset();
    }
  }
  
  // Temporaries.
  float phi = s.phi;

  q31_t * __restrict y = (q31_t *)yn;
  const q31_t * y_e = y + frames;
  
  for (; y != y_e; ) {

    const float wavemix = clipminmaxf(0.005f, p.saw_tri_mix, 0.995f);
    
    float sig = (1.f - wavemix) * osc_sawf(phi);
    sig += wavemix * osc_trif(phi);
    
    *(y++) = f32_to_q31(sig);
    
    // Increment phase based on frequency, wrapping in [0, 1)
    phi += s.w;
    phi -= (uint32_t)phi;
  }
  
  s.phi = phi;
}

void OSC_NOTEON(const user_osc_param_t * const params)
{
  s_osc.state.flags |= MyOsc::k_flag_reset;
}

void OSC_NOTEOFF(const user_osc_param_t * const params)
{
  (void)params;
}

void OSC_PARAM(uint16_t index, uint16_t value)
{ 
  MyOsc::Params &p = s_osc.params;
  MyOsc::State &s = s_osc.state;
  
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
    p.saw_tri_mix = param_val_to_f32(value);
    break;
    
  case k_user_osc_param_shiftshape:
    // 10bit parameter
    break;
    
  default:
    break;
  }
}

