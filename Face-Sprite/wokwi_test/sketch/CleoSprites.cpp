#include <Arduino.h>
#include "CleoSprites.h"
#include "CleoBitmaps.h"

static_assert(epd_bitmap_allArray_LEN == 12,
              "CleoBitmaps.h must contain exactly 12 generated frames");
static_assert((sizeof(epd_bitmap_allArray) / sizeof(epd_bitmap_allArray[0])) == 12,
              "The bitmap pointer table must contain 12 entries.");

static const uint8_t* const idleFrames[] = {
  epd_bitmap_allArray[6], epd_bitmap_allArray[7]};

static const uint8_t* const annoyedFrames[] = {
  epd_bitmap_allArray[0], epd_bitmap_allArray[1]};

static const uint8_t* const bigCoughFrames[] = {
  epd_bitmap_allArray[2], epd_bitmap_allArray[3]};

static const uint8_t* const hotFrames[] = {
  epd_bitmap_allArray[4], epd_bitmap_allArray[5]};

static const uint8_t* const petFrames[] = {
  epd_bitmap_allArray[8], epd_bitmap_allArray[9]};

static const uint8_t* const smallCoughFrames[] = {
  epd_bitmap_allArray[10], epd_bitmap_allArray[11]};

const AnimationSequence CLEO_ANIMATIONS[] = {
  {"idle",          idleFrames,       2,  500,  true},
  {"annoyed",       annoyedFrames,    2,  500,  true},
  {"big_cough",     bigCoughFrames,   2,  500,  true},
  {"hot",           hotFrames,        2,  500,  true},
  {"pet",           petFrames,        2,  500,  true},
  {"small_cough",   smallCoughFrames, 2,  500,  true},
};

const size_t CLEO_ANIMATION_COUNT = sizeof(CLEO_ANIMATIONS) / sizeof(CLEO_ANIMATIONS[0]);
