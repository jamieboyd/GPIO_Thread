#include "StepperMotor_thread.h"


/******************************************************************************************
******************Non-Class functions passed to the pulsedThread **********************
Initializes a Stepper motor. InitData is an array of 4 pins (A, B, A\, and B\) and stepper mode.
We init all pins low (half step motor free to spin) and init initial direction is 0
last Modified:
2016/12/12 by Jamie Boyd - initial version
2017/03/04 by Jamie Boyd - replacing wiringPi
*/
int StepperMotor_Init (void * initDataP, void *  volatile &taskDataP){
	// make sure GPIO peripheral is mapped
	int errCode;
	if (GPIOperi == nullptr){
		GPIOperi = new bcm_peripheral {GPIO_BASE};
		errCode = map_peripheral(GPIOperi, IFACE_DEV_GPIOMEM);
		if (errCode){
			return 1;
		}
	}
	// initData is a pointer to our init structure
	StepperMotorInitPtr initData = (StepperMotorInitPtr)initDataP;
	// task data is a pointer to taskCustomData that needs to be initialized with our custom structure
	taskDataP = (StepperMotorPtr) new StepperMotorStruct;
	// 2 pins for full steps or 4 pins for half steps ?
	int nPins;
	if (initData->stepperMode == kSTEPPERMOTORHALF){
		taskDataP->stepperMode = kSTEPPERMOTORHALF;
		nPins =4;
		taskDataP->curPos = -1;  // all 0 leaves motor free to spin for half step
	}else{
		taskDataP->stepperMode = kSTEPPERMOTORFULL;
		nPins = 2;
		taskDataP->curPos =2; // all 0 corresponds to position 2 for full step
	}
	// copy GPIO base address to taskData
	taskDataP->GPIOaddr = initData->GPIOaddr;
	for (int iPin =0; iPin < nPins; iPin +=1){
		// calculate pinBit
		taskDataP->pinBits[iPin] =  1 << initData->pins[iPin];
		// initialize pin for output and set it low
		*(taskDataP->GPIOaddr + ((taskDataP->pinBits[iPin]) /10)) &= ~(7<<(((taskDataP->pinBits[iPin]) %10)*3));
		*(taskDataP->GPIOaddr  + ((taskDataP->pinBits[iBit])/10)) |=  (1<<(((taskDataP->pinBits[iPin])%10)*3));
		*(taskDataP->GPIOaddr + 10) =taskDataP->pinBits[iPin];
	}
	taskDataP ->curDir =0;
	return 0;
}

/************************************************************************************
callback functions for FullStep. Move motor forward or backward one step.
last Modified:
2016/12/12 by Jamie Boyd - initial version
2016/12/27 by Jamie Boyd - modified functions for forwards and reverse directions
*/
void StepperMotor_Full_forward (void * volatile taskDataP){
	StepperMotorPtr taskData = (StepperMotorPtr)taskDataP;
	switch (taskData->curPos){
		case 0:
			*(taskData->GPIOaddr + 10) =taskData->pinBits[0];
			//digitalWrite (taskData->pins[0], LOW);
			taskData->curPos = 1;
			break;
		case 1:
			*(taskData->GPIOaddr + 10) =taskData->pinBits[1];
			//digitalWrite (taskData->pins[1], LOW);
			taskData->curPos = 2;
			break;
		case 2:
			*(taskData->GPIOaddr + 7) =taskData->pinBits[0];
			//digitalWrite (taskData->pins[0], HIGH);
			taskData->curPos = 3;
			break;
		case 3:
			*(taskData->GPIOaddr + 7) =taskData->pinBits[1];
			//digitalWrite (taskData->pins[1], HIGH);
			taskData->curPos = 0;
			break;
		}
	}
	
void StepperMotor_Full_backward (void * volatile taskDataP){
	StepperMotorPtr taskData = (StepperMotorPtr)taskDataP;
	switch (taskData->curPos){
		case 0:
			*(taskData->GPIOaddr + 10) =taskData->pinBits[1];
			//digitalWrite (taskData->pins[1], LOW);
			taskData->curPos = 3;
			break;
		case 1:
			*(taskData->GPIOaddr + 10) =taskData->pinBits[0];
			//digitalWrite (taskData->pins[0], LOW);
			taskData->curPos = 0;
			break;
		case 2:
			*(taskData->GPIOaddr + 7) =taskData->pinBits[1];
			//digitalWrite (taskData->pins[1], HIGH);
			taskData->curPos = 1;
			break;
		case 3:
			*(taskData->GPIOaddr + 7) =taskData->pinBits[0];
			//digitalWrite (taskData->pins[0], HIGH);
			taskData->curPos = 2;
			break;
	}
}

/************************************************************************************
callback functions for HalfStep. Moves motor forward or backward half a step.
last Modified:
2016/12/12 by Jamie Boyd - initial version
2016/12/27 by Jamie Boyd - modified functions for forwards and reverse directions
*/
void StepperMotor_Half_forward (void * volatile taskDataP){
	StepperMotorPtr taskData = (StepperMotorPtr)taskDataP;
	switch (taskData->curPos){
		case -1: // starting with everything turned off, i.e., in unhold state
			*(taskData->GPIOaddr + 7) =taskData->pinBits[0];
			//digitalWrite (taskData->pins[0], HIGH);
			taskData->curPos = 0;
			break;
		case 0:
			*(taskData->GPIOaddr + 7) =taskData->pinBits[1];
			//digitalWrite (taskData->pins[1], HIGH);
			taskData->curPos = 1;
			break;
		case 1:
			*(taskData->GPIOaddr + 10) =taskData->pinBits[0];
			//digitalWrite (taskData->pins[0], LOW);
			taskData->curPos = 2;
			break;
		case 2:
			*(taskData->GPIOaddr + 7) =taskData->pinBits[2];
			//digitalWrite (taskData->pins[2], HIGH);
			taskData->curPos = 3;
			break;
		case 3:
			*(taskData->GPIOaddr + 10) =taskData->pinBits[1];
			//digitalWrite (taskData->pins[1], LOW);
			taskData->curPos = 4;
			break;
		case 4:
			*(taskData->GPIOaddr + 7) =taskData->pinBits[3];
			//digitalWrite (taskData->pins[3], HIGH);
			taskData->curPos = 5;
			break;
		case 5:
			*(taskData->GPIOaddr + 10) =taskData->pinBits[2];
			//digitalWrite (taskData->pins[2], LOW);
			taskData->curPos = 6;
			break;
		case 6:
			*(taskData->GPIOaddr + 7) =taskData->pinBits[0];
			//digitalWrite (taskData->pins[0], HIGH);
			taskData->curPos = 7;
			break;
		case 7:
			*(taskData->GPIOaddr + 10) =taskData->pinBits[3];
			//digitalWrite (taskData->pins[3], LOW);
			taskData->curPos= 0;
			break;
	}
}

void StepperMotor_Half_backward (void * volatile taskDataP){
	StepperMotorPtr taskData = (StepperMotorPtr)taskDataP;
	switch (taskData->curPos){
		case -1: // starting with everything turned off, i.e., in unhold state
			*(taskData->GPIOaddr + 7) =taskData->pinBits[0];
			//digitalWrite (taskData->pins[0], HIGH);
			taskData->curPos = 0;
			break;
		case 0:
			*(taskData->GPIOaddr + 7) =taskData->pinBits[3];
			//digitalWrite (taskData->pins[3], HIGH);
			taskData->curPos = 7;
			break;
		case 1:
			*(taskData->GPIOaddr + 10) =taskData->pinBits[1];
			//digitalWrite (taskData->pins[1], LOW);
			taskData->curPos= 0;
			break;
		case 2:
			*(taskData->GPIOaddr + 7) =taskData->pinBits[0];
			//digitalWrite (taskData->pins[0], HIGH);
			taskData->curPos= 1;
			break;
		case 3:
			*(taskData->GPIOaddr + 10) =taskData->pinBits[2];
			//digitalWrite (taskData->pins[2], LOW);
			taskData->curPos= 2;
			break;
		case 4:
			*(taskData->GPIOaddr + 7) =taskData->pinBits[1];
			//digitalWrite (taskData->pins[1], HIGH);
			taskData->curPos= 3;
			break;
		case 5:
			*(taskData->GPIOaddr + 10) =taskData->pinBits[3];
			//digitalWrite (taskData->pins[3], LOW);
			taskData->curPos = 4;
			break;
		case 6:
			*(taskData->GPIOaddr + 7) =taskData->pinBits[2];
			//digitalWrite (taskData->pins[2], HIGH);
			taskData->curPos= 5;
			break;
		case 7:
			*(taskData->GPIOaddr + 10) =taskData->pinBits[0];
			//digitalWrite (taskData->pins[0], LOW);
			taskData->curPos= 6;
			break;
	}
}

/*********************************************************************************
Callback function to request a movement from a stepper motor 
modData is pointer to an integer for requested number and direction of steps 
positive values go forward, negative go back 
You want to install this function with modCustom with isLocking set, as it changes theTask
Last Modified:
2016/12/30 by Jamie Boyd - initial version, incorporates and supercedes setDirection callback */
int StepperMotor_setMoveCallback (void * modData, taskParams * theTask){
	// modData is a pointer to an integer, number and direction of steps
	int * moveRequest= (int *)modData;
	unsigned int moveSteps;
	int moveDir;
	if (*moveRequest < 0){
		moveDir = kSTEPPERREVERSE;
		moveSteps = *moveRequest * -1;
	}else{
		moveDir = kSTEPPERFORWARD;
		moveSteps= * moveRequest;
	}
	// get pointer to taskData 
	StepperMotorPtr taskData = (StepperMotorPtr)theTask->taskCustomData;
	// set stepper function, if needed, and set current direction for next time
	if ((moveDir ==kSTEPPERFORWARD ) && (taskData->curDir  != kSTEPPERFORWARD)){
		taskData->curDir = kSTEPPERFORWARD;
		if (taskData->stepperMode == kSTEPPERMOTORFULL){
			theTask->hiFunc = &StepperMotor_Full_forward;
		}else{
			theTask->hiFunc = &StepperMotor_Half_forward;
		}
	}else{
		if ((moveDir ==kSTEPPERREVERSE) && (taskData->curDir != kSTEPPERREVERSE)){
			taskData->curDir = kSTEPPERREVERSE;
			if (taskData->stepperMode == kSTEPPERMOTORFULL){
				theTask->hiFunc = &StepperMotor_Full_backward;
			}else{
				theTask->hiFunc = &StepperMotor_Half_backward;
			}
		}
	}
	// set theTask variables nPulses and increment doTask
	theTask->nPulses = moveSteps;
	theTask->doTask += 1;
	return 0;
}


/***********************************************************************************************
Callback to relax a half step stepper motor, because all 4 lines are controlled individually, no 
TTL inverter used, we can relax the tension on it by setting all lines low, and user can manually 
rotate it. Mod data is a pointer to an integer, 0 to free the stepper motor, 1 to unfree the motor - set to pos 1 with both phases active
returns 0 if no error, calling _setHold on a fullStep motor is flagged as an error
last modified:
2016/12/12 by Jamie Boyd - initial version
*/
int StepperMotorHalf_setHoldCallBack (void * modData, taskParams *  theTask){
	int * setHold = (int *)modData;
	StepperMotorPtr taskData = (StepperMotorPtr)theTask->taskCustomData;
	if (taskData->stepperMode == kSTEPPERMOTORFULL){
		return 1;
	}else{
		if ((*setHold== 0) && (taskData->curPos > -1)){ // changing from held to free
			for (int iPin=0; iPin < 4; iPin++){
				*(taskData->GPIOaddr + 10) =taskData->pinBits[iPin];
				//digitalWrite (taskData->pins[iPin], LOW);
			}
			taskData->curPos = -1;
		}else{
			if ((*setHold == 1) && (taskData->curPos == -1)){ // changing from free to held
				*(taskData->GPIOaddr + 7) =taskData->pinBits[0];
				*(taskData->GPIOaddr + 7) =taskData->pinBits[1];
				//digitalWrite (taskData->pins[0], HIGH);
				//digitalWrite (taskData->pins[1], HIGH);
				taskData->curPos = 1;
			}
		}
		return 0;
	}
}


/******************************************************************************************
*************************** pulsed Thread Stepper Motor Class ****************************/
// Constructor makes fullStep or halfStep stepper motor
ptStepperMotor::ptStepperMotor(int pinA, int pinB, int pinAbar, int pinBbar, int mode, float p_scaling, float p_speed, int accuracyLevel){
	// set scaling
	scaling = p_scaling;	// steps/unit (calibrated for whatever your unit is in, metres, degrees)
	speed = p_speed;	// units/second
	float frequency = p_scaling * p_speed;	// number of steps/second needed to acheive desired speed
	position =0;		// in units (can be positive or negative, istart at 0)
	// make and fill a stepper motor init struct as required by mode
	StepperMotorInitStruct initData;
	initData.stepperMode = mode;
	initData.pins [0]=pinA;
	initData.pins [1]=pinB;
	if (mode == kSTEPPERMOTORHALF){
		initData.pins [2]=pinAbar;
		initData.pins [3]=pinBbar;
	}
	
	/*make the thread - delay =0 as we don't need separate hi/lo, duration = calculated step time, number of steps =2 -just so we know it is a train
	leave hiFunc and loFunc as nullptr for now, they will be set before first move */ 
	unsigned int stepTime = (unsigned int) (1e06/frequency); // step time is in microseconds
	int errVar;
	stepperThread = new pulsedThread ((unsigned int)0, stepTime, (unsigned int)2, (void * volatile) &initData, &StepperMotor_Init, nullptr, nullptr, accuracyLevel, errVar);
	if (errVar != 0){
		printf ("Thread creation error: %d Don't use the returned thread pointer, it will be null\n", errVar);
	}
}


ptStepperMotor::~ptStepperMotor(){
	stepperThread->~pulsedThread();
}

void ptStepperMotor::moveAbs (float position_p){
	int * newStepsPtr = new int;
	* newStepsPtr = (int)(scaling * (position_p - position));
	stepperThread->modCustom (&StepperMotor_setMoveCallback, (void *) newStepsPtr, 1);
	position = position_p;
}

	
void ptStepperMotor::moveRel (float distance){
	// calculate steps to travel
	int * newStepsPtr = new int;
	* newStepsPtr = (int)(scaling * distance);
	stepperThread->modCustom (&StepperMotor_setMoveCallback, (void *) newStepsPtr, 1);
	position += distance;
}


void ptStepperMotor::setZero (){
	position =0;
}

void ptStepperMotor::setScaling (float newScaling){
	scaling = newScaling;
	float frequency = scaling * speed;
	stepperThread-> modDur ((unsigned int) (1e06/frequency));
}

void ptStepperMotor::setSpeed (float newSpeed){
	speed = newSpeed;
	float frequency = scaling * speed;
	stepperThread-> modDur ((unsigned int) (1e06/frequency));
}


float ptStepperMotor::getScaling (){
	return scaling;
}

float ptStepperMotor::getSpeed (){
	return speed;
}
float ptStepperMotor::getPosition (){
	return position;
}

int ptStepperMotor::isBusy(){
	return stepperThread->isBusy();
}

int ptStepperMotor::waitOnBusy(float timeOut){
	return stepperThread->waitOnBusy(timeOut);
}

/* calls modCustom to set or unset holding on a half-step stepper motor */
void ptStepperMotor::setHold (int hold){
	int * setHoldPtr = new int;
	* setHoldPtr = hold;
	stepperThread -> modCustom (&StepperMotorHalf_setHoldCallBack, (void *) setHoldPtr, 1);
}

