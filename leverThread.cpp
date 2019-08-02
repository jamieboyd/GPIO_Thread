
#include "leverThread.h"
/* ************************************* Init function for thread **********************************************
* Last Modified:
* 2019/06/07 by Jamie Boyd - adding code for motorDir */
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
	wiringPiI2CWriteReg8(taskData->i2c_fd, 0, 0);
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

	if (initDataPtr->startCuerPin > 0){
		taskData->startCueTime = initDataPtr->startCueTime;
		std::cout<< "hi" << "\n";
		if (initDataPtr->startCuerFreq ==0){
			taskData->startCuer = SimpleGPIO_thread::SimpleGPIO_threadMaker (initDataPtr->startCuerPin, 0, (unsigned int) 1000,  (unsigned int) 1000,  (unsigned int) 1, 1);
			taskData->startMode = kGOALMODE_HILO;
		}else{
			taskData->startCuer = SimpleGPIO_thread::SimpleGPIO_threadMaker (initDataPtr->startCuerPin, 0, (float) initDataPtr->startCuerFreq , (float) 0.5, (float) 0, 1);
			taskData->startMode = kGOALMODE_TRAIN;
		}
	}else{
		taskData->startCuer  = nullptr;
		taskData->startMode = kGOALMODE_NONE;
	}
	// lever decoder, reversed or not
	taskData->isReversed = initDataPtr->isReversed;
	// task cuing details
	taskData->isCued = initDataPtr->isCued;
	taskData->timeBetweenTrials = initDataPtr->timeBetweenTrials;
	taskData->nToGoalOrCircular = initDataPtr->nToGoalOrCircular;
	// copy pointer to lever position buffer
	taskData->positionData = initDataPtr->positionData;
	taskData->nPositionData = initDataPtr->nPositionData;
	// make force data (force data is integer, and must range from 0 to 4095)
	taskData->nForceData = initDataPtr->nForceData;
	taskData->forceData = new int [initDataPtr->nForceData];
	taskData->iForce = 0;
	// motor direction and symmetry
	taskData -> motorIsReversed = initDataPtr-> motorIsReversed;
	if ((initDataPtr-> motorDirPin) == 0){ // motor pin = 0 signals bi-directional force control
		taskData-> motorHasDirPin = false;
		taskData -> motorDir = nullptr;
	}else{
		taskData-> motorHasDirPin = true;
		// motorDir is set by a threadlessGPIO. 0 is always towards the start position, 1 is always away.
		taskData -> motorDir = newThreadlessGPIO (initDataPtr-> motorDirPin, initDataPtr-> motorIsReversed ? 1 : 0);
	}
	// initialize iPosition to 0 - other initialization?
	taskData->iPosition=0;
	taskData->forceStartPos = initDataPtr->nPositionData; // no force will be applied cause we never get to here
	taskData->trialComplete = true;
	// initialize reasonable values for task data - these should be changed each trial
	taskData->goalBottom =50;
	taskData->goalTop = 150;
	taskData->nHoldTicks = 125;
	taskData->currHoldTicks = 0;
	taskData->constForce=1000;
	return 0;
}

/* ***************************************High function - we don't have a low function ****************************************
Uncued trial: Runs as an infinite train , filling circular buffer till we pass goal bottom, then runs a trial, stopping infinite train when trial is over
cued trial: Starts a train of finite length (nPositionData), though we might not collect data till end of train, if trial is aborted we still have to wait till end of train

Sets trial position: 0 = lever has not been to goal area, 1 = lever is in goal area, -1 lever left goal area before hold time expired,
2 = got to perturbation, -2 means  lever left goal area during perturbation (not  enabled - we don't check for "out of bounds" after perturbation starts)
* Last Modified:
* 2019/06/07 by Jamie Boyd - adding code for motorDir */
void lever_Hi (void * taskData){
	// cast task data to  leverStruct
	leverThreadStructPtr leverTaskPtr = (leverThreadStructPtr) taskData;
	// read quadrature decoder into position data, and into leverPosition, so we can get lever position easily during a trial
	leverTaskPtr->spi_wpData[0] = kQD_READ_COUNTER;
	leverTaskPtr->spi_wpData[1] = 0;
	leverTaskPtr->spi_wpData[2] = 0;
	wiringPiSPIDataRW(kQD_CS_LINE, leverTaskPtr->spi_wpData, 3);
	int16_t leverPosition;
	if (leverTaskPtr -> isReversed){
		leverPosition = (int16_t)(65536 - (256 * leverTaskPtr->spi_wpData[1]  + leverTaskPtr->spi_wpData[2]));
	}else{
		leverPosition = (int16_t)(256 * leverTaskPtr->spi_wpData[1] + leverTaskPtr->spi_wpData[2]);
	}
	leverTaskPtr->leverPosition= leverPosition;
	// if trial is complete but thread is still runing, we may be spinning our wheels a bit
	if  (!(leverTaskPtr->trialComplete)){
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
		// trial position specific stuff
		switch (leverTaskPtr -> trialPos){
			case 0: // 0 means lever not moved into goal area yet, check if lever has moved into goal area
			{
				if (leverPosition >=  leverTaskPtr -> goalBottom){
					leverTaskPtr -> trialPos = 1; // lever moved into goal area (for a cued trial, it happened before time ran out)
					leverTaskPtr ->currHoldTicks ++;
					leverTaskPtr->breakPos = leverTaskPtr->iPosition; // record where we entered goal area
					if (leverTaskPtr -> isCued){ // for cued trial, increment position in lever position array
						leverTaskPtr->iPosition +=1;
					}else{  // for uncued trial, jump to end of circular buffer
						leverTaskPtr->iPosition =  leverTaskPtr->nToGoalOrCircular;
					}
				}else{ // check if time for getting into goal area has expired for cued trial, or if it is time to reset iPositon for uncued
					if (leverTaskPtr->iPosition == leverTaskPtr->nToGoalOrCircular ){
						if (leverTaskPtr->isCued){
							leverTaskPtr->nToFinish = leverTaskPtr->iPosition; // lever did not get to goal area before time ran out so that's that
						}else{ // for un-cued trial, do wrap-around of circular buffer
							leverTaskPtr->iPosition = 0;
						}
					}else{
						leverTaskPtr->iPosition +=1;
					}
				}
				break;
			}
			case 1:  // 1 means lever in goal area, check if lever is still in goal area
			{
				if ((leverPosition <  leverTaskPtr -> goalBottom) || (leverPosition >  leverTaskPtr -> goalTop)){
					leverTaskPtr ->trialPos = -1; // trial ends here
					leverTaskPtr->nToFinish = leverTaskPtr->iPosition; // lever left goal area so that's that
				}else {
					leverTaskPtr->iPosition +=1;
					// check for seting force
					leverTaskPtr ->currHoldTicks ++;
					if(leverTaskPtr -> currHoldTicks >= leverTaskPtr -> nHoldTicks) {
						leverTaskPtr->nToFinish = leverTaskPtr->iPosition; // Succesful trial
					}
					if (leverTaskPtr -> iPosition ==  leverTaskPtr->forceStartPos){
						leverTaskPtr->trialPos = 2; // trial position for getting to perturb start
					}
				}
				break;
			}
			case 2: // 2 means perturbation has started. For symmetric force, numbers lower than 2048 pull towards the start pos. higher numbers push away from start pos
					// with motorDirPin, negative numbers mean pull towards start, set GPIO low. positive numbers mean push away from start, set GPIO higjh
			{
				if (leverTaskPtr->iForce < leverTaskPtr->nForceData){
					leverTaskPtr ->currHoldTicks ++;
					int leverForce =leverTaskPtr->forceData[leverTaskPtr->iForce];
					/* no need for this code if perturb force is scrunched to constant force
					if (leverTaskPtr-> motorhasDirPin){
						if (leverTaskPtr->iForce == 0){
							if (leverForce < 0){ // current lever force is less than 0, force towards start position, but dirPin shoul dbe set correctly already
								leverForce = -leverForce;
							}
						}else {
							if (leverForce < 0){ // current lever force is less than 0, force towards start position
								leverForce = -leverForce;
								if ([leverTaskPtr->iForce -1] >= 0){ // need to set state of direction output
									setThreadlessGPIO (leverTaskPtr->motorDir, kLEVER_BACKWARDS);
								}
							}else{ // lever force is >= than 0
								if (leverTaskPtr->forceData[leverTaskPtr->iForce -1] < 0)){ // need to switch state of direction output
								setThreadlessGPIO (leverTaskPtr->motorDir, kLEVER_BACKWARDS);
								}
							}
						}
					}
					*/
#if FORCEMODE == AOUT
					wiringPiI2CWriteReg8(leverTaskPtr->i2c_fd, (leverForce  >> 8) & 0x0F, leverForce  & 0xFF);
#elif FORCEMODE == PWM
					*(leverTaskPtr->dataRegister1) = leverForce;
#endif
					leverTaskPtr->iForce +=1;
				}
				// for now, comment out checking for goal position during perturbation so
				//if ((leverPosition <  leverTaskPtr -> goalBottom) || (leverPosition >  leverTaskPtr -> goalTop)){
				//	leverTaskPtr ->trialPos = -2; // trial ends here
				//	leverTaskPtr->nToFinish = leverTaskPtr->iPosition; // lever left goal area so that's that
				//}else{
				leverTaskPtr->iPosition +=1;
				//}
				break;
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
* 2019/06/07 by Jamie Boyd - adding code for motorDir
* 2019/03/05 by Jamie Boyd - fixing unsigned vs signed for 16 bit ints for position
* 2019/03/04 by Jamie Boyd - adding PWM option, plus some reorg of duplication in logic
*/
int leverThread_zeroLeverCallback (void * modData, taskParams * theTask){

	leverThreadStructPtr leverTaskPtr = (leverThreadStructPtr) theTask->taskData;
	int resetZero = *(int *) modData;
	// configure a time spec to sleep for 0.05 seconds
	struct timespec sleeper;
	sleeper.tv_sec = 0;
	sleeper.tv_nsec = 5e07;
	// divide force range into 20
	int dacMax, dacBase = leverTaskPtr->constForce;
	if (leverTaskPtr -> motorHasDirPin){
		setThreadlessGPIO (leverTaskPtr->motorDir, kLEVER_BACKWARDS);
		if (leverTaskPtr -> motorIsReversed){
			dacMax= 3500;
		}else{
			dacMax = 600;
		}
	}else{
		if (leverTaskPtr -> motorIsReversed){
			dacMax= 3800;
		}else{
			dacMax = 300;
		}
	}
	float dacIncr = (dacMax - dacBase)/20;
	int dacOut;
	int ii;
	int returnVal;
	// increment force gradually
	for (ii=0; ii < 20; ii +=1){
		dacOut = (int) (dacBase + (ii * dacIncr));
#if FORCEMODE == AOUT
		wiringPiI2CWriteReg8 (leverTaskPtr->i2c_fd, (dacOut  >> 8) & 0x0F, dacOut & 0xFF);
#elif FORCEMODE == PWM
		*(leverTaskPtr->dataRegister1) = dacOut;
#endif
		nanosleep (&sleeper, NULL);
	}
	// for returning the lever to previosly set 0 position, check that it is near original 0
	// Read Counter
	leverTaskPtr->spi_wpData[0] = kQD_READ_COUNTER;
	leverTaskPtr->spi_wpData[1] = 0;
	leverTaskPtr->spi_wpData[2] = 0;
	wiringPiSPIDataRW(kQD_CS_LINE, leverTaskPtr->spi_wpData, 3);
	if (leverTaskPtr -> isReversed){
		returnVal = (int) (65536 - ( 256 * leverTaskPtr->spi_wpData[1] + leverTaskPtr->spi_wpData[2]));
	}else{
		returnVal = (int) (256 * leverTaskPtr->spi_wpData[1] + leverTaskPtr->spi_wpData[2]);
	}

	if (resetZero){
		// clear counter
		leverTaskPtr->spi_wpData[0] = kQD_CLEAR_COUNTER;
		wiringPiSPIDataRW(kQD_CS_LINE, leverTaskPtr->spi_wpData, 1);
	}
	// set force on lever back to base force  = constant force
	dacOut = dacBase;
#if FORCEMODE == AOUT
	wiringPiI2CWriteReg8 (leverTaskPtr->i2c_fd, (dacOut  >> 8) & 0x0F, dacOut & 0xFF);
#elif FORCEMODE == PWM
	*(leverTaskPtr->dataRegister1) = dacOut;
#endif

	delete (int *) modData;
	return returnVal;
}

 /* ******************* ThreadMaker with Integer pulse duration, delay, and number of pulses timing description inputs ********************
* Last Modified:
* 2019/06/07 by Jamie Boyd - adding code for motorDir
* 2018/02/08 by Jamie Boyd - Initial Version */
leverThread * leverThread::leverThreadMaker (int16_t * positionData, unsigned int nPositionData, bool isCuedP, unsigned int nToGoalOrCircularP, int isReversed, int goalCuerPinOrZero, float cuerFreqOrZero, int MotorDirPinOrZero, int motorIsReversed, int startCuerPin, float startCuerFreq, float startCueTime, float timeBetweenTrials) {

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
	initStruct->timeBetweenTrials = timeBetweenTrials;
	initStruct->nForceData=kMAX_FORCE_ARRAY_SIZE;
	initStruct->nToGoalOrCircular = nToGoalOrCircularP;
	initStruct-> motorDirPin = MotorDirPinOrZero;
	initStruct -> motorIsReversed = motorIsReversed;
	std::cout << "START CUE PIN IN MAKER" << startCuerPin << "\n";
	initStruct -> startCuerPin = startCuerPin;
	initStruct -> startCuerFreq = startCuerFreq;
	initStruct -> startCueTime = startCueTime;
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
* This function sets the saved value, but does not apply the force. theForce ranges from 0 to 1, backwards direction is assumed
* Last Modified:
* 2019/06/07 by Jamie Boyd - adding code for motorDir, changed input to floating point scaled 0 to 1
* 2018/03/26 by Jamie Boyd - initial version */
void leverThread::setConstForce (float theForce){
	if (theForce < 0){
		theForce =0;
	}else{
		if (theForce > 1){
			theForce= 1;
		}
	}
	constantForce = theForce;
	int calcVal;
	if (taskPtr -> motorHasDirPin){ // no Force to max Force scaled from 0 to 4097
		calcVal = (int) (theForce * 4095);
	}else{
		if (taskPtr -> motorIsReversed){ // max Reverse force = 0, No force = 2047, max forward force =4095
			calcVal = 2048 + (int) (theForce * 2047);
		}else{
			calcVal = 2047 - (int) (theForce * 2047);
		}
	}
	taskPtr->constForce = calcVal;
}

/* ********************* returns the value for constant force, scaled from 0 to 1, backwards direction assumed ********************
* Last Modified:
* 2019/06/07 by Jamie Boyd - adding code for motorDir, changed return val to floating point scaled 0 to 1
* 2018/03/26 by Jamie Boyd - initial version */
float leverThread::getConstForce (void){
	return constantForce;
}

/* ********************** returns the truth that the leverThread is set to start froma cue ************************************* */
bool leverThread::isCued (void){
	return taskPtr->isCued;
}

/* ************* sets leverThread to start from a cue, if isCuedP is true, or to be uncued, if isCuedP is false**********************/
void leverThread::setCue (bool isCuedP){
	taskPtr->isCued =isCuedP ;
	if (isCuedP){
		modTrainLength (taskPtr->nToGoalOrCircular + taskPtr->nHoldTicks);
	}else{
		 modTrainLength (0);
	 }
}

/* ***************************************Gets current lever position from thread data, if trial is running*********************
else , reads the lever position directly and sets the value in the thread data
last modified:
2019/03/04 by Jamie Boyd - making lever position data 2 byte signed
2018/04/09 by Jamie Boyd - added code to check directly if no task in progress */
int16_t leverThread::getLeverPos (void){
	if (taskPtr->trialComplete){
		taskPtr->spi_wpData[0] = kQD_READ_COUNTER;
		taskPtr->spi_wpData[1] = 0;
		taskPtr->spi_wpData[2] = 0;
		wiringPiSPIDataRW(kQD_CS_LINE, taskPtr->spi_wpData, 3);
		int16_t leverPosition;
		if ( taskPtr-> isReversed){
			leverPosition = (int16_t)(65536 - (256 * taskPtr->spi_wpData[1] + taskPtr->spi_wpData[2]));
		}else{
			leverPosition = (int16_t)(256 * taskPtr->spi_wpData[1] + taskPtr->spi_wpData[2]);
		}
		taskPtr->leverPosition= leverPosition;
	}
	return taskPtr->leverPosition;
}

/* *********************** Set the number of ticks the mouse has to get lever into goal position to keep trial alive ******* */
void leverThread::setTicksToGoal (unsigned int ticksToGoal){
	taskPtr->nToGoalOrCircular = ticksToGoal;
}


/* ************ applies a given force, scaled from 0 to 1, with direction  ****************************
Last Modified:
* 2019/06/11 by Jamie Boyd - flipped motorIsReversed handling,
* 2019/06/07 by Jamie Boyd - added direction option, made theForce a float scaled from 0 to 1
* 2019/03/04 by Jamie Boyd - modified for PWM force option
*  2018/03/26 by Jamie Boyd - initial version */
void leverThread::applyForce (float theForce, int direction){
	if (theForce < 0){
		theForce =0;
	}else{
		if (theForce > 1){
			theForce = 1;
		}
	}
	int calcVal;
	if (taskPtr -> motorHasDirPin){ // no Force to max Force scaled from 0 to 4097
		calcVal = (int) (theForce * 4095);
		setThreadlessGPIO (taskPtr->motorDir, direction);
	}else{
		if (taskPtr -> motorIsReversed){
			if (direction == kLEVER_BACKWARDS){ // max Reverse force = 0, No force = 2047, max forward force =4095
				calcVal = 2048 + (int) (theForce * 2047);
			}else{
				calcVal = 2047 -  (int) (theForce * 2047);
			}
		}else{ // motor is not reversed
			if (direction == kLEVER_BACKWARDS){ // max Reverse force = 0, No force = 2047, max forward force =4095
				calcVal = 2047 - (int) (theForce * 2047);
			}else{
				calcVal = 2048 + (int) (theForce * 2047);
			}
		}
	}
	// write the data
#if FORCEMODE == AOUT
	wiringPiI2CWriteReg8(taskPtr->i2c_fd, (calcVal >> 8) & 0x0F, calcVal  & 0xFF);
#elif FORCEMODE == PWM
	*(taskPtr->dataRegister1) = calcVal;
#endif
}

/* ********************* Applies currently set value for constant force ****************************
* Last Modified:
* 2019/03/04 by Jamie Boyd - modified for PWM force option
*  2018/03/26 by Jamie Boyd - initial version */
void leverThread::applyConstForce (void){
	if (taskPtr-> motorHasDirPin){
		setThreadlessGPIO (taskPtr->motorDir, kLEVER_BACKWARDS);
	}
	int theForce =  taskPtr->constForce;
	// write the data
#if FORCEMODE == AOUT
	wiringPiI2CWriteReg8(taskPtr->i2c_fd, (theForce >> 8) & 0x0F, theForce  & 0xFF);
#elif FORCEMODE == PWM
	*(taskPtr->dataRegister1) = theForce;
#endif
}

/* ************************************** Zeroing the Lever ************************************************
Last Modified:
*  2019/06/10 by Jamie Boyd - changed check zero functionality to reset 0 functionality
*  2018/03/26 by Jamie Boyd - initial version */
int leverThread::zeroLever (int resetZero, int isLocking){

	int * modePtr = new int;
	* modePtr = resetZero;
	int returnVal = modCustom (&leverThread_zeroLeverCallback, (void * ) modePtr, isLocking);
	return returnVal;
}


/* *********************************** sets number of points  in force array used for sigmidal force ramp, max = kMAX_FORCE_ARRAY_SIZE
Last Modified 2019/05/13 by Jamie Boyd - initial version */
void leverThread::setPerturbLength (unsigned int perturbLength){

	if (perturbLength > kMAX_FORCE_ARRAY_SIZE){
		taskPtr->nForceData = kMAX_FORCE_ARRAY_SIZE;
	}else{
		taskPtr->nForceData =perturbLength;
	}
}

/* *********************************** returns number of points in force array used for  , max = kMAX_FORCE_ARRAY_SIZE
Last Modified 2019/05/13 by Jamie Boyd - initial version */
unsigned int leverThread::leverThread::getPerturbLength (void){

	return taskPtr->nForceData;
}


/* *********************************** Setting perturbation ***********************************
* fills the array with force data
* Last Modified:
* 2018/06/07 by Jamie Boyd - added code for motorDirPin, changed perturbForce to float
* 2018/04/09 by jamie Boyd - corrected for negative forces. We never go true negative, just subtract from constant force
* 2018/03/26 by Jamie Boyd - initial version */
void leverThread::setPerturbForce (float perturbForceP){
	// positive perturb force means adding to the back-directed constant force
	if (this->constantForce + perturbForceP > 1){
		this->perturbForce = 1 - this->constantForce;
	}else{
		if (this->constantForce + perturbForceP < 0){
			this->perturbForce = - (this->constantForce);
		}else{
			this->perturbForce = perturbForceP;
		}
	}
	float perturb = this->perturbForce;
	unsigned int iForce, nForces= taskPtr->nForceData ;
	float halfWay = (nForces -1)/2;
	float rate = (nForces -1)/10;
	float base = this->constantForce;
	float theForce;
	int calcVal;
 #if beVerbose
	printf ("Perturbforce = %.3f; constant Force = %.3f;\n" , this->perturbForce, this->constantForce);
	printf ("force array:\n");
#endif
	for (iForce =0; iForce <  nForces-1; iForce +=1){
		theForce= (base + perturb/(1 + exp (-(float) (iForce - halfWay)/rate)));
		if (taskPtr -> motorHasDirPin){ // no Force to max Force scaled from 0 to 4097
			calcVal = (int) (theForce * 4095);
		}else{
			if (taskPtr -> motorIsReversed){
				calcVal = 2048 + (int) (theForce * 2047);
			}else{ // motor is not reversed
				calcVal = 2047 - (int) (theForce * 2047);
			}
		}
		taskPtr->forceData [iForce] = calcVal;
 #if beVerbose
		printf ("%.4f (%i), ", theForce, calcVal);
#endif
	}
	theForce = base + perturb;
	if (taskPtr -> motorHasDirPin){ // no Force to max Force scaled from 0 to 4097
		calcVal = (int) (theForce * 4095);
	}else{
		if (taskPtr -> motorIsReversed){
			calcVal = 2048 + (int) (theForce * 2047);
		}else{ // motor is not reversed
			calcVal = 2047 - (int) (theForce * 2047);
		}
	}
	taskPtr->forceData [iForce] = calcVal;
 #if beVerbose
	printf ("%.4f (%i), ", theForce, calcVal);
	printf ("\n");
#endif
}

/* ************************ Sets position where perturb force is applied *****************
last modified 2018/03/26 by Jamie Boyd - initial version */
void leverThread::setPerturbStartPos(unsigned int perturbStartPos){
	taskPtr->forceStartPos =  taskPtr->nToGoalOrCircular + perturbStartPos;
}

/* **************************** Turns perturbation off **************************************
 * Last Modified:
 * 2019/03/05 by Jamie Boyd */
void leverThread::setPerturbOff (void){
	taskPtr->forceStartPos = taskPtr->nPositionData;
}

/* ******************************************************* Starting, Stopping, Checking Trials ****************************************************
Starts a trial, either cued or un-cued
last modified 2018/03/26 by Jamie Boyd - initial version */
void leverThread::startTrial (void){
	taskPtr->iPosition =0;
	taskPtr->iForce = 0;
	taskPtr->trialPos =0;
	taskPtr->trialComplete =false;
	taskPtr->inGoal=false;
	taskPtr->nToFinish = taskPtr->nToGoalOrCircular + taskPtr->nHoldTicks;
	taskPtr->breakPos = taskPtr->nToFinish + 1;
	if (taskPtr->isCued){
		std::cout << "Is Cued" << "\n";
		if (taskPtr->startCuer != nullptr){
				std::cout << "Cue play" << "\n";
				if (taskPtr->startMode == kGOALMODE_HILO){
					taskPtr->startCuer->setLevel(1,1);
					std::this_thread::sleep_for(std::chrono::milliseconds((int) (taskPtr->startCueTime*1000)));
					taskPtr->startCuer->setLevel(0,1);
				}else{
					taskPtr->startCuer->startInfiniteTrain ();
					std::this_thread::sleep_for(std::chrono::milliseconds((int) (taskPtr->startCueTime*1000)));
					taskPtr->startCuer->stopInfiniteTrain ();
				}
				std::cout << "Cue End" << "\n";
			}

		modTrainLength (taskPtr->nToFinish);
		DoTask ();
	}else{
		for (unsigned int iPosition =0;iPosition < taskPtr->nToGoalOrCircular; iPosition +=1){
			taskPtr->positionData [iPosition] = 0;
		}
		startInfiniteTrain();
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
		} else {
			UnDoTasks();
		        std::this_thread::sleep_for(std::chrono::milliseconds((int) (taskPtr->timeBetweenTrials*1000)));
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
void leverThread::setHoldParams (int16_t goalBottomP, int16_t goalTopP, unsigned int nHoldTicksP){
	taskPtr->goalBottom =goalBottomP;
	taskPtr->goalTop = goalTopP;
	taskPtr->nHoldTicks = nHoldTicksP;
}
