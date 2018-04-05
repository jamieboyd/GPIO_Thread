
#include "CountermandPulse.h"

/* ***************** countermandable Hi Callback ******************************
Task to do on High tick, sets GPIO line high or low depending on polarity
unless countermanded
last modified:
2018/04/04 by Jamie Boyd - added Lo func and wasCountermanded boolean
2018/03/16 by Jamie Boyd - initial version */
void Countermand_Hi (void *  taskData){
	CountermandPulseStructPtr cmpTaskPtr = (CountermandPulseStructPtr) taskData;
	if (cmpTaskPtr->countermand == 1){ // countermand requested
		cmpTaskPtr->countermand = 2; // countermand in progress
	}else{
		*(cmpTaskPtr->GPIOperiHi) = cmpTaskPtr->pinBit;
	}
}

/* ********************************** countermand low callback *****************************************************
if countermand = 0, normal pulse. if countermand =1, was asked to be countermanded, but too late for countermand_Hi
to countermand it, if countermand is 2, then pulse was countermanded by countermand_Hi */
void Countermand_Lo (void *  taskData){
	CountermandPulseStructPtr cmpTaskPtr = (CountermandPulseStructPtr) taskData;
	if (cmpTaskPtr->countermand == 2){ // if 2, countermand in progress, no need to set low
		cmpTaskPtr->wasCountermanded= true;
	}else{
		*(cmpTaskPtr->GPIOperiLo) = cmpTaskPtr->pinBit;
		cmpTaskPtr->wasCountermanded= false;
	}
	cmpTaskPtr->countermand = 0; // reset countermand to 0
}

/* **************** modFunc that sets taskData to a new CountermandPulse struct and copies over data from original *********************
SimpleGPIO struct. Needed because we want to call SimpleGPIO_thread constructor 
last modified:
2018/04/04 by Jamie Boyd - initial version */
int countermandSetTaskData (void * modData, taskParams * theTask){
	CountermandPulseStructPtr newTaskData= new CountermandPulseStruct;
	SimpleGPIOStructPtr oldTaskData = (SimpleGPIOStructPtr)theTask->taskData;
	newTaskData->GPIOperiHi = oldTaskData->GPIOperiHi;
	newTaskData->GPIOperiLo = oldTaskData->GPIOperiLo;
	newTaskData->pinBit = oldTaskData->pinBit;
	newTaskData->countermand = 0;
	newTaskData->wasCountermanded = false;
	delete oldTaskData;
	theTask->taskData = (void *)newTaskData;
	return 0;
}


/* ****************************** Custom delete Function *****************************************************/
void CountermandPulse_delTask (void * taskData){
	delete (CountermandPulseStructPtr) taskData;
}


/* *************** Thread maker makes SimpleGPIO init data struct, calls constructor, and does some postcreation modification *************
Last Modified:
2018/04/04 by Jamie Boyd - added modCustom for setting extra data
2018/03/16 by Jamie Boyd  - initial version */
CountermandPulse * CountermandPulse::CountermandPulse_threadMaker (int pin, int polarity, unsigned int delayUsecs, unsigned int durUsecs, int accuracyLevel){
	// make and fill an init struct for SimpleGPIO
	SimpleGPIOInitStructPtr initStruct = new SimpleGPIOInitStruct;
	initStruct->thePin = pin;
	initStruct->thePolarity = polarity;
	initStruct->GPIOperiAddr =  useGpioPeri ();
	if (initStruct->GPIOperiAddr == nullptr){
#if beVerbose
		printf ("CountermandPulse_threadMaker failed to map GPIO peripheral.\n");
#endif
		return nullptr;
	}
	int errCode =0;
	// call CountermandPulse constructor, which calls SimpleGPIO_thread contructor, which calls pulsedThread constructor
	CountermandPulse * newCountermandPulse = new CountermandPulse (pin, polarity, delayUsecs, durUsecs, (void *) initStruct, accuracyLevel, errCode);
	if (errCode){
#if beVerbose
		printf ("CountermandPulse_threadMaker Failed to make pulsed thread.\n");
#endif
		return nullptr;
	}
	// set countermanable high and lo functions in place of regular hi and lo functions
	newCountermandPulse->setHighFunc (&Countermand_Hi);
	newCountermandPulse->setLowFunc (&Countermand_Lo);
	// set custom delete function for task data
	newCountermandPulse->setTaskDataDelFunc (&CountermandPulse_delTask);
	// replace SimpleGPIOStruct with CountermandPulseStruct, which has an added field at the end
	newCountermandPulse->modCustom (countermandSetTaskData, nullptr, 0);
	// make a task pointer for easy direct access to thread task data for countermanding without benefit of mutex
	newCountermandPulse->taskStructPtr = (CountermandPulseStructPtr)newCountermandPulse->getTaskData ();
	return newCountermandPulse;
}

/* *************************** to countermand, just set countermand in the task pointer to 1, and current pulse will be countermanded
IF countermand is received before delay is up
Last Modified:
2018/04/04 by Jamie Boyd - initial version*/
void CountermandPulse::countermand (void){
	if (taskStructPtr->countermand== 0){
		taskStructPtr->countermand = 1;
	}
}

/* ************************ returns truth that last pulse was countermanded ********************************
Last Modified:
2018/04/04 by Jamie Boyd - initial version */
bool CountermandPulse::wasCountermanded (void){
	return taskStructPtr->wasCountermanded;
}