#pragma once

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
  RADAR_MODE_NONE = 0,
  RADAR_MODE_MOV = 1,
  RADAR_MODE_OCC = 2,
} RadarMode;

typedef struct {
  RadarMode mode;
  float distance_m;
  uint32_t last_ms; // millis() of last mov/occ line (raw)
} RadarLive;

typedef struct {
  float rmax;
  float mth1;
  float mth2;
  float mth3;
  float clear_s; // ESP-local: hold presence after last detect (s)
  float mov_s;   // ESP-local: hold MOV before OCC (s)
  bool valid;    // true after radar get_all keys parsed
  uint32_t seq;
  uint32_t updated_ms;
} RadarConfig;

bool radarSensorBegin(void);
void radarSensorPoll(void);
void radarSensorSendCmd(const char *cmd);

// Filtered live view (clearance + movement timers applied).
void radarSensorGetLive(RadarLive *out);
void radarSensorGetConfig(RadarConfig *out);

void radarSensorRequestGetAll(void);

// Apply rmax/mth* to module (+ save if requested). clear_s/mov_s always → ESP NVS.
void radarSensorApplyConfig(float rmax, float mth1, float mth2, float mth3,
                            float clear_s, float mov_s, bool saveModule);

#ifdef __cplusplus
}
#endif
