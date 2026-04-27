#ifndef PODCAST_SILENCE_REMOVER_SRC_AUDIO_CLASSIFIER_HPP
#define PODCAST_SILENCE_REMOVER_SRC_AUDIO_CLASSIFIER_HPP

#include <cstddef>
#include <cstdint>
#include <deque>
#include "consts.h"

class AudioClassifier {
public:
  enum Tag : int {
    kVoice          = 0,
    kSilence        = 1,
    kWordStart      = 2,
    kSentenceStart  = 3,
    kParagraphStart = 4
  };

  explicit AudioClassifier(int bufferFrames = kSegmenterBufferFrames);

  Tag processFrame(bool isVoice);

  const std::deque<Tag>& getTagBuffer() const {
    return history_;
  }
private:
  std::deque<Tag> history_;
  size_t maxFrames_;
  int silenceFramesCounter_ = 100;
};

#endif