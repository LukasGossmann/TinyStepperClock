#ifndef RTC_H
#define RTC_H

#include "pico/types.h"

extern bool enableHourlyAnimation;
extern uint8_t animationStartHour;
extern uint8_t animationEndHour;
void rtcInit(datetime_t *t);
void enableRtcAlarm();
void disableRtcAlarm();

#endif