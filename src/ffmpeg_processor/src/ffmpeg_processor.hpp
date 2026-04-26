#ifndef PODCAST_SILENCE_REMOVER_SRC_FFMPEG_PROCESSOR_HPP
#define PODCAST_SILENCE_REMOVER_SRC_FFMPEG_PROCESSOR_HPP

#include <cstddef>
extern "C" {
#include <libavutil/channel_layout.h>
#include <libavutil/samplefmt.h>
}

#include <cstdint>

#include "audio_classifier.hpp"
#include "audio_noise_level.hpp"
#include "audio_resampler.hpp"
#include "audio_simple_voice_detect.hpp"
#include "audio_voice_activity_detect.hpp"
#include "consts.h"

struct AudioSpec {
  uint32_t sample_rate;
  bool is_mono;
  uint8_t bits_per_sample;
  bool is_signed;
  bool is_float;
};

struct FfmpegResult {
  uint32_t packet_num;
  uint8_t chunk_type;
};

class FfmpegProcessor {
 public:
  explicit FfmpegProcessor(const AudioSpec& inputSpec, float marginDb = kDefaultMarginDb, int vadMode = kDefaultVADMode,
                           int noiseWindowMs = kNoiseWindowTargetMs);
  FfmpegResult process(uint32_t packet_num, const uint8_t* buffer, size_t buffer_len);
 private:
  int inputSampleRate_;
  int frameSamples_;
  AVSampleFormat inputFormat_;
  AVChannelLayout inputChLayout_{};

  AudioResampler resampler_;
  AudioNoiseLevel noiseLevel_;
  AudioSimpleVoiceDetect simpleDetector_;
  AudioVoiceActivityDetect vadDetector_;
  AudioClassifier classifier_;
};

#endif // PODCAST_SILENCE_REMOVER_SRC_FFMPEG_PROCESSOR_HPP
