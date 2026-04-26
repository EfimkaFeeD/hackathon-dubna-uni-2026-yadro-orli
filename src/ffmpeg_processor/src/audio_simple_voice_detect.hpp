#ifndef PODCAST_SILENCE_REMOVER_SRC_AUDIO_SIMPLE_VOICE_DETECT_HPP
#define PODCAST_SILENCE_REMOVER_SRC_AUDIO_SIMPLE_VOICE_DETECT_HPP

#include <cstddef>
#include <cstdint>
#include <vector>

#include "consts.h"

class AudioSimpleVoiceDetect {
 public:
  explicit AudioSimpleVoiceDetect(float noiseFloorDb, float marginDb = kDefaultMarginDb);
  std::vector<bool> detect(const int16_t* samples, size_t totalSamples) const;
  void setNoiseFloorDb(float db) {
    noiseFloorDb_ = db;
  }
  void setMarginDb(float db) {
    marginDb_ = db;
  }
 private:
  float noiseFloorDb_;
  float marginDb_;
};

#endif // PODCAST_SILENCE_REMOVER_SRC_AUDIO_SIMPLE_VOICE_DETECT_HPP
