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

xSemaphoreHandle up_semaphore;
xSemaphoreHandle down_semaphore;
SemaphoreHandle_t mutex;
void delay(int ms)
{for(int k=0;k<3185;k++)
			for(int i=0;i<3185;i++)
		{
			for(int j=0;j<ms;j++);
		}
}


/*void run_motor(void *pv)
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
*/

void motor_up_task(void *params)
{
	xSemaphoreTake(up_semaphore,0);
	for(;;)
	{
		xSemaphoreTake(up_semaphore,portMAX_DELAY);
		xSemaphoreTake(mutex,portMAX_DELAY);
		motor_stop();
		vTaskDelay(pdMS_TO_TICKS(5000));
		motor_run(CLOCKWISE);
		xSemaphoreGive(mutex);
	}
	
	
}

void motor_down_task(void *params)
{
	xSemaphoreTake(down_semaphore,0);
	for(;;)
	{
		xSemaphoreTake(down_semaphore,portMAX_DELAY);
		xSemaphoreTake(mutex,portMAX_DELAY);
		motor_stop();
		delay(5000);
		motor_run(ANTICLOCKWISE);
		xSemaphoreGive(mutex);
	}
	
	
}









void GPIOF_Handler(void)
{
	  if(GPIOIntStatus(GPIOF_BASE,true)==1<<0)
  {
   portBASE_TYPE xHigherPriorityTaskWoken = pdFALSE; 
	xSemaphoreGiveFromISR(up_semaphore,&xHigherPriorityTaskWoken);
	GPIOIntClear(GPIOF_BASE,GPIO_INT_PIN_0);
	portEND_SWITCHING_ISR(xHigherPriorityTaskWoken);
	
  }
	else if(GPIOIntStatus(GPIOF_BASE,true)==1<<4)
  {
   portBASE_TYPE xHigherPriorityTaskWoken = pdFALSE; 
	xSemaphoreGiveFromISR(down_semaphore,&xHigherPriorityTaskWoken);
	GPIOIntClear(GPIOF_BASE,GPIO_INT_PIN_4);
	portEND_SWITCHING_ISR(xHigherPriorityTaskWoken);
	
  }
	
}


int main()
{
	IntMasterEnable();
	vSemaphoreCreateBinary(up_semaphore);
	vSemaphoreCreateBinary(down_semaphore);
	mutex = xSemaphoreCreateMutex();
	
	motor_init();
	
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);
  while(!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOF));
  GPIOUnlockPin(GPIOF_BASE,GPIO_PIN_4|GPIO_PIN_0);
	GPIOPinTypeGPIOInput(GPIOF_BASE,GPIO_PIN_0|GPIO_PIN_4);
	GPIOPadConfigSet(GPIOF_BASE,GPIO_PIN_0|GPIO_PIN_4,GPIO_STRENGTH_2MA,GPIO_PIN_TYPE_STD_WPU);
	
	GPIOIntEnable(GPIOF_BASE, GPIO_INT_PIN_0|GPIO_PIN_4); 
  GPIOIntTypeSet(GPIOF_BASE, GPIO_PIN_0|GPIO_PIN_4, GPIO_FALLING_EDGE); 
  GPIOIntRegister(GPIOF_BASE, GPIOF_Handler);
	IntPrioritySet(INT_GPIOF, 0xE0);

	xTaskCreate( motor_down_task, "down", 240, NULL, 2, NULL );
	xTaskCreate( motor_up_task, "up", 240, NULL, 2, NULL );
	


	vTaskStartScheduler();
	
	// The following line should never be reached.  Failure to allocate enough
	//	memory from the heap would be one reason.
	for (;;);
	
}

