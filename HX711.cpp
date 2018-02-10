#include "HX711.h"

/*
 pins and address base for memory mapped addresses 
typedef struct SimpleGPIOInitStruct{
	int theClockPin; // pin to use for the GPIO output for clock
	int theDataPin; //pin to use for the GPIO input for data
	volatile unsigned int * GPIOperiAddr; // base address needed when writing to registers for setting and unsetting
}HX711InitStruct, *HX711InitStructPtr;


this C-style struct contains all the relevant thread variables and task variables, and is shared with the pulsedThread
typedef struct HX711struct {
	unsigned int * GPIOperiHi; // address of register to WRITE pin bit to on Hi for clock
	unsigned int * GPIOperiLo; // address of register to WRITE pin bit to on Lo for clock
	unsigned int pinBit;	// clock pin number translated to bit position in register 
	unsigned int * GPIOperiData; // address of register to READ for the data
	unsigned int dataPinBit;	// data pin number translated to bit position in register 
        int * dataArray;			// an array of 24 integers used to hold the set bits for a single weighing
	unsigned int dataBitPos;	// tracks where we are in the 24 data bit positions
        int * pow2;				// precomputed array of powers of 2 used to translate set bits into data
	float * weightData; 		// pointer to the array to be filled with data, an array of floats
	int getWeights; 			// calling function sets this to number of  weights  requested, or 0 to abort a series of weights
	int gotWeights; 			// thread sets this to number of  weights  as they are obtained
}HX711struct, * HX711structPtr;
*/


/***************************************Initialization callback function ****************************************
Copies pinBit and set/unset register adressses to  task data
last modified:
2018/02/09 by Jamie Boyd - initial version  modified from stand-alone non-pulsedThread version*/
int HX711_Init (void * initDataP, void *  &taskDataP){
	// task data pointer is a void pointer that needs to be initialized to a pointer to taskData and filled from our custom init structure 
	HX711structPtr taskData  = new HX711struct;
	taskDataP = taskData;
	// initData is a pointer to our custom init structure
	HX711InitStructPtr initDataPtr = (HX711InitStructPtr) initDataP;
	// calculate address to ON and OFF register  - HX711 always uses low to high pulses
	taskData->GPIOperiHi = (unsigned int *) (initDataPtr->GPIOperiAddr + 7);
	taskData->GPIOperiLo = (unsigned int *) (initDataPtr->GPIOperiAddr + 10);
	// calculate pin Bit for clock
	taskData->pinBit =  1 << initDataPtr->theClockPin;
	// initialize pin for output
	*(initDataPtr->GPIOperiAddr + ((initDataPtr->theClockPin) /10)) &= ~(7<<(((initDataPtr->theClockPin) %10)*3));
	*(initDataPtr->GPIOperiAddr + ((initDataPtr->theClockPin)/10)) |=  (1<<(((initDataPtr->theClockPin)%10)*3));
	// put pin in low state at start
	*(taskData->GPIOperiLo ) = taskData->pinBit ;
	// calculate address to data reading register
	taskData->GPIOperiData = (unsigned int *) (initDataPtr->GPIOperiAddr + 13);
	// initialize data pin for input
	*(initDataPtr->GPIOperiAddr + ((initDataPtr->theDataPin) /10)) &= ~(7<<(((initDataPtr->theDataPin) %10)*3));
	// calculate pin Bit for data
	taskData->dataPinBit =  1 << initDataPtr->theDataPin;
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


// *********************************************************************************************
// threaded function for weighing needs to be a C  style function, not a class method
// It gets a bunch of weights as fast as possible, until array is full or thread is interrupted
// Last Modified 2016/08/16 by Jamie Boyd
extern "C" void* HX711ThreadFunc (void * tData){
	// cast tData to task param stuct pointer
	taskParams *theTask = (taskParams *) tData;
	
	 // set durations for timing for data/clock pulse length
	struct timeval durTime;
		struct timeval delayTime;
		struct timeval actualTime;
		struct timeval expectedTime;
	durTime.tv_sec = 0;
	durTime.tv_usec =1;
	delayTime.tv_sec = 0;
	delayTime.tv_usec =1;
	//give thread high priority
	struct sched_param param ;
	param.sched_priority = sched_get_priority_max (SCHED_RR) ;
	pthread_setschedparam (pthread_self (), SCHED_RR, &param) ;
	// loop forever, waiting for a request to weigh
	unsigned int dataVal;
	for (;;){
		// get the lock on getWeight and wait for weights to be requested
		pthread_mutex_lock (&theTask->taskMutex);
		while (theTask->getWeights==0)
			pthread_cond_wait(&theTask->taskVar, &theTask->taskMutex);
		//printf ("Thread was signalled.\n");
		// Unlock the mutex right away so calling thread can give countermanding signal
		pthread_mutex_unlock (&theTask->taskMutex);
		for (theTask->gotWeights =0; theTask->gotWeights < theTask->getWeights; theTask->gotWeights ++){
			// zero data array
			for (int ibit =0; ibit < 24; ibit++)
				theTask->dataArray [ibit] =0;
			// wait for data pin to go low
			do{
				dataVal = GPIO_READ (theTask->GPIOperi ->addr, theTask->dataPinBit);
			}
			while (dataVal == theTask->dataPinBit);
			//while (digitalRead (theTask->dataPin) == HIGH){}
			// get data for each of 24 bits
			gettimeofday (&expectedTime, NULL);
			for (int ibit =0; ibit < 24; ibit++){
				// set clock pin high, wait dur for data bit to be set
				GPIO_SET (theTask->GPIOperi ->addr, theTask->clockPinBit);
				//digitalWrite (theTask->clockPin, HIGH) ;
				timeradd (&expectedTime, &durTime, &expectedTime);
				for (gettimeofday (&actualTime, NULL); (timercmp (&actualTime, &expectedTime, <)); gettimeofday (&actualTime, NULL));
				// read data
				dataVal = GPIO_READ (theTask->GPIOperi ->addr, theTask->dataPinBit) ;
				if (dataVal == theTask->dataPinBit)
				//if (digitalRead (theTask->dataPin) == HIGH)
					theTask->dataArray [ibit] = 1;
				// set clock pin low, wait for delay period
				GPIO_CLR (theTask->GPIOperi ->addr, theTask->clockPinBit);
				//digitalWrite (theTask->clockPin, LOW) ;
				timeradd (&expectedTime, &delayTime, &expectedTime);
				for (gettimeofday (&actualTime, NULL); (timercmp (&actualTime, &expectedTime, <)); gettimeofday (&actualTime, NULL));
			}
			// write out another clock pulse with no data collection to set scaling to channel A, high gain
			GPIO_SET (theTask->GPIOperi ->addr, theTask->clockPinBit);
			//digitalWrite (theTask->clockPin, HIGH) ;
			timeradd (&expectedTime, &delayTime, &expectedTime);
			for (gettimeofday (&actualTime, NULL); (timercmp (&actualTime, &expectedTime, <)); gettimeofday (&actualTime, NULL));
			GPIO_CLR (theTask->GPIOperi ->addr, theTask->clockPinBit);
			//digitalWrite (theTask->clockPin, LOW) ;
			timeradd (&expectedTime, &delayTime, &expectedTime);
			for (gettimeofday (&actualTime, NULL); (timercmp (&actualTime, &expectedTime, <)); gettimeofday (&actualTime, NULL));
			// get values for each bit set from pre-computer array of powers of 2
			theTask->weightData [theTask->gotWeights] =theTask->dataArray [0] * theTask->pow2 [0];
			for (int ibit =1; ibit < 24; ibit ++)
				theTask->weightData [theTask->gotWeights] += theTask->dataArray [ibit] * theTask->pow2 [ibit];
			theTask->weightData [theTask->gotWeights] = (theTask->weightData [theTask->gotWeights]  - theTask->tareValue) * theTask->scaling;
		}
		theTask->getWeights = 0;
	}
	return NULL;
}


HX711* HX711_threadMaker  (int dataPin, int clockPin, float scaling){
	// map GPIO peripheral
	int errCode;
	errCode = mapGPIOperi ();
	if (errCode){
		return nullptr;
	}
	// make and fill an init struct
	HX711InitStruct initStruct;
	initStruct.theClockPin=clockPin;
	initStruct.theDataPin = dataPin;
	
	initStruct.GPIOperiAddr = GPIOperi->addr;
	
	int theClockPin; // pin to use for the GPIO output for clock
	int theDataPin; //pin to use for the GPIO input for data
	volatile unsigned int * GPIOperiAddr; 
	
	
HX711::HX711 (int dataPinP, int clockPinP, float scalingP, int &errCode){

	
	
	// call SimpleGPIO_thread constructor, which just calls pulsedThread contructor
	SimpleGPIO_thread * newGPIO_thread = new SimpleGPIO_thread (delayUsecs, durUsecs, nPulses, (void *) &initStruct, &SimpleGPIO_Init, &SimpleGPIO_Lo, &SimpleGPIO_Hi, accuracyLevel, errCode);
	
	
	
	dataPinBit = 1 << dataPin;
	clockPinBit = 1 << clockPin;
	GPIOperi = new bcm_peripheral {GPIO_BASE};
	errCode = map_peripheral (GPIOperi, IFACE_DEV_GPIOMEM);
	if (errCode == 0){
		// set data pin for input
		INP_GPIO(GPIOperi ->addr, dataPin);
		// set clock pin for output, and set clock low
		INP_GPIO(GPIOperi ->addr, clockPin);
		OUT_GPIO (GPIOperi ->addr, clockPin);
		GPIO_SET(GPIOperi ->addr, clockPinBit);
		//pinMode (dataPin, INPUT) ; // DATA
		//pinMode (clockPin, OUTPUT) ; // Clock
		//digitalWrite (clockPin, LOW) ;
		// pre-compute array for value of each bit as its corresponding power of 2
		// backwards because data is read in high bit first
		for (int i=0; i<24; i++)
			pow2[i]= pow (2, (23 - i));
		pow2[0] *= -1;	// most significant bit is negative in two's complement
		// set durations for timing for data/clock pulse length
		durTime.tv_sec = 0;
		durTime.tv_usec =1;
		delayTime.tv_sec = 0;
		delayTime.tv_usec =1;
		// make a thread, for threaded reads into an array
		// be sure to point thetask.weightData to a real array before calling the threaded version
		theTask.dataPinBit=dataPinBit;
		theTask.clockPinBit = clockPinBit;
		theTask.GPIOperi=GPIOperi;
		theTask.scaling = scaling;
		theTask.tareValue = 0;
		theTask.dataArray = dataArray;
		theTask.pow2 = pow2;
		theTask.getWeights =0;
		theTask.gotWeights=0;
		// init mutex and condition var
		pthread_mutex_init(&theTask.taskMutex, NULL);
		pthread_cond_init (&theTask.taskVar, NULL);
		// create thread
		pthread_create(&theTask.taskThread, NULL, &HX711ThreadFunc, (void *)&theTask);
		
		
			dataPin = dataPinP;
	clockPin = clockPinP;
	scaling = scalingP;
	}
}

/* takes a series of readings and stores the average value as a tare value to be
	subtracted from subsequent readings. Tare value is not scaled, but in raw A/D units*/
void HX711::tare (int nAvg, bool printVals){
	if (isPoweredUp == false)
	this->turnON ();
   
	tareValue = 0.0;
	for (int iread =0; iread < nAvg; iread++){
		tareValue += this->readValue ();
	}
	tareValue /= nAvg;
	theTask.tareValue = tareValue;
}

/* Returns the stored tare value */
float HX711::getTareValue (void){
	return tareValue;
}

/* Takes a series of readings, averages them, subtractes the tare value, and 
applies the scaling factor and returns the scaled average */
float HX711::weigh (int nAvg, bool printVals){
	if (isPoweredUp == false)
		this->turnON ();
	float readAvg =0;   
	for (int iread =0; iread < nAvg; iread++)
		readAvg += this->readValue ();
	readAvg /= nAvg;
	return ((readAvg - tareValue) * scaling);
}

/* starts the threaded version filling in a passed-in array of weight values */
void HX711::weighThreadStart (float * weights, int nWeights){
	pthread_mutex_lock (&theTask.taskMutex);
	theTask.weightData = weights;
	theTask.getWeights = nWeights;
	pthread_cond_signal(&theTask.taskVar);
	pthread_mutex_unlock( &theTask.taskMutex );
}

/* stops the threaded version and returns the number of weights so far obtained */
int HX711::weighThreadStop (void){
	theTask.getWeights = 0;
	int nWeights = theTask.gotWeights;
	theTask.gotWeights=0;
	return  nWeights;
}

/* checks how many weights have been obtained so far, but does not stop the thread */
int HX711::weighThreadCheck (void){
	return theTask.gotWeights;
}

/* Gets the saved data pin GPIO number. Note the GPIO pin can only be set
	when initialized */
int HX711::getDataPin (void){
	return dataPin;
}

/* Gets the saved clock pin GPIO number. Note the GPIO pin can only be set
	when initialized */
int HX711::getClockPin(void){
	return clockPin;
}

/* Setter and getter for scaling */
float HX711::getScaling (void){
	return scaling;
}

void HX711::setScaling (float newScaling){
	scaling = newScaling;
	theTask.scaling = scaling;
}

/* Set the clock pin high for 50ms to put the HX711 into a low power state */
void HX711::turnOFF (void){
	isPoweredUp = false;
	GPIO_SET(GPIOperi->addr, clockPinBit) ;
	//digitalWrite (clockPin, HIGH) ;
}

/* set the clock pin low to wake the HX711 after putting it into a low power state
wait 2 microseconds (which shuld be lots of time) to give the device time to wake */
void HX711::turnON(void){
	isPoweredUp = true;
	GPIO_CLR(GPIOperi->addr, clockPinBit) ;
	//digitalWrite (clockPin, LOW) ;
	// wait a few microseconds before returning to give device time to wake up
	gettimeofday (&expectedTime, NULL);
	timeradd (&expectedTime, &durTime, &expectedTime);
	timeradd (&expectedTime, &durTime, &expectedTime);
	for ( gettimeofday (&actualTime, NULL);(timercmp (&actualTime, &expectedTime, <)); gettimeofday (&actualTime, NULL));
	//delay (2000);
}

/* reads a single value from the HX711 and returns the signed integer value 
	without taring or scaling */
int HX711::readValue (void){
	// zero data array
	for (int ibit =0; ibit < 24; ibit++){
		dataArray [ibit] =0;
	}
	// wait for data pin to go low
	unsigned int dataVal;
	do{
		dataVal= GPIO_READ(GPIOperi->addr, dataPinBit);
	} while (dataVal == dataPinBit);
	//while (digitalRead (dataPin) == HIGH){}
	// get data for each of 24 bits
	gettimeofday (&expectedTime, NULL);
	for (int ibit =0; ibit < 24; ibit++){
		// set clock pin high, wait dur for data bit to be set
		GPIO_SET (GPIOperi->addr, clockPinBit);
		//digitalWrite (clockPin, HIGH) ;
		timeradd (&expectedTime, &durTime, &expectedTime);
		for ( gettimeofday (&actualTime, NULL);(timercmp (&actualTime, &expectedTime, <)); gettimeofday (&actualTime, NULL));
		// read data
		dataVal=GPIO_READ (GPIOperi->addr, dataPinBit);
		if ( dataVal == dataPinBit){
			// if (digitalRead (dataPin) == HIGH)
			dataArray [ibit] = 1;
		}
		// set clock pin low, wait for delay period
		GPIO_CLR (GPIOperi->addr, clockPinBit);
		//digitalWrite (clockPin, LOW) ;
		timeradd (&expectedTime, &delayTime, &expectedTime);
		for ( gettimeofday (&actualTime, NULL) ;(timercmp (&actualTime, &expectedTime, <)); gettimeofday (&actualTime, NULL));
	}
	// write out another clock pulse with no data collection to set scaling to channel A, high gain
	GPIO_SET (GPIOperi->addr, clockPinBit);
	//digitalWrite (clockPin, HIGH) ;
	timeradd (&expectedTime, &delayTime, &expectedTime);
	gettimeofday (&actualTime, NULL);
	for ( ;(timercmp (&actualTime, &expectedTime, <)); gettimeofday (&actualTime, NULL));
   GPIO_CLR (GPIOperi->addr, clockPinBit);
	//digitalWrite (clockPin, LOW) ;
	timeradd (&expectedTime, &delayTime, &expectedTime);
	for (gettimeofday (&actualTime, NULL) ;(timercmp (&actualTime, &expectedTime, <)); gettimeofday (&actualTime, NULL));
	// get values for each bit set from pre-computer array of powers of 2
	int result =0;
	for (int ibit =0; ibit < 24; ibit ++)
		result += dataArray [ibit] * pow2 [ibit];
	return result;
}

