#include "audio_voice_activity_detect.hpp"

#include <cstdint>
#include <cstring>
#include <fvad.h>
#include <stdexcept>
#include <vector>

#include "consts.h"

AudioVoiceActivityDetect::AudioVoiceActivityDetect(int mode, int sampleRate) : sampleRate_(sampleRate), mode_(mode) {
  resetVad();
}

AudioVoiceActivityDetect::~AudioVoiceActivityDetect() {
  if (vad_ != nullptr) {
    fvad_free(vad_);
  }
}

std::vector<bool>
AudioVoiceActivityDetect::detect(const int16_t* audio, size_t totalSamples, const std::vector<bool>& simpleMask) {
  if ((audio == nullptr) || totalSamples == 0 || (totalSamples % kOutputSamples) != 0) {
    return {};
  }
  if (simpleMask.size() != totalSamples / kOutputSamples) {
    return {};
  }

  size_t numFrames = simpleMask.size();
  std::vector<bool> finalMask;
  finalMask.reserve(numFrames);

  for (size_t f = 0; f < numFrames; ++f) {
    bool simpleSaysVoice = simpleMask.at(f);
    if (!simpleSaysVoice) {
      finalMask.push_back(false);
      continue;
    }

    const int16_t* frame = audio + (f * kOutputSamples);
    int vadResult        = fvad_process(vad_, frame, kOutputSamples);
    if (vadResult < 0) {
      finalMask.push_back(false);
      continue;
    }
    finalMask.push_back(vadResult == 1);
  }

  return finalMask;
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
