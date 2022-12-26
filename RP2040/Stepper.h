#ifndef STEPPER_H
#define STEPPER_H

extern struct stepper hourStepper;
extern struct stepper minuteStepper;

void init_stepper(struct stepper *stepper);
void stepper_step(struct stepper *stepper, bool forward);

#endif