#include "audio_voice_activity_detect.hpp"

extern "C" {
#include <fvad.h>
}

#include <cstdint>
#include <stdexcept>

#include "consts.h"

AudioVoiceActivityDetect::AudioVoiceActivityDetect(int mode, int sampleRate) : sampleRate_(sampleRate), mode_(mode) {
  resetVad();
}

AudioVoiceActivityDetect::~AudioVoiceActivityDetect() {
  if (vad_ != nullptr) {
    fvad_free(vad_);
  }
}

bool
AudioVoiceActivityDetect::isVoice(const int16_t* frame, bool simpleVoice) const {
  if (frame == nullptr) {
    return false;
  }
  int result = fvad_process(vad_, frame, kOutputSamples);
  return (result == 1);
}

void
AudioVoiceActivityDetect::resetVad() {
  if (vad_ != nullptr) {
    fvad_free(vad_);
  }
  vad_ = fvad_new();
  if (vad_ == nullptr) {
    throw std::runtime_error("fvad_new failed");
  }
  if (fvad_set_mode(vad_, mode_) < 0) {
    fvad_free(vad_);
    vad_ = nullptr;
    throw std::runtime_error("fvad_set_mode failed");
  }
  if (fvad_set_sample_rate(vad_, sampleRate_) < 0) {
    fvad_free(vad_);
    vad_ = nullptr;
    throw std::runtime_error("fvad_set_sample_rate failed");
  }
}
