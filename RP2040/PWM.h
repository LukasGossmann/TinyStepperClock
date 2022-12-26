#ifndef PWM_H
#define PWM_H

#include "pico/stdlib.h"

void seekClockHands(datetime_t *dateTime);
void clear50hzTimerIrq();
void configurePwmAs50hzTimer(irq_handler_t pwmWrapIrqHandler);
void deconfigurePwmFrom50hzTimer(irq_handler_t pwmWrapIrqHandler);

#endif