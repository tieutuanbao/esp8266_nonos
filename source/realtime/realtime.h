#ifndef __REALTIME_H__
#define __REALTIME_H__

typedef struct {
  int	tm_sec;
  int	tm_min;
  int	tm_hour;
  int	tm_mday;
  int	tm_mon;
  int	tm_year;
  int	tm_wday;
  int	tm_yday;
  int	tm_isdst;
} realtime_t;

void realtime_init();
char *realtime_get_string();
realtime_t realtime_get();

#endif