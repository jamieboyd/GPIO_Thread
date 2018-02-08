#include "SimpleGPIO_thread.h"
//#include <math.h>

/* ***************************have to initialize static data outside of the class *************************************/
bcm_peripheralPtr SimpleGPIO_thread::GPIOperi = nullptr;
int SimpleGPIO_thread::GPIOperi_users=0;

/* *************************** Functions that are called from thread can not be class methods *********************

***************************************Initialization callback function ****************************************
Copies pinBit and set/unset register adressses to  task data
last modified:
2018/02/01 by Jamie Boyd - initial version  */
int SimpleGPIO_Init (void * initDataP, void *  &taskDataP){
	
	// task data pointer is a void pointer that needs to be initialized to a pointer to taskData and filled from our custom init structure 
	SimpleGPIOStructPtr taskData  = new SimpleGPIOStruct;
	taskDataP = taskData;
	// initData is a pointer to our custom init structure
	SimpleGPIOStructPtr initData = (SimpleGPIOStructPtr) initDataP;
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
void SimpleGPIO_Lo (void *  taskData, unsigned int SignalBits){
	SimpleGPIOStructPtr gpioTaskPtr = (SimpleGPIOStructPtr) taskData;
	*(gpioTaskPtr->GPIOperiLo) = gpioTaskPtr->pinBit;
}

/* ***************** Hi Callback ******************************
Task to do on High tick, sets GPIO line high or ow depending on polarity
last modified:
2016/12/07 by Jamie Boyd - initial version */
void SimpleGPIO_Hi (void *  taskData, unsigned int SignalBits){
	SimpleGPIOStructPtr gpioTaskPtr = (SimpleGPIOStructPtr) taskData;
	*(gpioTaskPtr->GPIOperiHi) =gpioTaskPtr->pinBit;
}

/* **************** Custom data mod callbacks *****************************

********************Changes GPIO pin bit********************************
modData is a pointer to an int for the new pin bit to use
last modified:
2018/02/02 by Jamie Boyd - initial version */
int SimpleGPIO_setPinCallback (void * modData, taskParams * theTask){
	SimpleGPIOStructPtr gpioTaskPtr = (SimpleGPIOStructPtr)theTask->taskData;
	gpioTaskPtr->pinBit = *(int *)modData;
	return 0;
}

/* ********************** Sets GPIO level by writing to the Hi or Lo address, as appropriate ************************* 
modData is a pointer to an int, 0 for low function, non-zero for high function. If polarity of pulse is reversed,
so also will be notion of setting hi vs setting lo 
last modified:
2018/02/02 by Jamie Boyd - initial version */
int SimpleGPIO_setLevelCallBack (void * modData, taskParams * theTask){
	int theLevel = * (int *)modData;
	SimpleGPIOStructPtr gpioTaskPtr = (SimpleGPIOStructPtr)theTask->taskData;		
	if (theLevel ==0){
		*(gpioTaskPtr->GPIOperiLo) = gpioTaskPtr->pinBit;
	}else{
		*(gpioTaskPtr->GPIOperiHi) = gpioTaskPtr->pinBit;
	}
	return 0;
}

/* ****************************** Custom delete Function *****************************************************/
void SimpleGPIO_delTask (void * taskData){
	SimpleGPIOStructPtr gpioTaskPtr = (SimpleGPIOStructPtr) taskData;
	delete (gpioTaskPtr);
}

 /* *************************** SimpleGPIO_thread Class Methods *******************************************************
 
 ******************** ThreadMaker with Integer pulse duration, delay, and number of pulses timing description inputs ********************
 Last Modified:
 2018/02/01 by Jamie Boyd - Initial Version */
SimpleGPIO_thread * SimpleGPIO_thread::SimpleGPIO_threadMaker (int thePin, int polarity, unsigned int delayUsecs, unsigned int  durUsecs, unsigned int nPulses, int accuracyLevel) {
	int errCode;
	// make and fill an init struct
	SimpleGPIOStruct initStruct;
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
	newGPIO_thread->setTaskDataDelFunc (&SimpleGPIO_delTask);
	return newGPIO_thread;
}

/* ******************* ThreadMaker with floating point frequency, duration, and duty cycle timing description inputs ********************
Last Modified:
2018/02/01 by Jamie Boyd - Initial Version */
SimpleGPIO_thread * SimpleGPIO_thread::SimpleGPIO_threadMaker (int thePin, int polarity, float frequency, float dutyCycle, float trainDuration, int accuracyLevel){
	int errCode;
	// make and fill an init struct
	SimpleGPIOStruct initStruct;
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
	newGPIO_thread->setTaskDataDelFunc (&SimpleGPIO_delTask);
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
	
	
	
	