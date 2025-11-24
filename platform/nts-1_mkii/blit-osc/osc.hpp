#pragma once
/*
    BSD 3-Clause License

    Copyright (c) 2023, KORG INC.
    All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright notice,
this list of conditions and the following disclaimer.

    * Redistributions in binary form must reproduce the above copyright notice,
      this list of conditions and the following disclaimer in the documentation
      and/or other materials provided with the distribution.

    * Neither the name of the copyright holder nor the names of its
      contributors may be used to endorse or promote products derived from
      this software without specific prior written permission.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
    AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
    FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
    DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
    SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
    CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

//*/

/*
 *  File: osc.h
 *
 *  Dummy oscillator template instance.
 *
 */
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
  float w0_;
  float lfo_;

  // local variables related to audio processing
  float phasor_;
};
