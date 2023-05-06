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

#define LIMIT_MAX 10000000
#define LIMIT_MIN 0
#define AUTO 1
#define MANUAL 2
int limit=0; //should be global variable in all modules

void delay(int ms)
{for(int k=0;k<3185;k++)
			for(int i=0;i<3185;i++)
		{
			for(int j=0;j<ms;j++);
		}
}



//Passenger variables common between up and down
xSemaphoreHandle Mutex_P;
bool downIsPressed = false;
bool UpIsPressed = false;
//Passenger Up
SemaphoreHandle_t Semaphore_P_Init_Up;
SemaphoreHandle_t Semaphore_P_Up;
QueueHandle_t xQueue_P_Up;
//Passenegr Down
SemaphoreHandle_t Semaphore_P_Init_Down;
SemaphoreHandle_t Semaphore_P_Down;
QueueHandle_t xQueue_P_Down;


/*
P_Init_Up and P_Up has two seperate binary semaphores with same ISR 
ISR is triggered by the button 
ISR gives both semaphores
P_Init_Up is higher priority than P_Up
*/


//InitPassengerUp
void P_Init_Up(void *pvParameters)
{
	xSemaphoreTake( Semaphore_P_Init_Up, 0 );
	for (;;) {
		xSemaphoreTake( Semaphore_P_Init_Up, portMAX_DELAY );
		
		delay(100);  //value can be edited
		int Queue_Value=MANUAL;
		//checks if button is still pressed after task delay
		if(GPIOPinRead(GPIOF_BASE,GPIO_PIN_0) == 0x01){
			vTaskDelay( pdMS_TO_TICKS(20));
				if(GPIOPinRead(GPIOF_BASE,GPIO_PIN_0) == 0x01){ //switch 1
					Queue_Value=AUTO;
				}
		}		
	xQueueSendToBack(xQueue_P_Up, &Queue_Value, 0);
	}
	
}

//manual PassengerUp & autoPassengerUp
void P_Up(void *pvParameters){
	int Queue_Value;
	xSemaphoreTake( Semaphore_P_Up, 0 );
	for(;;){
		xSemaphoreTake( Semaphore_P_Up, portMAX_DELAY );
		xQueueReceive(xQueue_P_Up, &Queue_Value, portMAX_DELAY);
		xSemaphoreTake( Mutex_P,portMAX_DELAY );
		{		
		if(limit < LIMIT_MAX){
			//motor_run(CLOCKWISE);//assuming clockwise is upwards
			motor_run(CLOCKWISE);
			//GPIOPinWrite(GPIOB_BASE,GPIO_PIN_0|GPIO_PIN_1,GPIO_PIN_1);
			}
		if(Queue_Value == AUTO){
			//hnsht8al automatic
			//while bt-check 3ala el limit
			while(limit < LIMIT_MAX && !downIsPressed){
			//limit is incremented
				limit++;
			}
			
		}
		else{
			//yb2a el value b 2
			//hn4t8al manual, tool ma hwa dayes 3ala el button -> increment el limit
			while((GPIOPinRead(GPIOF_BASE,GPIO_PIN_0) == 0x00) && limit < LIMIT_MAX){
				//limit is incremented
				limit++;
			}
			
		}
		//motorstop
		motor_stop();
		//give mutex
	}xSemaphoreGive( Mutex_P );
	}	
}

//InitPassengerDown
void P_Init_Down(void *pvParameters)
{
	xSemaphoreTake( Semaphore_P_Init_Down, 0 );
	for (;;) {
		xSemaphoreTake( Semaphore_P_Init_Down, portMAX_DELAY );
		
		vTaskDelay( pdMS_TO_TICKS(100));  //can be edited
		int Queue_Value=MANUAL;
		
		if((GPIOPinRead(GPIOF_BASE,GPIO_PIN_4) == 0x10)){
			vTaskDelay( pdMS_TO_TICKS(20));
				if((GPIOPinRead(GPIOF_BASE,GPIO_PIN_4) == 0x10)){ //switch 2
					Queue_Value=AUTO;
				}
		}		
	xQueueSendToBack(xQueue_P_Down, &Queue_Value, 0);
	}
}

//manual Passengerdown & autoPassengerdown
void P_Down(void *pvParameters){
	int Queue_Value;
	xSemaphoreTake( Semaphore_P_Down, 0 );
	for(;;){
		xSemaphoreTake( Semaphore_P_Down, portMAX_DELAY );
		xQueueReceive(xQueue_P_Down, &Queue_Value, portMAX_DELAY);
		xSemaphoreTake( Mutex_P, portMAX_DELAY );
		{		
		if(limit > LIMIT_MIN){
			//motor_run(AntiCLOCKWISE);//assuming Anticlockwise is downwards
			motor_run(ANTICLOCKWISE);
			}
		if(Queue_Value == 1){
			//hnsht8al automatic
			//while bt-check 3ala el limit
			while(limit > LIMIT_MIN && !UpIsPressed){
			//limit is decremented
				limit--;
			}
			
		}
		else{
			//yb2a el value b 2
			//hn4t8al manual
			//while 
			while((GPIOPinRead(GPIOF_BASE,GPIO_PIN_4) == 0x00) && limit > LIMIT_MIN){
				//limit is decremented
				limit--;
			}
			
		}
		//motorstop
		motor_stop();
		//give mutex
	}xSemaphoreGive( Mutex_P );
	}	
}


void GPIOF_Handler(void)
{
	portBASE_TYPE xHigherPriorityTaskWoken1 = pdFALSE; //hwa 3ady yeb2a one flag keda??
	portBASE_TYPE xHigherPriorityTaskWoken2 = pdFALSE;
	
	
	if((GPIOIntStatus(GPIOF_BASE,true) == 1<<0)){
		UpIsPressed=true;
	GPIOIntClear(GPIOF_BASE,GPIO_INT_PIN_0);
	xSemaphoreGiveFromISR(Semaphore_P_Init_Up,&xHigherPriorityTaskWoken1);
	xSemaphoreGiveFromISR(Semaphore_P_Up,&xHigherPriorityTaskWoken1);
	portEND_SWITCHING_ISR(xHigherPriorityTaskWoken1);
		
	}
	if((GPIOIntStatus(GPIOF_BASE,true) == 1<<4)){
		downIsPressed = true;
	GPIOIntClear(GPIOF_BASE,GPIO_INT_PIN_4);
	xSemaphoreGiveFromISR(Semaphore_P_Init_Down,&xHigherPriorityTaskWoken2);
	xSemaphoreGiveFromISR(Semaphore_P_Down,&xHigherPriorityTaskWoken2);
	portEND_SWITCHING_ISR(xHigherPriorityTaskWoken2);
	}
	
}

int main()
{
	IntMasterEnable();
	
	//Passenger semaphore creation
	vSemaphoreCreateBinary(Semaphore_P_Init_Up);
	vSemaphoreCreateBinary(Semaphore_P_Up);
	vSemaphoreCreateBinary(Semaphore_P_Init_Down);
	vSemaphoreCreateBinary(Semaphore_P_Down);
	
	//Passenger mutex creation
	Mutex_P = xSemaphoreCreateMutex();
	
	//Passenger queue creation
	xQueue_P_Up = xQueueCreate( 1, sizeof( int ) );
	xQueue_P_Down = xQueueCreate(1, sizeof( int ));
	
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
	

	//Passenger Up tasks
	xTaskCreate(P_Init_Up, "P_init_Up", 256, NULL, 2, NULL);
	xTaskCreate(P_Up, "P_Up", 256, NULL, 1, NULL);
	//Passenger Down tasks
	xTaskCreate(P_Init_Down, "P_Init_Down", 256, NULL, 2, NULL);
	xTaskCreate(P_Down, "P_Down", 256, NULL, 1, NULL);
	

	// Startup of the FreeRTOS scheduler.  The program should block here.  
	vTaskStartScheduler();
	
	// The following line should never be reached.  Failure to allocate enough
	//	memory from the heap would be one reason.
	for (;;);
	
}