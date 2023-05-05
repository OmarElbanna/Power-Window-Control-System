#ifndef MOTOR_H
#define MOTOR_H
#include <stdint.h>
#include <stdbool.h>
#include "driverlib/gpio.h"
#include "driverlib/sysctl.h"
#include "driverlib/systick.h"
#include "driverlib/interrupt.h"
#include "driverlib/sysctl.h"
#define CLOCKWISE 0
#define ANTICLOCKWISE 1
void motor_init(void);
void motor_run(uint8_t direction);
void motor_stop(void);




#endif