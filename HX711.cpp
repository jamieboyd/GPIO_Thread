#include "HX711.h"



/* **************************************Initialization callback function ****************************************
Copies pinBit and set/unset register adressses to task data
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
	taskData-> pinBit =  1 << initDataPtr->theClockPin;
	// initialize pin for output
	*(initDataPtr->GPIOperiAddr + ((initDataPtr->theClockPin) /10)) &= ~(7<<(((initDataPtr->theClockPin) %10)*3));
	*(initDataPtr->GPIOperiAddr + ((initDataPtr->theClockPin)/10)) |=  (1<<(((initDataPtr->theClockPin)%10)*3));
	// put pin in low state at start - this will tell HX711 to turn on
	*(taskData->GPIOperiLo ) = taskData->pinBit ;
	// calculate address to data reading register
	taskData->GPIOperiData = (unsigned int *) (initDataPtr->GPIOperiAddr + 13);
	// initialize data pin for input
	*(initDataPtr->GPIOperiAddr + ((initDataPtr->theDataPin) /10)) &= ~(7<<(((initDataPtr->theDataPin) %10)*3));
	// calculate pin Bit for data
	taskData->dataPinBit =  1 << initDataPtr->theDataPin;
	// pre-compute array for value of each bit as its corresponding power of 2
	// backwards because data is read in high bit first
	for (int i=0; i < 24; i++)
		taskData->pow2[i]= pow (2, (23 - i));
	taskData->pow2[0] *= -1;    // most significant bit is negative in two's complement
	taskData->dataBitPos = 0;
	// weight data saved in passed in array
	taskData->weightData =initDataPtr->weightData;
	taskData->nWeights = initDataPtr->nWeights;
	taskData->scaling = initDataPtr->scaling;
	taskData->iWeight = 0;
	taskData->tareVal =0;
	return 0; // 
}

/* ***************** Hi Callback ******************************
 Task to do on High tick, sets clock line high
 last modified:
 2016/12/07 by Jamie Boyd - initial version */
void HX711_Hi (void *  taskData){
	HX711structPtr taskPtr = (HX711structPtr) taskData;
	if (taskData->dataBitPos==0){
		// zero this weight
		taskData->thisWeight = 0;
		// wait for data pin to go low before first bit
		while (*(taskPtr->GPIOperData) & taskData->dataPinBit){} ;
	}
	// set clock pin high
	*(taskPtr->GPIOperiHi) = gpioTaskPtr->pinBit;
}

/* ***************** Lo Callback ******************************
 Task to do on Low tick, read data bit and set GPIO Clock line low
last modified:
2016/12/07 by Jamie Boyd - initial version */
void HX711_Lo (void *  taskData){
	HX711structPtr taskPtr = (HX711structPtr) taskData;
	if (taskPtr->dataBitPos < 24){
		if (*(taskPtr->GPIOperData) & taskPtr->dataPinBit){
			taskPtr->thisWeight += taskPtr->pow2[task->dataBitPos];
		}
		taskPtr->dataBitPos +=1;
	}else{
		taskPtr->dataBitPos = 0;
		taskPtr->weightData [taskPtr->iWeight] = (float) (taskPtr->thisWeight - taskPtr->tareVal) * taskPtr->scaling;
		taskPtr->iWeight +=1;
	}
	// set clock pin low
	*(taskPtr->GPIOperiLo) = taskPtr->pinBit;
}

/* ************* Custom data delete function *********************/
void HX711_delTask (void * taskData){
	HX711Ptr taskPtr = (HX711Ptr) taskData;
	delete (taskPtr);
}


HX711* HX711_threadMaker  (int dataPin, int clockPin, float scaling, float * weightData, unsigned int nWeights){

	// make and fill an init struct
	HX711InitStructPtr initStruct= new HX711InitStructPtr;
	// get GPIO peripheral addresss
	initStruct->GPIOperiAddr = useGpioPeri ();
	if (initStruct->GPIOperiAddr == nullptr){
#if beVerbose
        printf ("HX711_threadMaker failed to map GPIO peripheral.\n");
#endif
		return nullptr;
	}
	// fill out rest of struct
	initStruct->theDataPin = dataPin;
	initStruct->theClockPin=clockPin;
	initStruct->weightData = weightData;
	initStruct->nWeights = nWeights;
	// call constructor with initStruct
	int errCode;
	HX711 * newHX711 = new HX711(dataPin, clockPin, 1, 1, 25, (void *) initStruct, &HX711_Init, &HX711_Lo, &HX711_Hi, 2 , errCode);
	if (errCode){
		return nullptr;
	}
	// set custom task delete function
	newHX711->setTaskDataDelFunc (&HX711_delTask);
	return newHX711;
}

/* takes a series of readings and stores the average value as a tare value to be
	subtracted from subsequent readings. Tare value is not scaled, but in raw A/D units*/
void HX711::tare (int nAvg, bool printVals){
	if (isPoweredUp == false)
		turnON ();
	HX711structPtr taskPtr = (HX711structPtr) taskData;
	taskPtr->iWeight =0;
	taskPtr->tareValue = 0.0;
	for (int iread = 0; iread < nAvg; iread++){
		doTask();
		waitOnBusy(0.25);
		taskPtr->tareValue += (float)taskPtr->thisWeight;
	}
	taskPtr->tareValue /= nAvg;
}

/* Returns the stored tare value */
float HX711::getTareValue (void){
	return tareValue;
}

/* Takes a series of readings, averages them, and returns the scaled average */
float HX711::weigh (int nAvg, bool printVals){
	if (isPoweredUp == false)
		turnON ();
	taskPtr->iWeight =0;
	doTasks(nAvg);
	waitOnBusy(2);
	float readAvg =0;   
	for (int iread =0; iread < nAvg; iread++)
		readAvg += taskPtr->weightData [iRead];
	readAvg /= nAvg;
	return readAvg;
}

/* starts the thread weighing into the array */
void HX711::weighThreadStart (int nWeights){
	taskPtr->iWeight =0;
	if (nWeights > 
	doTasks (nWeights);
}

/* stops the threaded version and returns the number of weights so far obtained */
int HX711::weighThreadStop (void){
	 0;
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

