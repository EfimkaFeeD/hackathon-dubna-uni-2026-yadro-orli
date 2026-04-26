#ifndef PODCAST_SILENCE_REMOVER_SRC_AUDIO_CLASSIFIER_HPP
#define PODCAST_SILENCE_REMOVER_SRC_AUDIO_CLASSIFIER_HPP

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

  // Process a frame and return its tag (0‑4).
  Tag processFrame(bool isVoice);

  // Access the last bufferFrames tags (for external analysis).
  const std::deque<Tag>& getTagBuffer() const {
    return history_;
  }
 private:
  std::deque<Tag> history_; // circular buffer of assigned tags
  size_t maxFrames_;        // maximum size of the buffer
};

#endif // PODCAST_SILENCE_REMOVER_SRC_AUDIO_CLASSIFIER_HPP
