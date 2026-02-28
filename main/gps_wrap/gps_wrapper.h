#pragma once
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* =======================
   FEED GPS
   ======================= */

void gps_encode_char(char c);

/* =======================
   SPEED
   ======================= */

int   gps_speed_updated(void);
float gps_get_speed_mph(void);

/* =======================
   FIX STATUS
   ======================= */

bool  gps_has_fix(void);
int   gps_sats_used(void);
float gps_hdop(void);

/* =======================
   LOCATION ACCESS
   ======================= */

bool   gps_location_updated(void);
bool   gps_location_valid(void);
double gps_get_lat(void);
double gps_get_lon(void);
float gps_get_course_deg(void);

/* =======================
   DISTANCE
   ======================= */

double gps_distance_between(double lat1,
                            double lon1,
                            double lat2,
                            double lon2);

#ifdef __cplusplus
}
#endif