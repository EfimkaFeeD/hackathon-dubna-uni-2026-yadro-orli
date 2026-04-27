#ifndef PODCAST_SILENCE_REMOVER_SRC_CONSTS_H
#define PODCAST_SILENCE_REMOVER_SRC_CONSTS_H

#include <cstdint>

// AudioSlicer
constexpr int kOutputSampleRate = 48000;
constexpr int kOutputFrameMs    = 20;
constexpr int kOutputSamples    = (kOutputSampleRate * kOutputFrameMs) / 1000;

constexpr double kPercentileOfNoise = 0.05;
constexpr int kNoiseWindowTargetMs  = 10000;


constexpr float kDefaultMarginDb = 4.0F;


constexpr int kDefaultVADMode = 3;


constexpr int kSegmenterBufferMs     = 30000;
constexpr int kSegmenterBufferFrames = kSegmenterBufferMs / kOutputFrameMs;
constexpr int kHangoverFrames        = 8; // Защитный интервал (~160мс)

constexpr int kWordStartMs      = 80;
constexpr int kSentenceStartMs  = 400;
constexpr int kParagraphStartMs = 800;

constexpr int kGapWordStartFrames      = kWordStartMs / kOutputFrameMs;
constexpr int kGapSentenceStartFrames  = kSentenceStartMs / kOutputFrameMs;
constexpr int kGapParagraphStartFrames = kParagraphStartMs / kOutputFrameMs;

#endif