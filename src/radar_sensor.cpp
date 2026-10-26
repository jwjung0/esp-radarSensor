#include "radar_sensor.h"

#include "board_pins.h"

#include <Arduino.h>
#include <Preferences.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const size_t kLineCap = 128;
static const char *kNvsNs = "radar";
static const float kDefClearS = 5.0f;
static const float kDefMovS = 1.0f;

static char s_line[kLineCap];
static size_t s_len = 0;

static RadarLive s_live = {RADAR_MODE_NONE, 0.f, 0};
static uint32_t s_lastMovMs = 0;
static RadarConfig s_cfg = {8.f, 60.f, 30.f, 20.f, kDefClearS, kDefMovS,
                            false, 0, 0};
static Preferences s_prefs;
// Ignore get_all overwrites briefly after Apply (module ACK/get_all race).
static uint32_t s_cfgHoldUntilMs = 0;
static bool s_preferMthMov = false; // true once we see mth*_mov from module

static float clampf(float v, float lo, float hi) {
  if (v < lo) {
    return lo;
  }
  if (v > hi) {
    return hi;
  }
  return v;
}

static void loadTimingFromNvs(void) {
  s_prefs.begin(kNvsNs, true);
  s_cfg.clear_s = clampf(s_prefs.getFloat("clear_s", kDefClearS), 0.5f, 20.f);
  s_cfg.mov_s = clampf(s_prefs.getFloat("mov_s", kDefMovS), 0.5f, 10.f);
  s_prefs.end();
}

static void saveTimingToNvs(void) {
  s_prefs.begin(kNvsNs, false);
  s_prefs.putFloat("clear_s", s_cfg.clear_s);
  s_prefs.putFloat("mov_s", s_cfg.mov_s);
  s_prefs.end();
}

void radarSensorSendCmd(const char *cmd) {
  if (cmd == nullptr || cmd[0] == '\0') {
    return;
  }
  Serial2.printf("%s\r\n", cmd);
  Serial.printf(">> radar: %s\n", cmd);
}

bool radarSensorBegin(void) {
  s_len = 0;
  s_live = {RADAR_MODE_NONE, 0.f, 0};
  s_lastMovMs = 0;
  s_cfg = {8.f, 60.f, 30.f, 20.f, kDefClearS, kDefMovS, false, 0, 0};
  loadTimingFromNvs();
  Serial.printf("timing clear=%.1fs mov=%.1fs (ESP NVS)\n", s_cfg.clear_s,
                s_cfg.mov_s);

  Serial2.begin(RADAR_UART_BAUD, SERIAL_8N1, PIN_RADAR_RX, PIN_RADAR_TX);
  delay(200);
  radarSensorSendCmd("get_all");
  return true;
}

void radarSensorGetLive(RadarLive *out) {
  if (out == nullptr) {
    return;
  }
  *out = s_live;
  const uint32_t now = millis();
  const uint32_t clearMs =
      static_cast<uint32_t>(s_cfg.clear_s * 1000.f + 0.5f);
  const uint32_t movMs = static_cast<uint32_t>(s_cfg.mov_s * 1000.f + 0.5f);

  if (s_live.mode == RADAR_MODE_NONE || s_live.last_ms == 0) {
    out->mode = RADAR_MODE_NONE;
    return;
  }

  // Clearance: hold last presence until clear_s after last detect.
  if ((now - s_live.last_ms) > clearMs) {
    out->mode = RADAR_MODE_NONE;
    return;
  }

  // Movement time: after MOV, keep MOV until mov_s elapses with only OCC.
  if (s_live.mode == RADAR_MODE_OCC && s_lastMovMs != 0 &&
      (now - s_lastMovMs) < movMs) {
    out->mode = RADAR_MODE_MOV;
  }
}

void radarSensorGetConfig(RadarConfig *out) {
  if (out == nullptr) {
    return;
  }
  *out = s_cfg;
}

void radarSensorRequestGetAll(void) { radarSensorSendCmd("get_all"); }

static void pumpRadarMs(uint32_t ms) {
  const uint32_t t0 = millis();
  while ((millis() - t0) < ms) {
    radarSensorPoll();
    delay(1);
  }
}

static void sendRadarCmdWait(const char *cmd, uint32_t waitMs) {
  radarSensorSendCmd(cmd);
  Serial2.flush();
  pumpRadarMs(waitMs);
}

void radarSensorApplyConfig(float rmax, float mth1, float mth2, float mth3,
                            float clear_s, float mov_s, bool saveModule) {
  s_cfg.rmax = clampf(rmax, 0.4f, 12.f);
  s_cfg.mth1 = clampf(mth1, 10.f, 600.f);
  s_cfg.mth2 = clampf(mth2, 10.f, 300.f);
  s_cfg.mth3 = clampf(mth3, 10.f, 200.f);
  s_cfg.clear_s = clampf(clear_s, 0.5f, 20.f);
  s_cfg.mov_s = clampf(mov_s, 0.5f, 10.f);
  s_cfg.valid = true;
  s_cfg.seq++;
  s_cfg.updated_ms = millis();
  // Hold local cfg so late get_all / echoes cannot snap values back.
  s_cfgHoldUntilMs = millis() + 4000;
  saveTimingToNvs();

  char buf[48];
  // Send both naming styles (ESPHome uses mth1=; get_all often prints mth1_mov).
  snprintf(buf, sizeof(buf), "mth1_mov=%.0f", s_cfg.mth1);
  sendRadarCmdWait(buf, 150);
  snprintf(buf, sizeof(buf), "mth2_mov=%.0f", s_cfg.mth2);
  sendRadarCmdWait(buf, 150);
  snprintf(buf, sizeof(buf), "mth3_mov=%.0f", s_cfg.mth3);
  sendRadarCmdWait(buf, 150);
  snprintf(buf, sizeof(buf), "mth1=%.0f", s_cfg.mth1);
  sendRadarCmdWait(buf, 150);
  snprintf(buf, sizeof(buf), "mth2=%.0f", s_cfg.mth2);
  sendRadarCmdWait(buf, 150);
  snprintf(buf, sizeof(buf), "mth3=%.0f", s_cfg.mth3);
  sendRadarCmdWait(buf, 150);
  snprintf(buf, sizeof(buf), "rmax=%.1f", s_cfg.rmax);
  sendRadarCmdWait(buf, 150);
  if (saveModule) {
    sendRadarCmdWait("save", 250);
  }
  // Skip immediate get_all — it raced and restored old values into s_cfg/UI.
  Serial.printf(">> apply done preferMthMov=%d (Reload to verify)\n",
                (int)s_preferMthMov);
}

static void trimInPlace(char *s) {
  size_t n = strlen(s);
  while (n > 0 && isspace((unsigned char)s[n - 1])) {
    s[--n] = '\0';
  }
  size_t i = 0;
  while (s[i] != '\0' && isspace((unsigned char)s[i])) {
    ++i;
  }
  if (i > 0) {
    memmove(s, s + i, n - i + 1);
  }
}

static void setConfigKey(const char *key, float value) {
  if (millis() < s_cfgHoldUntilMs) {
    return;
  }
  bool touched = false;
  if (strcmp(key, "rmax") == 0) {
    s_cfg.rmax = value;
    touched = true;
  } else if (strcmp(key, "mth1") == 0 || strcmp(key, "mth1_mov") == 0 ||
             strcmp(key, "th1_mov") == 0) {
    if (strcmp(key, "mth1_mov") == 0 || strcmp(key, "th1_mov") == 0) {
      s_preferMthMov = true;
    }
    s_cfg.mth1 = value;
    touched = true;
  } else if (strcmp(key, "mth2") == 0 || strcmp(key, "mth2_mov") == 0 ||
             strcmp(key, "th2_mov") == 0) {
    if (strcmp(key, "mth2_mov") == 0 || strcmp(key, "th2_mov") == 0) {
      s_preferMthMov = true;
    }
    s_cfg.mth2 = value;
    touched = true;
  } else if (strcmp(key, "mth3") == 0 || strcmp(key, "mth3_mov") == 0 ||
             strcmp(key, "th3_mov") == 0) {
    if (strcmp(key, "mth3_mov") == 0 || strcmp(key, "th3_mov") == 0) {
      s_preferMthMov = true;
    }
    s_cfg.mth3 = value;
    touched = true;
  }
  if (!touched) {
    return;
  }
  s_cfg.valid = true;
  s_cfg.seq++;
  s_cfg.updated_ms = millis();
}

static bool tryParseConfigLine(const char *line) {
  if (strchr(line, ',') != nullptr) {
    return false;
  }

  static const char *kKeys[] = {
      "mth1_mov", "mth2_mov", "mth3_mov", "th1_mov", "th2_mov", "th3_mov",
      "mth1",     "mth2",     "mth3",     "rmax",
  };

  bool any = false;
  for (size_t i = 0; i < sizeof(kKeys) / sizeof(kKeys[0]); ++i) {
    const char *key = kKeys[i];
    const size_t keyLen = strlen(key);

    char isPat[40];
    snprintf(isPat, sizeof(isPat), "%s is ", key);
    const char *hit = strstr(line, isPat);
    if (hit != nullptr) {
      setConfigKey(key, strtof(hit + strlen(isPat), nullptr));
      any = true;
      continue;
    }

    char eqPat[40];
    snprintf(eqPat, sizeof(eqPat), "%s=", key);
    hit = strstr(line, eqPat);
    if (hit != nullptr) {
      if (hit > line) {
        const char prev = hit[-1];
        if (isalnum((unsigned char)prev) || prev == '_') {
          continue;
        }
      }
      setConfigKey(key, strtof(hit + keyLen + 1, nullptr));
      any = true;
    }
  }
  return any;
}

static void handleLine(char *line) {
  trimInPlace(line);
  if (line[0] == '\0') {
    return;
  }

  const bool moving = (strncmp(line, "mov", 3) == 0);
  const bool occ = (strncmp(line, "occ", 3) == 0);
  char *eq = strchr(line, '=');

  if ((moving || occ) && eq != nullptr && eq[1] != '\0') {
    float dist = strtof(eq + 1, nullptr);
    const uint32_t now = millis();
    s_live.mode = moving ? RADAR_MODE_MOV : RADAR_MODE_OCC;
    s_live.distance_m = dist;
    s_live.last_ms = now;
    if (moving) {
      s_lastMovMs = now;
      Serial.printf("MOV %.2f m\n", dist);
    } else {
      Serial.printf("OCC %.2f m\n", dist);
    }
    return;
  }

  tryParseConfigLine(line);
  Serial.printf("RADAR: %s\n", line);
}

void radarSensorPoll(void) {
  while (Serial2.available() > 0) {
    const int c = Serial2.read();
    if (c < 0) {
      break;
    }

    if (c == '\n') {
      s_line[s_len] = '\0';
      handleLine(s_line);
      s_len = 0;
      continue;
    }

    if (c == '\r') {
      continue;
    }

    if (s_len + 1 < kLineCap) {
      s_line[s_len++] = static_cast<char>(c);
    } else {
      s_len = 0;
    }
  }
}
