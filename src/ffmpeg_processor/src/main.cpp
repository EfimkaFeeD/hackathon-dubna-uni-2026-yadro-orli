#include <cstdint>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>

extern "C" {
#include <libavutil/avutil.h>
#include <libavutil/channel_layout.h>
#include <libavutil/frame.h>
#include <libavutil/log.h>
#include <libavutil/samplefmt.h>
}

#include "audio_file_to_ffmpeg_format.hpp"
#include "consts.h"
#include "ffmpeg_processor.hpp"

static size_t
interleaveFrame(AVFrame* frame, std::vector<uint8_t>& buffer) {
  if ((frame == nullptr) || frame->nb_samples <= 0) {
    return 0;
  }

  int planes =
    (av_sample_fmt_is_planar(static_cast<AVSampleFormat>(frame->format)) != 0) ? frame->ch_layout.nb_channels : 1;
  size_t bps   = av_get_bytes_per_sample(static_cast<AVSampleFormat>(frame->format));
  size_t total = frame->nb_samples * frame->ch_layout.nb_channels * bps;
  buffer.resize(total);

  if (planes == 1) {
    memcpy(buffer.data(), frame->data[0], total);
  } else {
    uint8_t* dst = buffer.data();
    for (int s = 0; s < frame->nb_samples; ++s) {
      for (int ch = 0; ch < frame->ch_layout.nb_channels; ++ch) {
        memcpy(dst, frame->data[ch] + (s * bps), bps);
        dst += bps;
      }
    }
  }
  return total;
}

int
main(int argc, char* argv[]) {
  const char* filename = (argc > 1) ? argv[1] : "input.mp3";
  av_log_set_level(AV_LOG_QUIET);

  AudioFileToFFmpegFormat source(filename);

  AudioSpec spec{};
  spec.sample_rate     = static_cast<uint32_t>(source.getSampleRate());
  spec.is_mono         = source.getChannels() == 1;
  AVSampleFormat fmt   = source.getSampleFormat();
  spec.bits_per_sample = av_get_bytes_per_sample(fmt) * 8;
  spec.is_float =
    (fmt == AV_SAMPLE_FMT_FLT || fmt == AV_SAMPLE_FMT_FLTP || fmt == AV_SAMPLE_FMT_DBL || fmt == AV_SAMPLE_FMT_DBLP);
  spec.is_signed = !(fmt == AV_SAMPLE_FMT_U8 || fmt == AV_SAMPLE_FMT_U8P);

  constexpr float kMarginDb = 6.0F;
  FfmpegProcessor processor(spec, kMarginDb, 3);

  uint32_t packetNum   = 0;
  int64_t totalFrames  = 0;
  int64_t tagCounts[5] = {};

  std::cout << "Processing file: " << filename << " (" << spec.sample_rate << " Hz, "
            << (spec.is_mono ? "mono" : "stereo") << ")\n";

  std::vector<uint8_t> rawBuffer;

  while (true) {
    AVFrame* frame = source.getNextFrame();
    if (frame == nullptr) {
      break;
    }
    size_t bufSize = interleaveFrame(frame, rawBuffer);
    if (bufSize == 0) {
      continue;
    }

    PluginResult res = processor.process(packetNum, rawBuffer.data(), bufSize);

    ++tagCounts[res.chunk_type];
    ++packetNum;
    ++totalFrames;

    if (packetNum % 1000 == 0) {
      std::cout << "Packets: " << packetNum << " | Voice/Word/Sent/Parag: " << tagCounts[0] << "/" << tagCounts[2]
                << "/" << tagCounts[3] << "/" << tagCounts[4] << "  Silence: " << tagCounts[1] << "\n";
    }
  }

  std::cout << "\nDone.\n";
  std::cout << "Total packets : " << packetNum << "\n";
  std::cout << "Voice         : " << tagCounts[0] << "\n";
  std::cout << "Silence       : " << tagCounts[1] << "\n";
  std::cout << "Word starts   : " << tagCounts[2] << "\n";
  std::cout << "Sentence starts: " << tagCounts[3] << "\n";
  std::cout << "Paragraph starts: " << tagCounts[4] << "\n";
  return 0;
}
