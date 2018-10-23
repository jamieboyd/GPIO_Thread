#include "SR_stepper_thread.h"


/* *************************** Functions that are called from thread *********************

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
	// init iterators
	taskData->iMotorAB =0;
	taskData->iMotor = nMotors -1;
	taskData->iTrain =0;
	delete initDataPtr;
	return 0; // 
	
}

/* ********************************* High function, runs at start of pulse ************************************
sets shift_reg_pin low, sets value of data pin for the motor we are working on, sets up counters for next value 
Last Modified:
2018/10/22 by Jamie Boyd - initial version */ 
void SR_stepper_Hi (void * taskDataP){
	// cast void pointer to taskdataPtr
	SR_StepperStructPtr taskData = (SR_StepperStructPtr) taskDataP;
	// set shift register pin lo
	*(taskData->GPIOperiLo ) = taskData->shift_reg_bit ;
	// set data pin high or low, getting data for motor and position from shiftData
	int dataBit = shiftData [(4 * motorDataPos [iMotor]) + iMotorAB];
	if (dataBit == 0){
		*(taskData->GPIOperiLo ) = taskData->data_bit;
	}else{
		*(taskData->GPIOperiLo ) = taskData->data_bit;
	}
	// increment position, iMotorAB
	taskData->iMotorAB +=1;
	// reset motorAB to 0 if we have done all 4 positions
	if (taskData->iMotorAB == 3){
		taskData->iMotorAB = 0;
		// if iTrain < number of trains requested for this motor, move to start of next set of 4 positions for this motor
		if (taskData->iTrain < taskData->nTrains[taskData->iMotor]){
			// handle wrap-around for positive direction
			if (([motorDataPos [iMotor] == 7) && (taskData->motorDir [taskData->iMotor] == 1)){
				[motorDataPos [iMotor] = 0;
			}else{
				// handle wrap-around for negative direction
				if (([motorDataPos [iMotor] == 0) && (taskData->motorDir [taskData->iMotor] == -1)){
					[motorDataPos [iMotor] =7;
				}else{
					// handle normal progression of +1 or -1, or not going anywhere, based on motorDir
					taskData->motorDataPos[taskData->iMotor] += taskData->motorDir [taskData->iMotor];
				}
			}
		}
		// decrement iMotor, as we have finished 4 positions for this motor
		taskData->iMotor -= 1;
		// if we have reached end of motors, reset to top of motors, increment iTrain
		if (taskData->iMotor < 0){
			taskData->iMotor = taskData->nMotors -1;
			taskData->iTrain +=1;
		}
	}
}

/* ******************************* Low Function ********************************************** 	
sets shift_reg pin HIGH. Sets storage resgister low on first pulse of train
Last Modified:
2018/10/22 by Jamie Boyd - initial version */
void SR_stepper_Lo (void *  taskDataP){
	// cast void pointer to taskdataPtr
	SR_StepperStructPtr taskData = (SR_StepperStructPtr) taskDataP;
	// set shift register pin HIGH
	*(taskData->GPIOperiHi ) = taskData->shift_reg_bit ;
	// on 1st pulse of train (it could be any pulse), set storage register pin LOW
	if ((taskData->iMotor == taskData->nMotors -1) && (taskData->iMotorAB == 0)) {
		*(taskData->GPIOperiLo ) = taskData->stor_reg_bit;
	}
}

/* ********************************** End function after each train **************************************
sets stor_reg pin high.  Needs no endFunc data, uses same taskData used by hi and lo funcs
Last Modified:
2018/10/22 by Jamie Boyd - initial version */
void SR_stepper_EndFunc (void * endFuncData, taskParams * theTask){
	SR_StepperStructPtr taskData = (SR_StepperStructPtr)theTask->taskData;
	*(taskData->GPIOperiHi ) = taskData->stor_reg_bit;
}

/* ********************************* SR_stepper_thread class methods *****************************************

*************************************** Static thread maker ***********************************************
returns a new SR_stepper_thread
Last Modified:
2018/10/23 by Jamie Boyd - initial version */
SR_stepper_thread * SR_stepper_thread::SR_stepper_threadMaker (int data_pinP, int shift_reg_pinP, int stor_reg_pinP, int nMotorsP, float steps_per_secP, int accuracyLevel) {
	// make and fill an init struct
	SR_StepperInitStructPtr  initStruct = new SR_StepperInitStruct;
	initStruct->data_Pin = data_pinP;
	initStruct->shift_reg_pin = shift_reg_pinP;
	initStruct->stor_reg_pin=stor_reg_pinP;
	initStruct->GPIOperiAddr = useGpioPeri ();
	if (initStruct->GPIOperiAddr == nullptr){
#if beVerbose
        printf ("SR_stepper_threadMaker failed to map GPIO peripheral.\n");
#endif
		return nullptr;
	}
	int errCode =0;
	unsigned int durUsecs = (unsigned int) (5e05/steps_per_secP);
	// call SR_stepper_thread constructor, which calls pulsedThread contructor
	SR_stepper_thread * newSR_stepper = new SR_stepper_thread (data_pinP, shift_reg_pinP, stor_reg_pinP, nMotorsP, durUsecs, (void *) initStruct, &SR_stepper_init, accuracyLevel, errCode);
	if (errCode){
#if beVerbose
		printf ("SimpleGPIO_threadMaker failed to make SimpleGPIO_thread.\n");
#endif
		return nullptr;
	}
	// set custom task delete function
	newSR_stepper->setTaskDataDelFunc (&SimpleGPIO_delTask);
	return newSR_stepper;
}
(int data_pinP, int shift_reg_pinP, int stor_reg_pinP, int nMotorsP, unsigned int durUsecs, void * initData, int accuracyLevel, int &errCode)