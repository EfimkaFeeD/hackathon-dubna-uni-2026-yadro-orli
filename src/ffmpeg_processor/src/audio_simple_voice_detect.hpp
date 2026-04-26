#ifndef PODCAST_SILENCE_REMOVER_SRC_AUDIO_SIMPLE_VOICE_DETECT_HPP
#define PODCAST_SILENCE_REMOVER_SRC_AUDIO_SIMPLE_VOICE_DETECT_HPP

#include <cstdint>
#include "consts.h"

class AudioSimpleVoiceDetect {
 public:
  explicit AudioSimpleVoiceDetect(float marginDb = kDefaultMarginDb);

  [[nodiscard]] bool isVoice(const int16_t* frame, float noiseFloorDb) const;

  void setMarginDb(float db) {
    marginDb_ = db;
  }
 private:
  float marginDb_;
};

#endif // PODCAST_SILENCE_REMOVER_SRC_AUDIO_SIMPLE_VOICE_DETECT_HPP
