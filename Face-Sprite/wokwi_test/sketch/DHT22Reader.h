#pragma once
#include <Arduino.h>
#include <string.h>

enum DhtStatus : uint8_t {
  DHT_OK = 0,
  DHT_ERR_NO_RESPONSE,   // line never moved: wiring / power / pull-up
  DHT_ERR_TOO_FEW_BITS,  // sensor started answering but stopped early
  DHT_ERR_CHECKSUM       // 40 bits received but data is corrupt
};

inline const char* dhtStatusText(DhtStatus s) {
  switch (s) {
    case DHT_OK:               return "ok";
    case DHT_ERR_NO_RESPONSE:  return "no response (check wiring/power/pull-up)";
    case DHT_ERR_TOO_FEW_BITS: return "incomplete frame";
    case DHT_ERR_CHECKSUM:     return "checksum mismatch";
  }
  return "?";
}

static portMUX_TYPE dhtMux = portMUX_INITIALIZER_UNLOCKED;

inline DhtStatus readDht22(uint8_t pin, float &tempC, float &hum, uint8_t raw[5]) {
  uint16_t widths[64];
  uint8_t pulseCount = 0;
  uint8_t edgeCount = 0;

  // 1) Start signal: host pulls the line LOW for >1 ms.
  pinMode(pin, OUTPUT);
  digitalWrite(pin, LOW);
  delayMicroseconds(1500);

  // 2) Release the line (pin mode change done OUTSIDE the critical section).
  digitalWrite(pin, HIGH);
  pinMode(pin, INPUT_PULLUP);

  // 3) Record the width of every HIGH pulse until the line goes idle.
  portENTER_CRITICAL(&dhtMux);
  const uint32_t startUs = micros();
  uint32_t lastEdgeUs = startUs;
  uint32_t riseUs = 0;
  bool seenRise = false;
  int prev = digitalRead(pin);

  while (pulseCount < 64) {
    const int level = digitalRead(pin);
    const uint32_t nowUs = micros();

    if (level != prev) {
      prev = level;
      lastEdgeUs = nowUs;
      edgeCount++;
      if (level == HIGH) {
        riseUs = nowUs;
        seenRise = true;
      } else if (seenRise) {
        widths[pulseCount++] = (uint16_t)(nowUs - riseUs);
        seenRise = false;
      }
    } else if ((nowUs - lastEdgeUs) > 400) {
      break;                                  // line idle -> frame finished
    }

    if ((nowUs - startUs) > 15000) break;     // absolute safety timeout
  }
  portEXIT_CRITICAL(&dhtMux);

  // 4) Decode: last 40 HIGH pulses are the data bits (~27us = 0, ~70us = 1).
  if (edgeCount == 0) return DHT_ERR_NO_RESPONSE;
  if (pulseCount < 40) return DHT_ERR_TOO_FEW_BITS;

  const uint8_t first = pulseCount - 40;
  memset(raw, 0, 5);
  for (uint8_t i = 0; i < 40; i++) {
    raw[i / 8] <<= 1;
    if (widths[first + i] > 50) raw[i / 8] |= 1;
  }

  if (((raw[0] + raw[1] + raw[2] + raw[3]) & 0xFF) != raw[4]) {
    return DHT_ERR_CHECKSUM;
  }

  hum = (float)((raw[0] << 8) | raw[1]) * 0.1f;
  const int16_t t = ((raw[2] & 0x7F) << 8) | raw[3];
  tempC = (float)t * 0.1f;
  if (raw[2] & 0x80) tempC = -tempC;
  return DHT_OK;
}