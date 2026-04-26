#include "audio_file_to_ffmpeg_format.hpp"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavcodec/codec.h>
#include <libavcodec/packet.h>
#include <libavformat/avformat.h>
#include <libavutil/audio_fifo.h>
#include <libavutil/avutil.h>
#include <libavutil/channel_layout.h>
#include <libavutil/error.h>
#include <libavutil/frame.h>
#include <libavutil/samplefmt.h>
}

#include <cerrno>
#include <cstdint>
#include <stdexcept>

#include "consts.h"

AudioFileToFFmpegFormat::AudioFileToFFmpegFormat(const char* filename) {
  if (!openFile(filename)) {
    throw std::runtime_error("Failed to open audio file");
  }
}

AudioFileToFFmpegFormat::~AudioFileToFFmpegFormat() {
  cleanup();
}

AVFrame*
AudioFileToFFmpegFormat::getNextFrame() {
  if (av_audio_fifo_size(fifo_) < frameSamples_) {
    int ret = decodeToFifo(frameSamples_);
    if (ret < 0) {
      return nullptr;
    }
    if (av_audio_fifo_size(fifo_) < frameSamples_) {
      return nullptr;
    }
  }
  av_frame_unref(currentFrame_);
  currentFrame_->format      = codecCtx_->sample_fmt;
  currentFrame_->sample_rate = codecCtx_->sample_rate;
  currentFrame_->nb_samples  = frameSamples_;
  av_channel_layout_copy(&currentFrame_->ch_layout, &codecCtx_->ch_layout);
  if (av_frame_get_buffer(currentFrame_, 0) < 0) {
    return nullptr;
  }

  if (av_audio_fifo_read(fifo_, reinterpret_cast<void**>(currentFrame_->data), frameSamples_) < frameSamples_) {
    return nullptr;
  }

  return currentFrame_;
}

void
AudioFileToFFmpegFormat::cleanup() {
  if (fifo_ != nullptr) {
    av_audio_fifo_free(fifo_);
    fifo_ = nullptr;
  }
  if (currentFrame_ != nullptr) {
    av_frame_unref(currentFrame_);
    av_frame_free(&currentFrame_);
  }
  if (codecCtx_ != nullptr) {
    avcodec_free_context(&codecCtx_);
  }
  if (fmtCtx_ != nullptr) {
    avformat_close_input(&fmtCtx_);
  }
}

bool
AudioFileToFFmpegFormat::openFile(const char* filename) {
  if (avformat_open_input(&fmtCtx_, filename, nullptr, nullptr) < 0) {
    return false;
  }

  if (avformat_find_stream_info(fmtCtx_, nullptr) < 0) {
    cleanup();
    return false;
  }

  audioStream_ = av_find_best_stream(fmtCtx_, AVMEDIA_TYPE_AUDIO, -1, -1, nullptr, 0);
  if (audioStream_ < 0) {
    cleanup();
    return false;
  }

  AVStream* stream     = fmtCtx_->streams[audioStream_];
  const AVCodec* codec = avcodec_find_decoder(stream->codecpar->codec_id);
  if (codec == nullptr) {
    cleanup();
    return false;
  }

  codecCtx_ = avcodec_alloc_context3(codec);
  if (codecCtx_ == nullptr) {
    cleanup();
    return false;
  }

  if (avcodec_parameters_to_context(codecCtx_, stream->codecpar) < 0) {
    cleanup();
    return false;
  }

  if (avcodec_open2(codecCtx_, codec, nullptr) < 0) {
    cleanup();
    return false;
  }

  int64_t samplesPer10ms = (codecCtx_->sample_rate * kOutputFrameMs) / 1000;
  if ((codecCtx_->sample_rate * kOutputFrameMs) % 1000 != 0) {
    cleanup();
    return false;
  }
  frameSamples_ = samplesPer10ms;
  fifo_         = av_audio_fifo_alloc(codecCtx_->sample_fmt, codecCtx_->ch_layout.nb_channels, 1);
  if (fifo_ == nullptr) {
    cleanup();
    return false;
  }

  currentFrame_ = av_frame_alloc();
  if (currentFrame_ == nullptr) {
    cleanup();
    return false;
  }

  return true;
}

int
AudioFileToFFmpegFormat::decodeToFifo(int minSamples) {
  while (av_audio_fifo_size(fifo_) < minSamples && !eof_) {
    AVPacket* pkt = av_packet_alloc();
    if (pkt == nullptr) {
      return -1;
    }

    int ret = av_read_frame(fmtCtx_, pkt);
    if (ret < 0) {
      av_packet_free(&pkt);
      if (ret == AVERROR_EOF) {
        eof_ = true;
        break;
      }
      return ret;
    }

    if (pkt->stream_index == audioStream_) {
      ret = avcodec_send_packet(codecCtx_, pkt);
      if (ret < 0 && ret != AVERROR(EAGAIN)) {
        av_packet_free(&pkt);
        return ret;
      }

      while (ret >= 0) {
        AVFrame* decodedFrame = av_frame_alloc();
        if (decodedFrame == nullptr) {
          av_packet_free(&pkt);
          return -1;
        }
        ret = avcodec_receive_frame(codecCtx_, decodedFrame);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
          av_frame_free(&decodedFrame);
          break;
        }
        if (ret < 0) {
          av_frame_free(&decodedFrame);
          av_packet_free(&pkt);
          return ret;
        }

        if (av_audio_fifo_write(fifo_, reinterpret_cast<void**>(decodedFrame->extended_data),
                                decodedFrame->nb_samples) < decodedFrame->nb_samples) {
          av_frame_free(&decodedFrame);
          av_packet_free(&pkt);
          return -1;
        }
        av_frame_free(&decodedFrame);
      }
    }
    av_packet_free(&pkt);
  }
  return av_audio_fifo_size(fifo_);
}
