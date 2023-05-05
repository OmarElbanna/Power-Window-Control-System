#include <stdint.h>
#include <FreeRTOS.h>
#include <task.h>
#include <stdbool.h>
#include "inc/hw_memmap.h"
#include "driverlib/gpio.h"
#include "driverlib/sysctl.h"
#include "driverlib/systick.h"
#include "driverlib/interrupt.h"
#include "driverlib/sysctl.h"
#include "driverlib/uart.h"
#include "queue.h"
#include "semphr.h"
#include "motor.h"

xSemaphoreHandle semaphore;int x = 0;


void run_motor(void *pv)
{
	xSemaphoreTake(semaphore,0);
	for(;;){
	xSemaphoreTake(semaphore,portMAX_DELAY);
	motor_run(CLOCKWISE);
		vTaskDelay(pdMS_TO_TICKS(5000));
		motor_stop();
		vTaskDelay(pdMS_TO_TICKS(5000));
		motor_run(ANTICLOCKWISE);
		vTaskDelay(pdMS_TO_TICKS(5000));
		motor_stop();
		vTaskDelay(pdMS_TO_TICKS(5000));
	}
}

void continous(void* params)
{
	for(;;)
	{
		x++;
	}
	
	
}








void GPIOF_Handler(void)
{
	portBASE_TYPE xHigherPriorityTaskWoken = pdFALSE;
	xSemaphoreGiveFromISR(semaphore,&xHigherPriorityTaskWoken);
	GPIOIntClear(GPIOF_BASE,GPIO_INT_PIN_0);
	portEND_SWITCHING_ISR(xHigherPriorityTaskWoken);
	
}


int main()
{
	IntMasterEnable();
	vSemaphoreCreateBinary(semaphore);
	
	motor_init();
	
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);
  while(!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOF));
  GPIOUnlockPin(GPIOF_BASE,GPIO_PIN_4|GPIO_PIN_0);
	GPIOPinTypeGPIOInput(GPIOF_BASE,GPIO_PIN_0|GPIO_PIN_4);
	GPIOPadConfigSet(GPIOF_BASE,GPIO_PIN_0|GPIO_PIN_4,GPIO_STRENGTH_2MA,GPIO_PIN_TYPE_STD_WPU);
	
	GPIOIntEnable(GPIOF_BASE, GPIO_INT_PIN_0); 
  GPIOIntTypeSet(GPIOF_BASE, GPIO_PIN_0, GPIO_FALLING_EDGE); 
  GPIOIntRegister(GPIOF_BASE, GPIOF_Handler);
	IntPrioritySet(INT_GPIOF, 0xE0);

	xTaskCreate( run_motor, "init", 240, NULL, 2, NULL );
	xTaskCreate (continous,"init", 240, NULL, 1, NULL);


	vTaskStartScheduler();
	
	// The following line should never be reached.  Failure to allocate enough
	//	memory from the heap would be one reason.
	for (;;);
	
}

