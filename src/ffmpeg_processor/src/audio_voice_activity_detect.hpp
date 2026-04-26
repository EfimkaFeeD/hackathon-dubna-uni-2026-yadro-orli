#ifndef PODCAST_SILENCE_REMOVER_SRC_AUDIO_VOICE_ACTIVITY_DETECT_HPP
#define PODCAST_SILENCE_REMOVER_SRC_AUDIO_VOICE_ACTIVITY_DETECT_HPP

#include <cstdint>
#include <vector>

struct Fvad;

class AudioVoiceActivityDetect {
 public:
  explicit AudioVoiceActivityDetect(int mode = 1, int sampleRate = 48000);
  ~AudioVoiceActivityDetect();

  AudioVoiceActivityDetect(const AudioVoiceActivityDetect&)            = delete;
  AudioVoiceActivityDetect& operator=(const AudioVoiceActivityDetect&) = delete;

  std::vector<bool> detect(const int16_t* audio, size_t totalSamples, const std::vector<bool>& simpleMask);
 private:
  Fvad* vad_ = nullptr;
  int sampleRate_;
  int mode_;
  void resetVad();
};

#endif // PODCAST_SILENCE_REMOVER_SRC_AUDIO_VOICE_ACTIVITY_DETECT_HPP
