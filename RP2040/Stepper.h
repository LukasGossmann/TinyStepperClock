#ifndef STEPPER_H
#define STEPPER_H

extern struct stepper hourStepper;
extern struct stepper minuteStepper;

void initStepper(struct stepper *stepper);
void stepperStep(struct stepper *stepper, bool forward);

#endif