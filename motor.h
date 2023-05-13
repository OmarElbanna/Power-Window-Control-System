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
#define IDLE 2
void motor_init(void);
void motor_run(uint8_t direction);
void motor_stop(void);
uint8_t get_state();
void delay(int ms);




#endif