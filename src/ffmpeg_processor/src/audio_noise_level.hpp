#ifndef PODCAST_SILENCE_REMOVER_SRC_AUDIO_NOISE_LEVEL_HPP
#define PODCAST_SILENCE_REMOVER_SRC_AUDIO_NOISE_LEVEL_HPP

#include <cstddef>
#include <cstdint>
#include <deque>

#include "consts.h"

class AudioNoiseLevel {
public:
  explicit AudioNoiseLevel(int windowMs = kNoiseWindowTargetMs);

  float updateAndGetNoiseFloor(const int16_t* frame);

private:
  std::deque<float> frameDb_;
  size_t maxFrames_;
};

#endif // PODCAST_SILENCE_REMOVER_SRC_AUDIO_NOISE_LEVEL_HPP
