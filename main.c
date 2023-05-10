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

//**************************************************************************//
//																																					//
// 													Declarations																		//
//																																					//
//**************************************************************************//

// Flags
// bool up_limit = false;
// bool down_limit = false;
bool driver_flag = false;
bool lock_flag = false;
bool jam_flag = false;
QueueHandle_t q_up_limit;
QueueHandle_t q_down_limit;
QueueHandle_t q_driver_flag;
QueueHandle_t q_lock_flag;
QueueHandle_t q_jam__flag;


SemaphoreHandle_t mutex;

// Passenger Semaphores
xSemaphoreHandle p_up_semaphore;
xSemaphoreHandle p_down_semaphore;

// Driver Semaphores
xSemaphoreHandle d_up_semaphore;
xSemaphoreHandle d_down_semaphore;

SemaphoreHandle_t jam_semaphore;

//**************************************************************************//
//																																					//
// 												Interrupt Service Routines												//
//																																					//
//**************************************************************************//

void GPIOD_Handler(void);
void buttons_init(void)
{
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOD);
	while (!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOD))
		;
	GPIOUnlockPin(GPIOD_BASE, GPIO_PIN_3 | GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2);
	GPIOPinTypeGPIOInput(GPIOD_BASE, GPIO_PIN_3 | GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2);
	GPIOPadConfigSet(GPIOD_BASE, GPIO_PIN_3 | GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2, GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD_WPD);
	GPIOIntEnable(GPIOD_BASE, GPIO_PIN_3 | GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2);
	GPIOIntTypeSet(GPIOD_BASE, GPIO_PIN_3 | GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2, GPIO_RISING_EDGE);
	GPIOIntRegister(GPIOD_BASE, GPIOD_Handler);
	IntPrioritySet(INT_GPIOD, 0xE0);
}
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
}

void GPIOD_Handler(void)
{
	portBASE_TYPE xHigherPriorityTaskWoken = pdFALSE;

	// driver_up

	if (GPIOIntStatus(GPIOD_BASE, true) == 1 << 2)
	{
		driver_flag = true;

		xSemaphoreGiveFromISR(d_up_semaphore, &xHigherPriorityTaskWoken);
		GPIOIntClear(GPIOD_BASE, GPIO_INT_PIN_2);
		portEND_SWITCHING_ISR(xHigherPriorityTaskWoken);
	}
	// driver_down
	else if (GPIOIntStatus(GPIOD_BASE, true) == 1 << 3)
	{
		driver_flag = true;

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
	if (GPIOIntStatus(GPIOC_BASE, true) == 1 << 4)
	{
		GPIOIntClear(GPIOC_BASE, GPIO_INT_PIN_4);
		lock_flag ^= 0x1;
	}
	else if (GPIOIntStatus(GPIOC_BASE, true) == 1 << 5)
	{
		GPIOIntClear(GPIOC_BASE, GPIO_INT_PIN_5);
		if (get_state() == CLOCKWISE)
		{
			jam_flag = true;
			portBASE_TYPE xHigherPriorityTaskWoken = pdFALSE;
			xSemaphoreGiveFromISR(jam_semaphore, &xHigherPriorityTaskWoken);
			portEND_SWITCHING_ISR(xHigherPriorityTaskWoken);
		}
	}
}

// Lock & Jam
void PortC_init()
{
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOC);
	while (!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOC))
		;
	GPIOUnlockPin(GPIOC_BASE, GPIO_PIN_4 | GPIO_PIN_5);
	GPIOPinTypeGPIOInput(GPIOC_BASE, GPIO_PIN_4 | GPIO_PIN_5);
	GPIOPadConfigSet(GPIOC_BASE, GPIO_PIN_4 | GPIO_PIN_5, GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD_WPD);

	GPIOIntEnable(GPIOC_BASE, GPIO_INT_PIN_4 | GPIO_PIN_5);
	GPIOIntTypeSet(GPIOC_BASE, GPIO_PIN_5, GPIO_FALLING_EDGE);
	GPIOIntTypeSet(GPIOC_BASE, GPIO_PIN_4, GPIO_FALLING_EDGE);
	GPIOIntRegister(GPIOC_BASE, GPIOC_Handler);
	IntPrioritySet(INT_GPIOC, 0xA0);
}

// Limits Port Configuration
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
	GPIOIntRegister(GPIOE_BASE, GPIOE_Handler);
	IntPrioritySet(INT_GPIOE, 0xA0);
}

void delay(int ms)
{
	for (int i = 0; i < ms; i++)
	{
		for (int j = 0; j < 3180; j++)
			;
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
		vTaskSuspend(NULL);
	}
}

// Passanager Up Task
void p_motor_up_task(void *params)
{
	xSemaphoreTake(p_up_semaphore, 0);
	for (;;)
	{
		xSemaphoreTake(p_up_semaphore, portMAX_DELAY);
		xSemaphoreTake(mutex, portMAX_DELAY);
		// driver_flag = false;
		int up_limit;
		xQueuePeek(q_up_limit, &up_limit, 0);
		if (up_limit)
		{
			xSemaphoreGive(mutex);
		}
		else
		{
			motor_stop();
			motor_run(CLOCKWISE);
			// down_limit = false;
			int value = 0;
			xQueueOverwrite(q_down_limit, &value);
			delay(500);
			if (GPIOPinRead(GPIOD_BASE, GPIO_PIN_0))
			{
				// Manual
				while (GPIOPinRead(GPIOD_BASE, GPIO_PIN_0) && !up_limit && !driver_flag && !lock_flag && !jam_flag)
				{
					xQueuePeek(q_up_limit, &up_limit, 0);
				}
			}
			else
			{
				// Automatic
				// while (!up_limit && driver_flag )
				while (!up_limit && !driver_flag && !lock_flag && !jam_flag)
				{
					xQueuePeek(q_up_limit, &up_limit, 0);
					int down = GPIOPinRead(GPIOD_BASE, GPIO_PIN_1);
					if (down)
						break;
				}
			}
			motor_stop();
			jam_flag = false;
			xSemaphoreGive(mutex);
		}
	}
}

void p_motor_down_task(void *params)
{
	xSemaphoreTake(p_down_semaphore, 0);
	for (;;)
	{
		xSemaphoreTake(p_down_semaphore, portMAX_DELAY);
		xSemaphoreTake(mutex, portMAX_DELAY);

		// driver_flag = false;

		int down_limit;
		xQueuePeek(q_down_limit, &down_limit, 0);

		if (down_limit)
		{
			xSemaphoreGive(mutex);
		}
		else
		{
			motor_stop();
			motor_run(ANTICLOCKWISE);
			// up_limit = false;
			int value = 0;
			xQueueOverwrite(q_up_limit, &value);
			delay(500);
			if (GPIOPinRead(GPIOD_BASE, GPIO_PIN_1))
			{
				// Manual
				while (GPIOPinRead(GPIOD_BASE, GPIO_PIN_1) && !down_limit && !driver_flag && !lock_flag)
				{
					xQueuePeek(q_down_limit, &down_limit, 0);
				}
			}
			else
			{
				// Automatic
				// while (!down_limit && driver_flag)
				while (!down_limit && !driver_flag && !lock_flag)
				{
					xQueuePeek(q_down_limit, &down_limit, 0);
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

void d_motor_up_task(void *params)
{
	xSemaphoreTake(d_up_semaphore, 0);
	for (;;)
	{
		xSemaphoreTake(d_up_semaphore, portMAX_DELAY);
		xSemaphoreTake(mutex, portMAX_DELAY);
		int up_limit;
		xQueuePeek(q_up_limit, &up_limit, 0);
		if (up_limit)
		{
			driver_flag = false;
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
				}
			}
			else
			{
				// Automatic
				while (!up_limit && !jam_flag)
				{
					xQueuePeek(q_up_limit, &up_limit, 0);
					int down = GPIOPinRead(GPIOD_BASE, GPIO_PIN_3);
					if (down)
						break;
				}
			}
			motor_stop();
			driver_flag = false;
			jam_flag = false;
			xSemaphoreGive(mutex);
		}
	}
}

void d_motor_down_task(void *params)
{
	xSemaphoreTake(d_down_semaphore, 0);
	for (;;)
	{
		xSemaphoreTake(d_down_semaphore, portMAX_DELAY);
		xSemaphoreTake(mutex, portMAX_DELAY);

		int down_limit;
		xQueuePeek(q_down_limit, &down_limit, 0);

		if (down_limit)
		{
			driver_flag = false;
			xSemaphoreGive(mutex);
		}
		else
		{
			motor_stop();
			motor_run(ANTICLOCKWISE);
			// up_limit = false;
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
			driver_flag = false;
			xSemaphoreGive(mutex);
		}
	}
}

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

void idle(void *params)
{
	for (;;)
	{
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
	limit_init();
	motor_init();
	PortC_init();
	buttons_init();

	xTaskCreate(p_motor_down_task, "down", 240, NULL, 2, NULL);
	xTaskCreate(p_motor_up_task, "up", 240, NULL, 2, NULL);

	xTaskCreate(d_motor_down_task, "down", 240, NULL, 3, NULL);
	xTaskCreate(d_motor_up_task, "up", 240, NULL, 3, NULL);
	xTaskCreate(idle, "idle", 240, NULL, 1, NULL);
	xTaskCreate(jam, "jam", 240, NULL, 4, NULL);
	xTaskCreate(q_init, "q_init", 240, NULL, 4, NULL);

	vTaskStartScheduler();

	// The following line should never be reached.  Failure to allocate enough
	//	memory from the heap would be one reason.
	for (;;)
		;
}
