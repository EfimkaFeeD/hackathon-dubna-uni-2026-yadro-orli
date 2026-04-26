#include "audio_accumulator.hpp"

#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <utility>

#include "consts.h"

AudioAccumulator::AudioAccumulator(int targetSamples) : targetSamples_(targetSamples) {
  if (targetSamples <= 0) {
    throw std::invalid_argument("Target samples must be positive");
  }
  buffer_.reserve(targetSamples_);
}

bool
AudioAccumulator::addFrame(const int16_t* frame) {
  buffer_.insert(buffer_.end(), frame, frame + kAccumulatorFrameSamples);
  return buffer_.size() >= targetSamples_;
}

std::pair<const int16_t*, size_t>
AudioAccumulator::getAccumulatedData() {
  if (buffer_.empty()) {
    return {nullptr, 0};
  }

  const int16_t* data = buffer_.data();
  size_t samples      = buffer_.size();
  buffer_.clear();
  return {data, samples};
}
