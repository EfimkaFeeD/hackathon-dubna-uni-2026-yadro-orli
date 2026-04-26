#ifndef PODCAST_SILENCE_REMOVER_SRC_CONSTS_H
#define PODCAST_SILENCE_REMOVER_SRC_CONSTS_H

#include <cstdint>

// AudioSlicer
constexpr int kOutputSampleRate = 48000; // 48 kHz
constexpr int kOutputFrameMs    = 10;
constexpr int kOutputSamples    = (kOutputSampleRate * kOutputFrameMs) / 1000; // 480
constexpr int kOutputBufferSize = kOutputSamples * sizeof(int16_t);

// AudioAccumulator settings
constexpr int kAccumulatorFrameSamples  = kOutputSamples;
constexpr int kAccumulatorFrameMs       = kOutputFrameMs;
constexpr int kAccumulatorTargetMs      = 10000;                                             // 10s
constexpr int kAccumulatorTargetSamples = (kOutputSampleRate * kAccumulatorTargetMs) / 1000; // 480000

#endif // PODCAST_SILENCE_REMOVER_SRC_CONSTS_H
