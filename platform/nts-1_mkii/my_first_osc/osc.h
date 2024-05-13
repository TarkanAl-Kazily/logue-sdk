#pragma once
/*
    BSD 3-Clause License

    Copyright (c) 2023, KORG INC.
    All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright notice, this
      list of conditions and the following disclaimer.

    * Redistributions in binary form must reproduce the above copyright notice,
      this list of conditions and the following disclaimer in the documentation
      and/or other materials provided with the distribution.

    * Neither the name of the copyright holder nor the names of its
      contributors may be used to endorse or promote products derived from
      this software without specific prior written permission.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
    AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
    DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
    FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
    DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
    SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
    CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
    OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
    OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

//*/

/*
 *  File: osc.h
 *
 *  Dummy oscillator template instance.
 *
 */

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <climits>

#include "unit_osc.h"   // Note: Include base definitions for osc units

#include "utils/int_math.h"   // for clipminmaxi32()

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
class Osc {
 public:
  /*===========================================================================*/
  /* Public Data Structures/Types/Enums. */
  /*===========================================================================*/

  // Types
  enum {
      k_flags_none    = 0,
      k_flag_reset    = 1<<1,
      k_flag_max_detune = 1<<2,
  };

  enum {
    SHAPE = 0U,
    ALT,
    NUM_PARAMS
  };

  // Note: Make sure that default param values correspond to declarations in header.c
  struct Params {
    // max_detune
    float shape{0.f};
    // detune spread fine control
    float alt{0.f};

    void reset() {
      shape = 0.f;
      alt = 0.f;
    }
  };

  /**
   * @brief State for the oscillator.
   */
  struct State {
      // Current phase for the oscillator (phi)
      float phi[N] = {0};
      // Current frequency for the oscillator (omega)
      float w[N] = {0};
      // Current max detune between all the oscillators
      float max_detune = kMinDetune;
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

  /*===========================================================================*/
  /* Lifecycle Methods. */
  /*===========================================================================*/

  Osc<N>(void) {}
  ~Osc<N>(void) {} // Note: will never actually be called for statically allocated instances

  inline int8_t Init(const unit_runtime_desc_t * desc) {
    if (!desc)
      return k_unit_err_undef;
    
    // Note: make sure the unit is being loaded to the correct platform/module target
    if (desc->target != unit_header.target)
      return k_unit_err_target;
    
    // Note: check API compatibility with the one this unit was built against
    if (!UNIT_API_IS_COMPAT(desc->api))
      return k_unit_err_api_version;

    // Check compatibility of samplerate with unit, for NTS-1 MKII should be 48000
    if (desc->samplerate != 48000)
      return k_unit_err_samplerate;

    // Check compatibility of frame geometry
    // Note: NTS-1 mkII oscillators can make use of the audio input depending on the routing options in global settings, see product documentation for details.
    if (desc->input_channels != 2 || desc->output_channels != 1)  // should be stereo input / mono output
      return k_unit_err_geometry;

    // Note: SDRAM is not available from the oscillator runtime environment
    
    // Cache the runtime descriptor for later use
    runtime_desc_ = *desc;

    // Make sure parameters are reset to default values
    params_.reset();
    state_.reset();
    
    return k_unit_err_none;
  }

  inline void Teardown() {
    // Note: cleanup and release resources if any
  }

  inline void Reset() {
    // Note: Reset effect state, excluding exposed parameter values.
  }

  inline void Resume() {
    // Note: Effect will resume and exit suspend state. Usually means the synth
    // was selected and the render callback will be called again
  }

  inline void Suspend() {
    // Note: Effect will enter suspend state. Usually means another effect was
    // selected and thus the render callback will not be called
  }

  /**
   * @brief Updates the pitch with the new note frequency
   *
   * @param w Frequency of the new note
   */
  inline void updatePitch(const float w) {
    if (N == 1) {
      state_.w[0] = w;
      return;
    }

    const float detune_spread = clipminmaxf(0.000f, params_.alt, 1.0f);
    // Between 0 and 4 octaves wide
    const float max_detune = linintf(detune_spread, 0.0f, state_.max_detune);
    const float detune_delta = max_detune / (N - 1);
    float detune_i = - max_detune / 2.0f;
    for (int i = 0; i < N; i++) {
        state_.w[i] = w * fasterpow2f(detune_i);
        detune_i += detune_delta;
    }
  }

  /*===========================================================================*/
  /* Other Public Methods. */
  /*===========================================================================*/

  fast_inline void Process(const float * in, float * out, size_t frames) {
    const float * __restrict in_p = in;
    float * __restrict out_p = out;
    const float * out_e = out_p + frames;  // assuming mono output

    const uint32_t flags = state_.flags;
    state_.flags = k_flags_none;

    // Update the maximum detune range
    if (flags & k_flag_max_detune) {
        state_.max_detune = linintf(params_.shape, kMinDetune, kMaxDetune);
    }

    // Handle new note / pitch value
    const unit_runtime_osc_context_t *ctxt = static_cast<const unit_runtime_osc_context_t *>(runtime_desc_.hooks.runtime_context);
    updatePitch(osc_w0f_for_note((ctxt->pitch)>>8, ctxt->pitch & 0xFF));

    // Note reset
    if (flags & k_flag_reset) {
      state_.reset();
    }

    for (; out_p != out_e; in_p += 2, out_p += 1) {
      // Process/generate samples here
      float sig = 0.0f;

      for (int i = 0; i < N; i++) {
        sig += osc_sawf(state_.phi[i]);

        // Increment phase based on frequency, wrapping in [0, 1)
        state_.phi[i] += state_.w[i];
        state_.phi[i] -= (uint32_t) state_.phi[i];
      }

      sig *= kFactor;

      *out_p = sig;
    }
  }

  inline void setParameter(uint8_t index, int32_t value) {
    switch (index) {
    case SHAPE:
      // 10bit 0-1023 parameter
      value = clipminmaxi32(0, value, 1023);
      params_.shape = param_10bit_to_f32(value); // 0 .. 1023 -> 0.0 .. 1.0
      state_.flags |= k_flag_max_detune;
      break;

    case ALT:
      // 10bit 0-1023 parameter
      value = clipminmaxi32(0, value, 1023);
      params_.alt = param_10bit_to_f32(value); // 0 .. 1023 -> 0.0 .. 1.0
      break;

    default:
      break;
    }
  }

  inline int32_t getParameterValue(uint8_t index) const {
    switch (index) {
    case SHAPE:
      // 10bit 0-1023 parameter
      return param_f32_to_10bit(params_.shape);
      break;

    case ALT:
      // 10bit 0-1023 parameter
      return param_f32_to_10bit(params_.alt);
      break;

    default:
      break;
    }

    return INT_MIN; // Note: will be handled as invalid
  }

  inline const char * getParameterStrValue(uint8_t index, int32_t value) const {
    // Note: String memory must be accessible even after function returned.
    //       It can be assumed that caller will have copied or used the string
    //       before the next call to getParameterStrValue
    
    return nullptr;
  }

  inline void setTempo(uint32_t tempo) {
    // const float bpmf = (tempo >> 16) + (tempo & 0xFFFF) / static_cast<float>(0x10000);
    (void)tempo;
  }

  inline void tempo4ppqnTick(uint32_t counter) {
    (void)counter;
  }

  inline void NoteOn(uint8_t note, uint8_t velo) {
    (uint8_t)note;
    (uint8_t)velo;
    state_.flags |= k_flag_reset;
  }

  inline void NoteOff(uint8_t note) {
    (uint8_t)note;
  }

  inline void AllNoteOff() {
  }

  inline void PitchBend(uint8_t bend) {
    (uint8_t)bend;
  }

  inline void ChannelPressure(uint8_t press) {
    (uint8_t)press;
  }

  inline void AfterTouch(uint8_t note, uint8_t press) {
    (uint8_t)note;
    (uint8_t)press;
  }

  
  /*===========================================================================*/
  /* Static Members. */
  /*===========================================================================*/
  
 private:
  /*===========================================================================*/
  /* Private Member Variables. */
  /*===========================================================================*/

  std::atomic_uint_fast32_t flags_;

  unit_runtime_desc_t runtime_desc_;

  Params params_;
  State state_;

  /*===========================================================================*/
  /* Private Methods. */
  /*===========================================================================*/

  /*===========================================================================*/
  /* Constants. */
  /*===========================================================================*/
  static constexpr float kFactor = 1.0f / N;
  static constexpr float kMinDetune = 0.0833f;
  static constexpr float kMaxDetune = 8.0f;
};
