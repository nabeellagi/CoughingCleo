#include <Arduino.h>
#include <Wire.h>

#include "SpriteAnimator.h"
#include "CleoSprites.h"

// DISPLAY DRIVER SELECTOR
// 0 = Wokwi's SSD1306 simulator part.
// 1 = the SH1106-based OLED module on the physical PCB.
#define USE_SH1106_DRIVER 0

#if USE_SH1106_DRIVER
  #include <Adafruit_SH110X.h>
  Adafruit_SH1106G oled(128, 64, &Wire, -1);
  static constexpr uint16_t OLED_WHITE = SH110X_WHITE;
#else
  #include <Adafruit_SSD1306.h>
  Adafruit_SSD1306 oled(128, 64, &Wire, -1);
  static constexpr uint16_t OLED_WHITE = SSD1306_WHITE;
#endif

// Pin assignment copied from PCB.kicad_sch.
static constexpr uint8_t OLED_SDA_PIN = 8;
static constexpr uint8_t OLED_SCL_PIN = 9;
static constexpr uint8_t FACE_BUTTON_PIN = 1;
static constexpr uint8_t OLED_I2C_ADDRESS = 0x3C;
static constexpr uint32_t BUTTON_DEBOUNCE_MS = 35;

SpriteAnimator cleoAnimator(CLEO_ANIMATIONS, CLEO_ANIMATION_COUNT);

bool lastButtonReading = false;
bool stableButtonState = false;
uint32_t lastButtonChangeMs = 0;

bool beginOled() {
#if USE_SH1106_DRIVER
  return oled.begin(OLED_I2C_ADDRESS, true);
#else
  return oled.begin(SSD1306_SWITCHCAPVCC, OLED_I2C_ADDRESS);
#endif
}

void drawCurrentSprite() {
  const uint8_t* bitmap = cleoAnimator.currentFrame();
  if (bitmap == nullptr) {
    Serial.println(F("No bitmap selected. Check CleoBitmaps.h."));
    return;
  }

  oled.clearDisplay();
  oled.drawBitmap(0, 0, bitmap, 128, 64, OLED_WHITE);
  oled.display();

  Serial.print(F("Animation: "));
  Serial.print(cleoAnimator.currentName());
  Serial.print(F(" | frame: "));
  Serial.println(cleoAnimator.currentFrameIndex());
}

bool wasFacePressed(uint32_t nowMs) {
  const bool reading = (digitalRead(FACE_BUTTON_PIN) == LOW);

  if (reading != lastButtonReading) {
    lastButtonReading = reading;
    lastButtonChangeMs = nowMs;
  }

  if ((nowMs - lastButtonChangeMs) >= BUTTON_DEBOUNCE_MS &&
      reading != stableButtonState) {
    stableButtonState = reading;
    return stableButtonState;
  }

  return false;
}

void setup() {
  Serial.begin(115200);
  delay(100); // Short serial-startup settle only. Animation timing is non-blocking.
  Serial.println();
  Serial.println(F("=== COUGHING CLEO OLED PROTOTYPE ==="));

  pinMode(FACE_BUTTON_PIN, INPUT_PULLUP);
  lastButtonReading = (digitalRead(FACE_BUTTON_PIN) == LOW);
  stableButtonState = lastButtonReading;
  lastButtonChangeMs = millis();

  Wire.begin(OLED_SDA_PIN, OLED_SCL_PIN);

  if (!beginOled()) {
    Serial.println(F("OLED init FAILED. Check I2C wiring, address, and driver."));
    while (true) {
      delay(1000);
    }
  }

  Serial.println(F("OLED init OK."));

  oled.clearDisplay();
  oled.setTextColor(OLED_WHITE);
  oled.setTextSize(1);
  oled.setCursor(0, 0);
  oled.println(F("COUGHING CLEO"));
  oled.display();

  cleoAnimator.begin(0, millis()); 
  drawCurrentSprite();
}

void loop() {
  const uint32_t nowMs = millis();
  bool redraw = false;

  if(wasFacePressed(nowMs)){
    if(cleoAnimator.next(nowMs)){
      Serial.print(F("FACE PRESSED -> Next : "));
      Serial.print(cleoAnimator.currentName());
      redraw = true;
    }
  }

  if(cleoAnimator.update(nowMs)){
    redraw = true;
  }

  if(redraw){
    drawCurrentSprite();
  }

}