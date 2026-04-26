#include "audio_simple_voice_detect.hpp"

#include <cmath>
#include <algorithm>
#include <cstdint>

#include "consts.h"

AudioSimpleVoiceDetect::AudioSimpleVoiceDetect(float marginDb)
    : marginDb_(marginDb) {}

bool AudioSimpleVoiceDetect::isVoice(const int16_t* frame, float noiseFloorDb) const {
  if (frame == nullptr) {
    return false;
  }

  double sumSq = 0.0;
  for (int i = 0; i < kOutputSamples; ++i) {
    auto s = static_cast<double>(frame[i]);
    sumSq += s * s;
  }
  double rms = std::sqrt(sumSq / kOutputSamples);

  float rmsDb = 20.0F * std::log10(std::max(rms / 32768.0, 1e-10));

  return rmsDb > (noiseFloorDb + marginDb_);
}
