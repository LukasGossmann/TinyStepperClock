#include "pico/stdlib.h"
#include <stdio.h>

#include "Stepper.h"

/*
3: AOUT1
2: AOUT2
1: BOUT1
0: BOUT2

1=mosfet active
0=mostfet inactive
*/
const uint32_t stepSequence[] = {0b1000, 0b0010, 0b0100, 0b0001};
const uint32_t stepSequenceLength = count_of(halfStepSequence);

struct stepper
{
    uint32_t gpio_mask;
    uint32_t gpio_shift;
    uint32_t microstep_index;
};

struct stepper hourStepper = {
    0b00001111,
    0,
    0,
};
struct stepper minuteStepper = {
    0b11110000,
    4,
    0,
};

void init_stepper(struct stepper *stepper)
{
    gpio_init_mask(stepper->gpio_mask);
    gpio_set_dir_out_masked(stepper->gpio_mask);
    gpio_put_masked(stepper->gpio_mask, stepSequence[0] << stepper->gpio_shift);
}

void stepper_step(struct stepper *stepper, bool forward)
{
    if (forward)
    {
        if (stepper->microstep_index < stepSequenceLength - 1)
            stepper->microstep_index++;
        else
            stepper->microstep_index = 0;
    }
    else
    {
        if (stepper->microstep_index > 0)
            stepper->microstep_index--;
        else
            stepper->microstep_index = stepSequenceLength - 1;
    }

    uint32_t value = stepSequence[stepper->microstep_index];
    uint32_t shiftedValue = value << stepper->gpio_shift;

    gpio_put_masked(stepper->gpio_mask, shiftedValue);
}
