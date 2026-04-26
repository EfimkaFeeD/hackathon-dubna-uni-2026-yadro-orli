#ifndef PODCAST_SILENCE_REMOVER_SRC_AUDIO_VOICE_ACTIVITY_DETECT_HPP
#define PODCAST_SILENCE_REMOVER_SRC_AUDIO_VOICE_ACTIVITY_DETECT_HPP

#include <cstdint>

#include "consts.h"

struct Fvad;

class AudioVoiceActivityDetect {
 public:
  explicit AudioVoiceActivityDetect(int mode = kDefaultVADMode, int sampleRate = kOutputSampleRate);
  ~AudioVoiceActivityDetect();
  AudioVoiceActivityDetect(const AudioVoiceActivityDetect&)            = delete;
  AudioVoiceActivityDetect& operator=(const AudioVoiceActivityDetect&) = delete;

  [[nodiscard]] bool isVoice(const int16_t* frame, bool simpleVoice) const;
 private:
  Fvad* vad_ = nullptr;
  int sampleRate_;
  int mode_;
  void resetVad();
};

#endif // PODCAST_SILENCE_REMOVER_SRC_AUDIO_VOICE_ACTIVITY_DETECT_HPP
