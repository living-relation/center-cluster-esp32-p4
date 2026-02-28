#include "TinyGPS++.h"
#include "gps_wrapper.h"

static TinyGPSPlus gps;

extern "C" {

void gps_encode_char(char c){
    gps.encode(c);
}

/* =======================
   SPEED
   ======================= */

int gps_speed_updated(void){
    return gps.speed.isUpdated();
}

float gps_get_speed_mph(void){
    if (gps.speed.isValid())
        return gps.speed.mph();
    return 0.0f;
}

/* =======================
   FIX STATUS
   ======================= */

bool gps_has_fix(void){
    return gps.location.isValid() &&
           gps.location.age() < 2000;
}

int gps_sats_used(void){
    if (gps.satellites.isValid())
        return gps.satellites.value();
    return 0;
}

float gps_hdop(void){
    if (gps.hdop.isValid())
        return gps.hdop.hdop();
    return -1.0f;
}

/* =======================
   LOCATION ACCESS
   ======================= */

bool gps_location_updated(void){
    return gps.location.isUpdated();
}

bool gps_location_valid(void){
    return gps.location.isValid();
}

double gps_get_lat(void){
    return gps.location.lat();
}

double gps_get_lon(void){
    return gps.location.lng();
}
float gps_get_course_deg(void){
    if (gps.course.isValid())
        return gps.course.deg();
    return 0.0f;
}

/* =======================
   DISTANCE
   ======================= */

double gps_distance_between(double lat1,
                            double lon1,
                            double lat2,
                            double lon2){
    return TinyGPSPlus::distanceBetween(
        lat1,
        lon1,
        lat2,
        lon2
    );
}

}