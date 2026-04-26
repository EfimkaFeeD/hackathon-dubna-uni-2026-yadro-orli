#include <cstdint>
#include <iostream>
#include <string>

extern "C" {
#include <libavutil/channel_layout.h>
#include <libavutil/frame.h>
#include <libavutil/log.h>
}

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
  AudioNoiseLevel noiseLevel(kNoiseWindowTargetMs);

  constexpr float kMarginDb = 6.0f;

  AudioSimpleVoiceDetect simpleDetector(kMarginDb);

  AudioVoiceActivityDetect vadDetector(1);

  int64_t frameCount = 0;
  int64_t voiceCount = 0;

  std::cout << "Processing file: " << filename << " (frame-by-frame)\n";

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

    float noiseDb    = noiseLevel.updateAndGetNoiseFloor(pcm);
    bool simpleVoice = simpleDetector.isVoice(pcm, noiseDb);
    bool finalVoice  = vadDetector.isVoice(pcm, simpleVoice);

    if (finalVoice) {
      ++voiceCount;
    }
    ++frameCount;

    if ((frameCount % 1000) == 0) {
      std::cout << "Frames: " << frameCount << "  |  noise floor: " << noiseDb << " dB  |  voice so far: " << voiceCount
                << "\n";
    }
  }

  std::cout << "\nProcessing complete.\n";
  std::cout << "Total frames processed: " << frameCount << "\n";
  std::cout << "Total voice frames : " << voiceCount << "\n";
  return 0;
}
