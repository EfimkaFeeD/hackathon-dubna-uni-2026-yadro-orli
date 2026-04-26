#ifndef PODCAST_SILENCE_REMOVER_SRC_AUDIO_NOISE_LEVEL_HPP
#define PODCAST_SILENCE_REMOVER_SRC_AUDIO_NOISE_LEVEL_HPP

#include <cstddef>
#include <cstdint>

class AudioNoiseLevel {
 public:
  float computeNoiseFloor(const int16_t* samples, size_t count);
 private:
  static float computePercentileNoiseFloor(const int16_t* samples, size_t count);
};

#endif // PODCAST_SILENCE_REMOVER_SRC_AUDIO_NOISE_LEVEL_HPP
