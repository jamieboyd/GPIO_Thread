
#include "lever_thread.h"

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
	leverTaskPtr->leverPosition= leverTaskPtr->spi_wpData[1];
	//printf ("Lever position = %d.\n", leverTaskPtr->leverPosition);
	// if we are at the end of a trial, then don't overwrite any data
	if (leverTaskPtr->iPosition < leverTaskPtr-> nPositionData){
		leverTaskPtr->positionData [leverTaskPtr->iPosition] = leverTaskPtr->leverPosition;
		// light the lamp, or sound the horn
			if (leverTaskPtr->goalCuer != nullptr){

		if ((leverTaskPtr->inGoal== false)&&((leverTaskPtr->leverPosition > leverTaskPtr->goalBottom)&&(leverTaskPtr->leverPosition < leverTaskPtr->goalTop))){
			leverTaskPtr->inGoal = true;
			if (leverTaskPtr->goalMode == 1){
				leverTaskPtr->goalCuer->setLevel(1,1);
			}else{
				leverTaskPtr->goalCuer->startInfiniteTrain ();
			}
		}else{
			if ((leverTaskPtr->inGoal== true)&&((leverTaskPtr->leverPosition < leverTaskPtr->goalBottom) || (leverTaskPtr->leverPosition > leverTaskPtr->goalTop))){
				leverTaskPtr->inGoal = false;
				if (leverTaskPtr->goalMode == 1){
					leverTaskPtr->goalCuer->setLevel(0,1);
				}else{
					leverTaskPtr->goalCuer->stopInfiniteTrain ();
				}
			}
		}
	}
		// check for breakout of circular buffer
		uint8_t lastLeverPos;
		if (leverTaskPtr->iPosition == 0){
			lastLeverPos = leverTaskPtr->positionData [leverTaskPtr->circularBreak - 1];
		}else{
			lastLeverPos = leverTaskPtr->positionData  [(leverTaskPtr->iPosition)-1];
		}
		if ((lastLeverPos <  leverTaskPtr->leverTrialStartPos) && (leverTaskPtr->leverPosition >  leverTaskPtr->leverTrialStartPos)){
			leverTaskPtr->circularBreak = leverTaskPtr->iPosition;
			leverTaskPtr->iPosition = leverTaskPtr->nCircular;
		}
		// check for seting force
		if (leverTaskPtr->iPosition == leverTaskPtr->forceStartPos){
			leverTaskPtr->doForce = true;
			leverTaskPtr->iForce =0;
		}
		if (leverTaskPtr->doForce){
			// write the DAC constant 
			wiringPiI2CWrite (leverTaskPtr->i2c_fd, kDAC_WRITEDAC); // put it in DAC mode
			// write the data
			leverTaskPtr->leverForce =leverTaskPtr->forceData[leverTaskPtr->iForce] ;
			wiringPiI2CWriteReg8(leverTaskPtr->i2c_fd, (leverTaskPtr->leverForce  >> 8) & 0x0F, leverTaskPtr->leverForce  & 0xFF);
			leverTaskPtr->iForce +=1;
			if (leverTaskPtr->iForce == leverTaskPtr->nForceData){ // all out of forces but leave force at final ramp pos until the end of the trial
				leverTaskPtr->doForce = false;
			}
		}
		leverTaskPtr->iPosition +=1;
	}else{ // we are at end of a trial.
		// 
	}
}


/* ************* Custom task data delete function *********************/
void lever_thread_delTask (void * taskData){
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
*/


/* *************************** lever_thread Class Methods *******************************************************
 
 ******************** ThreadMaker with Integer pulse duration, delay, and number of pulses timing description inputs ********************
 Last Modified:
 2018/02/08 by Jamie Boyd - Initial Version */
lever_thread * lever_thread::lever_threadMaker (uint8_t * positionData, unsigned int nPositionData, unsigned int nCircular, int goalCuerPin, float cuerFreq) {
	
	// make and fill a leverTask struct
	leverThreadInitStructPtr initStruct = new leverThreadInitStruct;
	initStruct->positionData = positionData;
	initStruct->nPositionData = nPositionData;
	initStruct->nCircular = nCircular;
	initStruct->goalCuerPin = goalCuerPin;
	initStruct->cuerFreq = cuerFreq;
	
	// call class constructor which  calls pulsedThread constructor
	int errCode;
	lever_thread * newLever = new lever_thread ((void *) initStruct, errCode);
	if (errCode){
#if beVerbose
		printf ("lever_thread_threadMaker failed to make lever_thread object.\n");
#endif
		return nullptr;
	}
	// set custom task delete function
	newLever->setTaskDataDelFunc (&lever_thread_delTask);
	// make a lever_thread pointer for easy direct access to thread task data 
	newLever->taskPtr = (leverThreadStructPtr)newLever->getTaskData ();
	return newLever;

}




void lever_thread::setConstForce (int theForce){
	
}
	

int lever_thread::getConstForce (void){
	return taskPtr->constForce;
}	


