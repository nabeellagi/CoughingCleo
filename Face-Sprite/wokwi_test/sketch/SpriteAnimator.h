#pragma once
#include <stddef.h>
#include <stdint.h>

struct AnimationSequence {
  const char* name;
  const uint8_t* const* frames;
  uint8_t frameCount;
  uint32_t frameIntervalMs;
  bool loop;
};

class SpriteAnimator {
 public:
  SpriteAnimator(const AnimationSequence* sequences, size_t sequenceCount);

  bool begin(size_t initialSequence, uint32_t nowMs);
  bool play(size_t sequenceIndex, uint32_t nowMs);
  bool next(uint32_t nowMs);

  bool update(uint32_t nowMs);

  const uint8_t* currentFrame() const;
  const char* currentName() const;
  size_t currentSequenceIndex() const;
  uint8_t currentFrameIndex() const;
  size_t sequenceCount() const;
  bool isStarted() const;

 private:
  const AnimationSequence* sequences_;
  size_t sequenceCount_;
  size_t sequenceIndex_;
  uint8_t frameIndex_;
  uint32_t lastFrameAtMs_;
  bool started_;
  bool finished_;
};