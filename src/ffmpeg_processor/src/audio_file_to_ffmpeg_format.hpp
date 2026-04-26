#ifndef PODCAST_SILENCE_REMOVER_SRC_AUDIO_FILE_TO_FFMPEG_FORMAT_HPP
#define PODCAST_SILENCE_REMOVER_SRC_AUDIO_FILE_TO_FFMPEG_FORMAT_HPP

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
