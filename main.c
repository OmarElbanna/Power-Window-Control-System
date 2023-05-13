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
#include "button.h"

//**************************************************************************//
//																																					//
// 													Declarations																		//
//																																					//
//**************************************************************************//

// Flags Queues
QueueHandle_t q_up_limit, q_down_limit, q_driver_flag, q_lock_flag, q_jam_flag;

// Motor Mutex
SemaphoreHandle_t mutex;

// Passenger Semaphores
xSemaphoreHandle p_up_semaphore;
xSemaphoreHandle p_down_semaphore;

// Driver Semaphores
xSemaphoreHandle d_up_semaphore;
xSemaphoreHandle d_down_semaphore;

// Jam Task Semaphore
SemaphoreHandle_t jam_semaphore;

//**************************************************************************//
//																																					//
// 												Interrupt Service Routines												//
//																																					//
//**************************************************************************//


// Limits ISR
void GPIOE_Handler(void)
{
	int value1 = 1;
	int value0 = 0;
	BaseType_t xHigherPriorityTaskWoken = pdFALSE;

	if (GPIOIntStatus(GPIOE_BASE, true) == 1 << 0)
	{
		// inserted the value 1 to q_up and 0 to q_down
		xQueueOverwriteFromISR(q_down_limit, &value0, &xHigherPriorityTaskWoken);
		xQueueOverwriteFromISR(q_up_limit, &value1, &xHigherPriorityTaskWoken);
		GPIOIntClear(GPIOE_BASE, GPIO_INT_PIN_0);
	}
	else if (GPIOIntStatus(GPIOE_BASE, true) == 1 << 1)
	// inserted the value 0 to q_up and 1 to q_down
	{
		xQueueOverwriteFromISR(q_down_limit, &value1, &xHigherPriorityTaskWoken);
		xQueueOverwriteFromISR(q_up_limit, &value0, &xHigherPriorityTaskWoken);
		GPIOIntClear(GPIOE_BASE, GPIO_INT_PIN_1);
	}
	else if (GPIOIntStatus(GPIOE_BASE, true) == 3)
	{
		GPIOIntClear(GPIOE_BASE, GPIO_INT_PIN_1 | GPIO_INT_PIN_0);
	}
}

void GPIOD_Handler(void)
{
	portBASE_TYPE xHigherPriorityTaskWoken = pdFALSE;

	int lock_flag, driver_value = 1,driver_flag=0;

	xQueuePeekFromISR(q_lock_flag, &lock_flag);
	xQueuePeekFromISR(q_driver_flag, &driver_flag);

	// driver_up

	if (GPIOIntStatus(GPIOD_BASE, true) == 1 << 2)
	{
		driver_value = 1;
		xQueueOverwriteFromISR(q_driver_flag, &driver_value, &xHigherPriorityTaskWoken);

		xSemaphoreGiveFromISR(d_up_semaphore, &xHigherPriorityTaskWoken);
		GPIOIntClear(GPIOD_BASE, GPIO_INT_PIN_2);
		portEND_SWITCHING_ISR(xHigherPriorityTaskWoken);
	}
	// driver_down
	else if (GPIOIntStatus(GPIOD_BASE, true) == 1 << 3)
	{
		driver_value = 1;
		xQueueOverwriteFromISR(q_driver_flag, &driver_value, &xHigherPriorityTaskWoken);

		xSemaphoreGiveFromISR(d_down_semaphore, &xHigherPriorityTaskWoken);
		GPIOIntClear(GPIOD_BASE, GPIO_INT_PIN_3);
		portEND_SWITCHING_ISR(xHigherPriorityTaskWoken);
	}
	// Passenger_up
	else if (GPIOIntStatus(GPIOD_BASE, true) == 1 << 0)
	{
		
		
		if (driver_flag || lock_flag)
		{
			GPIOIntClear(GPIOD_BASE, GPIO_INT_PIN_0);
		}
		else
		{
			xSemaphoreGiveFromISR(p_up_semaphore, &xHigherPriorityTaskWoken);
			GPIOIntClear(GPIOD_BASE, GPIO_INT_PIN_0);
			portEND_SWITCHING_ISR(xHigherPriorityTaskWoken);
		}
	}
	// Passenger_down
	else if (GPIOIntStatus(GPIOD_BASE, true) == 1 << 1)
	{
		
		if (driver_flag || lock_flag)
		{
			GPIOIntClear(GPIOD_BASE, GPIO_INT_PIN_1);
		}
		else
		{
			xSemaphoreGiveFromISR(p_down_semaphore, &xHigherPriorityTaskWoken);
			GPIOIntClear(GPIOD_BASE, GPIO_INT_PIN_1);
			portEND_SWITCHING_ISR(xHigherPriorityTaskWoken);
		}
	}
}

void GPIOC_Handler()
{
	int lock_flag;
	xQueuePeekFromISR(q_lock_flag, &lock_flag);
	portBASE_TYPE xHigherPriorityTaskWoken = pdFALSE;

	if (GPIOIntStatus(GPIOC_BASE, true) == 1 << 4)
	{
		GPIOIntClear(GPIOC_BASE, GPIO_INT_PIN_4);
		lock_flag ^= 0x1;
		xQueueOverwriteFromISR(q_lock_flag, &lock_flag, &xHigherPriorityTaskWoken);
	}
	else if (GPIOIntStatus(GPIOC_BASE, true) == 1 << 5)
	{
		GPIOIntClear(GPIOC_BASE, GPIO_INT_PIN_5);
		if (get_state() == CLOCKWISE)
		{
			int jam_value = 1;
			xQueueOverwriteFromISR(q_jam_flag, &jam_value, &xHigherPriorityTaskWoken);

			xSemaphoreGiveFromISR(jam_semaphore, &xHigherPriorityTaskWoken);
			portEND_SWITCHING_ISR(xHigherPriorityTaskWoken);
		}
	}
}



//**************************************************************************//
//																																					//
// 																Tasks																			//
//																																					//
//**************************************************************************//

// queue initialization task
void q_init(void *params)
{
	for (;;)
	{
		int up_initial = 0;
		xQueueSendToBack(q_up_limit, &up_initial, 0);
		xQueueSendToBack(q_down_limit, &up_initial, 0);
		xQueueSendToBack(q_driver_flag, &up_initial, 0);
		xQueueSendToBack(q_lock_flag, &up_initial, 0);
		xQueueSendToBack(q_jam_flag, &up_initial, 0);

		vTaskSuspend(NULL);
	}
}

// Passenager Up Task
void p_motor_up_task(void *params)
{
	xSemaphoreTake(p_up_semaphore, 0);
	for (;;)
	{
		xSemaphoreTake(p_up_semaphore, portMAX_DELAY);
		xSemaphoreTake(mutex, portMAX_DELAY);

		int up_limit, lock_flag, jam_flag, driver_flag;
		xQueuePeek(q_jam_flag, &jam_flag, 0);
		xQueuePeek(q_driver_flag, &driver_flag, 0);
		xQueuePeek(q_up_limit, &up_limit, 0);
		xQueuePeek(q_lock_flag, &lock_flag, 0);

		if (up_limit)
		{
			xSemaphoreGive(mutex);
		}
		else
		{
			motor_stop();
			motor_run(CLOCKWISE);

			int value = 0;
			xQueueOverwrite(q_down_limit, &value);
			delay(500);
			if (GPIOPinRead(GPIOD_BASE, GPIO_PIN_0))
			{
				// Manual
				while (GPIOPinRead(GPIOD_BASE, GPIO_PIN_0) && !up_limit && !driver_flag && !lock_flag && !jam_flag)
				{
					xQueuePeek(q_up_limit, &up_limit, 0);
					xQueuePeek(q_lock_flag, &lock_flag, 0);
					xQueuePeek(q_jam_flag, &jam_flag, 0);
					xQueuePeek(q_driver_flag, &driver_flag, 0);
				}
			}
			else
			{
				// Automatic
				while (!up_limit && !driver_flag && !lock_flag && !jam_flag)
				{

					xQueuePeek(q_up_limit, &up_limit, 0);
					xQueuePeek(q_jam_flag, &jam_flag, 0);
					xQueuePeek(q_lock_flag, &lock_flag, 0);
					xQueuePeek(q_driver_flag, &driver_flag, 0);

					int down = GPIOPinRead(GPIOD_BASE, GPIO_PIN_1);
					if (down)
						break;
				}
			}
			motor_stop();

			int jam_value = 0;
			xQueueOverwrite(q_jam_flag, &jam_value);

			xSemaphoreGive(mutex);
		}
	}
}
// Passenager Down Task
void p_motor_down_task(void *params)
{
	xSemaphoreTake(p_down_semaphore, 0);
	for (;;)
	{
		xSemaphoreTake(p_down_semaphore, portMAX_DELAY);
		xSemaphoreTake(mutex, portMAX_DELAY);

		int down_limit, lock_flag, driver_flag;

		xQueuePeek(q_lock_flag, &lock_flag, 0);
		xQueuePeek(q_down_limit, &down_limit, 0);
		xQueuePeek(q_driver_flag, &driver_flag, 0);

		if (down_limit)
		{
			xSemaphoreGive(mutex);
		}
		else
		{
			motor_stop();
			motor_run(ANTICLOCKWISE);
			int value = 0;
			xQueueOverwrite(q_up_limit, &value);
			delay(500);
			if (GPIOPinRead(GPIOD_BASE, GPIO_PIN_1))
			{
				// Manual
				while (GPIOPinRead(GPIOD_BASE, GPIO_PIN_1) && !down_limit && !driver_flag && !lock_flag)
				{
					xQueuePeek(q_driver_flag, &driver_flag, 0);

					xQueuePeek(q_down_limit, &down_limit, 0);
					xQueuePeek(q_lock_flag, &lock_flag, 0);
				}
			}
			else
			{
				// Automatic
				while (!down_limit && !driver_flag && !lock_flag)
				{
					xQueuePeek(q_down_limit, &down_limit, 0);
					xQueuePeek(q_lock_flag, &lock_flag, 0);
					xQueuePeek(q_driver_flag, &driver_flag, 0);

					int up = GPIOPinRead(GPIOD_BASE, GPIO_PIN_0);
					if (up)
						break;
				}
			}
			motor_stop();
			xSemaphoreGive(mutex);
		}
	}
}

// Driver Up Task
void d_motor_up_task(void *params)
{
	xSemaphoreTake(d_up_semaphore, 0);
	for (;;)
	{
		xSemaphoreTake(d_up_semaphore, portMAX_DELAY);
		xSemaphoreTake(mutex, portMAX_DELAY);
		int up_limit, jam_flag, driver_flag = 0;
		xQueuePeek(q_jam_flag, &jam_flag, 0);
		xQueuePeek(q_up_limit, &up_limit, 0);
		if (up_limit)
		{
			xQueueOverwrite(q_driver_flag, &driver_flag);
			xSemaphoreGive(mutex);
		}
		else
		{
			motor_stop();
			motor_run(CLOCKWISE);
			int value = 0;
			xQueueOverwrite(q_down_limit, &value);
			delay(500);
			if (GPIOPinRead(GPIOD_BASE, GPIO_PIN_2))
			{
				// Manual
				while (GPIOPinRead(GPIOD_BASE, GPIO_PIN_2) && !up_limit && !jam_flag)
				{
					xQueuePeek(q_up_limit, &up_limit, 0);
					xQueuePeek(q_jam_flag, &jam_flag, 0);
				}
			}
			else
			{
				// Automatic
				while (!up_limit && !jam_flag)
				{
					xQueuePeek(q_up_limit, &up_limit, 0);
					xQueuePeek(q_jam_flag, &jam_flag, 0);

					int down = GPIOPinRead(GPIOD_BASE, GPIO_PIN_3);
					if (down)
						break;
				}
			}
			motor_stop();
			int jam_value = 0;
			xQueueOverwrite(q_jam_flag, &jam_value);
			xQueueOverwrite(q_driver_flag, &driver_flag);

			xSemaphoreGive(mutex);
		}
	}
}

// Driver Down Task
void d_motor_down_task(void *params)
{
	xSemaphoreTake(d_down_semaphore, 0);
	for (;;)
	{
		xSemaphoreTake(d_down_semaphore, portMAX_DELAY);
		xSemaphoreTake(mutex, portMAX_DELAY);

		int down_limit, driver_flag = 0;
		xQueuePeek(q_down_limit, &down_limit, 0);

		if (down_limit)
		{
			xQueueOverwrite(q_driver_flag, &driver_flag);

			xSemaphoreGive(mutex);
		}
		else
		{
			motor_stop();
			motor_run(ANTICLOCKWISE);
			int value = 0;
			xQueueOverwrite(q_up_limit, &value);
			delay(500);
			if (GPIOPinRead(GPIOD_BASE, GPIO_PIN_3))
			{
				// Manual

				while (GPIOPinRead(GPIOD_BASE, GPIO_PIN_3) && !down_limit)
				{
					xQueuePeek(q_down_limit, &down_limit, 0);
				}
			}
			else
			{
				// Automatic
				while (!down_limit)
				{
					xQueuePeek(q_down_limit, &down_limit, 0);
					int up = GPIOPinRead(GPIOD_BASE, GPIO_PIN_2);
					if (up)
						break;
				}
			}
			motor_stop();
			xQueueOverwrite(q_driver_flag, &driver_flag);
			xSemaphoreGive(mutex);
		}
	}
}
// Jam Protection Task
void jam(void *params)
{
	xSemaphoreTake(jam_semaphore, 0);
	for (;;)
	{
		xSemaphoreTake(jam_semaphore, portMAX_DELAY);
		xSemaphoreTake(mutex, portMAX_DELAY);
		motor_stop();
		delay(500);
		motor_run(ANTICLOCKWISE);
		delay(500);
		motor_stop();
		xSemaphoreGive(mutex);
	}
}

int main()
{
	IntMasterEnable();
	vSemaphoreCreateBinary(p_up_semaphore);
	vSemaphoreCreateBinary(p_down_semaphore);

	vSemaphoreCreateBinary(d_up_semaphore);
	vSemaphoreCreateBinary(d_down_semaphore);
	mutex = xSemaphoreCreateMutex();

	vSemaphoreCreateBinary(jam_semaphore);

	q_up_limit = xQueueCreate(1, sizeof(int));
	q_down_limit = xQueueCreate(1, sizeof(int));
	q_lock_flag = xQueueCreate(1, sizeof(int));
	q_driver_flag = xQueueCreate(1, sizeof(int));
	q_jam_flag = xQueueCreate(1, sizeof(int));

	motor_init();
	
	limit_init();
	GPIOIntRegister(GPIOE_BASE, GPIOE_Handler);
	IntPrioritySet(INT_GPIOE, 0xA0);
	
	
	JL_init();
	GPIOIntRegister(GPIOC_BASE, GPIOC_Handler);
	IntPrioritySet(INT_GPIOC, 0xA0);
	
	PD_init();
	GPIOIntRegister(GPIOD_BASE, GPIOD_Handler);
	IntPrioritySet(INT_GPIOD, 0xE0);

	xTaskCreate(p_motor_down_task, "passenger down", 240, NULL, 2, NULL);
	xTaskCreate(p_motor_up_task, "passenger up", 240, NULL, 2, NULL);

	xTaskCreate(d_motor_down_task, "driver down", 240, NULL, 3, NULL);
	xTaskCreate(d_motor_up_task, "driver up", 240, NULL, 3, NULL);
	xTaskCreate(jam, "jam", 240, NULL, 4, NULL);
	xTaskCreate(q_init, "q_init", 240, NULL, 4, NULL);

	vTaskStartScheduler();

	// The following line should never be reached.  Failure to allocate enough
	//	memory from the heap would be one reason.
	for (;;);
}

// Idle Task
void vApplicationIdleHook(void)
{
	
}
