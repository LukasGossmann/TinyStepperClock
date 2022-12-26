#include "RTC.h"
#include "Stepper.h"
#include "WS2812.h"

#include <stdio.h>

bool enableHourlyAnimation;

// 0 to 23
uint8_t animationStartHour;

// 0 to 23
uint8_t animationEndHour;

void updateRtcAlarm();

void rtc_callback()
{
    datetime_t dateTime;
    rtc_disable_alarm();
    rtc_get_datetime(&dateTime);

    // The minute hand takes a step every 60 seconds (1 step per minute)
    // 1 hour = 60 steps
    // 1 minute = 1 step

    // Seconds:
    // 00 01 02 03 04 05 06 07 08 09 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26 27 28 29
    // ^
    // 30 31 32 33 34 35 36 37 38 39 40 41 42 43 44 45 46 47 48 49 50 51 52 53 54 55 56 57 58 59
    stepper_step(&minuteStepper, false);

    // The hour hand takes a step every 12 minutes (6 steps per hour)
    // 12 hours = 60 steps
    // 1 hour = 5 steps
    // 12 minutes = 1 step

    // Minutes:
    // 00 01 02 03 04 05 06 07 08 09 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26 27 28 29
    // ^                                   ^                                   ^
    // 30 31 32 33 34 35 36 37 38 39 40 41 42 43 44 45 46 47 48 49 50 51 52 53 54 55 56 57 58 59
    //                   ^                                   ^
    if (dateTime.sec == 0 && ((dateTime.min % 12) == 0))
        stepper_step(&hourStepper, false);

    if (dateTime.min == 0 && dateTime.sec == 0)
    {
        if (enableHourlyAnimation)
            if (dateTime.hour >= animationStartHour && dateTime.hour <= animationEndHour)
                ws2812_do_pattern();
    }

    updateRtcAlarm(&dateTime);
}

void updateRtcAlarm()
{
    // RTC will wake up the pico every 60 seconds
    static datetime_t dateTime = {
        -1, // year
        -1, // month
        -1, // day
        -1, // dotw
        -1, // hour
        -1, // min
        0,  // sec
    };

    rtc_set_alarm(&dateTime, rtc_callback);
    rtc_enable_alarm();
}

void rtcInit(datetime_t *t)
{
    rtc_init();
    rtc_set_datetime(t);
    updateRtcAlarm();
}