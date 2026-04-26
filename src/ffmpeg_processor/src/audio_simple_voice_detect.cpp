#include "audio_simple_voice_detect.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <vector>

#include "consts.h"

AudioSimpleVoiceDetect::AudioSimpleVoiceDetect(float noiseFloorDb, float marginDb) :
        noiseFloorDb_(noiseFloorDb), marginDb_(marginDb) {}

std::vector<bool>
AudioSimpleVoiceDetect::detect(const int16_t* samples, size_t totalSamples) const {
  if ((samples == nullptr) || totalSamples == 0 || (totalSamples % kOutputSamples) != 0) {
    return {};
  }

  const size_t numFrames = totalSamples / kOutputSamples;
  std::vector<bool> result;
  result.reserve(numFrames);

  const float thresholdDb = noiseFloorDb_ + marginDb_;

  for (size_t f = 0; f < numFrames; ++f) {
    const int16_t* frame = samples + (f * kOutputSamples);

    double sumSq = 0.0;
    for (int i = 0; i < kOutputSamples; ++i) {
      auto s = static_cast<double>(frame[i]);
      sumSq += s * s;
    }
    double rms  = std::sqrt(sumSq / kOutputSamples);
    float rmsDb = 20.0F * std::log10(std::max(rms / 32768.0, 1e-10));

    bool isVoice = (rmsDb > thresholdDb);
    result.push_back(isVoice);
  }

  return result;
}
