#include "button.h"

// Driver & Passenger Buttons
void PD_init(void)
{
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOD);
	while (!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOD))
		;
	GPIOUnlockPin(GPIOD_BASE, GPIO_PIN_3 | GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2);
	GPIOPinTypeGPIOInput(GPIOD_BASE, GPIO_PIN_3 | GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2);
	GPIOPadConfigSet(GPIOD_BASE, GPIO_PIN_3 | GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2, GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD_WPD);
	GPIOIntEnable(GPIOD_BASE, GPIO_PIN_3 | GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2);
	GPIOIntTypeSet(GPIOD_BASE, GPIO_PIN_3 | GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2, GPIO_RISING_EDGE);

}

// Lock & Jam Buttons
void JL_init()
{
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOC);
	while (!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOC))
		;
	GPIOUnlockPin(GPIOC_BASE, GPIO_PIN_4 | GPIO_PIN_5);
	GPIOPinTypeGPIOInput(GPIOC_BASE, GPIO_PIN_4 | GPIO_PIN_5);
	GPIOPadConfigSet(GPIOC_BASE, GPIO_PIN_4 | GPIO_PIN_5, GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD_WPD);

	GPIOIntEnable(GPIOC_BASE, GPIO_INT_PIN_4 | GPIO_PIN_5);
	GPIOIntTypeSet(GPIOC_BASE, GPIO_PIN_5 | GPIO_PIN_4, GPIO_FALLING_EDGE);
	// GPIOIntTypeSet(GPIOC_BASE, GPIO_PIN_4, GPIO_FALLING_EDGE);

}

// Limit Switches 
void limit_init()
{
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOE);
	while (!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOE))
		;
	GPIOUnlockPin(GPIOE_BASE, GPIO_PIN_0 | GPIO_PIN_1);
	GPIOPinTypeGPIOInput(GPIOE_BASE, GPIO_PIN_0 | GPIO_PIN_1);
	GPIOPadConfigSet(GPIOE_BASE, GPIO_PIN_0 | GPIO_PIN_1, GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD_WPD);

	GPIOIntEnable(GPIOE_BASE, GPIO_INT_PIN_0 | GPIO_PIN_1);
	GPIOIntTypeSet(GPIOE_BASE, GPIO_PIN_0 | GPIO_PIN_1, GPIO_FALLING_EDGE);

}