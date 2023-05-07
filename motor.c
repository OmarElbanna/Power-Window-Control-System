#include "motor.h"
#include <FreeRTOS.h>
void motor_init(void)
{
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);
  while(!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOB));
  GPIOPinTypeGPIOOutput(GPIOB_BASE, GPIO_PIN_1 | GPIO_PIN_0);
  GPIOUnlockPin(GPIOB_BASE,GPIO_PIN_0|GPIO_PIN_1);
  GPIOPinWrite(GPIOB_BASE,GPIO_PIN_1|GPIO_PIN_0,0x00);
}

void motor_run(uint8_t direction)
{

	
	if(direction==CLOCKWISE)
	{
		GPIOPinWrite(GPIOB_BASE,GPIO_PIN_0|GPIO_PIN_1,GPIO_PIN_1);
		
	}
	else if(direction==ANTICLOCKWISE)
	{
		GPIOPinWrite(GPIOB_BASE,GPIO_PIN_0|GPIO_PIN_1,GPIO_PIN_0);
	}	
}
void motor_stop(void)
{
	GPIOPinWrite(GPIOB_BASE,GPIO_PIN_1|GPIO_PIN_0,0x00);
}