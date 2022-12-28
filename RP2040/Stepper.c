#include "hardware/gpio.h"

#include "Stepper.h"

/*
0: AOUT1
1: AOUT2
2: BOUT1
3: BOUT2

1=mosfet active
0=mostfet inactive
*/

/// @brief Sequence for controling the h-bridge so the stepper motor takes 4 full steps.
const uint32_t stepSequence[] = {0b1000, 0b0010, 0b0100, 0b0001};
const uint32_t stepSequenceLength = count_of(stepSequence);

/// @brief Configuration for a stepper motor
struct stepper
{
    /// @brief Bit mask for the gpio pins used to drive the h-bridge of the stepper.
    /// All used pins need to be consecutive.
    uint32_t gpio_mask;

    /// @brief Number of bits the values of the step sequence need to be shifted by to align with the gpio mask.
    uint32_t gpio_shift;

    /// @brief Initial index of the step sequnce array. Leave at zero.
    uint32_t step_index;
};

/// @brief Configuration for the hour stepper motor.
struct stepper hourStepper = {
    0b00001111,
    0,
    0,
};

/// @brief Configuration for the minute stepper motor.
struct stepper minuteStepper = {
    0b11110000,
    4,
    0,
};

/// @brief Initializes the gpio pins for a stepper motor using the given configuration.
/// @param stepper The stepper motor to configure.
void initStepper(struct stepper *stepper)
{
    gpio_init_mask(stepper->gpio_mask);
    gpio_set_dir_out_masked(stepper->gpio_mask);
    gpio_put_masked(stepper->gpio_mask, stepSequence[0] << stepper->gpio_shift);
}

/// @brief Moves the given stepper motor in the indicated direction.
/// @param stepper The stepper motor to move.
/// @param forward The direction to move the stepper motor.
void stepperStep(struct stepper *stepper, bool forward)
{
    if (forward)
    {
        if (stepper->step_index < stepSequenceLength - 1)
            stepper->step_index++;
        else
            stepper->step_index = 0;
    }
    else
    {
        if (stepper->step_index > 0)
            stepper->step_index--;
        else
            stepper->step_index = stepSequenceLength - 1;
    }

    uint32_t value = stepSequence[stepper->step_index];
    uint32_t shiftedValue = value << stepper->gpio_shift;

    gpio_put_masked(stepper->gpio_mask, shiftedValue);
}
