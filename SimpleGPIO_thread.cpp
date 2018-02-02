#include "SimpleGPIO_thread.h"


bcm_peripheralPtr SimpleGPIO_thread::GPIOperi = nullptr;
int SimpleGPIO_thread::GPIOperi_users=0;

/* *************************** Functions that are called from thread can not be class methods ********************************************
Initialization callback function. Initialize the pin for output and set it high or low as requested at start
last modified:
2018/02/01 by Jamie Boyd - initial version  */
int SimpleGPIO_Init (void * initDataP, void *  &taskDataP){
	
	// task data pointer is a void pointer that needs to be initialized to a pointer to taskCustomData and filled from our custom init structure 
	SimpleGPIOStructPtr taskData  = new SimpleGPIOStruct;
	taskDataP = taskData;
	// initData is a pointer to our custom init structure
	SimpleGPIOInitStructPtr initData = (SimpleGPIOInitStructPtr) initDataP;
	// copy pin number from init Data to taskData
	taskData->pin = initData->pin;
	taskData->pinBit = initData->pinBit;
	taskData->GPIOperiHi = initData->GPIOperiHi;
	taskData->GPIOperiLo = initData->GPIOperiLo;
	return 0; // 
}

/* ***********************************************
// lo callback. Task to do on Low tick, sets GPIO line low
last modified:
2016/12/07 by Jamie Boyd - initial version
************************************************/
void SimpleGPIO_Lo (void *  taskData){
	SimpleGPIOStructPtr gpioTaskPtr = (SimpleGPIOStructPtr) taskData;
	*(gpioTaskPtr->GPIOperiLo) = gpioTaskPtr->pinBit;
}

/* ***********************************************
// hi callback. Task to do on High tick, sets GPIO line high
last modified:
2016/12/07 by Jamie Boyd - initial version
*************************************************/
void SimpleGPIO_Hi (void *  taskData){
	SimpleGPIOStructPtr gpioTaskPtr = (SimpleGPIOStructPtr) taskData;
	*(gpioTaskPtr->GPIOperiHi) =gpioTaskPtr->pinBit;
}

/* **************************************************
 * delete function for custom task data, called when pulsedThread is killed 
 * if you explicitly install it with pulsedThread setCustomDataDelFunc
 * last modified:
 * 2017/12/06 by Jamie Boyd - initial version */
 void SimpleGPIO_customDel(void *  taskData){
	 SimpleGPIOStructPtr gpioTaskPtr = (SimpleGPIOStructPtr) taskData;
	 delete (gpioTaskPtr);
	 taskData = nullptr;
#if beVerbose
	 printf("Many are made but fewer are deleted\n");
#endif
 }

 /* ******************************************** SimpleGPIO_thread Class Methods ******************************************************************************/
SimpleGPIO_thread *  SimpleGPIO_thread::SimpleGPIO_threadMaker (int thePin, int polarity, unsigned int delayUsecs, unsigned int  durUsecs, unsigned int nPulses, int accuracyLevel) {
	
	// make and fill an init struct
	SimpleGPIOInitStruct initStruct;
	initStruct.pin = thePin;
	// map GPIO peripheral, if needed
	int errCode;
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
	int errVar;
	SimpleGPIO_thread * newGPIO_thread = new SimpleGPIO_thread (delayUsecs, durUsecs, nPulses, (void *) &initStruct, &SimpleGPIO_Init, &SimpleGPIO_Lo, &SimpleGPIO_Hi, accuracyLevel, errVar);
	if (errVar){
#if beVerbose
		printf ("Failed to make pulsed thread.\n");
#endif
		return nullptr;
	}
	 // increment static GPIOperi_users . When destructing, if no other users, delete the mapping
	GPIOperi_users +=1;
	newGPIO_thread->setCustomDataDelFunc (&SimpleGPIO_customDel);
	return newGPIO_thread;
}

SimpleGPIO_thread::~SimpleGPIO_thread (){
	GPIOperi_users -=1;
	 if (GPIOperi_users ==0){
		 unmap_peripheral (GPIOperi);
		 delete (GPIOperi);
	 }
}

/*
void SimpleGPIO_thread::setPin (int pin){
	
	
	getPin ();
	setLevel (int level);
	
	
	
	
	void setPin (int pin);
	int getPin (void );
	void setLevel (int level);
*/	
	
	
	
	
	
	