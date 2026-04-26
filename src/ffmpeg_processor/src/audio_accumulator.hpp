#ifndef PODCAST_SILENCE_REMOVER_SRC_AUDIO_ACCUMULATOR_HPP
#define PODCAST_SILENCE_REMOVER_SRC_AUDIO_ACCUMULATOR_HPP

#include <cstddef>
#include <cstdint>
#include <utility>
#include <vector>

#include "consts.h"

class AudioAccumulator {
 public:
  explicit AudioAccumulator(int targetSamples = kAccumulatorTargetSamples);

  [[nodiscard]] bool addFrame(const int16_t* frame);

  std::pair<const int16_t*, size_t> getAccumulatedData();
  [[nodiscard]] bool isReady() const {
    return buffer_.size() >= targetSamples_;
  }

  void reset() {
    buffer_.clear();
  }

  [[nodiscard]] size_t currentSize() const {
    return buffer_.size();
  }
 private:
  std::vector<int16_t> buffer_;
  size_t targetSamples_;
};

#endif // PODCAST_SILENCE_REMOVER_SRC_AUDIO_ACCUMULATOR_HPP
