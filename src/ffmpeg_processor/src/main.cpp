extern "C" {
#include <libavutil/channel_layout.h>
#include <libavutil/frame.h>
#include <libavutil/log.h>
}

#include <cstddef>
#include <cstdint>
#include <iostream>
#include <string>

#include "audio_accumulator.hpp"
#include "audio_file_to_ffmpeg_format.hpp"
#include "audio_noise_level.hpp"
#include "audio_resampler.hpp"
#include "audio_simple_voice_detect.hpp"
#include "audio_voice_activity_detect.hpp"
#include "consts.h"

int
main(int argc, char* argv[]) {
  const char* filename = (argc > 1) ? argv[1] : "input.mp3";

  av_log_set_level(AV_LOG_QUIET);

  AudioFileToFFmpegFormat source(filename);
  AudioResampler resampler;
  AudioAccumulator accum;
  AudioNoiseLevel noiseLevel;
  AudioVoiceActivityDetect vadDetector(1);

  int chunkIndex          = 0;
  size_t totalVoiceFrames = 0;

  std::cout << "Processing file: " << filename << "\n";

  while (true) {
    AVFrame* originalFrame = source.getNextFrame();
    if (originalFrame == nullptr) {
      break;
    }

    const int16_t* pcm = resampler.processFrame(originalFrame);
    if (pcm == nullptr) {
      std::cerr << "Resampling failed\n";
      return 1;
    }

    bool full = accum.addFrame(pcm);

    if (full) {
      chunkIndex++;
      auto [bigData, bigSamples] = accum.getAccumulatedData();

      float noiseDb = noiseLevel.computeNoiseFloor(bigData, bigSamples);

      constexpr float kMarginDb = 6.0f;
      AudioSimpleVoiceDetect simpleDetector(noiseDb, kMarginDb);
      auto simpleMask = simpleDetector.detect(bigData, bigSamples);
      auto finalMask  = vadDetector.detect(bigData, bigSamples, simpleMask);

      size_t voiceFrames = 0;
      for (bool v : finalMask) {
        if (v) {
          ++voiceFrames;
        }
      }
      totalVoiceFrames += voiceFrames;

      std::cout << "\n=== Chunk " << chunkIndex << " ===\n";
      std::cout << "  Duration      : " << (bigSamples * 1000.0 / kOutputSampleRate) << " ms\n";
      std::cout << "  Noise floor   : " << noiseDb << " dB\n";
      std::cout << "  Voice frames  : " << voiceFrames << " / " << finalMask.size() << "\n";

      std::cout << "  First 50 decisions  : ";
      for (size_t i = 0; i < 50 && i < finalMask.size(); ++i) {
        std::cout << (finalMask[i] ? 'V' : 'S');
      }
      std::cout << "\n  Last 50 decisions   : ";
      size_t start = (finalMask.size() > 50) ? finalMask.size() - 50 : 0;
      for (size_t i = start; i < finalMask.size(); ++i) {
        std::cout << (finalMask.at(i) ? 'V' : 'S');
      }
      std::cout << "\n";
    }
  }

  std::cout << "\nProcessing complete.\n";
  std::cout << "Total chunks processed: " << chunkIndex << "\n";
  std::cout << "Total voice frames: " << totalVoiceFrames << "\n";
  return 0;
}
