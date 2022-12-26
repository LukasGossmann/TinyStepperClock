#include "hardware/pwm.h"
#include "hardware/gpio.h"

#include "PWM.h"
#include "Stepper.h"

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

const uint32_t slice_num = 0;

inline void clear50hzTimerIrq()
{
    pwm_clear_irq(slice_num);
}

void configurePwmAs50hzTimer(irq_handler_t pwmWrapIrqHandler)
{
    // Configure for 50hz
    pwm_config pwmConfig = pwm_get_default_config();
    pwm_config_set_clkdiv_int_frac(&pwmConfig, 40, 0);
    pwm_config_set_wrap(&pwmConfig, 62499);
    pwm_config_set_output_polarity(&pwmConfig, true, false);

    pwm_init(slice_num, &pwmConfig, false);
    pwm_set_chan_level(slice_num, PWM_CHAN_A, 62499);

    // Global irq setup
    pwm_clear_irq(slice_num);
    pwm_set_irq_enabled(slice_num, true);
    irq_set_exclusive_handler(PWM_IRQ_WRAP, pwmWrapIrqHandler);
    irq_set_enabled(PWM_IRQ_WRAP, true);

    pwm_set_enabled(slice_num, true);
}

void deconfigurePwmFrom50hzTimer(irq_handler_t pwmWrapIrqHandler)
{
    // Global irq setup
    pwm_set_enabled(slice_num, false);
    pwm_clear_irq(slice_num);
    pwm_set_irq_enabled(slice_num, false);
    irq_set_enabled(PWM_IRQ_WRAP, false);

    // Only remove handler if one exists otherwise it might crash
    if (irq_get_exclusive_handler(PWM_IRQ_WRAP) != NULL)
        irq_remove_handler(PWM_IRQ_WRAP, pwmWrapIrqHandler);
}

volatile uint32_t hourStepCount = 0;
volatile uint32_t minuteStepCount = 0;

void pwm_wrap_irq_handler_seek()
{
    if (minuteStepCount > 0)
    {
        minuteStepCount--;
        stepper_step(&minuteStepper, false);
    }

    if (hourStepCount > 0)
    {
        hourStepCount--;
        stepper_step(&hourStepper, false);
    }

    if (minuteStepCount == 0 && hourStepCount == 0)
        pwm_set_enabled(slice_num, false);

    clear50hzTimerIrq();
}

void seekClockHands(datetime_t *dateTime)
{
    uint32_t tmpHourStepCount;
    uint32_t tmpMinuteStepCount;
    convertTimeToSteps(dateTime, &tmpHourStepCount, &tmpMinuteStepCount);

    hourStepCount = tmpHourStepCount;
    minuteStepCount = tmpMinuteStepCount;

    // printf("Steps to current time: hour: %i, minute: %i\n", hourStepCount, minuteStepCount);

    if ((hourStepCount > 0) || (minuteStepCount > 0))
    {
        configurePwmAs50hzTimer(&pwm_wrap_irq_handler_seek);

        // Wait until pwm unit got disabled by the seek irq handler
        while ((pwm_hw->en & (1 << slice_num)) != 0)
            tight_loop_contents();

        deconfigurePwmFrom50hzTimer(&pwm_wrap_irq_handler_seek);
    }
}