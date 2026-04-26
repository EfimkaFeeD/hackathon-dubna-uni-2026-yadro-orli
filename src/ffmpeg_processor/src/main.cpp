extern "C" {
#include <libavutil/channel_layout.h>
#include <libavutil/frame.h>
#include <libavutil/samplefmt.h>
}

#include <cmath>
#include <cstdint>
#include <cstring>
#include <iomanip>
#include <iostream>

#include "audio_accumulator.hpp"
#include "audio_noise_level.hpp"
#include "audio_resampler.hpp"
#include "audio_simple_voice_detect.hpp"
#include "consts.h"

static void
fillDC(AVFrame* frame, float value) {
  float* left  = reinterpret_cast<float*>(frame->extended_data[0]);
  float* right = reinterpret_cast<float*>(frame->extended_data[1]);
  for (int i = 0; i < frame->nb_samples; i++) {
    left[i]  = value;
    right[i] = value;
  }
}

static void
fillQuietSine(AVFrame* frame, double freq, double sampleRate, double timeOffset, float amplitude) {
  float* left  = reinterpret_cast<float*>(frame->extended_data[0]);
  float* right = reinterpret_cast<float*>(frame->extended_data[1]);
  for (int i = 0; i < frame->nb_samples; i++) {
    double t  = static_cast<double>(i) / sampleRate + timeOffset;
    float val = static_cast<float>(amplitude * std::sin(2.0 * M_PI * freq * t));
    left[i]   = val;
    right[i]  = val;
  }
}

int
main() {
  constexpr int kInputSampleRate = 44100;
  constexpr int kInputFrameMs    = 10;
  constexpr int nb_samples       = kInputSampleRate * kInputFrameMs / 1000;
  static_assert(nb_samples > 0);

  constexpr int kTotalFrames = kAccumulatorTargetSamples / kOutputSamples;
  static_assert(kAccumulatorTargetSamples % kOutputSamples == 0,
                "Target duration must be a multiple of frame duration");

  AVFrame* inputFrame = av_frame_alloc();
  if (inputFrame == nullptr) {
    std::cerr << "Failed to allocate input frame.\n";
    return 1;
  }
  inputFrame->format      = AV_SAMPLE_FMT_FLTP;
  inputFrame->sample_rate = kInputSampleRate;
  inputFrame->nb_samples  = nb_samples;
  av_channel_layout_default(&inputFrame->ch_layout, 2);

  if (av_frame_get_buffer(inputFrame, 0) < 0) {
    std::cerr << "Failed to allocate input buffer.\n";
    av_frame_free(&inputFrame);
    return 1;
  }

  AudioResampler resampler;
  AudioAccumulator accum;

  std::cout << "Processing " << kTotalFrames << " frames (" << kTotalFrames * kInputFrameMs << " ms) ...\n";

  double timeOffset = 0.0;
  for (int frameIdx = 0; frameIdx < kTotalFrames; ++frameIdx) {
    bool isVoiceBlock = ((frameIdx / 100) % 2 == 0);

    if (isVoiceBlock) {
      fillDC(inputFrame, 1.0f);
    } else {
      fillQuietSine(inputFrame, 440.0, kInputSampleRate, timeOffset, 0.01f);
    }

    timeOffset += kInputFrameMs / 1000.0;

    const int16_t* pcm = resampler.processFrame(inputFrame);
    if (pcm == nullptr) {
      std::cerr << "Resampling failed at frame " << frameIdx << "!\n";
      av_frame_free(&inputFrame);
      return 1;
    }

    bool ready = accum.addFrame(pcm);

    if ((frameIdx + 1) % 100 == 0) {
      std::cout << "  Frame " << frameIdx + 1 << "/" << kTotalFrames << " (accumulated " << accum.currentSize()
                << " samples)\n";
    }

    if (frameIdx == 0) {
      std::cout << "First 10 samples of first frame (resampled): ";
      for (int i = 0; i < 10; ++i) {
        std::cout << pcm[i] << " ";
      }
      std::cout << "\n";
    }

    if (ready && frameIdx == kTotalFrames - 1) {
      std::cout << "Accumulator is ready!\n";
    }
  }

  if (!accum.isReady()) {
    std::cerr << "Accumulator not ready after " << kTotalFrames << " frames!\n";
    av_frame_free(&inputFrame);
    return 1;
  }

  auto [bigData, bigSamples] = accum.getAccumulatedData();
  std::cout << "\nAccumulated chunk:\n"
            << "  Total samples : " << bigSamples << "\n"
            << "  Duration      : " << (bigSamples * 1000.0 / kOutputSampleRate) << " ms\n"
            << "  First 10 samples : ";
  for (int i = 0; i < 10 && i < bigSamples; ++i)
    std::cout << bigData[i] << " ";
  std::cout << "\n"
            << "  Last 10 samples  : ";
  for (int i = bigSamples - 10; i < bigSamples; ++i)
    std::cout << bigData[i] << " ";
  std::cout << "\n";
 AudioNoiseLevel noiseLevel;
  float noiseDb = noiseLevel.computeNoiseFloor(bigData, bigSamples);
  std::cout << "  Noise floor : " << noiseDb << " dB\n";

  constexpr float kMarginDb = 6.0f;
  AudioSimpleVoiceDetect detector(noiseDb, kMarginDb);
  auto voiceMask = detector.detect(bigData, bigSamples);

  size_t voiceFrames = 0;
  for (bool v : voiceMask) {
    if (v) {
      ++voiceFrames;
    }
  }
  std::cout << "Voice frames: " << voiceFrames << " / " << voiceMask.size() << "\n";

  std::cout << "First 200 decisions: ";
  for (size_t i = 0; i < 200 && i < voiceMask.size(); ++i) {
    std::cout << (voiceMask[i] ? 'V' : 'S');
  }
  std::cout << "\n";

  av_frame_free(&inputFrame);
  std::cout << "\nTest passed.\n";
  return 0;
}
