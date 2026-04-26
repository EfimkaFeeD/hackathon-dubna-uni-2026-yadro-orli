#ifndef PODCAST_SILENCE_REMOVER_SRC_CONSTS_H
#define PODCAST_SILENCE_REMOVER_SRC_CONSTS_H

#include <cstdint>

// AudioSlicer
constexpr int kOutputSampleRate = 48000; // 48 kHz
constexpr int kOutputFrameMs    = 10;
constexpr int kOutputSamples    = (kOutputSampleRate * kOutputFrameMs) / 1000; // 480
constexpr int kOutputBufferSize = kOutputSamples * sizeof(int16_t);

// AudioNoiseLevel
constexpr double kPercentileOfNoise = 0.10; // 10%
constexpr int kNoiseWindowTargetMs  = 10000;

// AudioSimpleVoiceDetect
constexpr float kDefaultMarginDb = 3.0F;

// AudioVoiceActivityDetect
constexpr int kDefaultVADMode = 1;

// AudioSegmenter
constexpr int kSegmenterBufferMs     = 30000;                               // 30 seconds
constexpr int kSegmenterBufferFrames = kSegmenterBufferMs / kOutputFrameMs; // 3000

constexpr int kWordStartMs      = 150;
constexpr int kSentenceStartMs  = 300;
constexpr int kParagraphStartMs = 500;

constexpr int kGapWordStartFrames      = kWordStartMs / kOutputFrameMs;      // 15
constexpr int kGapSentenceStartFrames  = kSentenceStartMs / kOutputFrameMs;  // 30
constexpr int kGapParagraphStartFrames = kParagraphStartMs / kOutputFrameMs; // 50

#endif // PODCAST_SILENCE_REMOVER_SRC_CONSTS_H
