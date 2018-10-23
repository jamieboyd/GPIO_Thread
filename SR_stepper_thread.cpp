#include "SR_stepper_thread.h"


/* *************************** Functions that are called from thread can not be class methods *********************

***************************************Initialization callback function ****************************************

Last Modified:
2018/10/22 by Jamie Boyd - initial Version */

int SR_stepper_init (void * initDataP, void *  &taskDataP){
	// task data pointer is a void pointer that needs to be initialized to a pointer to taskData and filled from our custom init structure 
	SR_stepperStructPtr taskData  = new SR_stepperStruct;
	taskDataP = taskData;
	// initData is a pointer to our custom init structure
	SR_StepperInitStructPtr initDataPtr = (SR_StepperInitStructPtr) initDataP;
	// copy nMotors from init
	taskData->nMotors = initDataPtr->nMotors;
	// stuff copied and modified from SimpleGPIO
	// calculate address to ON and OFF register as Hi or Lo as appropriate to save a level of indirection later
	taskData->GPIOperiHi = (unsigned int *) (initDataPtr->GPIOperiAddr + 7);
	taskData->GPIOperiLo = (unsigned int *) (initDataPtr->GPIOperiAddr + 10);
	// calculate pin Bits
	taskData->data_bit =  1 << initDataPtr->data_pin;
	taskData->shift_reg_bit = 1 << initDataPtr->shift_reg_pin;
	taskData->stor_reg_bit = 1 << initDataPtr->stor_reg_pin;
	// initialize pins for output
	*(initDataPtr->GPIOperiAddr + ((initDataPtr->data_pin) /10)) &= ~(7<<(((initDataPtr->data_pin) %10)*3));
	*(initDataPtr->GPIOperiAddr + ((initDataPtr->data_pin)/10)) |=  (1<<(((initDataPtr->data_pin)%10)*3));
	*(initDataPtr->GPIOperiAddr + ((initDataPtr->shift_reg_pin) /10)) &= ~(7<<(((initDataPtr->shift_reg_pin) %10)*3));
	*(initDataPtr->GPIOperiAddr + ((initDataPtr->shift_reg_pin)/10)) |=  (1<<(((initDataPtr->shift_reg_pin)%10)*3));
	*(initDataPtr->GPIOperiAddr + ((initDataPtr->stor_reg_pin) /10)) &= ~(7<<(((initDataPtr->stor_reg_pin) %10)*3));
	*(initDataPtr->GPIOperiAddr + ((initDataPtr->stor_reg_pin)/10)) |=  (1<<(((initDataPtr->stor_reg_pin)%10)*3));
	// put pins in selected start state, shift_reg starts high, stor_reg starts low, data doesn't matter
	*(taskData->GPIOperiHi ) = taskData->shift_reg_bit ;
	*(taskData->GPIOperiLo ) = taskData->stor_reg_bit ;
	delete initDataPtr;
	return 0; // 
	
}

/* ********************************* High function, runs at start of pulse ************************************
sets shift_reg_pin low, sets next value of data pin for the motor we are working on, on first train sets stor_reg low */ 
void SR_stepper_Hi (void *  taskData){
	*(taskData->GPIOperiLo ) = taskData->shift_reg_bit ;
	
}

/* ******************************* Low Function ********************************************** 	
sets shift_reg pin HIGH */
void SR_stepper_Lo (void *  taskData){
	*(taskData->GPIOperiHi ) = taskData->shift_reg_bit ;
}

/* ********************************** end function after each train **************************************
sets stor_reg high, increments counters for motors,*/
void SR_stepper_End (void * endFuncData, taskParams * theTask){
}
		
	
SR_stepper_thread (int data_pin, int shift_reg_pin, int stor_reg_pin, int nMotors, float meters_per_step, float meters_per_sint accuracyLevel) : 
