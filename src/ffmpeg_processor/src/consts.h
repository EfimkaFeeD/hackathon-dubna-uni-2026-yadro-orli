#ifndef PODCAST_SILENCE_REMOVER_SRC_CONSTS_H
#define PODCAST_SILENCE_REMOVER_SRC_CONSTS_H

#include <cstdint>

constexpr int kOutputSampleRate = 48000;
constexpr int kOutputFrameMs    = 10;
constexpr int kOutputSamples    = (kOutputSampleRate * kOutputFrameMs) / 1000;
constexpr int kOutputBufferSize = kOutputSamples * sizeof(int16_t);

#endif // PODCAST_SILENCE_REMOVER_SRC_CONSTS_H
