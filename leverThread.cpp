
#include "leverThread.h"

int lever_init (void * initDataP, void *  &taskDataP){
	// task data pointer is a void pointer that needs to be initialized to a pointer to taskData and filled from our custom init structure 
	leverThreadStructPtr taskData  = new leverThreadStruct;
	taskDataP = taskData;
	// initData is a pointer to our custom init structure
	leverThreadInitStructPtr initDataPtr = (leverThreadInitStructPtr) initDataP;
	// ensure wiringpi is setup for GPIO
	wiringPiSetupGpio();
#if FORCEMODE == AOUT
	// initialize the DAC - further calls to wiringPi DO use the file descriptor, so save i2c file descriptor
	taskData->i2c_fd = wiringPiI2CSetup(kDAC_ADDRESS);
	if (taskData->i2c_fd  <= 0){
		return 1;
	}
	wiringPiI2CWrite (taskData->i2c_fd, kDAC_WRITEDAC);
#elif FORCEMODE == PWM
	int mapResult = PWM_thread::mapPeripherals ();
	if (mapResult){
		return 1;
	}
	// set clock for PWM from input parmamaters
	float setFrequency = PWM_thread::setClock (PWM_FREQ, PWM_RANGE);
	if (setFrequency < 0){
		return 1;
	}
	taskData->setFrequency = setFrequency;
	taskData->pwmRange=PWM_RANGE;
	// save ctl and data register addresses in task data for easy access
	taskData->ctlRegister = PWMperi->addr + PWM_CTL;
	taskData->dataRegister1 = PWMperi->addr + PWM_DAT1;
	// start in enabled state, so set data output = 0
	*(taskData->dataRegister1) = 0;
	// setup channel 1
	unsigned int registerVal = *(taskData->ctlRegister);
	registerVal &= ~(PWM_MODE1 | PWM_MODE2);
	*(taskData->ctlRegister) =0;
	// set up GPIO
	INP_GPIO(GPIOperi ->addr, 18);           // Set GPIO 18 to input to clear bits
	SET_GPIO_ALT(GPIOperi ->addr, 18, 5);     // Set GPIO 18 to Alt5 function PWM0
	// set bits and offsets appropriately for channel 1
	registerVal |= PWM_MSEN1; // put PWM in MS Mode
	registerVal &= ~PWM_POLA1;  // clear reverse polarity bit
	registerVal &= ~PWM_SBIT1; // clear OFFstate bit for low
	registerVal |= PWM_PWEN1;
	// set the control register with registerVal
	*(taskData->ctlRegister) = registerVal;
#endif
	// initialize quad decoder - it returns spi file descriptor, but further calls to wiringPi don't use it
	if (wiringPiSPISetup(kQD_CS_LINE, kQD_CLOCK_FREQ) == -1){
		return 2;
	}
	// clear status
	taskData->spi_wpData[0] = kQD_CLEAR_STATUS;
	wiringPiSPIDataRW(kQD_CS_LINE, taskData->spi_wpData, 1);
	// clear counter
	taskData->spi_wpData[0] = kQD_CLEAR_COUNTER;
	wiringPiSPIDataRW(kQD_CS_LINE, taskData->spi_wpData, 1);
	// set write mode to 4x (highest resolution) and 2 byteMode (e.g., 2 bytes = 0-65535 before counter rolls over)
	taskData->spi_wpData[0] = kQD_WRITE_MODE0;
	taskData->spi_wpData[1] = kQD_FOURX_COUNT;
	wiringPiSPIDataRW (kQD_CS_LINE, taskData->spi_wpData, 2);
	taskData->spi_wpData [0] = kQD_WRITE_MODE1;
	taskData->spi_wpData [1] =kQD_TWOBYTE_COUNTER;
	wiringPiSPIDataRW (kQD_CS_LINE, taskData->spi_wpData, 2);
	
	// make a goal cuer
	if (initDataPtr->goalCuerPin > 0){
		if (initDataPtr->cuerFreq ==0){
			taskData->goalCuer = SimpleGPIO_thread::SimpleGPIO_threadMaker (initDataPtr->goalCuerPin, 0, (unsigned int) 1000,  (unsigned int) 1000,  (unsigned int) 1, 1);
			taskData->goalMode = kGOALMODE_HILO;
		}else{
			taskData->goalCuer = SimpleGPIO_thread::SimpleGPIO_threadMaker (initDataPtr->goalCuerPin, 0, (float) initDataPtr->cuerFreq , (float) 0.5, (float) 0, 1);
			taskData->goalMode = kGOALMODE_TRAIN;
		}
	}else{
		taskData->goalCuer  = nullptr;
		taskData->goalMode = kGOALMODE_NONE;
	}
	// lever decoder, reversed or not
	taskData->isReversed = initDataPtr->isReversed;
	// task cuing details
	taskData->isCued = initDataPtr->isCued;
	taskData->nToGoalOrCircular = initDataPtr->nToGoalOrCircular;
	// copy pointer to lever position buffer
	taskData->positionData = initDataPtr->positionData;
	taskData->nPositionData = initDataPtr->nPositionData;
	// make force data
	taskData->nForceData = initDataPtr->nForceData;
	taskData->forceData = new int [initDataPtr->nForceData];
	taskData->iForce = 0;
	// initialize iPosition to 0 - other initialization?
	taskData->iPosition=0;
	taskData->forceStartPos = initDataPtr->nPositionData; // no force will be applied cause we never get to here
	taskData->trialComplete = true;
	// initialize reasonable values
	taskData->goalBottom =10;
	taskData->goalTop = 100;
	taskData->nHoldTicks = 100;
	taskData->constForce=1000;
	return 0;
}

/* ***************************************High function - we don't have a low function ****************************************
Runs as an infinite train when mouse is present, filling circular buffer till we pass threshold
Or run as a cued trial with a train of length nPositionData, you do the cue and start the train
*/

void lever_Hi (void * taskData){
	// cast task data to  leverStruct
	leverThreadStructPtr leverTaskPtr = (leverThreadStructPtr) taskData;
	// read quadrature decoder into position data, and into leverPosition, so we can get lever position easily during a trial
	leverTaskPtr->spi_wpData[0] = kQD_READ_COUNTER;
	leverTaskPtr->spi_wpData[1] = 0;
	leverTaskPtr->spi_wpData[2] = 0;
	wiringPiSPIDataRW(kQD_CS_LINE, leverTaskPtr->spi_wpData, 3);
	uint16_t leverPosition;
	if (leverTaskPtr -> isReversed){
		leverPosition = 65536 - (256 * leverTaskPtr->spi_wpData[1]  + leverTaskPtr->spi_wpData[2]);
	}else{
		leverPosition = 256 * leverTaskPtr->spi_wpData[1] + leverTaskPtr->spi_wpData[2];
	}
	leverTaskPtr->leverPosition= leverPosition;
	// this if statement is needed for un-cued trials, which are infinite trains
	if (leverTaskPtr->iPosition < leverTaskPtr->nToFinish){
		// record lever data
		leverTaskPtr->positionData [leverTaskPtr->iPosition] = leverPosition;
		// signal in-goal
		if (leverTaskPtr->goalCuer != nullptr){
			if ((leverTaskPtr->inGoal== false)&&((leverPosition > leverTaskPtr->goalBottom)&&(leverPosition < leverTaskPtr->goalTop))){
				leverTaskPtr->inGoal = true;
				if (leverTaskPtr->goalMode == kGOALMODE_HILO){
					leverTaskPtr->goalCuer->setLevel(1,1);
				}else{
					leverTaskPtr->goalCuer->startInfiniteTrain ();
				}
			}else{
				if ((leverTaskPtr->inGoal== true)&&((leverPosition < leverTaskPtr->goalBottom) || (leverPosition > leverTaskPtr->goalTop))){
					leverTaskPtr->inGoal = false;
					if (leverTaskPtr->goalMode == kGOALMODE_HILO){
						leverTaskPtr->goalCuer->setLevel(0,1);
					}else{
						leverTaskPtr->goalCuer->stopInfiniteTrain ();
					}
				}
			}
		}
		// check for seting force
		if ((leverTaskPtr->iPosition  >= leverTaskPtr->forceStartPos) && (leverTaskPtr->iForce < leverTaskPtr->nForceData)){
			int leverForce =leverTaskPtr->forceData[leverTaskPtr->iForce] ;
#if FORCEMODE == AOUT
			wiringPiI2CWriteReg8(leverTaskPtr->i2c_fd, (leverForce  >> 8) & 0x0F, leverForce  & 0xFF);
#elif FORCEMODE == PWM
			*(leverTaskPtr->dataRegister1) = leverForce;
#endif
			leverTaskPtr->iForce +=1;
		}
		// increment position in lever position array, before we set it back to 0 for un-cued trial 
		leverTaskPtr->iPosition +=1;
		// trial position specific stuff
		if (leverTaskPtr -> trialPos ==1){// 1 means lever not moved into goal area yet
			if (leverPosition >  leverTaskPtr -> goalBottom){
				leverTaskPtr -> trialPos = 2; // lever moved into goal area (for a cued trial, it happened before time ran out)
				leverTaskPtr->breakPos = leverTaskPtr->iPosition; // record where we entered goal area
				if (!(leverTaskPtr->isCued)){
					// for uncued trial, jump to end of circular buffer
					leverTaskPtr->iPosition =  leverTaskPtr->nToGoalOrCircular; 
				}
			}else{ // check if time for getting into goal area expired for cued trial, or time to reset iPosiiton for uncued
				if (leverTaskPtr->iPosition == leverTaskPtr->nToGoalOrCircular ){
					if (leverTaskPtr->isCued){
						leverTaskPtr -> trialPos = -1; // lever did not get to goal area before time ran out
					}else{ // for un-cued trial, do wrap-around of circular buffer
						leverTaskPtr->iPosition = 0;
					}
				}
			}
		}else{
			if (leverTaskPtr ->trialPos ==2){ // check if we are still in goal range 
				if (leverTaskPtr->inGoal == false){
					leverTaskPtr ->trialPos = -2;
				}
			}
		}
		// check if we are done
		if (leverTaskPtr->iPosition == leverTaskPtr->nToFinish){
			leverTaskPtr->trialComplete =true;
			// set lever to constant force, in case this was a perturb force trial
			int leverForce =leverTaskPtr->constForce;
#if FORCEMODE == AOUT
			wiringPiI2CWriteReg8(leverTaskPtr->i2c_fd, (leverForce  >> 8) & 0x0F, leverForce  & 0xFF);
#elif FORCEMODE == PWM
			*(leverTaskPtr->dataRegister1) = leverForce;
#endif
			// make sure goal cuer is turned off
			if (leverTaskPtr->goalCuer != nullptr){
				if (leverTaskPtr->goalMode == kGOALMODE_HILO){
					leverTaskPtr->goalCuer->setLevel(0,1);
				}else{
					leverTaskPtr->goalCuer->stopInfiniteTrain ();
				}
			}
		}
	}
}


/* ************* Custom task data delete function *********************/
void leverThread_delTask (void * taskData){
	leverThreadStructPtr taskPtr = (leverThreadStructPtr) taskData;
	delete [] taskPtr->forceData;
	delete taskPtr;
}


/* ********************************* zero Lever **********************************************
mod data is int, 0 for returning lever to existing start position (if possible) or 1 for railing lever and
setting a new zeroed position
* Last Modified:
* 2019/03/04 by Jamie Boyd - adding PWM option, plus some reorg of duplication in logic
*/
int leverThread_zeroLeverCallback (void * modData, taskParams * theTask){
	
	leverThreadStructPtr leverTaskPtr = (leverThreadStructPtr) theTask->taskData;
	int mode = *(int *) modData;
	// configure a time spec to sleep for 0.05 seconds
	struct timespec sleeper;
	sleeper.tv_sec = 0;
	sleeper.tv_nsec = 5e07;
	// divide force range into 20 
	int dacBase = leverTaskPtr->constForce;
	float dacIncr = (3500 - dacBase)/20;
	int dacOut;
	uint16_t leverPos;
	int ii;
	int returnVal =0;
	// increment force gradually
	for (ii=0; ii < 20; ii +=1){
		dacOut = (uint16_t) (dacBase + (ii * dacIncr));
#if FORCEMODE == AOUT
		wiringPiI2CWriteReg8 (leverTaskPtr->i2c_fd, (dacOut  >> 8) & 0x0F, dacOut & 0xFF);
#elif FORCEMODE == PWM
		*(leverTaskPtr->dataRegister1) = dacOut;
#endif
		nanosleep (&sleeper, NULL);
	}
	if (mode == 0){ // 0 for returning the lever to 0 position, check that it is near original 0
		// Read Counter
		leverTaskPtr->spi_wpData[0] = kQD_READ_COUNTER;
		leverTaskPtr->spi_wpData[1] = 0;
		leverTaskPtr->spi_wpData[2] = 0;
		wiringPiSPIDataRW(kQD_CS_LINE, leverTaskPtr->spi_wpData, 3);
		if (leverTaskPtr -> isReversed){
			leverPos = 65536 - ( 256 * leverTaskPtr->spi_wpData[1] + leverTaskPtr->spi_wpData[2]);
		}else{
			leverPos = 256 * leverTaskPtr->spi_wpData[1] + leverTaskPtr->spi_wpData[2];
		}
		if ((leverPos > 10) && (leverPos < 65525)){
			returnVal = 1;
		}
	}
	// clear counter
	leverTaskPtr->spi_wpData[0] = kQD_CLEAR_COUNTER;
	wiringPiSPIDataRW(kQD_CS_LINE, leverTaskPtr->spi_wpData, 1);
	// set force on lever back to base force
	dacOut = (uint16_t) dacBase;
#if FORCEMODE == AOUT
	wiringPiI2CWriteReg8 (leverTaskPtr->i2c_fd, (dacOut  >> 8) & 0x0F, dacOut & 0xFF);
#elif FORCEMODE == PWM
	*(leverTaskPtr->dataRegister1) = dacOut;
#endif	
	delete (int *) modData;
	return returnVal;
}

 /* ******************* ThreadMaker with Integer pulse duration, delay, and number of pulses timing description inputs ********************
 Last Modified:
 2018/02/08 by Jamie Boyd - Initial Version */
leverThread * leverThread::leverThreadMaker (uint16_t * positionData, unsigned int nPositionData, bool isCuedP, unsigned int nToGoalOrCircularP,   int isReversed, int goalCuerPinOrZero, float cuerFreqOrZero) {
	
	int errCode;
	leverThread * newLever;
	// make and fill a leverTask struct
	leverThreadInitStructPtr initStruct = new leverThreadInitStruct;
	initStruct->positionData = positionData;
	initStruct->nPositionData = nPositionData;
	initStruct->isCued = isCuedP;
	initStruct->isReversed = isReversed;
	initStruct->goalCuerPin = goalCuerPinOrZero; // zero if we don't have in-goal cue
	initStruct->cuerFreq = cuerFreqOrZero;		// freq is zero for a DC on or off task
	initStruct->nForceData=kFORCE_ARRAY_SIZE;
	initStruct->nToGoalOrCircular = nToGoalOrCircularP;
	if (isCuedP){				// if isCued trials , initialize thread with pulses in a train equal to posiiton array size
		newLever = new leverThread ((void *) initStruct, nPositionData, errCode);
	}else{	// ifor uncued trials, initialize thread with 0 pulses, AKA infinite train
		newLever = new leverThread ((void *) initStruct, (unsigned int) 0, errCode);	
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
	return newLever;
}

/* ***********************************************Constant Force ************************************************
This function sets the saved value, but does not apply the force
Last Modified 2018/03/26 by Jamie Boyd - initial version */
void leverThread::setConstForce (int theForce){
	if (theForce < 0){
		taskPtr->constForce =0;
	}else{
		if (theForce > 4095){
			taskPtr->constForce = 4095;
		} else{
			taskPtr->constForce = theForce;
		}
	}
}

/* ********************* returns the vlaue for constant force ********************
Last Modified 2018/03/26 by Jamie Boyd - initial version */
int leverThread::getConstForce (void){
	return taskPtr->constForce;
}


bool leverThread::isCued (void){
	return taskPtr->isCued;
}


void leverThread::setCue (bool isCuedP){
	taskPtr->isCued =isCuedP ;
	if (isCuedP){
		modTrainLength (0);
	}
}

/* ***************************************Gets current lever position from thread data, if trial is running*********************
else , reads the lever position directly and sets the value in the thread data
last modified:
2018/04/09 by Jamie Boyd - added code to check directly if no task in progress */
uint16_t leverThread::getLeverPos (void){
	if ( taskPtr->trialComplete){
		taskPtr->spi_wpData[0] = kQD_READ_COUNTER;
		taskPtr->spi_wpData[1] = 0;
		wiringPiSPIDataRW(kQD_CS_LINE, taskPtr->spi_wpData, 2);
		uint16_t leverPosition;
	
	if ( taskPtr-> isReversed){
			leverPosition = 65536 - ( 256 * taskPtr->spi_wpData[1] + taskPtr->spi_wpData[2]);
		}else{
			leverPosition = 256 * taskPtr->spi_wpData[1] + taskPtr->spi_wpData[2];
		}

	
	taskPtr->leverPosition= leverPosition;
	}
	return taskPtr->leverPosition;
}

/* ************ applies a given force ****************************
If theForce is less than 0, or greater then 4095, it is scrunched
Force is also scrunched to max, 4095
Last Modified:
* 2019/03/04 by Jamie Boyd - modified for PWM force option
*  2018/03/26 by Jamie Boyd - initial version */
void leverThread::applyForce (int theForce){
	if (theForce < 0){
		theForce =0;
	}else{
		if (theForce > 4095){
			theForce = 4095;
		}
	}
	// write the data
#if FORCEMODE == AOUT
	wiringPiI2CWriteReg8(taskPtr->i2c_fd, (theForce >> 8) & 0x0F, theForce  & 0xFF);
#elif FORCEMODE == PWM
	*(taskPtr->dataRegister1) = theForce;
#endif
}

/* ********************* Applies currently set value for constant force ****************************
* Last Modified:
* 2019/03/04 by Jamie Boyd - modified for PWM force option
*  2018/03/26 by Jamie Boyd - initial version */ 
void leverThread::applyConstForce (void){
	int theForce =  taskPtr->constForce;
	// write the data
#if FORCEMODE == AOUT
	wiringPiI2CWriteReg8(taskPtr->i2c_fd, (theForce >> 8) & 0x0F, theForce  & 0xFF);
#elif FORCEMODE == PWM
	*(taskPtr->dataRegister1) = theForce;
#endif
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
last modified 2018/04/09 by jamie Boyd - corrected for negative forces
last modified 2018/03/26 by Jamie Boyd - initial version */
void leverThread::setPerturbForce (int perturbForceP){
	if (taskPtr->constForce + perturbForceP < 0){
		taskPtr->perturbForce  = - (taskPtr->constForce);
	}else{
		if (taskPtr->constForce + perturbForceP > 4095){
			taskPtr->perturbForce = 4095 - taskPtr->constForce;
		}else{
			taskPtr->perturbForce = perturbForceP;
		}
	}
	unsigned int nForceDataM1 = taskPtr->nForceData -1;
	unsigned int iPt;
	float halfWay = nForceDataM1/2;
	float rate = nForceDataM1/10;
	float base = taskPtr->constForce;
 #if beVerbose	
	printf ("force array:");
#endif
	for (iPt =0; iPt <  nForceDataM1; iPt +=1){
		taskPtr->forceData [iPt] = (int) (base + taskPtr->perturbForce/(1 + exp (-(iPt - halfWay)/rate)));
 #if beVerbose		
		printf ("%d, ", taskPtr->forceData [iPt]);
#endif		
	}
	taskPtr->forceData[iPt] = taskPtr->constForce + taskPtr->perturbForce ;
 #if beVerbose	
	printf ("%d\n", taskPtr->forceData [iPt]);
#endif
}

/* ************************ Sets position where perturb force is applied *****************
last modified 2018/03/26 by Jamie Boyd - initial version */
void leverThread::setPerturbStartPos(unsigned int perturbStartPos){
	taskPtr->forceStartPos =  taskPtr->nToGoalOrCircular + perturbStartPos;
}

void leverThread::setPerturbOff (void){
	taskPtr->forceStartPos = taskPtr->nPositionData;
}

/* ******************************************************* Starting, Stopping, Checking Trials **************************************************** 
Starts a trial, either cued or un-cued 
last modified 2018/03/26 by Jamie Boyd - initial version */
void leverThread::startTrial (void){
	taskPtr->iPosition =0;
	taskPtr->iForce = 0;
	taskPtr->trialPos =1;
	taskPtr->trialComplete =false;
	taskPtr->inGoal=false;
	taskPtr->nToFinish = taskPtr->nToGoalOrCircular + taskPtr->nHoldTicks;
	if (taskPtr->isCued){
		modTrainLength (taskPtr->nToFinish);
		DoTask ();
	}else{
		for (unsigned int iPosition =0;iPosition < taskPtr->nToGoalOrCircular; iPosition +=1){
			taskPtr->positionData [iPosition] = 0;
			startInfiniteTrain();
		}
	}
	
}

/* *********************************Checks if trial is complete or what stage it is at *********************************************
Returns truth that a trial is completed, sets trial code to trial code, which will be 2 at end of a successful trial
last modified 2018/03/26 by Jamie Boyd - initial version */
bool leverThread::checkTrial(int &trialCode, unsigned int &goalEntryPos){
	trialCode = taskPtr->trialPos;
	goalEntryPos = taskPtr->breakPos;
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
		taskPtr->trialComplete =true;
		// make sure goal cuer is turned off
		if (taskPtr->goalCuer != nullptr){
			if (taskPtr->goalMode == kGOALMODE_HILO){
				taskPtr->goalCuer->setLevel(0,1);
			}else{
				taskPtr->goalCuer->stopInfiniteTrain ();
			}
		}
	}
}

/* ************************************** tests the in-goal cuer manually***********************************************
If you dont have a goal cuer, it does nothing
last modified:
2018/03/28 by Jamie Boyd - added constants for mode, eliminated test for goalCuer, should be handled by init
2018/03/27 by Jamie Boyd - initial version */
void leverThread::doGoalCue (int offOn){
	
	if (taskPtr->goalMode == kGOALMODE_HILO){
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

/* ******************************************* Sets hold goal width and time paramaters ************************************
lever force paramaters are set separately
last Modified:
2019/03/01 by Jamie Boyd - changed encoder values from 1 byte to 2 bytes 
2018/03/28 by jamie Boyd - initial verison */
void leverThread::setHoldParams (uint16_t goalBottomP, uint16_t goalTopP, unsigned int nHoldTicksP){
	taskPtr->goalBottom =goalBottomP;
	taskPtr->goalTop = goalTopP;
	taskPtr->nHoldTicks = nHoldTicksP;
}


