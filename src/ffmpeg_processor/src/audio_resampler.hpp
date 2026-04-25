#ifndef PODCAST_SILENCE_REMOVER_SRC_AUDIO_RESAMPLER_HPP
#define PODCAST_SILENCE_REMOVER_SRC_AUDIO_RESAMPLER_HPP

#include <cstdint>

extern "C" {
#include <libavutil/channel_layout.h>
#include <libavutil/frame.h>
#include <libavutil/samplefmt.h>
#include <libswresample/swresample.h>
}

#include <array>

#include "consts.h"

class AudioResampler {
 public:
  AudioResampler();
  ~AudioResampler();

  AudioResampler(const AudioResampler&)            = delete;
  AudioResampler& operator=(const AudioResampler&) = delete;
  const int16_t* processFrame(const AVFrame* inputFrame);
 private:
  SwrContext* swrCtx_ = nullptr;

  AVChannelLayout outChLayout_ = AV_CHANNEL_LAYOUT_MONO;
  AVSampleFormat outSampleFmt_ = AV_SAMPLE_FMT_S16;
  int outSampleRate_           = kOutputSampleRate;

  int lastInSampleRate           = -1;
  AVSampleFormat lastInSampleFmt = AV_SAMPLE_FMT_NONE;
  AVChannelLayout lastInChLayout = {};

  std::array<int16_t, kOutputSamples> outputBuffer_{};

  bool initSwr(const AVFrame* frame);

  void freeSwr();
};

#endif // PODCAST_SILENCE_REMOVER_SRC_AUDIO_RESAMPLER_HPP
