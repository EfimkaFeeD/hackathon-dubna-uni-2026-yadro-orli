#ifndef PODCAST_SILENCE_REMOVER_SRC_AUDIO_FILE_TO_FFMPEG_FORMAT_HPP
#define PODCAST_SILENCE_REMOVER_SRC_AUDIO_FILE_TO_FFMPEG_FORMAT_HPP

#include <libavutil/samplefmt.h>
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/audio_fifo.h>
#include <libavutil/frame.h>
}

#include <cstdint>

class AudioFileToFFmpegFormat {
 public:
  explicit AudioFileToFFmpegFormat(const char* filename);
  ~AudioFileToFFmpegFormat();
  AudioFileToFFmpegFormat(const AudioFileToFFmpegFormat&)            = delete;
  AudioFileToFFmpegFormat& operator=(const AudioFileToFFmpegFormat&) = delete;

  AVFrame* getNextFrame();

  [[nodiscard]] int getSampleRate() const {
    return (codecCtx_ != nullptr) ? codecCtx_->sample_rate : 0;
  }
  [[nodiscard]] int getChannels() const {
    return (codecCtx_ != nullptr) ? codecCtx_->ch_layout.nb_channels : 0;
  }
  [[nodiscard]] AVSampleFormat getSampleFormat() const {
    return (codecCtx_ != nullptr) ? codecCtx_->sample_fmt : AV_SAMPLE_FMT_NONE;
  }
  [[nodiscard]] bool isPlanar() const {
    return ((codecCtx_ != nullptr) ? av_sample_fmt_is_planar(codecCtx_->sample_fmt) : 0) != 0;
  }
 private:
  AVFormatContext* fmtCtx_  = nullptr;
  AVCodecContext* codecCtx_ = nullptr;
  int audioStream_          = -1;
  AVAudioFifo* fifo_        = nullptr;
  AVFrame* currentFrame_    = nullptr;
  int64_t frameSamples_     = 0;
  bool eof_                 = false;

  void cleanup();
  bool openFile(const char* filename);
  int decodeToFifo(int minSamples);
};

#endif // PODCAST_SILENCE_REMOVER_SRC_AUDIO_FILE_TO_FFMPEG_FORMAT_HPP
