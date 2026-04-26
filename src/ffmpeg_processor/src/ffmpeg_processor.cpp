#include "ffmpeg_processor.hpp"
#include <cstdint>
#include "consts.h"

extern "C" {
#include <libavutil/channel_layout.h>
#include <libavutil/frame.h>
#include <libavutil/samplefmt.h>
}

#include <cstring>
#include <stdexcept>

static AVSampleFormat
toAvSampleFormat(const AudioSpec& spec) {
  if (spec.is_float) {
    switch (spec.bits_per_sample) {
      case 32: return AV_SAMPLE_FMT_FLT;
      case 64: return AV_SAMPLE_FMT_DBL;
      default: break;
    }
  } else {
    switch (spec.bits_per_sample) {
      case 8: return AV_SAMPLE_FMT_U8;
      case 16: return spec.is_signed ? AV_SAMPLE_FMT_S16 : AV_SAMPLE_FMT_NONE;
      case 32: return spec.is_signed ? AV_SAMPLE_FMT_S32 : AV_SAMPLE_FMT_NONE;
      default: break;
    }
  }
  throw std::runtime_error("Unsupported audio format");
}

FfmpegProcessor::FfmpegProcessor(const AudioSpec& inputSpec, float marginDb, int vadMode, int noiseWindowMs) :
        inputSampleRate_(static_cast<int>(inputSpec.sample_rate)),
        noiseLevel_(noiseWindowMs),
        simpleDetector_(marginDb),
        vadDetector_(vadMode) {
  int64_t samples = (static_cast<int64_t>(inputSampleRate_) * kOutputFrameMs) / 1000;
  if ((static_cast<int64_t>(inputSampleRate_) * kOutputFrameMs) % 1000 != 0) {
    throw std::runtime_error("Sample rate not compatible with 10 ms frames");
  }
  frameSamples_ = static_cast<int>(samples);
  inputFormat_  = toAvSampleFormat(inputSpec);

  av_channel_layout_default(&inputChLayout_, inputSpec.is_mono ? 1 : 2);
}

FfmpegResult
FfmpegProcessor::process(uint32_t packet_num, const uint8_t* buffer, size_t buffer_len) {
  size_t bytesPerSample = av_get_bytes_per_sample(inputFormat_);
  size_t expectedBytes  = frameSamples_ * bytesPerSample * inputChLayout_.nb_channels;
  if ((buffer == nullptr) || buffer_len < expectedBytes) {
    return {.packet_num = packet_num, .chunk_type = static_cast<uint8_t>(AudioClassifier::kSilence)};
  }

  AVFrame* inputFrame = av_frame_alloc();
  if (inputFrame == nullptr) {
    return {.packet_num = packet_num, .chunk_type = static_cast<uint8_t>(AudioClassifier::kSilence)};
  }
  inputFrame->format      = inputFormat_;
  inputFrame->sample_rate = inputSampleRate_;
  inputFrame->nb_samples  = frameSamples_;
  av_channel_layout_copy(&inputFrame->ch_layout, &inputChLayout_);

  if (av_frame_get_buffer(inputFrame, 0) < 0) {
    av_frame_free(&inputFrame);
    return {.packet_num = packet_num, .chunk_type = static_cast<uint8_t>(AudioClassifier::kSilence)};
  }
  memcpy(inputFrame->data[0], buffer, expectedBytes);

  const int16_t* pcm = resampler_.processFrame(inputFrame);
  av_frame_free(&inputFrame);

  if (pcm == nullptr) {
    return {.packet_num = packet_num, .chunk_type = static_cast<uint8_t>(AudioClassifier::kSilence)};
  }

  float noiseDb            = noiseLevel_.updateAndGetNoiseFloor(pcm);
  bool simpleVoice         = simpleDetector_.isVoice(pcm, noiseDb);
  bool finalVoice          = vadDetector_.isVoice(pcm, simpleVoice);
  AudioClassifier::Tag tag = classifier_.processFrame(finalVoice);

  return {.packet_num = packet_num, .chunk_type = static_cast<uint8_t>(tag)};
}
