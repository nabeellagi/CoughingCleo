#include <Arduino.h>
#include <Wire.h>
#include "DHT22Reader.h"
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


/*
=================================================================
PIN MAP TEST
================================================================
*/

// OLED
static constexpr uint8_t OLED_SDA_PIN = 8;
static constexpr uint8_t OLED_SCL_PIN = 9;
static constexpr uint8_t FACE_BUTTON_PIN = 1;
static constexpr uint8_t OLED_I2C_ADDRESS = 0x3C;

// Others
static constexpr uint8_t DHT_PIN = 4;
static constexpr uint8_t MQ2_ANALOG_PIN = 5;
static constexpr uint8_t PIR_PIN = 6;


/*
===============================================================
TIMING
===============================================================
*/

static constexpr uint32_t BUTTON_DEBOUNCE_MS = 50;
static constexpr uint32_t DHT_READ_INTERVAL_MS = 2000;
static constexpr uint32_t SENSOR_SCREEN_INTERVAL_MS = 1000;
static constexpr uint32_t NO_PET_TIMEOUT_MS = 1000 * 5;

/*
============================================================
THRESHOLD FOR TESTING
============================================================
*/

static constexpr float HOT_TEMP_C = 30.0f;
static constexpr int MQ2_SMALL_COUGH_THRESHOLD = 3360;
static constexpr int MQ2_BIG_COUGH_THRESHOLD = 3999;

SpriteAnimator cleoAnimator(CLEO_ANIMATIONS, CLEO_ANIMATION_COUNT);

/*
===========================================================
SPRITE STUFFF
===========================================================
*/


enum CleoAnimation : size_t {
  ANIM_IDLE = 0,
  ANIM_ANNOYED,
  ANIM_BIG_COUGH,
  ANIM_HOT,
  ANIM_PET,
  ANIM_SMALL_COUGH
};

enum DisplayMode {
  FACE_MODE,
  SENSOR_MODE
};

DisplayMode displayMode = FACE_MODE;

/*
===============================================
LATEST SENSOR VALUE 
==============================================
*/

float temperatureC = NAN;
float humidity = NAN;
int gasRaw = 0;
bool motionDetected = false;

// Timing State
uint32_t lastDHTReadMs = 0;
uint32_t lastSensorScreenDrawMs = 0;
uint32_t lastPetInteractionMs = 0;

bool petStartedFromAnnoyed = false;
bool wasMotionDetected = false;

// Button Debounce
volatile bool facePressedFlag = false;
volatile uint32_t lastButtonIsrMs = 0;

void IRAM_ATTR onFaceButtonISR() {
  const uint32_t now = millis();

  if((now - lastButtonIsrMs) >= BUTTON_DEBOUNCE_MS){
    lastButtonIsrMs = now;
    facePressedFlag = true;
  }
}

bool beginOled() {
#if USE_SH1106_DRIVER
  return oled.begin(OLED_I2C_ADDRESS, true);
#else
  return oled.begin(SSD1306_SWITCHCAPVCC, OLED_I2C_ADDRESS);
#endif
}

bool wasFacePressed() {
  if(!facePressedFlag) return false;
  facePressedFlag = false;
  return true;
}

void updateSensors(uint32_t nowMs) {
  gasRaw = analogRead(MQ2_ANALOG_PIN);
  motionDetected = (digitalRead(PIR_PIN) == HIGH);

  if(nowMs - lastDHTReadMs >= DHT_READ_INTERVAL_MS || lastDHTReadMs == 0) {
    lastDHTReadMs = nowMs;

    float newHumidity = NAN;
    float newTemperature = NAN;
    uint8_t rawBytes[5] = {0};

    const DhtStatus dhtStatus = readDht22(DHT_PIN, newTemperature, newHumidity, rawBytes);

    if(dhtStatus == DHT_OK){
      humidity = newHumidity;
      temperatureC = newTemperature;
    }


    // Console/Terminal
    Serial.println(F("----SENSOR UDPATE ----"));

    Serial.print(F("Temperature : "));
    Serial.print(temperatureC);
    Serial.println(F(" C"));

    Serial.print(F("Humidity : "));
    Serial.print(humidity);
    Serial.println(F(" %"));

    Serial.print(F("MQ-2 raw : "));
    Serial.println(gasRaw);


    Serial.print(F("PIR MOTION : "));
    Serial.println(motionDetected ? F("YES") : F("NO"));
  }
}

/*
=====================================================
PRIORITY
======================================================
1. Bad Air --> BIG_COUGH / SMALL_COUGH
2. Motion --> PET
3. Clean + hot --> HOT
4. Clean + no interaction --> ANNOYED
5. Otherwise --> IDLE
*/

size_t chooseDesiredAnimation(uint32_t nowMs){
  // Bad air cases
  if(gasRaw >= MQ2_BIG_COUGH_THRESHOLD) {
    return ANIM_BIG_COUGH;
  }
  if(gasRaw >= MQ2_SMALL_COUGH_THRESHOLD) {
    return ANIM_SMALL_COUGH;
  }
  
  // Good air cases
  if(motionDetected) {
    return ANIM_PET;
  }
  if(!isnan(temperatureC) && temperatureC >= HOT_TEMP_C) {
    return ANIM_HOT;
  }
  if(nowMs - lastPetInteractionMs >= NO_PET_TIMEOUT_MS) {
    return ANIM_ANNOYED;
  }
  return ANIM_IDLE;
}

void updateInteractionState(uint32_t nowMs) {
  const bool motionRisingEdge = motionDetected && !wasMotionDetected;
  const bool motionFallingEdge = !motionDetected && wasMotionDetected;

  if(motionRisingEdge){
    petStartedFromAnnoyed =
      cleoAnimator.isStarted() &&
      cleoAnimator.currentSequenceIndex() == ANIM_ANNOYED;

    lastPetInteractionMs = nowMs;
  }

  if(motionFallingEdge && petStartedFromAnnoyed) {
    petStartedFromAnnoyed = false;
    lastPetInteractionMs = nowMs;

    cleoAnimator.play(ANIM_IDLE, nowMs);
  }

  wasMotionDetected = motionDetected;
}

bool updateFaceLogic(uint32_t nowMs) {
  updateInteractionState(nowMs);

  const size_t desired = chooseDesiredAnimation(nowMs);

  if(!cleoAnimator.isStarted() || 
  cleoAnimator.currentSequenceIndex() != desired) {
    if(cleoAnimator.play(desired, nowMs)) {
      return true;
    }
  }
  return false;
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

void drawSensorDashboard() {
  oled.clearDisplay();
  oled.setTextColor(OLED_WHITE);
  oled.setTextSize(1);

  oled.setCursor(0, 0);
  oled.println(F("CLEO'S DATA LOG!"));

  oled.setCursor(0, 14);
  oled.print(F("Temp : "));
  if(isnan(temperatureC)) oled.println(F("--"));
  else {
    oled.print(temperatureC, 1);
    oled.println(F(" C"));
  }

  oled.setCursor(0, 25);
  oled.print(F("Hum : "));
  if(isnan(humidity)) oled.println(F("--"));
  else {
    oled.print(humidity, 1);
    oled.println("%");
  }

  oled.setCursor(0, 36);
  oled.print(F("Gas : "));
  oled.println(gasRaw);

  oled.display();
}

void toggleDisplayMode() {
  displayMode = (displayMode == FACE_MODE) ? SENSOR_MODE : FACE_MODE;
}

void setup() {
  Serial.begin(115200);
  delay(100);

  Serial.println(); 
  Serial.println(F("=== COUGHING CLEO OLED PROTOTYPE ==="));

  pinMode(FACE_BUTTON_PIN, INPUT_PULLUP);
  pinMode(PIR_PIN, INPUT);

  analogReadResolution(12);

  attachInterrupt(digitalPinToInterrupt(FACE_BUTTON_PIN), onFaceButtonISR, FALLING);

  Wire.begin(OLED_SDA_PIN, OLED_SCL_PIN);

  if (!beginOled()) {
    Serial.println(F("OLED init FAILED. Check I2C wiring, address, and driver."));
    while (true) {
      delay(1000);
    }
  }

  lastDHTReadMs = millis();

  Serial.println(F("OLED init OK."));

  const uint32_t nowMs = millis();
  lastPetInteractionMs = nowMs;
  cleoAnimator.begin(ANIM_IDLE, nowMs);

  drawCurrentSprite();
}

void loop() {
  const uint32_t nowMs = millis();
  
  bool redrawFace = false;

  updateSensors(nowMs);

  if(wasFacePressed()){
    toggleDisplayMode();

    if(displayMode == FACE_MODE) {
      drawCurrentSprite();
    } else {
      drawSensorDashboard();
    }
  }

  if(updateFaceLogic(nowMs)){
    redrawFace = true;
  }

  if(cleoAnimator.update(nowMs)){
    redrawFace = true;
  }

  if(displayMode == FACE_MODE && redrawFace){
    drawCurrentSprite();
  }

  if(displayMode == SENSOR_MODE && 
    (nowMs - lastSensorScreenDrawMs >= SENSOR_SCREEN_INTERVAL_MS)){
      lastSensorScreenDrawMs = nowMs;
      drawSensorDashboard();
    }
  delay(5);
}