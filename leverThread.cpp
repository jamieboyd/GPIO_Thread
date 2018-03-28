
#include "leverThread.h"

int lever_init (void * initDataP, void *  &taskDataP){
	// task data pointer is a void pointer that needs to be initialized to a pointer to taskData and filled from our custom init structure 
	leverThreadStructPtr taskData  = new leverThreadStruct;
	taskDataP = taskData;
	// initData is a pointer to our custom init structure
	leverThreadInitStructPtr initDataPtr = (leverThreadInitStructPtr) initDataP;
	
	// ensure wiringpi is setup for GPIO
	wiringPiSetupGpio();
	// initialize the DAC - further calls to wiringPi DO use the file descriptor, so save i2c file descriptor
	taskData->i2c_fd = wiringPiI2CSetup(kDAC_ADDRESS);
	if (taskData->i2c_fd <= 0){
		return 1;
	}
	
	// initialize quad decoder - it returns spi file descriptor, but further calls to wiringPi don't use it
	if (wiringPiSPISetup(kQD_CS_LINE, kQD_CLOCK_FREQ) == -1){
		return 2;
	}
	// initialize the quadrature decoder
	// clear status
	taskData->spi_wpData[0] = kQD_CLEAR_STATUS;
	wiringPiSPIDataRW(kQD_CS_LINE, taskData->spi_wpData, 1);
	// clear counter
	taskData->spi_wpData[0] = kQD_CLEAR_COUNTER;
	wiringPiSPIDataRW(kQD_CS_LINE, taskData->spi_wpData, 1);
	// set write mode to 4x (highest resolution) and 1 byteMode (e.g., 1 byte = 0-255 before counter rolls over)
	taskData->spi_wpData[0] = kQD_WRITE_MODE0;
	taskData->spi_wpData[1] = kQD_FOURX_COUNT;
	wiringPiSPIDataRW (kQD_CS_LINE, taskData->spi_wpData, 2);
	taskData->spi_wpData [0] = kQD_WRITE_MODE1;
	taskData->spi_wpData [1] =kQD_ONEBYTE_COUNTER;
	wiringPiSPIDataRW (kQD_CS_LINE, taskData->spi_wpData, 2);
	
	// make a cuer
	if (initDataPtr->goalCuerPin > 0){
		if (initDataPtr->cuerFreq ==0){
			taskData->goalCuer = SimpleGPIO_thread::SimpleGPIO_threadMaker (initDataPtr->goalCuerPin, 1, (float) 100, (float) 0.5, (float) 0.1, 1);
			taskData->goalMode = 0;
		}else{
			taskData->goalCuer = SimpleGPIO_thread::SimpleGPIO_threadMaker (initDataPtr->goalCuerPin, 1, (float) initDataPtr->cuerFreq , (float) 0.5, (float) 0, 1);
			taskData->goalMode = 1;
		}
	}else{
		taskData->goalCuer  = nullptr;
	}

	// copy pointer to lever position buffer
	taskData->positionData = initDataPtr->positionData;
	taskData->nPositionData = initDataPtr->nPositionData;
	taskData->nCircular = initDataPtr->nCircular;
	// init force data
	taskData->nForceData = initDataPtr->nForceData;
	taskData->forceData = new int [initDataPtr->nForceData];
	// initialize iPosition to 0 - other initialization?
	taskData->iPosition=0;
	taskData->forceStartPos = initDataPtr->nPositionData; // no force will be applied cause we never get to here
	
	// initialize values for testing, remember to reset
	taskData->goalBottom =20;
	taskData->goalTop = 250;
	taskData->nHoldTicks = 100;
	taskData->constForce=1000;
	printf ("Initing lever pos data\n");

	return 0;
}


/* ***************************************High function - we don't have a low function ****************************************
Runs as an infinite train when mouse is present, filling circular buffer till we pass threshold
Or run as a cued trial with a train of length nPositionData, you do the cue and start the train
*/

void lever_Hi (void * taskData){
	// cast task data to  leverStruct
	leverThreadStructPtr leverTaskPtr = (leverThreadStructPtr) taskData;
	// read quadrature decoder into position data, so we can get lever position always
	leverTaskPtr->spi_wpData[0] = kQD_READ_COUNTER;
	leverTaskPtr->spi_wpData[1] = 0;
	wiringPiSPIDataRW(kQD_CS_LINE, leverTaskPtr->spi_wpData, 2);
	uint8_t leverPosition = leverTaskPtr->spi_wpData[1];
	leverTaskPtr->leverPosition= leverPosition;
	//printf ("Lever position = %d.\n", leverTaskPtr->leverPosition);
	if (!(leverTaskPtr->trialComplete)){
		leverTaskPtr->positionData [leverTaskPtr->iPosition] = leverPosition;
		// light the lamp, or sound the horn
		if (leverTaskPtr->goalCuer != nullptr){
			if ((leverTaskPtr->inGoal== false)&&((leverPosition > leverTaskPtr->goalBottom)&&(leverPosition < leverTaskPtr->goalTop))){
				leverTaskPtr->inGoal = true;
				if (leverTaskPtr->goalMode == 0){
					leverTaskPtr->goalCuer->setLevel(1,1);
				}else{
					leverTaskPtr->goalCuer->startInfiniteTrain ();
				}
			}else{
				if ((leverTaskPtr->inGoal== true)&&((leverPosition < leverTaskPtr->goalBottom) || (leverPosition > leverTaskPtr->goalTop))){
					leverTaskPtr->inGoal = false;
					if (leverTaskPtr->goalMode == 1){
						leverTaskPtr->goalCuer->setLevel(0,1);
					}else{
						leverTaskPtr->goalCuer->stopInfiniteTrain ();
					}
				}
			}
		}
		// check for seting force, and maybe do the force
		if (leverTaskPtr->iPosition == leverTaskPtr->forceStartPos){
			leverTaskPtr->doForce = true;
			leverTaskPtr->iForce =0;
		}
		if (leverTaskPtr->doForce){
			// write the data
			int leverForce =leverTaskPtr->forceData[leverTaskPtr->iForce] ;
			wiringPiI2CWrite (leverTaskPtr->i2c_fd, kDAC_WRITEDAC); // DAC mode, not EEPROM
			wiringPiI2CWriteReg8(leverTaskPtr->i2c_fd, (leverForce  >> 8) & 0x0F, leverForce  & 0xFF);
			leverTaskPtr->iForce +=1;
			if (leverTaskPtr->iForce == leverTaskPtr->nForceData){ // all out of forces but leave force at final ramp pos until the end of the trial
				leverTaskPtr->doForce = false;
			}
		}
		// trial position specific stuff
		if (leverTaskPtr -> trialPos ==1){// 1 means lever not moved into goal area yet
			if (leverTaskPtr->isCued){
				if (leverPosition >  leverTaskPtr -> goalBottom){
					leverTaskPtr -> trialPos = 2;
					leverTaskPtr ->nToFinish = leverTaskPtr->iPosition  + leverTaskPtr->nHoldTicks;
					leverTaskPtr->circularBreak = leverTaskPtr->iPosition;
				}else{
					if (leverTaskPtr->iPosition ==  leverTaskPtr->nToGoal){
						leverTaskPtr -> trialPos = -1;
						leverTaskPtr ->nToFinish = leverTaskPtr->iPosition  + leverTaskPtr->nHoldTicks;
						leverTaskPtr->circularBreak = leverTaskPtr->iPosition;
					}
				}
			}else { // uncued trial
				if (leverPosition >  leverTaskPtr -> goalBottom){
					leverTaskPtr->circularBreak = leverTaskPtr->iPosition;
					leverTaskPtr->iPosition =  leverTaskPtr->nCircular; 
					leverTaskPtr ->trialPos =2;
				}else{
					// check for wraparound of circular buffer
					if (leverTaskPtr->iPosition ==  leverTaskPtr->nCircular){
						leverTaskPtr->iPosition = 1;
					}
				}
			}
		} else{
			if (leverTaskPtr ->trialPos ==2){ // check if we are still in goal range 
				if (leverTaskPtr->inGoal == false){
					leverTaskPtr ->trialPos = -2;
				}
			}
		}
	}
	// increment position and see if we are done
	leverTaskPtr->iPosition +=1;
	if (leverTaskPtr->iPosition == leverTaskPtr->nToFinish){
		leverTaskPtr->trialComplete =true;
		
		int leverForce =leverTaskPtr->constForce;
		wiringPiI2CWrite (leverTaskPtr->i2c_fd, kDAC_WRITEDAC); // DAC mode, not EEPROM
		wiringPiI2CWriteReg8(leverTaskPtr->i2c_fd, (leverForce  >> 8) & 0x0F, leverForce  & 0xFF);
	}
}


/* ************* Custom task data delete function *********************/
void leverThread_delTask (void * taskData){
	leverThreadStructPtr taskPtr = (leverThreadStructPtr) taskData;
	delete (taskPtr->forceData);
	delete (taskPtr);
}


/* ************************** zero Lever **********************************************
Paramater for  zeroing encoder at ralied position, vs returning lever to existing set start position (if possible)
 0 for taking the lever back to zero posiiton, 1 for  rezeroing the encoder as well as railing it,  */
int leverThread_zeroLeverCallback (void * modData, taskParams * theTask){
	
	leverThreadStructPtr leverTaskPtr = (leverThreadStructPtr) theTask->taskData;
	int mode = *(int *) modData;
	
	int returnVal;
	// configure a time spec to sleep for 0.05 seconds
	struct timespec sleeper;
	sleeper.tv_sec = 0;
	sleeper.tv_nsec = 5e07;
	
	int dacBase = leverTaskPtr->constForce;
	float dacIncr = (3500 - dacBase)/10;
	int dacOut;
	uint8_t prevLeverPos;
	uint8_t leverPos;
	if (mode == 1){ // 1 for  rezeroing the encoder as well as railing it, 0 for taking the lever back to zero posiiton 
		// clear counter
		leverTaskPtr->spi_wpData[0] = kQD_CLEAR_COUNTER;
		wiringPiSPIDataRW(kQD_CS_LINE, leverTaskPtr->spi_wpData, 1);
		prevLeverPos = 255;
	}else{
		leverTaskPtr->spi_wpData[0] = kQD_READ_COUNTER;
		leverTaskPtr->spi_wpData[1] = 0;
		wiringPiSPIDataRW(kQD_CS_LINE, leverTaskPtr->spi_wpData, 2);
		prevLeverPos = leverTaskPtr->spi_wpData[1];
	}
	// set initial value as constant force
	dacOut = (uint16_t) dacBase ;
	wiringPiI2CWrite (leverTaskPtr->i2c_fd, kDAC_WRITEDAC); // DAC mode, not EEPROM
	wiringPiI2CWriteReg8 (leverTaskPtr->i2c_fd, (dacOut  >> 8) & 0x0F, dacOut & 0xFF);
	for (int ii =0; ii < 10; ii +=1, prevLeverPos =leverPos){
		nanosleep (&sleeper, NULL) ;
		// check new position, see if we are moving
		leverTaskPtr->spi_wpData[0] = kQD_READ_COUNTER;
		leverTaskPtr->spi_wpData[1] = 0;
		wiringPiSPIDataRW(kQD_CS_LINE, leverTaskPtr->spi_wpData, 2);
		leverPos = leverTaskPtr->spi_wpData[1];
		// if lever is not moving in right direction, up the power
		if (leverPos >= prevLeverPos){
			dacOut = (uint16_t) (dacBase + (ii * dacIncr));
			wiringPiI2CWrite (leverTaskPtr->i2c_fd, kDAC_WRITEDAC); // DAC mode, not EEPROM
			wiringPiI2CWriteReg8 (leverTaskPtr->i2c_fd, (dacOut  >> 8) & 0x0F, dacOut & 0xFF);
		}
	}
	prevLeverPos =leverPos;
	// for just just putting lever back in position, check that lever is close to original 0
	if ((mode ==0) && ((leverPos <  2) || (prevLeverPos > 253))){
		returnVal =0;
	}else{
		int ii;
		for (ii =0; ii < 5; ii +=1, prevLeverPos =leverPos){
			nanosleep (&sleeper, NULL) ;
			leverTaskPtr->spi_wpData[0] = kQD_READ_COUNTER;
			leverTaskPtr->spi_wpData[1] = 0;
			wiringPiSPIDataRW(kQD_CS_LINE, leverTaskPtr->spi_wpData, 2);
			leverPos = leverTaskPtr->spi_wpData[1];
			if ((leverPos < prevLeverPos -2) || (leverPos > prevLeverPos +2)){
				break;
			}
		}
		// clear counter
		leverTaskPtr->spi_wpData[0] = kQD_CLEAR_COUNTER;
		wiringPiSPIDataRW(kQD_CS_LINE, leverTaskPtr->spi_wpData, 1);
		if (ii < 10){
			returnVal= 1;
		}else{
			returnVal= 0;
		}
	}
	delete((int *) modData);
	return returnVal;
}

 /* ******************* ThreadMaker with Integer pulse duration, delay, and number of pulses timing description inputs ********************
 Last Modified:
 2018/02/08 by Jamie Boyd - Initial Version */
leverThread * leverThread::leverThreadMaker (uint8_t * positionData, unsigned int nPositionData, unsigned int nCircularOrZero,  int goalCuerPinOrZero, float cuerFreqOrZero) {
	
	int errCode;
	leverThread * newLever ;
	// make and fill a leverTask struct
	leverThreadInitStructPtr initStruct = new leverThreadInitStruct;
	initStruct->positionData = positionData;
	initStruct->nPositionData = nPositionData;
	initStruct->goalCuerPin = goalCuerPinOrZero; // zero if we don't have in-goal cue
	initStruct->cuerFreq = cuerFreqOrZero;		// freq is zero for a DC on or off task
	if (nCircularOrZero == 0){				// if nCircular is 0, trials are cued
		initStruct->nCircular = nPositionData;	// make sure nCircular never happens
		newLever = new leverThread ((void *) initStruct, nPositionData, errCode); // initialize thread with pulses in a train equal to posiiton array size
	}else{
		initStruct->nCircular = nCircularOrZero;
		newLever = new leverThread ((void *) initStruct, (unsigned int) 0, errCode);	// initialize thread with 0 pulses, AKA infinite train
	}
	if (errCode){
#if beVerbose
		printf ("leverThreadMaker failed to make leverThread object.\n");
#endif
		return nullptr;
	}
	// set custom task delete function
	newLever->setTaskDataDelFunc (&leverThread_delTask);
	// make a leverThread pointer for easy direct access to thread task data 
	newLever->taskPtr = (leverThreadStructPtr)newLever->getTaskData ();
	if (nCircularOrZero == 0){
		newLever->cueMode = kUN_CUED;
	}else{
		newLever->cueMode = kCUED;
	}
	return newLever;
}

/* ***********************************************Constant Force ************************************************
This function sets the saved value, but does not apply the force
Last Modified 2018/03/26 by Jamie Boyd - initial version */
void leverThread::setConstForce (int theForce){
	taskPtr->constForce = theForce;
}

/* ********************* returns the vlaue for constant force ********************
Last Modified 2018/03/26 by Jamie Boyd - initial version */
int leverThread::getConstForce (void){
	return taskPtr->constForce;
}


uint8_t leverThread::getLeverPos (void){
	return taskPtr->leverPosition;
}

/* ************applies a given force ****************************
If theForce is less than 0, the value for constant force will be used
Force is also scrunched to max, 4095
Last Modified 2018/03/26 by Jamie Boyd - initial version */
void leverThread::applyForce (int theForce){
	if (theForce < 0){
		theForce = taskPtr->constForce;
	}
	if (theForce > 4095){
		theForce = 4095;
	}
	// write the data
	wiringPiI2CWriteReg8(taskPtr->i2c_fd, (theForce >> 8) & 0x0F, theForce  & 0xFF);
}


/* ************************************** Zeroing the Lever ************************************************
Last Modified 2018/03/26 by Jamie Boyd - initial version */
int leverThread::zeroLever (int mode, int isLocking){
	
	int * modePtr = new int;
	* modePtr = mode;
	int returnVal = modCustom (&leverThread_zeroLeverCallback, (void * ) modePtr, isLocking);
	return returnVal;
}

/* *********************************** Setting perturbation ***********************************
fills the array with force data 
last modified 2018/03/26 by Jamie Boyd - initial version */
void leverThread::setPerturbForce (int perturbForce){
	if (perturbForce < 0){
		perturbForce = 0;
	}
	if (perturbForce > 4095){
		perturbForce = 4095;
	}
	float halfWay = taskPtr->nForceData/2;
	float rate = taskPtr->nForceData/10;
	float base = taskPtr->constForce;
	for (unsigned int iPt =0; iPt <  taskPtr->nForceData; iPt +=1){
		taskPtr->forceData [iPt] = (int) (base + perturbForce/(1 + exp (-(iPt - halfWay)/rate)));
	}
}

/* ************************ Sets posiiton where perturb force is applied *****************
last modified 2018/03/26 by Jamie Boyd - initial version */
void leverThread::setPerturbStartPos(unsigned int perturbStartPos){
	taskPtr->forceStartPos =  perturbStartPos;
}


/* ******************************************************* Starting, Stopping, Checking Trials**************************************************** 
Starts a trial, either cued or un-cued 
last modified 2018/03/26 by Jamie Boyd - initial version */
void leverThread::startTrial (void){
	taskPtr->iPosition =1;
	taskPtr->trialPos =1;
	taskPtr->trialComplete =false;
	taskPtr->inGoal=false;
	taskPtr->doForce = false;
	leverTaskPtr->circularBreak=0;
	if (taskPtr->isCued){
		DoTask ();
	}else{
		taskPtr ->nToFinish = taskPtr->nCircular  + taskPtr->nHoldTicks;
		startInfiniteTrain ();
	}
}

/* *********************************Checks if trial is complete or what stage it is at *********************************************
Returns truth that a trial is completed, sets trial code to trial code, which will be 3 at end of a successful trial
last modified 2018/03/26 by Jamie Boyd - initial version */
bool leverThread::checkTrial(int &trialCode, unsigned int &goalEntryPos){
	trialCode = taskPtr->trialPos;
	goalEntryPos = taskPtr->circularBreak;
	bool isComplete = taskPtr->trialComplete;
	if (isComplete){
		if (!(taskPtr->isCued)){
			stopInfiniteTrain ();
		}
	}
	return isComplete;
}

void leverThread::abortUncuedTrial(void){
	if (!(taskPtr->isCued)){
		stopInfiniteTrain ();
	}
}

/* ************************************** tests the in-goal cuer manually***********************************************
If you dont have a goal cuer, it does nothing
last modified 2018/03/27 by Jamie Boyd - initial version */
void leverThread::doGoalCue (int offOn){
	if (taskPtr->goalCuer != nullptr){
		if (taskPtr->goalMode == 0){
			if (offOn){
				taskPtr->goalCuer->setLevel(1,0);
			}else{
				taskPtr->goalCuer->setLevel(0,0);
				}
		}else{
			if (offOn){
				taskPtr->goalCuer->startInfiniteTrain ();
			}else{
				taskPtr->goalCuer->stopInfiniteTrain ();
			}
		}
	}
}

void leverThread::setHoldParams (uint8_t goalBottomP, uint8_t goalTopP, unsigned int nHoldTicksP){
	taskPtr->goalBottom =goalBottomP;
	taskPtr->goalTop = goalTopP;
	taskPtr->nHoldTicks = nHoldTicksP;
}


