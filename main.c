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
bool down_limit = false;
void GPIOE_Handler(void)
{
	  if(GPIOIntStatus(GPIOE_BASE,true)==1<<0)
  {
		down_limit = false;
		up_limit = true;
		GPIOIntClear(GPIOE_BASE,GPIO_INT_PIN_0);
  }
	else if(GPIOIntStatus(GPIOE_BASE,true)==1<<1)
  {
		up_limit = false;
   	down_limit = true;
		GPIOIntClear(GPIOE_BASE,GPIO_INT_PIN_1);
  }
	
}

void limit_init()
{
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOE);
  while(!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOE));
  GPIOUnlockPin(GPIOE_BASE,GPIO_PIN_0|GPIO_PIN_1);
	GPIOPinTypeGPIOInput(GPIOE_BASE,GPIO_PIN_0|GPIO_PIN_1);
	GPIOPadConfigSet(GPIOE_BASE,GPIO_PIN_0|GPIO_PIN_1,GPIO_STRENGTH_2MA,GPIO_PIN_TYPE_STD_WPU);
	
	GPIOIntEnable(GPIOE_BASE, GPIO_INT_PIN_0|GPIO_PIN_1); 
  GPIOIntTypeSet(GPIOE_BASE, GPIO_PIN_0|GPIO_PIN_1, GPIO_FALLING_EDGE); 
  GPIOIntRegister(GPIOE_BASE, GPIOE_Handler);
	IntPrioritySet(INT_GPIOE, 0xA0);
	
	
}
//passenger semaphores
xSemaphoreHandle p_up_semaphore;
xSemaphoreHandle p_down_semaphore;
SemaphoreHandle_t p_mutex;

xSemaphoreHandle d_up_semaphore;
xSemaphoreHandle d_down_semaphore;
SemaphoreHandle_t d_mutex;
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

void p_motor_up_task(void *params)
{
	xSemaphoreTake(p_up_semaphore,0);
	for(;;)
	{
		xSemaphoreTake(p_up_semaphore,portMAX_DELAY);
		xSemaphoreTake(p_mutex,portMAX_DELAY);
		if (up_limit)
		{
			xSemaphoreGive(p_mutex);
		}
		else{
		motor_stop();
		motor_run(CLOCKWISE);
		down_limit = false;
			
		delay(1000);
		if (!GPIOPinRead(GPIOD_BASE,GPIO_PIN_0))
		{
			
			// Add a condition for the limit
		while(!GPIOPinRead(GPIOD_BASE,GPIO_PIN_0) && !up_limit);
		}
		else
		{
			// Add a condition for the limit
			while(!up_limit);
		}
		motor_stop();
		xSemaphoreGive(p_mutex);
	}
}
	
}

void p_motor_down_task(void *params)
{
	xSemaphoreTake(p_down_semaphore,0);
	for(;;)
	{
		xSemaphoreTake(p_down_semaphore,portMAX_DELAY);
		xSemaphoreTake(p_mutex,portMAX_DELAY);
		if (down_limit)
		{
			xSemaphoreGive(p_mutex);
		}
		else{
		motor_stop();
		//delay(3000);
		
		motor_run(ANTICLOCKWISE);
		up_limit = false;
		delay(1000);
		if (!GPIOPinRead(GPIOD_BASE,GPIO_PIN_1))
		{
			// Add a condition for the limit
		while(!GPIOPinRead(GPIOD_BASE,GPIO_PIN_1)&& !down_limit);
		}
		else
		{
			// Add a condition for tha limit
			while(!down_limit);
		}
		motor_stop();
		xSemaphoreGive(p_mutex);
	}
}
	
}



void d_motor_up_task(void *params)
{
	xSemaphoreTake(d_up_semaphore,0);
	for(;;)
	{
		xSemaphoreTake(d_up_semaphore,portMAX_DELAY);
		xSemaphoreTake(d_mutex,portMAX_DELAY);
		if (up_limit)
		{
			xSemaphoreGive(d_mutex);
		}
		else{
		motor_stop();
		motor_run(CLOCKWISE);
		down_limit = false;
			
		delay(1000);
		if (!GPIOPinRead(GPIOD_BASE,GPIO_PIN_2))
		{
			
			// Add a condition for the limit
		while(!GPIOPinRead(GPIOD_BASE,GPIO_PIN_2) && !up_limit);
		}
		else
		{
			// Add a condition for the limit
			while(!up_limit);
		}
		motor_stop();
		xSemaphoreGive(d_mutex);
	}
}
	
}

void d_motor_down_task(void *params)
{
	xSemaphoreTake(d_down_semaphore,0);
	for(;;)
	{
		xSemaphoreTake(d_down_semaphore,portMAX_DELAY);
		xSemaphoreTake(d_mutex,portMAX_DELAY);
		if (down_limit)
		{
			xSemaphoreGive(d_mutex);
		}
		else{
		motor_stop();
		//delay(3000);
		
		motor_run(ANTICLOCKWISE);
		up_limit = false;
		delay(1000);
		if (!GPIOPinRead(GPIOD_BASE,GPIO_PIN_3))
		{
			// Add a condition for the limit
		while(!GPIOPinRead(GPIOD_BASE,GPIO_PIN_3)&& !down_limit);
		}
		else
		{
			// Add a condition for tha limit
			while(!down_limit);
		}
		motor_stop();
		xSemaphoreGive(d_mutex);
	}
}
	
}














void GPIOD_Handler(void)
{
	  //Passenger_up
	  if(GPIOIntStatus(GPIOD_BASE,true)==1<<0)
  {
   portBASE_TYPE xHigherPriorityTaskWoken = pdFALSE; 
	xSemaphoreGiveFromISR(p_up_semaphore,&xHigherPriorityTaskWoken);
	GPIOIntClear(GPIOD_BASE,GPIO_INT_PIN_0);
	portEND_SWITCHING_ISR(xHigherPriorityTaskWoken);
	
  }
	//Passenger_down
	else if(GPIOIntStatus(GPIOD_BASE,true)==1<<1)
  {
   portBASE_TYPE xHigherPriorityTaskWoken = pdFALSE; 
	xSemaphoreGiveFromISR(p_down_semaphore,&xHigherPriorityTaskWoken);
	GPIOIntClear(GPIOD_BASE,GPIO_INT_PIN_1);
	portEND_SWITCHING_ISR(xHigherPriorityTaskWoken);
	
  }
	//driver_up
	
	else if(GPIOIntStatus(GPIOD_BASE,true)==1<<2)
  {
   portBASE_TYPE xHigherPriorityTaskWoken = pdFALSE; 
	xSemaphoreGiveFromISR(d_up_semaphore,&xHigherPriorityTaskWoken);
	GPIOIntClear(GPIOD_BASE,GPIO_INT_PIN_2);
	portEND_SWITCHING_ISR(xHigherPriorityTaskWoken);
	
  }
	//driver_down
	else if(GPIOIntStatus(GPIOD_BASE,true)==1<<3)
  {
   portBASE_TYPE xHigherPriorityTaskWoken = pdFALSE; 
	xSemaphoreGiveFromISR(d_down_semaphore,&xHigherPriorityTaskWoken);
	GPIOIntClear(GPIOD_BASE,GPIO_INT_PIN_3);
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
	vSemaphoreCreateBinary(p_up_semaphore);
	vSemaphoreCreateBinary(p_down_semaphore);
	p_mutex = xSemaphoreCreateMutex();
	
	vSemaphoreCreateBinary(d_up_semaphore);
	vSemaphoreCreateBinary(d_down_semaphore);
	d_mutex = xSemaphoreCreateMutex();
	
	limit_init();
	motor_init();
	
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOD);
  while(!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOD));
  GPIOUnlockPin(GPIOD_BASE,GPIO_PIN_3|GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_2);
	GPIOPinTypeGPIOInput(GPIOD_BASE,GPIO_PIN_3|GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_2);
	GPIOPadConfigSet(GPIOD_BASE,GPIO_PIN_3|GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_2,GPIO_STRENGTH_2MA,GPIO_PIN_TYPE_STD_WPU);
	
	GPIOIntEnable(GPIOD_BASE, GPIO_PIN_3|GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_2); 
  GPIOIntTypeSet(GPIOD_BASE, GPIO_PIN_3|GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_2, GPIO_FALLING_EDGE); 
  GPIOIntRegister(GPIOD_BASE, GPIOD_Handler);
	IntPrioritySet(INT_GPIOD, 0xE0);

	xTaskCreate( p_motor_down_task, "down", 240, NULL, 2, NULL );
	xTaskCreate( p_motor_up_task, "up", 240, NULL, 2, NULL );
	
	xTaskCreate( d_motor_down_task, "down", 240, NULL, 3, NULL );
	xTaskCreate( d_motor_up_task, "up", 240, NULL, 3, NULL );
	xTaskCreate( idle, "idle", 240, NULL, 1, NULL );
	


	vTaskStartScheduler();
	
	// The following line should never be reached.  Failure to allocate enough
	//	memory from the heap would be one reason.
	for (;;);
	
}

