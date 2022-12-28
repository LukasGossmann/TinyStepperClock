#include "hardware/pwm.h"

#include "PWM.h"
#include "Stepper.h"

/// @brief Converts the hour and minute of a datetime to the number of steps
/// that each motor has to move to get to the current time with both hands initially at the 12 o'clock position.
/// @param dateTime The time that we want to know the number of steps for
/// @param stepsToNewHourPosition The number of steps the hour hands needs to move
/// @param stepsToNewMinutePosition The number of steps the minute hand needs to move
void convertTimeToSteps(datetime_t *dateTime, uint32_t *stepsToNewHourPosition, uint32_t *stepsToNewMinutePosition)
{
    // Step positions are based on the home position (both hands at 12 o'clock)
    // Each stepper has to move 60 steps per revolution (20 full steps * 3:1 gear reduction)
    const uint32_t stepsPerRevolution = 20 * 3;
    const uint32_t stepsPerHour = stepsPerRevolution / 12; // 5

    uint32_t hourLimitedTo12HourTime = dateTime->hour % 12;
    uint32_t minuteOffset = (dateTime->min - (dateTime->min % 12)) / 12;

    *stepsToNewHourPosition = (hourLimitedTo12HourTime * stepsPerHour) + minuteOffset;

    *stepsToNewMinutePosition = dateTime->min;
}

const uint32_t pwmSliceNumber = 0;

/// @brief  Clears the pwm wrap irq for the slice number used as a timer
inline void clear50hzTimerIrq()
{
    pwm_clear_irq(pwmSliceNumber);
}

/// @brief Configures a pwm slice as a 50hz timer
/// @param pwmWrapIrqHandler The handler for the wrap event of the pwm slice
void configurePwmAs50hzTimer(irq_handler_t pwmWrapIrqHandler)
{
    // Configure for 50hz
    pwm_config pwmConfig = pwm_get_default_config();
    pwm_config_set_clkdiv_int_frac(&pwmConfig, 40, 0);
    pwm_config_set_wrap(&pwmConfig, 62499);

    pwm_init(pwmSliceNumber, &pwmConfig, false);
    pwm_set_chan_level(pwmSliceNumber, PWM_CHAN_A, 0);

    // Global irq setup
    pwm_clear_irq(pwmSliceNumber);
    pwm_set_irq_enabled(pwmSliceNumber, true);
    irq_set_exclusive_handler(PWM_IRQ_WRAP, pwmWrapIrqHandler);
    irq_set_enabled(PWM_IRQ_WRAP, true);

    pwm_set_enabled(pwmSliceNumber, true);
}

/// @brief Deconfigures a pwm slice from being used as a 50hz timer
/// @param pwmWrapIrqHandler
void deconfigurePwmFrom50hzTimer(irq_handler_t pwmWrapIrqHandler)
{
    // Global irq setup
    pwm_set_enabled(pwmSliceNumber, false);
    pwm_clear_irq(pwmSliceNumber);
    pwm_set_irq_enabled(pwmSliceNumber, false);
    irq_set_enabled(PWM_IRQ_WRAP, false);

    // Only remove handler if one exists otherwise it might crash
    if (irq_get_exclusive_handler(PWM_IRQ_WRAP) != NULL)
        irq_remove_handler(PWM_IRQ_WRAP, pwmWrapIrqHandler);
}

/// @brief When moving the clock hands to the current time this contains the number of steps the hour hand needs to move
volatile uint32_t hourStepCount = 0;

/// @brief When moving the clock hands to the current time this contains the number of steps the minute hand needs to move
volatile uint32_t minuteStepCount = 0;

/// @brief Moves the clock hands by their respective number of steps and disables the pwm unit once both are at the correct position
void pwmWrapIrqHandlerSeekClockHands()
{
    if (minuteStepCount > 0)
    {
        minuteStepCount--;
        stepperStep(&minuteStepper, false);
    }

    if (hourStepCount > 0)
    {
        hourStepCount--;
        stepperStep(&hourStepper, false);
    }

    if (minuteStepCount == 0 && hourStepCount == 0)
        pwm_set_enabled(pwmSliceNumber, false);

    clear50hzTimerIrq();
}

/// @brief Moves the clock hands to the correct position for the given time.
/// Both clock hands need to be at the 12 o'clock position initially.
/// @param dateTime The time that we want to move the clock hands to
void seekClockHands(datetime_t *dateTime)
{
    uint32_t tmpHourStepCount;
    uint32_t tmpMinuteStepCount;
    convertTimeToSteps(dateTime, &tmpHourStepCount, &tmpMinuteStepCount);

    hourStepCount = tmpHourStepCount;
    minuteStepCount = tmpMinuteStepCount;

    if ((hourStepCount > 0) || (minuteStepCount > 0))
    {
        configurePwmAs50hzTimer(&pwmWrapIrqHandlerSeekClockHands);

        // Wait until pwm unit got disabled by the seek irq handler
        while ((pwm_hw->en & (1 << pwmSliceNumber)) != 0)
            tight_loop_contents();

        deconfigurePwmFrom50hzTimer(&pwmWrapIrqHandlerSeekClockHands);
    }
}