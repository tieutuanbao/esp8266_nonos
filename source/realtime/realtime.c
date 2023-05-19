#include "realtime.h"
#include "sntp.h"
#include "time.h"

extern struct tm * ICACHE_FLASH_ATTR sntp_localtime(const time_t * tim_p);

void realtime_init() {
    sntp_set_timezone(7);
    sntp_set_timetype(false);
    sntp_setservername(0, "pool.ntp.org");
    sntp_init();
}

char *realtime_get_string() {
    uint32_t ts = sntp_get_current_timestamp();
    return sntp_get_real_time(ts);
}

realtime_t realtime_get() {
    realtime_t ret;
    uint32_t ts = sntp_get_current_timestamp();
    struct tm *GetTime = sntp_localtime(&ts);
    ret.tm_hour = GetTime->tm_hour;
    ret.tm_min = GetTime->tm_min;
    ret.tm_sec = GetTime->tm_sec;
    return ret;
}