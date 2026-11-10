#include "SpriteAnimator.h"

SpriteAnimator::SpriteAnimator(
    const AnimationSequence* sequences,
    size_t sequenceCount)
    : sequences_(sequences),
      sequenceCount_(sequenceCount),
      sequenceIndex_(0),
      frameIndex_(0),
      lastFrameAtMs_(0),
      started_(false),
      finished_(false) {}

bool SpriteAnimator::begin(size_t initialSequence, uint32_t nowMs) {
  if (sequences_ == nullptr || sequenceCount_ == 0) {
    started_ = false;
    return false;
  }

  if (initialSequence >= sequenceCount_) {
    initialSequence = 0;
  }

  return play(initialSequence, nowMs);
}

bool SpriteAnimator::play(size_t sequenceIndex, uint32_t nowMs) {
  if (sequences_ == nullptr || sequenceIndex >= sequenceCount_) {
    return false;
  }

  const AnimationSequence& sequence = sequences_[sequenceIndex];
  if (sequence.frames == nullptr || sequence.frameCount == 0) {
    return false;
  }

  sequenceIndex_ = sequenceIndex;
  frameIndex_ = 0;
  lastFrameAtMs_ = nowMs;
  started_ = true;
  finished_ = false;
  return true;
}

bool SpriteAnimator::next(uint32_t nowMs) {
  if (!started_ || sequenceCount_ == 0) {
    return begin(0, nowMs);
  }

  const size_t nextIndex = (sequenceIndex_ + 1U) % sequenceCount_;
  return play(nextIndex, nowMs);
}

bool SpriteAnimator::update(uint32_t nowMs) {
  if (!started_ || finished_ || sequences_ == nullptr) {
    return false;
  }

  const AnimationSequence& sequence = sequences_[sequenceIndex_];
  if (sequence.frameCount <= 1 || sequence.frameIntervalMs == 0) {
    return false;
  }

  // Unsigned subtraction is safe across millis() rollover.
  const uint32_t elapsed = nowMs - lastFrameAtMs_;
  const uint32_t steps = elapsed / sequence.frameIntervalMs;
  if (steps == 0) {
    return false;
  }

  // Preserve the original schedule to avoid timing drift after a slow loop pass.
  lastFrameAtMs_ += steps * sequence.frameIntervalMs;

  const uint8_t oldFrame = frameIndex_;
  if (sequence.loop) {
    frameIndex_ = static_cast<uint8_t>(
        (frameIndex_ + (steps % sequence.frameCount)) % sequence.frameCount);
  } else {
    const uint32_t targetFrame = static_cast<uint32_t>(frameIndex_) + steps;
    if (targetFrame >= sequence.frameCount) {
      frameIndex_ = static_cast<uint8_t>(sequence.frameCount - 1U);
      finished_ = true;
    } else {
      frameIndex_ = static_cast<uint8_t>(targetFrame);
    }
  }

  return frameIndex_ != oldFrame;
}

const uint8_t* SpriteAnimator::currentFrame() const {
  if (!started_ || sequences_ == nullptr) {
    return nullptr;
  }

  const AnimationSequence& sequence = sequences_[sequenceIndex_];
  if (sequence.frames == nullptr || frameIndex_ >= sequence.frameCount) {
    return nullptr;
  }

  return sequence.frames[frameIndex_];
}

const char* SpriteAnimator::currentName() const {
  if (!started_ || sequences_ == nullptr) {
    return "(not started)";
  }
  return sequences_[sequenceIndex_].name;
}

size_t SpriteAnimator::currentSequenceIndex() const {
  return sequenceIndex_;
}

uint8_t SpriteAnimator::currentFrameIndex() const {
  return frameIndex_;
}

size_t SpriteAnimator::sequenceCount() const {
  return sequenceCount_;
}

bool SpriteAnimator::isStarted() const {
  return started_;
}