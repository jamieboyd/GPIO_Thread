#include "SimpleGPIO_thread.h"
#include <math.h>
/* ***************************have to initialize static data outside of the class *************************************/
bcm_peripheralPtr SimpleGPIO_thread::GPIOperi = nullptr;
int SimpleGPIO_thread::GPIOperi_users=0;

/* *************************** Functions that are called from thread can not be class methods *********************

***************************************Initialization callback function ****************************************
Copies pinBit and set/unset register adressses to  task data
last modified:
2018/02/01 by Jamie Boyd - initial version  */
int SimpleGPIO_Init (void * initDataP, void *  &taskDataP){
	
	// task data pointer is a void pointer that needs to be initialized to a pointer to taskCustomData and filled from our custom init structure 
	SimpleGPIOStructPtr taskData  = new SimpleGPIOStruct;
	taskDataP = taskData;
	// initData is a pointer to our custom init structure
	SimpleGPIOInitStructPtr initData = (SimpleGPIOInitStructPtr) initDataP;
	// copy pin number from init Data to taskData
	taskData->pinBit = initData->pinBit;
	taskData->GPIOperiHi = initData->GPIOperiHi;
	taskData->GPIOperiLo = initData->GPIOperiLo;
	return 0; // 
}

/* ***************** Lo Callback ******************************
 Task to do on Low tick, sets GPIO line low or high depending on polarity
last modified:
2016/12/07 by Jamie Boyd - initial version */
void SimpleGPIO_Lo (void *  taskData){
	SimpleGPIOStructPtr gpioTaskPtr = (SimpleGPIOStructPtr) taskData;
	*(gpioTaskPtr->GPIOperiLo) = gpioTaskPtr->pinBit;
}

/* ***************** Hi Callback ******************************
Task to do on High tick, sets GPIO line high or ow depending on polarity
last modified:
2016/12/07 by Jamie Boyd - initial version */
void SimpleGPIO_Hi (void *  taskData){
	SimpleGPIOStructPtr gpioTaskPtr = (SimpleGPIOStructPtr) taskData;
	*(gpioTaskPtr->GPIOperiHi) =gpioTaskPtr->pinBit;
}

/* **************** Custom data mod callbacks *****************************

********************Changes GPIO pin bit********************************
modData is a pointer to an int for the new pin bit to use
last modified:
2018/02/02 by Jamie Boyd - initial version */
int SimpleGPIO_setPinCallback (void * modData, taskParams * theTask){
	SimpleGPIOStructPtr gpioTaskPtr = (SimpleGPIOStructPtr)theTask->taskCustomData;
	gpioTaskPtr->pinBit = *(int *)modData;
	delete ((int *)modData);
	return 0;
}

/*********************** Writes to the Hi or Lo address, as appropriate ************************* 
modData is a pointer to an int, 0 for low function, non-zero for high function. If polarity of pulse is reversed,
so also will be notion of setting hi vs setting lo 
last modified:
2018/02/02 by Jamie Boyd - initial version */
int SimpleGPIO_setLevelCallBack (void * modData, taskParams * theTask){
	int theLevel = * (int *)modData;
	SimpleGPIOStructPtr gpioTaskPtr = (SimpleGPIOStructPtr)theTask->taskCustomData;		
	if (theLevel ==0){
		*(gpioTaskPtr->GPIOperiLo) = gpioTaskPtr->pinBit;
	}else{
		*(gpioTaskPtr->GPIOperiHi) = gpioTaskPtr->pinBit;
	}
	delete ((int *)modData);
	return 0;
}

/* *************** delete function for custom task data ***********************************
Called when pulsedThread is killed, if you explicitly install it with pulsedThread setCustomDataDelFunc
last modified:
2017/12/06 by Jamie Boyd - initial version */
 void SimpleGPIO_customDel(void *  taskData){
	 SimpleGPIOStructPtr gpioTaskPtr = (SimpleGPIOStructPtr) taskData;
	 delete (gpioTaskPtr);
	 taskData = nullptr;
#if beVerbose
	 printf("Many are made but fewer are deleted\n");
#endif
 }

 

 /* *************************************CallBacks for EndFunc Options *************
Functions for using an array of values for frequency or duty cycle with an endFunc to cycle through the array using SimpleGPIOArrayStruct

***************************** Custom dataMod Callback**********************************
 Sets up the array of data used to output new values for frequency, duty cycle
Use with pulsedThread::modCustom
last modified:
2017/03/02 by Jamie Boyd - initial version */
int SimpleGPIO_setUpArrayCallback (void * modData, taskParams * theTask){
	// cast modData to SimpleGPIOArrayStructPtr
	SimpleGPIOArrayStructPtr modDataP = (SimpleGPIOArrayStructPtr)modData;
	// get the pointer to endFuncData from theTask and make a new SimpleGPIOArrayStruct for it
	SimpleGPIOStructPtr taskCustomDataPtr = (SimpleGPIOStructPtr)theTask->taskCustomData;
	SimpleGPIOArrayStructPtr endFuncDataPtr = (SimpleGPIOArrayStructPtr) taskCustomDataPtr->endFuncData;
	endFuncDataPtr= new SimpleGPIOArrayStruct;
	taskCustomDataPtr->endFuncData = endFuncDataPtr;
	endFuncDataPtr -> arrayData = modDataP->arrayData; // Don't copy data, just pointer to the data. So calling function must not delete arrayData while the thread is active
	endFuncDataPtr -> nData = modDataP->nData; // number of points in the array
	endFuncDataPtr -> arrayPos = modDataP->arrayPos; // position in the array to output
	delete ((SimpleGPIOArrayStructPtr)modData);
	return 0;
} 

void SimpleGPIO_FreqFromArrayEndFunc (taskParams * theTask){
	/*  endFuncData is SimpleGPIOArrayStructPtr includes
	float * arrayData
	unsigned int nData
	unsigned int arrayPos
	*/
	SimpleGPIOStructPtr taskData = (SimpleGPIOStructPtr)theTask->taskCustomData;
	SimpleGPIOArrayStructPtr arrayStructPtr = (SimpleGPIOArrayStructPtr)taskData->endFuncData;
	arrayStructPtr->arrayPos +=1;
	if(arrayStructPtr->arrayPos == arrayStructPtr->nData){
		arrayStructPtr->arrayPos =0;
	}
	times2Ticks (arrayStructPtr->arrayData[arrayStructPtr->arrayPos], theTask->trainDutyCycle, theTask->trainDuration, *theTask);
	theTask->trainFrequency = arrayStructPtr->arrayData[arrayStructPtr->arrayPos];
	theTask->doTask |= (kMODDUR | kMODDELAY);
}

void SimpleGPIO_DutyCycleFromArrayEndFunc (taskParams * theTask){
	/*  customData struct includes
	float * arrayData
	unsigned int nData
	unsigned int arrayPos
	*/
	SimpleGPIOStructPtr taskData = (SimpleGPIOStructPtr)theTask->taskCustomData;
	SimpleGPIOArrayStructPtr arrayStructPtr = (SimpleGPIOArrayStructPtr)taskData->endFuncData;
	arrayStructPtr->arrayPos +=1;
	if(arrayStructPtr->arrayPos == arrayStructPtr->nData){
		arrayStructPtr->arrayPos =0;
	}
	times2Ticks (theTask->trainFrequency, arrayStructPtr->arrayData[arrayStructPtr->arrayPos], theTask->trainDuration, *theTask);
	theTask->trainDutyCycle = arrayStructPtr->arrayData[arrayStructPtr->arrayPos];;
	theTask->doTask |= (kMODDUR | kMODDELAY);
}
 
 
 /* **************************************************
 * delete function for custom task data ONLY for a task using SimpleGPIOArrayStruct,
 *  called when pulsedThread is killed if you explicitly install it with
 *  pulsedThread setCustomDataDelFunc
 * last modified:
 * 2017/12/06 by Jamie Boyd - initial version */
 void SimpleGPIO_ArrayStructCustomDel(void * taskData){
	 SimpleGPIOStructPtr gpioTaskPtr = (SimpleGPIOStructPtr) taskData;
	 /* we assume that the endFunc data is a SimpleGPIOArrayStruct*/
	 if (gpioTaskPtr->endFuncData != nullptr){
		 SimpleGPIOArrayStructPtr arrayData = (SimpleGPIOArrayStructPtr)gpioTaskPtr->endFuncData;
		 delete (arrayData);
	 }
	 SimpleGPIO_customDel (taskData);
 }
 
 
 /* *************************** SimpleGPIO_thread Class Methods *******************************************************
 
 ******************** ThreadMaker with Integer pulse duration, delay, and number of pulses timing description inputs ********************
 Last Modified:
 2018/02/01 by Jamie Boyd - Initial Version */
SimpleGPIO_thread * SimpleGPIO_thread::SimpleGPIO_threadMaker (int thePin, int polarity, unsigned int delayUsecs, unsigned int  durUsecs, unsigned int nPulses, int accuracyLevel) {
	int errCode;
	// make and fill an init struct
	SimpleGPIOInitStruct initStruct;
	// map GPIO peripheral, if needed
	if (GPIOperi_users ==0) {
		GPIOperi = new bcm_peripheral {GPIO_BASE};
		errCode = map_peripheral(GPIOperi, IFACE_DEV_GPIOMEM);
		if (errCode){
			GPIOperi  = nullptr;
			return nullptr;
		}
	}
	// initialize pin for output
	*(GPIOperi->addr + ((thePin) /10)) &= ~(7<<(((thePin) %10)*3));
	*(GPIOperi->addr + ((thePin)/10)) |=  (1<<(((thePin)%10)*3));
	// save address to ON and OFF register as Hi or Lo as appropriate to save a level of indirection later
	if (polarity == 1){ // High to Low pulses
		initStruct.GPIOperiHi = (unsigned int *) GPIOperi->addr + 10;
		initStruct.GPIOperiLo = (unsigned int *) GPIOperi->addr + 7;
	}else{ // low to high pulses
		initStruct.GPIOperiHi = (unsigned int *) GPIOperi->addr + 7;
		initStruct.GPIOperiLo = (unsigned int *) GPIOperi->addr + 10;
	}
	// calculate pinBit
	initStruct.pinBit =  1 << thePin;
	// put pin in selected start state
	*(initStruct.GPIOperiLo ) =initStruct.pinBit ;
	// call SimpleGPIO_thread constructor, which just calls pulsedThread contructor
	// call SimpleGPIO_thread constructor, which just calls pulsedThread contructor
	SimpleGPIO_thread * newGPIO_thread = new SimpleGPIO_thread (delayUsecs, durUsecs, nPulses, (void *) &initStruct, &SimpleGPIO_Init, &SimpleGPIO_Lo, &SimpleGPIO_Hi, accuracyLevel, errCode);
	if (errCode){
#if beVerbose
		printf ("Failed to make pulsed thread.\n");
#endif
		return nullptr;
	}
	// fill in extra data fields
	newGPIO_thread->pinNumber = thePin;
	newGPIO_thread->polarity = polarity;
	 // increment static GPIOperi_users . When destructing, if no other users, delete the mapping
	GPIOperi_users +=1;
	newGPIO_thread->setCustomDataDelFunc (&SimpleGPIO_customDel);
	return newGPIO_thread;
}

/******************** ThreadMaker with floating point frequency, duration, and duty cycle timing description inputs ********************
Last Modified:
2018/02/01 by Jamie Boyd - Initial Version */
SimpleGPIO_thread * SimpleGPIO_thread::SimpleGPIO_threadMaker (int thePin, int polarity, float frequency, float dutyCycle, float trainDuration, int accuracyLevel){
	int errCode;
	// make and fill an init struct
	SimpleGPIOInitStruct initStruct;
	// map GPIO peripheral, if needed
	if (GPIOperi_users ==0) {
		GPIOperi = new bcm_peripheral {GPIO_BASE};
		errCode = map_peripheral(GPIOperi, IFACE_DEV_GPIOMEM);
		if (errCode){
			GPIOperi  = nullptr;
			return nullptr;
		}
	}
	// initialize pin for output
	*(GPIOperi->addr + ((thePin) /10)) &= ~(7<<(((thePin) %10)*3));
	*(GPIOperi->addr + ((thePin)/10)) |=  (1<<(((thePin)%10)*3));
	// save address to ON and OFF register as Hi or Lo as appropriate to save a level of indirection later
	if (polarity == 1){ // High to Low pulses
		initStruct.GPIOperiHi = (unsigned int *) GPIOperi->addr + 10;
		initStruct.GPIOperiLo = (unsigned int *) GPIOperi->addr + 7;
	}else{ // low to high pulses
		initStruct.GPIOperiHi = (unsigned int *) GPIOperi->addr + 7;
		initStruct.GPIOperiLo = (unsigned int *) GPIOperi->addr + 10;
	}
	// calculate pinBit
	initStruct.pinBit =  1 << thePin;
	// put pin in selected start state
	*(initStruct.GPIOperiLo ) =initStruct.pinBit ;
	// call SimpleGPIO_thread constructor, which just calls pulsedThread contructor
	SimpleGPIO_thread * newGPIO_thread = new SimpleGPIO_thread (frequency, dutyCycle, trainDuration, (void *) &initStruct, &SimpleGPIO_Init, &SimpleGPIO_Lo, &SimpleGPIO_Hi, accuracyLevel, errCode);
	if (errCode){
#if beVerbose
		printf ("Failed to make pulsed thread.\n");
#endif
		return nullptr;
	}
	// fill in extra data fields
	newGPIO_thread->pinNumber = thePin;
	newGPIO_thread->polarity = polarity;
	newGPIO_thread->endFuncArrayData = nullptr;
	 // increment static GPIOperi_users . When destructing, if no other users, delete the mapping
	GPIOperi_users +=1;
	newGPIO_thread->setCustomDataDelFunc (&SimpleGPIO_customDel);
	return newGPIO_thread;
}

/* ****************************** Destructor handles GPIO peripheral mapping*************************
Thread data is destroyed by the pulsedThread destructor
Last Modified:
2018/02/01 by Jamie Boyd - Initial Version */
SimpleGPIO_thread::~SimpleGPIO_thread (){
	GPIOperi_users -=1;
	 if (GPIOperi_users ==0){
		 unmap_peripheral (GPIOperi);
		 delete (GPIOperi);
	 }
}

/* ************************** Setting up End Functions for selecting train frequency/duty from an array ******************
sets up/change the array for SimpleGPIO_thread using the SimpleGPIO_setArrayCallback function.
last modified:
2018/02/02 by Jamie Boyd - initial version 
**************************************************/
int SimpleGPIO_thread::setUpEndFuncArray (float * newData, unsigned int nData, int isLocking){
	// fill an array struct from passed-in data
	SimpleGPIOArrayStructPtr setUpStruct = new SimpleGPIOArrayStruct;
	setUpStruct->arrayData = newData;
	setUpStruct->nData = nData;
	setUpStruct->arrayPos =0;
	int errVar = modCustom (&SimpleGPIO_setUpArrayCallback, (void *) setUpStruct, isLocking);
	if (errVar ==0){
		// set up the custom delete function specific to the array callback
		setCustomDataDelFunc (&SimpleGPIO_ArrayStructCustomDel);
	}
	return errVar;	
}


void SimpleGPIO_thread::setUpEndFuncCosArray (unsigned int arraySize, unsigned int period, float offset, float scaling){
	const double phi = 2 * 3.1415926535897;
	// make an array to output
	endFuncArrayData = new float [arraySize];
	for (unsigned int ii=0; ii< arraySize; ii +=1){
		endFuncArrayData [ii] = offset -  scaling * cos (phi * (double)(ii % period)/period);
	}
	setUpEndFuncArray (endFuncArrayData, arraySize, 1);
}

/* ****************************** utility functions - setters and getters *************************

************************************sets pin bit in task structure by installing custom modifier function 
Last Modified:
2018/02/02 by Jamie Boyd - Initial Version  */
int SimpleGPIO_thread::setPin (int newPin, int isLocking){
	
	pinNumber = newPin;
	int * newPinVal = new int;
	* newPinVal =  1 << newPin;
	int returnVal = modCustom (&SimpleGPIO_setPinCallback, (void *) newPinVal, isLocking);	
	return returnVal;
}

int SimpleGPIO_thread::getPin (void){
	return pinNumber;
}

int SimpleGPIO_thread::getPolarity (void){
	return polarity;
}


int SimpleGPIO_thread::setLevel (int level, int isLocking){
	int * setlevelPtr= new int;
	* setlevelPtr = level ;
	int returnVal = modCustom (&SimpleGPIO_setLevelCallBack, (void *) setlevelPtr, isLocking);	
	return returnVal;
}

/**/
	
	
	
	