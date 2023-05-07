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
bool up_limit = false;

void GPIOE_Handler(void)
{
	up_limit = true;
	GPIOIntClear(GPIOE_BASE,GPIO_INT_PIN_0);
}

void limit_init()
{
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOE);
  while(!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOE));
  GPIOUnlockPin(GPIOE_BASE,GPIO_PIN_0);
	GPIOPinTypeGPIOInput(GPIOE_BASE,GPIO_PIN_0);
	GPIOPadConfigSet(GPIOE_BASE,GPIO_PIN_0,GPIO_STRENGTH_2MA,GPIO_PIN_TYPE_STD_WPU);
	
	GPIOIntEnable(GPIOE_BASE, GPIO_INT_PIN_0); 
  GPIOIntTypeSet(GPIOE_BASE, GPIO_PIN_0, GPIO_FALLING_EDGE); 
  GPIOIntRegister(GPIOE_BASE, GPIOE_Handler);
	IntPrioritySet(INT_GPIOE, 0xA0);
	
	
}

xSemaphoreHandle up_semaphore;
xSemaphoreHandle down_semaphore;
SemaphoreHandle_t mutex;
void delay(int ms)
{
			for(int i=0;i<ms;i++)
		{
			for(int j=0;j<3180;j++);
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
		if (up_limit)
		{
			xSemaphoreGive(mutex);
		}
		else{
		motor_stop();
		motor_run(CLOCKWISE);
		delay(1000);
		if (!GPIOPinRead(GPIOF_BASE,GPIO_PIN_0))
		{
			// Add a condition for the limit
		while(!GPIOPinRead(GPIOF_BASE,GPIO_PIN_0) && !up_limit);
		}
		else
		{
			// Add a condition for the limit
			while(!up_limit);
		}
		motor_stop();
		xSemaphoreGive(mutex);
	}
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
		//delay(3000);
		
		motor_run(ANTICLOCKWISE);
		up_limit = false;
		delay(2000);
		if (!GPIOPinRead(GPIOF_BASE,GPIO_PIN_4))
		{
			// Add a condition for the limit
		while(!GPIOPinRead(GPIOF_BASE,GPIO_PIN_4));
		}
		else
		{
			// Add a condition for tha limit
			while(1);
		}
		motor_stop();
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

void idle(void * params)
{
	for(;;)
	{
		
	}
	
	
}
int main()
{
	IntMasterEnable();
	vSemaphoreCreateBinary(up_semaphore);
	vSemaphoreCreateBinary(down_semaphore);
	mutex = xSemaphoreCreateMutex();
	
	limit_init();
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
	xTaskCreate( idle, "idle", 240, NULL, 1, NULL );
	


	vTaskStartScheduler();
	
	// The following line should never be reached.  Failure to allocate enough
	//	memory from the heap would be one reason.
	for (;;);
	
}

