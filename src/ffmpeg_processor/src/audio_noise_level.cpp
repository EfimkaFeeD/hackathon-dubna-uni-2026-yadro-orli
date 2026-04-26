#include "audio_noise_level.hpp"

extern "C" {
#include <libavfilter/avfilter.h>
#include <libavfilter/buffersink.h>
#include <libavfilter/buffersrc.h>
#include <libavutil/channel_layout.h>
#include <libavutil/dict.h>
#include <libavutil/frame.h>
#include <libavutil/log.h>
#include <libavutil/mem.h>
#include <libavutil/opt.h>
#include <libavutil/samplefmt.h>
}

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <exception>
#include <stdexcept>
#include <string>

#include "consts.h"

float
AudioNoiseLevel::computeNoiseFloor(const int16_t* samples, size_t count) {
  int oldLogLevel = av_log_get_level();
  av_log_set_level(AV_LOG_QUIET);

  if ((samples == nullptr) || count == 0) {
    return 0.0F;
  }

  AVFilterGraph* filterGraph     = nullptr;
  AVFilterContext* buffersrcCtx  = nullptr;
  AVFilterContext* buffersinkCtx = nullptr;
  AVFilterContext* astatsCtx     = nullptr;
  AVFrame* inputFrame            = nullptr;
  AVFrame* outputFrame           = nullptr;
  float noiseFloorDb             = 0.0F;

  try {
    filterGraph = avfilter_graph_alloc();
    if (filterGraph == nullptr) {
      throw std::runtime_error("avfilter_graph_alloc");
    }

    const AVFilter* buffersrc = avfilter_get_by_name("abuffer");
    char args[256];
    snprintf(args, sizeof(args), "time_base=1/%d:sample_rate=%d:sample_fmt=s16:channel_layout=mono", kOutputSampleRate,
             kOutputSampleRate);
    if (avfilter_graph_create_filter(&buffersrcCtx, buffersrc, "in", args, nullptr, filterGraph) < 0) {
      throw std::runtime_error("abuffer");
    }

    const AVFilter* buffersink = avfilter_get_by_name("abuffersink");
    if (avfilter_graph_create_filter(&buffersinkCtx, buffersink, "out", nullptr, nullptr, filterGraph) < 0) {
      throw std::runtime_error("abuffersink");
    }

    const AVFilter* astats = avfilter_get_by_name("astats");
    if (avfilter_graph_create_filter(&astatsCtx, astats, "astats", "metadata=1", nullptr, filterGraph) < 0) {
      throw std::runtime_error("astats");
    }

    if (avfilter_link(buffersrcCtx, 0, astatsCtx, 0) < 0 || avfilter_link(astatsCtx, 0, buffersinkCtx, 0) < 0) {
      throw std::runtime_error("link");
    }

    if (avfilter_graph_config(filterGraph, nullptr) < 0) {
      throw std::runtime_error("config");
    }

    inputFrame = av_frame_alloc();
    if (inputFrame == nullptr) {
      throw std::runtime_error("input frame alloc");
    }
    inputFrame->format      = AV_SAMPLE_FMT_S16;
    inputFrame->sample_rate = kOutputSampleRate;
    inputFrame->nb_samples  = static_cast<int>(count);
    av_channel_layout_default(&inputFrame->ch_layout, 1);
    if (av_frame_get_buffer(inputFrame, 0) < 0) {
      throw std::runtime_error("input frame buffer");
    }
    memcpy(inputFrame->data[0], samples, count * sizeof(int16_t));

    if (av_buffersrc_add_frame(buffersrcCtx, inputFrame) < 0) {
      throw std::runtime_error("buffersrc add");
    }

    if (av_buffersrc_add_frame(buffersrcCtx, nullptr) < 0) {
      throw std::runtime_error("buffersrc flush");
    }
    outputFrame = av_frame_alloc();

    if (outputFrame == nullptr) {
      throw std::runtime_error("output frame alloc");
    }
    if (av_buffersink_get_frame(buffersinkCtx, outputFrame) < 0) {
      throw std::runtime_error("buffersink get frame");
    }

    const char* keys[]     = {"lavfi.astats.Overall.Noise_floor", "lavfi.astats.1.Noise_floor"};
    AVDictionaryEntry* tag = nullptr;
    for (const char* key : keys) {
      tag = av_dict_get(outputFrame->metadata, key, nullptr, 0);
      if (tag != nullptr) {
        break;
      }
    }

    if ((tag != nullptr) && (tag->value != nullptr)) {
      noiseFloorDb = std::stof(tag->value);
    }
  } catch (const std::exception& e) {
    std::fprintf(stderr, "AudioNoiseLevel error: %s\n", e.what());
    noiseFloorDb = 0.0F;
  }

  if (outputFrame != nullptr) {
    av_frame_unref(outputFrame);
    av_frame_free(&outputFrame);
  }
  if (inputFrame != nullptr) {
    av_frame_unref(inputFrame);
    av_frame_free(&inputFrame);
  }
  if (filterGraph != nullptr) {
    avfilter_graph_free(&filterGraph);
  }

  av_log_set_level(oldLogLevel);

  return noiseFloorDb;
}
