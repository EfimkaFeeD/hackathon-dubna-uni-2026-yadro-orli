#include "audio_noise_level.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <vector>

#include "consts.h"

static float
frameRmsDb(const int16_t* frame) {
  double sumSq = 0.0;
  for (int i = 0; i < kOutputSamples; ++i) {
    auto s = static_cast<double>(frame[i]);
    sumSq += s * s;
  }
  double rms = std::sqrt(sumSq / kOutputSamples);
  return 20.0F * std::log10(std::max(rms / 32768.0, 1e-10));
}

AudioNoiseLevel::AudioNoiseLevel(int windowMs) {
  if (windowMs <= 0) {
    windowMs = kNoiseWindowTargetMs;
  }
  maxFrames_ = static_cast<size_t>(windowMs / kOutputFrameMs);
  maxFrames_ = std::max<size_t>(maxFrames_, 1);
}

float
AudioNoiseLevel::updateAndGetNoiseFloor(const int16_t* frame) {
  frameDb_.push_back(frameRmsDb(frame));

  while (frameDb_.size() > maxFrames_) {
    frameDb_.pop_front();
  }

  if (frameDb_.empty()) {
    return -100.0F;
  }
  std::vector sorted(frameDb_.begin(), frameDb_.end());
  std::ranges::sort(sorted);
  auto idx = static_cast<size_t>(sorted.size() * kPercentileOfNoise);
  if (idx >= sorted.size()) {
    idx = sorted.size() - 1;
  }
  return sorted.at(idx);
}
