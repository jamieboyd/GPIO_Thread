#include "PWM_sin_thread.h"


/* ******************************************  PWM_sin_thread functions for pulsedThread *****************************************


 *********************************** PWM_sin Hi functions (no low function) ******************************
gets the next value from the array data to be output, depending on frequency, and writes it to the FIFO register
arrayData contains a sine wave with PWM_UPDATE_FREQ points.  When output at PWM_UPDATE_FREQ, you get a 1 Hz output.  The idea is to make a sine wave at
lowest ever needed frequency, in this case 1 Hz, and instead of changing the array data when we change frequencies, we change the step
by which we jump. We can easily do any multiple of the base frequency.  With 1Hz sine wave, we add 1 each time for 1 Hz, add 2 each time
for 2 Hz, etc, using the modulo operator % so we don't get tripped up at wrap-around. 

 *********************************** PWM Hi function when using FIFO for channel 1 ******************************
checks the FULL bit and feeds the FIFO from channel 1 array till it is full
Last Modified:
2018/09/24 - by Jamie Boyd - initial version */
void ptPWM_sin_FIFO_1 (void * taskDataP){
	ptPWMStructPtr taskData = (ptPWMStructPtr)taskDataP;
	if (*(taskData->statusRegister)  & (PWM_BERR | PWM_GAPO1)){
#if beVerbose
		printf ("status reg = 0x%x\n", *(taskData->statusRegister));
		*(taskData->statusRegister) |= (PWM_BERR | PWM_GAPO1);
#endif
	}
	while (!(*(taskData->statusRegister) & PWM_FULL1)){
		*(taskData->FIFOregister) = taskData->arrayData1[taskData->arrayPos1];
		taskData->arrayPos1 += taskData->startPos1;  // startPos is hijacked to use for frequency, so we can use same structs as base class. 
		if (taskData->arrayPos1 >= taskData->nData1){
			taskData->arrayPos1 = taskData->arrayPos1 % (taskData->nData1);
		}
	}
}

/* *********************************** PWM Hi function when using FIFO for channel 2 ******************************
checks the FULL bit and feeds the FIFO from channel 1 array till it is full
Last Modified:
2018/09/24 - by Jamie Boyd - initial version */
void ptPWM_sin_FIFO_2 (void * taskDataP){
	ptPWMStructPtr taskData = (ptPWMStructPtr)taskDataP;
		if (*(taskData->statusRegister)  & (PWM_BERR | PWM_GAPO2)){
#if beVerbose
		printf ("status reg = 0x%x\n", *(taskData->statusRegister));
		*(taskData->statusRegister) |= (PWM_BERR | PWM_GAPO2);
		
#endif
	}
	while (!(*(taskData->statusRegister) & PWM_FULL1)){
		*(taskData->FIFOregister) = taskData->arrayData2[taskData->arrayPos2];
		taskData->arrayPos2 += taskData->startPos2;  // startPos is hijacked to use for frequency, so we can use same structs as base class. 
		if (taskData->arrayPos2 >= taskData->nData2){
			taskData->arrayPos2 = taskData->arrayPos2 % (taskData->nData2);
		}
	}
}

/* *********************************** PWM Hi function when using FIFO with both channels ******************************
checks the FULL bit and feeds the FIFO from array for channels 1 and 2 alternately till it is full
Last Modified:
2018/09/24 - by Jamie Boyd - initial version */
void ptPWM_sin_FIFO_dual (void * taskDataP){
	ptPWMStructPtr taskData = (ptPWMStructPtr)taskDataP;
		if (*(taskData->statusRegister)  & (PWM_BERR | PWM_GAPO1 | PWM_GAPO2)){
#if beVerbose
		printf ("status reg = 0x%x\n", *(taskData->statusRegister));
		*(taskData->statusRegister) |= (PWM_BERR | PWM_GAPO1 | PWM_GAPO2);
#endif
	}
	while (!(*(taskData->statusRegister) & PWM_FULL1)){
		if (taskData->chanFIFO==1){
			*(taskData->FIFOregister) = taskData->arrayData1[taskData->arrayPos1];
			taskData->arrayPos1 += taskData->startPos1;
			if (taskData->arrayPos1 >= taskData->nData1){
				taskData->arrayPos1 = taskData->arrayPos1 % (taskData->nData1);
			}
			taskData->chanFIFO=2;
		}else{
			*(taskData->FIFOregister) = taskData->arrayData2[taskData->arrayPos2];
			taskData->arrayPos2 += taskData->startPos2;
			if (taskData->arrayPos2 >= taskData->nData2){
				taskData->arrayPos2 = taskData->arrayPos2 % (taskData->nData2);
			}
			taskData->chanFIFO=1;
		}
	}	
}


/* *************************** Sets frequency of sine wave to be output ***************
modData is a pointer to an unsigned int, the frequency in Hz, to be next output, and outputs it
last modified:
2018/09/13 by Jamie Boyd - initial version*/
int ptPWM_sin_setFreqCallback (void * modData, taskParams * theTask){
	ptPWMStructPtr taskData = (ptPWMStructPtr) theTask->taskData;
	ptPWMArrayModStructPtr modDataPtr = (ptPWMArrayModStructPtr) modData;
	if ((modDataPtr->channel) & 1){
		taskData->startPos1 = modDataPtr->startPos; // startPos is hijacked to use  for frequency, so we can use same structs as base class. 
	}
	if ((modDataPtr->channel) & 2){
		taskData->startPos2 = modDataPtr->startPos;
	}
	delete modDataPtr;
	return 0;
}


/* **********************************************  PWM_sin_thread class functions ************************************************

 ********************************************* PWM_sin_threadMaker***********************************************************
returns a pointer to a new PWM_sin_thread object, made by making a regular PWM_thread with required settings and recasting it
to a PWM_sin_thread
Last Modified:
2018/09/22 by Jamie Boyd - updated to use FIFO for PWM and better channel organization, less to do here,more in addChannel
2018/09/13 by Jamie Boyd - initial version */
PWM_sin_thread * PWM_sin_thread::PWM_sin_threadMaker (int channels){
	// ensure peripherals for PWM controller are mapped
	int mapResult = PWM_thread::mapPeripherals ();
	if (mapResult){
		printf ("Could not map peripherals for PWM access with return code %d.\n", mapResult);
		return nullptr;
	}
	// set clock for PWM from input parmamaters
	float realPWMfreq = PWM_thread::setClock (PWM_UPDATE_FREQ, PWM_RANGE);
	if (realPWMfreq < 0){
		printf ("Could not set clock for PWM with frequency = %d and range = %d.\n", PWM_UPDATE_FREQ, PWM_RANGE);
		return nullptr;
	}
#if beVerbose
		printf ("Calculated PWM update frequency = %.3f\n", realPWMfreq);
#endif
	// make init data struct
	ptPWM_init_StructPtr initData = new ptPWM_init_Struct;
	initData->useFIFO = 1;
	initData-> hiFuncREG = nullptr;
	initData->hiFuncFIF1 = &ptPWM_sin_FIFO_1;
	initData->hiFuncFIF2 = &ptPWM_sin_FIFO_2;
	initData->hiFuncFIFdual = &ptPWM_sin_FIFO_dual;	
	// call PWM_sin_thread constructor with init data, which calls pulsedThread constructor
	int errCode =0;
	PWM_sin_thread * newPWM_thread = new PWM_sin_thread (initData, errCode);
	if (errCode){
#if beVerbose
		printf ("PWM_threadMaker failed to make PWM_sin_thread with errCode %d.\n", errCode);
#endif
		return nullptr;
	}
	// set custom task delete function
	newPWM_thread->setTaskDataDelFunc (&ptPWM_delTask);
	// set fields for PWMfreq and PWM Range,
	newPWM_thread->PWMfreq = realPWMfreq;
	newPWM_thread->PWMrange = PWM_RANGE;
	// make sine wave array data and add channels
	unsigned int arraySize = (unsigned int)(realPWMfreq);
	newPWM_thread->dataArray= new int [arraySize];
	double offset =PWM_RANGE/2;
	for (unsigned int ii=0; ii< arraySize; ii +=1){
		newPWM_thread->dataArray [ii] = (unsigned int) (offset - offset * cos (PHI *((double) ii/ (double) arraySize)));
	}
	// add channels , each channel can use the same array 
	if (channels & 1){
		newPWM_thread->addChannel (1, 1, PWM_BALANCED, 0, 0, newPWM_thread->dataArray, arraySize);
	}
	if (channels & 2){
		newPWM_thread->addChannel (2, 1, PWM_BALANCED, 0, 0, newPWM_thread->dataArray, arraySize);
	}
	newPWM_thread->PWMchans = channels;
	// return new thread
	return newPWM_thread;
}

PWM_sin_thread::~PWM_sin_thread (){
#if beVerbose
	printf ("PWM_sin_thread destructor called.\n");
#endif
	delete dataArray;
}

/* ****************************** sets Frequency ************************************
Last Modified:
2018/08/08 by Jamie Boyd - Initial Version
2018/09/24 by Jamie Boyd added 2 channel stuff */
int PWM_sin_thread::setSinFrequency (unsigned int newFrequency, int channel, int isLocking){
	if (channel & 1){
		sinFrequency1 = newFrequency;
	}
	if (channel & 2){
		sinFrequency2 = newFrequency;
	}
	ptPWMArrayModStructPtr arrayMod = new ptPWMArrayModStruct;
	arrayMod->startPos = newFrequency;
	arrayMod->channel = channel;
	int returnVal = modCustom (&ptPWM_sin_setFreqCallback, (void *) arrayMod, isLocking);
	return returnVal;
}

unsigned int PWM_sin_thread::getSinFrequency (int channel){
	if (channel ==1){
		return sinFrequency1;
	}else{
		return sinFrequency2;
	}
}