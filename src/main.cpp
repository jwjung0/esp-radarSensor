#include "board_pins.h"
#include "radar_sensor.h"
#include "web_portal.h"

#include <Arduino.h>
#include <ctype.h>

static const unsigned long LED_BLINK_MS = 1000;
static unsigned long s_lastLedMs = 0;
static bool s_ledOn = false;

static const size_t kUsbLineCap = 128;
static char s_usbLine[kUsbLineCap];
static size_t s_usbLen = 0;

static void pollUsbCommands(void) {
  while (Serial.available() > 0) {
    const int c = Serial.read();
    if (c < 0) {
      break;
    }

    if (c == '\n' || c == '\r') {
      if (s_usbLen == 0) {
        continue;
      }
      s_usbLine[s_usbLen] = '\0';
      size_t start = 0;
      while (s_usbLine[start] != '\0' &&
             isspace(static_cast<unsigned char>(s_usbLine[start]))) {
        ++start;
      }
      if (s_usbLine[start] != '\0') {
        radarSensorSendCmd(s_usbLine + start);
      }
      s_usbLen = 0;
      continue;
    }

    if (s_usbLen + 1 < kUsbLineCap) {
      s_usbLine[s_usbLen++] = static_cast<char>(c);
    } else {
      s_usbLen = 0;
    }
  }
}

static void updateLedHeartbeat(void) {
  const unsigned long now = millis();
  if (now - s_lastLedMs < LED_BLINK_MS) {
    return;
  }
  s_lastLedMs = now;
  s_ledOn = !s_ledOn;
  digitalWrite(BOARD_LED_PIN, s_ledOn ? HIGH : LOW);
}

void setup() {
  pinMode(BOARD_LED_PIN, OUTPUT);
  digitalWrite(BOARD_LED_PIN, LOW);

  Serial.begin(115200);
  delay(1000);
  Serial.println();
  Serial.println("esp-ladarSensor — LD1125H UART2 + SoftAP monitor");
  Serial.printf("  ESP RX2=GPIO%d <-> LD1125H TX\n", PIN_RADAR_RX);
  Serial.printf("  ESP TX2=GPIO%d <-> LD1125H RX\n", PIN_RADAR_TX);
  Serial.printf("  baud=%d  LED=GPIO%d\n", RADAR_UART_BAUD, BOARD_LED_PIN);
  Serial.println("  USB: type radar cmds (get_all, rmax=6, save)");
  Serial.println();

  radarSensorBegin();
  webPortalBegin();
  s_lastLedMs = millis();
}

void loop() {
  radarSensorPoll();
  pollUsbCommands();
  webPortalLoop();
  updateLedHeartbeat();
}
