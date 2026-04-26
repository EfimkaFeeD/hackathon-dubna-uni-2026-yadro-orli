#include "audio_resampler.hpp"

#include <cstdint>

#include "consts.h"

extern "C" {
#include <libavutil/channel_layout.h>
#include <libavutil/frame.h>
#include <libavutil/samplefmt.h>
#include <libswresample/swresample.h>
}

AudioResampler::AudioResampler() = default;

AudioResampler::~AudioResampler() {
  freeSwr();
}

const int16_t*
AudioResampler::processFrame(const AVFrame* inputFrame) {
  if ((inputFrame == nullptr) || inputFrame->nb_samples <= 0) {
    return nullptr;
  }

  bool needReconfig = (swrCtx_ == nullptr) || (inputFrame->sample_rate != lastInSampleRate) ||
                      (inputFrame->format != lastInSampleFmt) ||
                      (av_channel_layout_compare(&inputFrame->ch_layout, &lastInChLayout) != 0);

  if (needReconfig) {
    freeSwr();
    if (!initSwr(inputFrame)) {
      return nullptr;
    }
  }

  const auto** inData = const_cast<const uint8_t**>(inputFrame->extended_data);
  int inCount         = inputFrame->nb_samples;

  uint8_t* outData[1] = {reinterpret_cast<uint8_t*>(outputBuffer_.data())};
  int outCount        = kOutputSamples;

  int converted = swr_convert(swrCtx_, outData, outCount, inData, inCount);
  if (converted < 0) {
    return nullptr;
  }

  return outputBuffer_.data();
}

bool
AudioResampler::initSwr(const AVFrame* frame) {
  if (swrCtx_) {
    swr_free(&swrCtx_);
  }

  AVChannelLayout in_ch_layout;
  av_channel_layout_default(&in_ch_layout, frame->ch_layout.nb_channels);

  int ret = swr_alloc_set_opts2(&swrCtx_, &outChLayout_, outSampleFmt_, outSampleRate_, &in_ch_layout,
                                static_cast<AVSampleFormat>(frame->format), frame->sample_rate, 0, nullptr);
  if (ret < 0) {
    char errbuf[128];
    av_strerror(ret, errbuf, sizeof(errbuf));
    fprintf(stderr, "swr_alloc_set_opts2 failed: %s\n", errbuf);
    av_channel_layout_uninit(&in_ch_layout);
    return false;
  }

  ret = swr_init(swrCtx_);
  if (ret < 0) {
    char errbuf[128];
    av_strerror(ret, errbuf, sizeof(errbuf));
    fprintf(stderr, "swr_init failed: %s\n", errbuf);
    swr_free(&swrCtx_);
    av_channel_layout_uninit(&in_ch_layout);
    return false;
  }

  lastInSampleRate = frame->sample_rate;
  lastInSampleFmt  = static_cast<AVSampleFormat>(frame->format);

  av_channel_layout_uninit(&lastInChLayout);
  av_channel_layout_copy(&lastInChLayout, &frame->ch_layout);

  av_channel_layout_uninit(&in_ch_layout);
  return true;
}

void
AudioResampler::freeSwr() {
  if (swrCtx_ != nullptr) {
    swr_free(&swrCtx_);
  }
  av_channel_layout_uninit(&lastInChLayout);
  lastInSampleRate = -1;
  lastInSampleFmt  = AV_SAMPLE_FMT_NONE;
}
