
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
	wiringPiI2CWrite (taskData->i2c_fd, kDAC_WRITEDAC); // leave it in DAC mode
	//leverTaskPtr->leverForce =leverTaskPtr->forceData[leverTaskPtr->iForce] ;
	wiringPiI2CWriteReg8(taskData->i2c_fd, (2048  >> 8) & 0x0F, 2048 & 0xFF);
	
	
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
		}else{
			taskData->goalCuer = SimpleGPIO_thread::SimpleGPIO_threadMaker (initDataPtr->goalCuerPin, 1, (float) initDataPtr->cuerFreq , (float) 0.5, (float) 0, 1);
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
	taskData->forceStartPos = initDataPtr->nForceData -100;

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
	if (leverTaskPtr->iPosition < leverTaskPtr-> nToFinish){
		leverTaskPtr->positionData [leverTaskPtr->iPosition] = leverPosition;
		// light the lamp, or sound the horn
		if (leverTaskPtr->goalCuer != nullptr){
			if ((leverTaskPtr->inGoal== false)&&((leverPosition > leverTaskPtr->goalBottom)&&(leverPosition < leverTaskPtr->goalTop))){
				leverTaskPtr->inGoal = true;
				if (leverTaskPtr->goalMode == 1){
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
			// write the DAC constant 
			wiringPiI2CWrite (leverTaskPtr->i2c_fd, kDAC_WRITEDAC); // put it in DAC mode
			// write the data
			int leverForce =leverTaskPtr->forceData[leverTaskPtr->iForce] ;
			wiringPiI2CWriteReg8(leverTaskPtr->i2c_fd, (leverForce  >> 8) & 0x0F, leverForce  & 0xFF);
			leverTaskPtr->iForce +=1;
			if (leverTaskPtr->iForce == leverTaskPtr->nForceData){ // all out of forces but leave force at final ramp pos until the end of the trial
				leverTaskPtr->doForce = false;
			}
		}
		// trial position specific stuff
		if (leverTaskPtr -> trialPos ==0){// 0 means lever not moved into goal area yet
			if (leverTaskPtr->isCued){
				if (leverPosition >  leverTaskPtr -> goalBottom){
					leverTaskPtr -> trialPos = 1;
				}else{
					if (leverTaskPtr->iPosition ==  leverTaskPtr->nToGoal){
						leverTaskPtr -> trialPos = -1;
					}
				}
			}else { // uncued trial
				if (leverPosition >  leverTaskPtr -> goalBottom){
					leverTaskPtr->circularBreak = leverTaskPtr->iPosition;
					leverTaskPtr->iPosition =  leverTaskPtr->nCircular; 
					leverTaskPtr ->trialPos =1;
				}else{
					// check for wraparound of circular buffer
					if (leverTaskPtr->iPosition ==  leverTaskPtr->nCircular){
						leverTaskPtr->iPosition = 1;
					}
				}
			}
		} else{
			if (leverTaskPtr ->trialPos ==1){ // check if we are still in goal range 
				if (leverTaskPtr->inGoal = false){
					leverTaskPtr ->trialPos = -1;
				}
			}
		}
	}
	// increment position and see if we are done
	leverTaskPtr->iPosition +=1;
	if (leverTaskPtr->iPosition == nToFinish){
		if (leverTaskPtr ->trialPos == 1){
			leverTaskPtr ->trialPos = 2;
		}else{
			leverTaskPtr ->trialPos = -2;
		}
		int leverForce =leverTaskPtr->constForce;
		wiringPiI2CWriteReg8(leverTaskPtr->i2c_fd, (leverForce  >> 8) & 0x0F, leverForce  & 0xFF);
	}
}


/* ************* Custom task data delete function *********************/
void leverThread_delTask (void * taskData){
	leverThreadStructPtr taskPtr = (leverThreadStructPtr) taskData;
	delete (taskPtr->forceData);
	delete (taskPtr);
}


typedef struct pertubSetStruct{
	unsigned int forceStartPos;
	float addedForce;
}pertubSetStruct, *perturbSetStructPtr;


/* ************************* Callbacks for thread *************************/
int leverThread_setPerturbCallback (void * modData, taskParams * theTask){
	perturbSetStructPtr perturbData= (perturbSetStructPtr)modData;
	leverThreadStructPtr taskPtr = (leverThreadStructPtr) theTask->taskData;
	float halfWay = taskPtr->nForceData/2;
	float rate = taskPtr->nForceData/10;
	float base = taskPtr->constForce;
	for (unsigned int iPt =0; iPt <  taskPtr->nForceData; iPt +=1){
		taskPtr->forceData [iPt] = (int) (base + perturbData->addedForce/(1 + exp (-(iPt - halfWay)/rate)));
	}
	taskPtr->forceStartPos =  perturbData->forceStartPos;
	return 0;
}

/* mod data is an integer for constant force */
int leverThread_setConstForceCallback (void * modData, taskParams * theTask){
	leverThreadStructPtr taskPtr = (leverThreadStructPtr) taskParams->taskData;
	int * newConstForce= (int * )modData;
	// write the DAC constant 
	wiringPiI2CWrite (leverTaskPtr->i2c_fd, kDAC_WRITEDAC); // put it in DAC mode
	// write the data
	leverTaskPtr->leverForce = * newConstForce ;
	wiringPiI2CWriteReg8(leverTaskPtr->i2c_fd, (leverTaskPtr->leverForce  >> 8) & 0x0F, leverTaskPtr->leverForce  & 0xFF);
	delete (newConstForce);
	return 0;
}

/* ************************** zero Lever ***********************************************/
int leverThread_zeroLeverCallback (void * modData, taskParams, * theTask){
	
	leverThreadStructPtr taskPtr = (leverThreadStructPtr) taskParams->taskData;
	// configure a time spec to sleep for 0.1 seconds
	struct timespec sleeper;
	sleeper.tv_sec = 0;
	sleeper.tv_nsec = 1e08;
	
	// clear counter
	spi_wpData[0] = kQD_CLEAR_COUNTER;
	wiringPiSPIDataRW(kQD_CS_LINE, taskData->spi_wpData, 1);
	

	uint8_t prevLeverPos = 255, leverPos;
	// make values for motor, and set inital value
	int dacBase = 500;
	float dacIncr = (4095 - dacBase)/10;
	int dacOut;
	bool returnVal = false;
	for (int ii =0; ii < 10; ii +=1){
		// set a value and wait,
		dacOut = (uint16_t) (dacBase + (ii * dacIncr));
		wiringPiI2CWriteReg8(leverTaskPtr->fd, (dacOut  >> 8) & 0x0F, dacOut & 0xFF);
		nanosleep (&sleeper, NULL) ;
		// check new position, see if we are moving
		
		leverPos= decoder->readOneByteCounter ();
		if ((leverPos < prevLeverPos + 2) && (leverPos > prevLeverPos - 2)){
			// we have stopped moving, zero decoder and break out of loop, with returnVal true
			decoder->clearCounter();
			returnVal= true;
			break;
		}else // we hvae not stopped moving, save new lever pos
			prevLeverPos = leverPos;
			
	}
	// zero lever force, whatever the outcome
	if (hasMotor)
		DAC->setValue (0);
	if (returnVal == false)
		printf ("Could not find zero position for lever.\n");
	return returnVal;
}


	
	
/* *************************** leverThread Class Methods *******************************************************
 
 ******************** ThreadMaker with Integer pulse duration, delay, and number of pulses timing description inputs ********************
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


/* mod data is an integer for constant force */
int leverThread_setConstForceCallback (void * modData, taskParams * theTask){
	leverThreadStructPtr taskPtr = (leverThreadStructPtr) theTask->taskData;
	int newConstForce= *(int * )modData;
	// write the DAC constant 
	wiringPiI2CWrite (taskPtr->i2c_fd, kDAC_WRITEDAC); // put it in DAC mode
	// write the data
	taskPtr->constForce = newConstForce;
	wiringPiI2CWriteReg8(taskPtr->i2c_fd, (newConstForce >> 8) & 0x0F, newConstForce  & 0xFF);
	delete ((int * )modData);
	return 0;
}

int leverThread::setConstForce (int theForce, int isLocking){
	int * newForceVal = new int;
	* newForceVal =  theForce;
	int returnVal = modCustom (&leverThread_setConstForceCallback, (void *) newForceVal, isLocking);	
	return returnVal;
}
	

int leverThread::getConstForce (void){
	return taskPtr->constForce;
}	


void leverThread::startTrial (void){
	taskPtr->iPosition =0;
	taskPtr->trialPosition =0;
	taskPtr->inGoal=false;
	leverTaskPtr->doForce = false;
	if (leverTaskPtr->isCued){
		DoTask ();
	}else{
		startInfiniteTrain ();
	}
}