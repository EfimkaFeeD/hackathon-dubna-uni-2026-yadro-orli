#include "audio_resampler.hpp"

#include "consts.h"

extern "C" {
#include <libavutil/channel_layout.h>
#include <libavutil/frame.h>
#include <libavutil/samplefmt.h>
}

#include <cmath>
#include <cstring>
#include <iostream>

static void
fillSineWave(AVFrame* frame, double freq, double sampleRate) {
  float* left  = reinterpret_cast<float*>(frame->extended_data[0]);
  float* right = reinterpret_cast<float*>(frame->extended_data[1]);
  for (int i = 0; i < frame->nb_samples; i++) {
    double t  = static_cast<double>(i) / sampleRate;
    float val = static_cast<float>(0.5 * std::sin(2.0 * M_PI * freq * t));
    left[i]   = val;
    right[i]  = val;
  }
}

int
main() {
  constexpr int kInputSampleRate = 44100;
  constexpr int kInputFrameMs    = 10;
  const int nb_samples           = kInputSampleRate * kInputFrameMs / 1000;
  static_assert(nb_samples > 0);

  AVFrame* inputFrame = av_frame_alloc();
  if (inputFrame == nullptr) {
    std::cerr << "Failed to allocate input frame.\n";
    return 1;
  }
  inputFrame->format      = AV_SAMPLE_FMT_FLTP;
  inputFrame->sample_rate = kInputSampleRate;
  inputFrame->nb_samples  = nb_samples;

  av_channel_layout_default(&inputFrame->ch_layout, 2);

  if (av_frame_get_buffer(inputFrame, 0) < 0) {
    std::cerr << "Failed to allocate input buffer.\n";
    av_frame_free(&inputFrame);
    return 1;
  }

  fillSineWave(inputFrame, 440.0, kInputSampleRate);

  AudioResampler resampler;
  const int16_t* out = resampler.processFrame(inputFrame);
  if (out == nullptr) {
    std::cerr << "Resampling failed!\n";
    av_frame_free(&inputFrame);
    return 1;
  }

  std::cout << "First 10 output samples (16-bit PCM, mono, 48 kHz):\n";
  for (int i = 0; i < 10 && i < kOutputSamples; ++i) {
    std::cout << out[i] << " ";
  }
  std::cout << "\n";

  std::cout << "Output sample count: " << kOutputSamples << " (" << kOutputBufferSize << " bytes)\n";

  av_frame_free(&inputFrame);
  std::cout << "Test passed.\n";
  return 0;
}
