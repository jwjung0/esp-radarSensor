#include "radar_filter.h"

static const float kDefQ = 0.05f;
static const float kDefR = 0.4f;

static float s_q = kDefQ;
static float s_r = kDefR;

static bool s_kfInit = false;
static float s_x = 0.f;
static float s_p = 1.f;

static float clampf(float v, float lo, float hi) {
  if (v < lo) {
    return lo;
  }
  if (v > hi) {
    return hi;
  }
  return v;
}

void radarFilterReset(void) {
  s_kfInit = false;
  s_x = 0.f;
  s_p = 1.f;
}

void radarFilterSetParams(float q, float r) {
  s_q = clampf(q, 0.01f, 0.5f);
  s_r = clampf(r, 0.1f, 2.0f);
}

void radarFilterGetParams(float *q, float *r) {
  if (q) {
    *q = s_q;
  }
  if (r) {
    *r = s_r;
  }
}

bool radarFilterReady(void) { return s_kfInit; }

float radarFilterUpdate(float distance_m) {
  if (!s_kfInit) {
    s_x = distance_m;
    s_p = 1.f;
    s_kfInit = true;
    return s_x;
  }

  // Constant-position 1D Kalman on raw distance.
  s_p = s_p + s_q;
  const float k = s_p / (s_p + s_r);
  s_x = s_x + k * (distance_m - s_x);
  s_p = (1.f - k) * s_p;
  return s_x;
}
