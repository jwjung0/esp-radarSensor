#pragma once

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

void radarFilterReset(void);
float radarFilterUpdate(float distance_m);
bool radarFilterReady(void);

void radarFilterSetParams(float q, float r);
void radarFilterGetParams(float *q, float *r);

#ifdef __cplusplus
}
#endif
