#ifndef VEHICLE_GPS_H
#define VEHICLE_GPS_H
#ifdef __cplusplus
extern "C" {
#endif
typedef struct {
    double latitude;
    double longitude;
    int satellites;
    int valid;
} GpsFix;
int gps_parse_sentence(const char *sentence, GpsFix *fix);
int gps_start(const char *device, unsigned int baud);
void gps_stop(void);
int gps_snapshot(GpsFix *fix);
#ifdef __cplusplus
}
#endif
#endif
