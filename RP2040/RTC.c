#include "hardware/rtc.h"

#include "RTC.h"
#include "Stepper.h"
#include "WS2812.h"

/// @brief Indicates whether the hourly animations are enabled
bool enableHourlyAnimation;

/// @brief The hour that the animations are supposed to start at
/// 0 to 23 inclusive
uint8_t animationStartHour;

/// @brief The hour that the animations are supposed to start end
/// 0 to 23 inclusive
uint8_t animationEndHour;

/// @brief Moves the clock hands when the rtc irq fires
void rtcAlarmHandler()
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
    stepperStep(&minuteStepper, false);

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
        stepperStep(&hourStepper, false);

    if (dateTime.min == 0 && dateTime.sec == 0)
    {
        if (enableHourlyAnimation)
            if (dateTime.hour >= animationStartHour && dateTime.hour <= animationEndHour)
                ws2812_do_pattern();
    }
}

/// @brief Enables the alarm of the rtc to wake the pico again on the next full minute
void enableRtcAlarm()
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

    rtc_set_alarm(&dateTime, rtcAlarmHandler);
    rtc_enable_alarm();
}

/// @brief Disables the alarm of the rtc
void disableRtcAlarm()
{
    rtc_disable_alarm();
}

/// @brief Initializes the rtc with the given time and sets the first alarm
/// @param t
void rtcInit(datetime_t *t)
{
    rtc_init();
    rtc_set_datetime(t);
    enableRtcAlarm();
}