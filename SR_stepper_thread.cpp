#include "SR_stepper_thread.h"


/* *************************** Functions that are called from thread *********************

***************************************Initialization callback function ****************************************

Last Modified:
2018/10/22 by Jamie Boyd - initial Version */
int SR_stepper_init (void * initDataP, void *  &taskDataP){
	// task data pointer is a void pointer that needs to be initialized to a pointer to taskData and filled from our custom init structure 
	SR_StepperStructPtr taskData  = new SR_StepperStruct;
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
	// zero steps and direction arrays
	for (int iMotor = 0; iMotor < taskData->nMotors; iMotor +=1){
		taskData->nSteps[iMotor] = 0;
		taskData->motorDir[iMotor] = 0;
	}
	taskData->iMotorAB =0;
	taskData-> iMotor = taskData->nMotors -1;
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
	int dataBit = taskData->shiftData [(4 * taskData->motorDataPos [taskData->iMotor]) + taskData->iMotorAB];
	if (dataBit){
		*(taskData->GPIOperiHi ) = taskData->data_bit;
	}else{
		*(taskData->GPIOperiLo) = taskData->data_bit;
	}
	// increment ABA/B/ position, iMotorAB
	taskData->iMotorAB +=1;
	// reset motorAB to 0 if we have done all 4 positions for this motor
	if (taskData->iMotorAB == 3){
		taskData->iMotorAB = 0;
		// are we still moving this motor? 
		if (taskData->nSteps[taskData->iMotor] != 0){
			// decrement needed steps for this motor, depending on direction
			taskData->nSteps[taskData->iMotor] -= taskData->motorDir [taskData->iMotor];
			// move to start of next set of 4 positions for this motor
			// handle wrap-around for positive direction
			if ((taskData->motorDataPos [taskData->iMotor] == 7) && (taskData->motorDir [taskData->iMotor] == 1)){
				taskData->motorDataPos [taskData->iMotor] = 0;
			}else{
				// handle wrap-around for negative direction
				if ((taskData->motorDataPos [taskData->iMotor] == 0) && (taskData->motorDir [taskData->iMotor] == -1)){
					taskData->motorDataPos [taskData->iMotor] =7;
				}else{
					// handle normal progression of +1 or -1, or not going anywhere, based on motorDir
					taskData->motorDataPos[taskData->iMotor] += taskData->motorDir [taskData->iMotor];
				}
			}
		}
		// decrement iMotor, as we have finished 4 positions for this motor
		taskData->iMotor -= 1;
		// if we have reached end of motors, reset to top of motors
		if (taskData->iMotor < 0){
			taskData->iMotor = taskData->nMotors -1;
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


/* ****************************** Custom delete Function ****************************************************
deletes taskData
Last Modified:
2018/10/23 by jamie Boyd - Initial verison */
void SR_stepper_delTask (void * taskData){
	SR_StepperStructPtr taskPtr = (SR_StepperStructPtr) taskData;
	delete (taskPtr);
}


/* ********************************* SR_stepper_thread class methods *****************************************

*************************************** Static thread maker ***********************************************
returns a new SR_stepper_thread
Last Modified:
2018/10/23 by Jamie Boyd - initial version */
SR_stepper_thread * SR_stepper_thread::SR_stepper_threadMaker (int data_pinP, int shift_reg_pinP, int stor_reg_pinP, int nMotorsP, float steps_per_secP, int accuracyLevel) {
	// make and fill an init struct
	SR_StepperInitStructPtr  initStruct = new SR_StepperInitStruct;
	initStruct->data_pin = data_pinP;
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
	unsigned int durUsecs = (unsigned int) (5e05/(steps_per_secP * nMotorsP * 4));
	unsigned int nPulses = nMotorsP * 4;
	// call SR_stepper_thread constructor, which calls pulsedThread contructor
	SR_stepper_thread * newSR_stepper = new SR_stepper_thread (durUsecs, nPulses, (void *) initStruct, accuracyLevel, errCode);
	if (errCode){
#if beVerbose
		printf ("SimpleGPIO_threadMaker failed to make SimpleGPIO_thread.\n");
#endif
		return nullptr;
	}
	// set custom task delete function
	newSR_stepper->setTaskDataDelFunc (&SR_stepper_delTask);
	// Set the SR_StepperStructPtr  used for easy direct access to thread task data 
	newSR_stepper->taskPtr = (SR_StepperStructPtr)newSR_stepper->getTaskData ();
	// set the end function (it does not use any any special data, so no need to set endFunc data)
	newSR_stepper->setEndFunc (&SR_stepper_EndFunc);
	return newSR_stepper;
}

/* ***************************************** move function for all stepper motors **********************************
values in array of distances are steps relative to current position, negative values for negative distances
Last Modified:
2018/10/23 by Jamie Boyd - initial version */
void SR_stepper_thread::moveSteps (int mDists [MAX_MOTORS]){
	// need maximum steps of all motors to get number of trains to request
	// also need to set counter variables in thread data
	this->getTaskMutex(); // get taskMutex - can, in theroy, be calling move with trains left to do
	int trainsInHand = this->isBusy();
	int neededTrains =0;
	for (int iMotor = 0; iMotor < this->taskPtr->nMotors; iMotor +=1){
		if (mDists [iMotor] > 0){
			this->taskPtr->motorDir [iMotor] = 1;
			this->taskPtr->nSteps [iMotor] += mDists [iMotor] ;
		}else{
			if (mDists [iMotor] < 0){
				this->taskPtr->motorDir [iMotor] = -1;
				this->taskPtr->nSteps [iMotor] += mDists [iMotor] ;
			}
		}
		neededTrains = neededTrains >= abs (this->taskPtr->nSteps [iMotor] ) ? neededTrains : abs (this->taskPtr->nSteps [iMotor] ) ;
		//neededTrains = max  (neededTrains, abs (this->taskPtr->nSteps [iMotor] )) ;
	}
	this->giveUpTaskMutex(); // give up taskMutex
	this->DoTasks (neededTrains - trainsInHand); // call doTasks with required number of trains
}


/* ********************************************** sets stepper motors free to move *********************************
mFree is an array where a 1 means unhold the motor by setting all 4 outputs to 0, and a 0 means to leave the motor as it is
not done in a threaded fashion, just done as class method using pulsedThread's sleep method 1 style, no great need for accuracy
Last modified:
2018/10/23 by Jamie Boyd - Initial Version */
int SR_stepper_thread::Free (int mFree [MAX_MOTORS]){
	// wait until we are no longer moving
	if (this->waitOnBusy(10)){
#if beVerbose
		printf (" waited for 10 seconds and thread was still busy, so could not set Free state.\n");
#endif
		return 1;
	}
	// get pulse timeing
	int pulseDurUsecs = this->getpulseDurUsecs ();
	// make a timespec for sleeping
	struct timespec sleeper; // used to sleep between ticks of the clock
	sleeper.tv_sec = pulseDurUsecs/1e06;
	sleeper.tv_nsec = (pulseDurUsecs - (sleeper.tv_sec * 1e06))* 1e03; 
	// lock the thread. 
	this->getTaskMutex(); // lock the thread, but thread should not be active
	// set storage register pin lo
	*(this->taskPtr->GPIOperiLo ) = this->taskPtr->stor_reg_bit ;
	// loop through A,B,A/,B/ for all motors
	int iMotorAB =0;
	int iMotor;
	int dataBit;
	for (iMotor = this->taskPtr->nMotors -1; iMotor >= 0; iMotor -=1){
		if (mFree [iMotor]){ // zero this motor to free it
			for (iMotorAB =0; iMotorAB < 4; iMotorAB += 1){
				// set shift register pin low
				*(this->taskPtr->GPIOperiLo ) = this->taskPtr->shift_reg_bit ;
				// set data pin low
				*(this->taskPtr->GPIOperiLo ) = this->taskPtr->data_bit ;
				// sleep for a bit
				nanosleep (&sleeper, NULL);
				// set shift register pin high
				*(this->taskPtr->GPIOperiHi) = this->taskPtr->shift_reg_bit ;
				// sleep for a bit
				nanosleep (&sleeper, NULL);
			}
		}else{ // leave this motor how it is by outputting current data
			for  (iMotorAB =0; iMotorAB < 4; iMotorAB += 1){
				// set shift register pin low
				*(this->taskPtr->GPIOperiLo ) = this->taskPtr->shift_reg_bit ;
				// set data pin
				dataBit = this->taskPtr->shiftData [(4 * this->taskPtr->motorDataPos [iMotor]) + iMotorAB];
				if (dataBit){
					*(this->taskPtr->GPIOperiHi) = this->taskPtr->data_bit ;
				}else{
					*(this->taskPtr->GPIOperiLo ) = this->taskPtr->data_bit ;
				}
				// sleep for a bit
				nanosleep (&sleeper, NULL);
				// set shift register pin hi
				*(this->taskPtr->GPIOperiHi) = this->taskPtr->shift_reg_bit ;
				// sleep for a bit
				nanosleep (&sleeper, NULL);
			}
		}
	}
	// set storage register pin High
	*(this->taskPtr->GPIOperiHi ) = this->taskPtr->stor_reg_bit ;
	// unlock the thread
	this->giveUpTaskMutex(); // give up taskMutex
	return 0;
}

/* *********************************************** after freeing, this function holds motors steady in place *************************
mHolds is an array where a 1 means to hold the motor firm by setting to position 7 where both coils are energized, and 0 is to leave the motor as is
2018/10/23 by Jamie Boyd - Initial Version */
int SR_stepper_thread::Hold (int mHold [MAX_MOTORS]){
	// wait until we are no longer moving
	if (this->waitOnBusy(10)){
#if beVerbose
		printf (" waited for 10 seconds and thread was still busy, so could not set Hold state.\n");
#endif
		return 1;
	}
	// get pulse timeing
	int pulseDurUsecs = this->getpulseDurUsecs ();
	// make a timespec for sleeping
	struct timespec sleeper; // used to sleep between ticks of the clock
	sleeper.tv_sec = pulseDurUsecs/1e06;
	sleeper.tv_nsec = (pulseDurUsecs - (sleeper.tv_sec * 1e06))* 1e03; 
	// lock the thread. 
	this->getTaskMutex(); // lock the thread, but thread should not be active
	// set storage register pin lo
	*(this->taskPtr->GPIOperiLo ) = this->taskPtr->stor_reg_bit ;
	// loop through A,B,A/,B/ for all motors
	int iMotorAB =0;
	int iMotor;
	int dataBit; 
	for (iMotor = this->taskPtr->nMotors -1; iMotor >= 0; iMotor -=1){
		if (mHold [iMotor]){ // set this motor to position 7 in the array of A,B,A/,B/ positions
			this->taskPtr->motorDataPos [iMotor] =7;
		}
		for  (iMotorAB =0; iMotorAB < 4; iMotorAB += 1){
			// set shift register pin low
			*(this->taskPtr->GPIOperiLo ) = this->taskPtr->shift_reg_bit ;
			// set data pin
			dataBit = this->taskPtr->shiftData [(4 * this->taskPtr->motorDataPos [iMotor]) + iMotorAB];
			if (dataBit){
				*(this->taskPtr->GPIOperiHi) = this->taskPtr->data_bit ;
			}else{
				*(this->taskPtr->GPIOperiLo ) = this->taskPtr->data_bit ;
			}
			// sleep for a bit
			nanosleep (&sleeper, NULL);
			// set shift register pin hi
			*(this->taskPtr->GPIOperiHi) = this->taskPtr->shift_reg_bit ;
			// sleep for a bit
			nanosleep (&sleeper, NULL);
		}
	}
	// set storage register pin High
	*(this->taskPtr->GPIOperiHi ) = this->taskPtr->stor_reg_bit ;
	// unlock the thread
	this->giveUpTaskMutex(); // give up taskMutex
	return 0;
}


/* ********************************************* Emergency Stop ***********************************************
calls unDoTasks and sets nSteps to 0 for all motors
Last Modified:
2018/10/23 by Jamie Boyd - initial Version */
int SR_stepper_thread::emergStop (){
	this->UnDoTasks (); 
	if (this->waitOnBusy(10)){
#if beVerbose
		printf (" waited for 10 seconds and thread was still busy, emergeStop not successful.\n");
#endif
		return 1;
	}
	// lock the thread. 
	this->getTaskMutex(); // lock the thread, but thread should not be active
	// zero steps and direction arrays
	for (int iMotor = 0; iMotor < this->taskPtr->nMotors; iMotor +=1){
		this->taskPtr->nSteps[iMotor] = 0;
		this->taskPtr->motorDir[iMotor] = 0;
	}
	this->taskPtr->iMotorAB =0;
	this->taskPtr->iMotor = this->taskPtr->nMotors -1;
	this->giveUpTaskMutex(); // give up taskMutex
	return 0;
}

/* **************************** Setters and Getters ********************************************

*************************** gets motor speed in steps per second *******************************
All motors move at same number of steps/second; train pulse time decreases with increasing number of motors
Last Modified:
2018/10/23 by Jamie Boyd - initial Version */
float SR_stepper_thread::getStepsPerSec (){
	return (float)this->getTrainFrequency()/((4 * this->taskPtr->nMotors));
}

/* ******************************* sets motor speed in steps per second *******************
Last Modified:
2018/10/23 by Jamie Boyd - initial Version */
int SR_stepper_thread::setStepsPerSec (float stepsPerSec){
	return this->modFreq (stepsPerSec * 4 * this->taskPtr->nMotors);
}

/* ****************************** Destructor handles GPIO peripheral mapping*************************
Thread data is destroyed by the pulsedThread destructor. All we need to do here is take care of GPIO peripheral mapping
Last Modified:
2018/10/023 by Jamie Boyd - Initial Version */
SR_stepper_thread::~SR_stepper_thread (){
	unUseGPIOperi();
}
