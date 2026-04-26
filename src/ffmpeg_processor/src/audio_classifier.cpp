#include "audio_classifier.hpp"

#include <ranges>

#include "consts.h"

AudioClassifier::AudioClassifier(int bufferFrames) {
  if (bufferFrames < 1) {
    bufferFrames = kSegmenterBufferFrames;
  }
  maxFrames_ = static_cast<size_t>(bufferFrames);
}

AudioClassifier::Tag
AudioClassifier::processFrame(bool isVoice) {
  Tag tag;

  if (isVoice) {
    int gapFrames = 0;
    for (auto& it : std::views::reverse(history_)) {
      if (it == kSilence) {
        ++gapFrames;
      } else {
        break;
      }
    }

    if (gapFrames >= kGapParagraphStartFrames) {
      tag = kParagraphStart;
    } else if (gapFrames >= kGapSentenceStartFrames) {
      tag = kSentenceStart;
    } else if (gapFrames >= kGapWordStartFrames) {
      tag = kWordStart;
    } else {
      tag = kVoice;
    }
  } else {
    tag = kSilence;
  }

  history_.push_back(tag);
  while (history_.size() > maxFrames_) {
    history_.pop_front();
  }

  return tag;
}
