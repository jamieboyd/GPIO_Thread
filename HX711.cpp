#include "HX711.h"


/* **************************************Initialization callback function ****************************************
Copies pinBit and set/unset register adressses to task data
last modified:
2018/03/01 by Jamie Boyd - eliminated redundant fields, added for tare vs weigh
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
	taskData-> clockPinBit =  1 << initDataPtr->theClockPin;
	// initialize pin for output
	*(initDataPtr->GPIOperiAddr + ((initDataPtr->theClockPin) /10)) &= ~(7<<(((initDataPtr->theClockPin) %10)*3));
	*(initDataPtr->GPIOperiAddr + ((initDataPtr->theClockPin)/10)) |=  (1<<(((initDataPtr->theClockPin)%10)*3));
	// put clock pin in low state at start - this will tell HX711 to turn on
	*(taskData->GPIOperiLo ) = taskData->clockPinBit ;
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
	// weight data saved in passed-in array - so data can be easily read from outside the thread
	taskData->weightData =initDataPtr->weightData;
	taskData->scaling = initDataPtr->scaling;
	taskData->iWeight = 0;
	taskData->dataBitPos=0;
	taskData->tareVal =0;
	return 0; // 
}

/* ***************** Hi Callback ******************************
 Task to do on High tick, sets clock line high. If this tick is first bit of a weight, 1) zero the weight and 2) wait
untill data pin goes high (HX711 holds data pin low untill a weight measurement is ready to send

 last modified:
2018/03/13 by Jamie Boyd - took out sleep for fastest performance - will try an interrupt ?
2018/03/05 by Jamie Boyd - added sleep if data pin is low, as max speed is only 10Hz to begin with
2018/03/01 by jamie Boyd - do calculations directly in data array
2017/12/07 by Jamie Boyd - initial version */
void HX711_Hi (void *  taskData){
	HX711structPtr taskPtr = (HX711structPtr) taskData;
	if (taskPtr->dataBitPos==0){
		// zero this weight position
		taskPtr->weightData [taskPtr->iWeight] = 0;
		// wait for data pin to go low before first bit. When output data is not ready for retrieval, digital output pin DOUT is held high.
		// might want to use an interrupt here 
		while (*((taskPtr->GPIOperiData)) & (taskPtr->dataPinBit)){};
	}
	// set clock pin high to shift out next bit of data
	*(taskPtr->GPIOperiHi) = taskPtr->clockPinBit;
}

/* ***************** Lo Callback ******************************
 Task to do on Low tick, read data bit and set GPIO Clock line low. If last bit, calculate scaled value
last modified:
2018/03/13 by Jamie Boyd - only set clock pin low from one place in the code
2018/03/01 by jamie Boyd - do calculations directly in data array
2017/12/07 by Jamie Boyd - initial version */
void HX711_Lo (void *  taskData){
	HX711structPtr taskPtr = (HX711structPtr) taskData;
	if (taskPtr->dataBitPos < 24){
		if (*(taskPtr->GPIOperiData) & taskPtr->dataPinBit){
			taskPtr->weightData [taskPtr->iWeight]  += taskPtr->pow2[taskPtr->dataBitPos];
		}
		taskPtr->dataBitPos +=1;
	}else{ // we have all the bits, so calculate weight and send out 25th pulse on clock pin for input and gain selection
		// if we are weighing, not calculating a tare value, subtract tare value and multiple by scaling 
		if (taskPtr->controlCode == kCTRL_WEIGH){
			taskPtr->weightData [taskPtr->iWeight] = (taskPtr->weightData [taskPtr->iWeight] - taskPtr->tareVal) * taskPtr->scaling;
		}
		// set dataBit pos to 0 and advance to next position in array to be ready for next weight
		taskPtr->dataBitPos = 0; 
		taskPtr->iWeight +=1;
	}
	// set clock pin low
	*(taskPtr->GPIOperiLo) = taskPtr->clockPinBit;
}


/* ************* Custom task data delete function *********************/
void HX711_delTask (void * taskData){
	HX711structPtr taskPtr = (HX711structPtr) taskData;
	delete (taskPtr);
}

/* ****************************** Destructor handles GPIO peripheral mapping*************************
Thread data is destroyed by the pulsedThread destructor.  All we need to do here is take care of GPIO peripheral mapping
HX711 object does not own the array of weight data, so should not delete it 
Last Modified:
2018/03/01 by Jamie Boyd - Initial Version copied from SimpleGPIO*/
HX711::~HX711 (){
	unUseGPIOperi();
}

/* ******************** Makes an HX711 thread returns a pointer to it **************************************
makes and fills an init structure and calls constructor
last modified:
2018/03/01 by Jamie Boyd - updated for modified constructor with fewer parameters */
HX711* HX711::HX711_threadMaker  (int dataPin, int clockPin, float scaling, float * weightData, unsigned int nWeightData){

	// make and fill an init struct
	HX711InitStructPtr initStruct= new HX711InitStruct;
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
	initStruct->scaling = scaling;
	initStruct->weightData = weightData;
	// call constructor with initStruct
	int errCode;
	HX711 * newHX711 = new HX711(dataPin, clockPin, nWeightData, (void *) initStruct, errCode);
	if (errCode){
#if beVerbose
        printf ("HX711_threadMaker failed to make HX711_thread object.\n");
#endif
		return nullptr;
	}
	// set custom task delete function
	newHX711->setTaskDataDelFunc (&HX711_delTask);
	// make an HX711TaskPtr pointer for easy direct access to thread task data 
	newHX711->HX711TaskPtr = (HX711structPtr)newHX711->getTaskData ();

	return newHX711;
}


/* ******************************* Setting the clock pin high for 50ms puts the HX711 into a low power state ******************************
Direct access to taskData, no checking if thread is busy. Don't turn HX711 Off or On while weighing
Last Modified:
2018/03/01 by Jamie Boyd - updated for pulsedThread subclass */
void HX711::turnOFF (void){
	//HX711structPtr HX711TaskPtr = (HX711structPtr)getTaskData (); // returns a pointer to the custom data for the task
	*(HX711TaskPtr->GPIOperiHi) = HX711TaskPtr->clockPinBit ;
	isPoweredUp = false;
}

/* **********************set the clock pin low to wake the HX711 after putting it into a low power state *********************************
Last Modified:
2018/03/11 by jamie Boyd - put in a sleep after measuring how long it takes this thing to wake up
2018/03/01 by Jamie Boyd - updated for pulsedThread subclass  */
void HX711::turnON(void){
	//HX711structPtr HX711TaskPtr = (HX711structPtr)getTaskData (); // returns a pointer to the custom data for the task
	*(HX711TaskPtr->GPIOperiLo) = HX711TaskPtr->clockPinBit;
	// HX711 takes 0.5 seconds to wake up
	struct timespec sleeper;
	sleeper.tv_sec = 0;
	sleeper.tv_nsec = 0.5E09;
	nanosleep (&sleeper, NULL);
	while (*(HX711TaskPtr->GPIOperiData) & HX711TaskPtr->dataPinBit){} ;
	isPoweredUp = true;
}

/* ************************** takes a series of readings and stores the average value as a tare value *****************************
Tare value is not scaled, but in raw A/D units
Last Modified:
2018/03/01 by Jamie Boyd - updated for pulsed thread, and to use field for weighing vs taring */
float HX711::tare (int nAvg, bool printVals){
	return readSynchronous (nAvg, printVals, kCTRL_TARE);
}


/* Takes a series of readings, averages them, and returns the scaled average */
float HX711::weigh (unsigned int nAvg, bool printVals){
	return readSynchronous (nAvg, printVals, kCTRL_WEIGH);
}


float HX711::readSynchronous (unsigned int nAvg, bool printVals, int weighMode){
	if (isPoweredUp == false){
		turnON ();
	}
	//HX711structPtr HX711TaskPtr = (HX711structPtr)getTaskData (); // returns a pointer to the custom data for the task
	if (nAvg > nWeightData){
		printf ("Requested number to average, %d, was greater than size of array, %d.\n", nAvg, nWeightData);
		return 0;
	}else{
		HX711TaskPtr->controlCode = weighMode;
		HX711TaskPtr-> iWeight = 0;
		HX711TaskPtr->dataBitPos =0;
		this->DoTasks (nAvg);
		
		int waitVal = this->waitOnBusy((float)(1 + nAvg/5));
		if (waitVal){
			//nAvg -= waitVal;
			printf ("Ony weighed %d times in alotted time.\n", nAvg );
			UnDoTasks ();
			
		}
		double resultVal =0;
		if (printVals){
			printf ("Values:");
		}
		for (unsigned int iAvg =0; iAvg < nAvg; iAvg +=1){
			resultVal += HX711TaskPtr->weightData[iAvg];
			if (printVals){
				printf ("%.3f, ", HX711TaskPtr->weightData[iAvg]);
			}
		}
		resultVal /= nAvg;
		if (printVals){
			printf ("\nAverage = %.3f.\n", resultVal);
		}
		if (weighMode == kCTRL_TARE){
			HX711TaskPtr->tareVal = (float) resultVal;
		}
		HX711TaskPtr-> iWeight = 0;
		return resultVal;
	}
}

/* ****************************** Returns the stored tare value **************************************************************************/
float HX711::getTareValue (void){
	//HX711structPtr HX711TaskPtr = (HX711structPtr)getTaskData (); // returns a pointer to the custom data for the task
	return HX711TaskPtr->tareVal ;
}


/* starts the thread weighing into the array */
void HX711::weighThreadStart (unsigned int nWeightsP){
	//HX711structPtr HX711TaskPtr = (HX711structPtr)getTaskData (); // returns a pointer to the custom data for the task
	if (nWeightsP > nWeightData){
		printf ("Requested number to weigh, %d, was greater than size of array, %d.\n", nWeightsP, nWeightData);
	}else{
		HX711TaskPtr->iWeight =0;
		HX711TaskPtr->controlCode = kCTRL_WEIGH;
		HX711TaskPtr->dataBitPos =0;
		DoTasks (nWeightsP);
	}
}

/* stops the threaded and returns the number of weights so far obtained */
unsigned int HX711::weighThreadStop (void){
	UnDoTasks ();
	//HX711structPtr HX711TaskPtr = (HX711structPtr)getTaskData (); // returns a pointer to the custom data for the task
	return  HX711TaskPtr->iWeight;
}

/* checks how many weights have been obtained so far, but does not stop the thread */
unsigned int HX711::weighThreadCheck (void){
	//HX711structPtr HX711TaskPtr = (HX711structPtr)getTaskData (); // returns a pointer to the custom data for the task
	return  HX711TaskPtr->iWeight;
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
	//HX711structPtr HX711TaskPtr = (HX711structPtr)getTaskData (); // returns a pointer to the custom data for the task
	return HX711TaskPtr->scaling;
}

void HX711::setScaling (float newScaling){
	//HX711structPtr HX711TaskPtr = (HX711structPtr)getTaskData (); // returns a pointer to the custom data for the task
	HX711TaskPtr->scaling = newScaling;
}

unsigned int HX711::getNweights (void){
	//HX711structPtr HX711TaskPtr = (HX711structPtr)getTaskData (); // returns a pointer to the custom data for the task
	return nWeightData;
}