#include "PWM_sin_thread.h"

static const unsigned int PWM_UPDATE_FREQ = 100e03; // The PWM output is updated at this frequency
static const float THREAD_UPDATE_FREQ = 10e03; // pulsed thread update frequency, slower than PWM update, try 10X slower
static const unsigned int PWM_RANGE = 1000; // data for sine wave ranges from 0 to 999
static const double PHI = 6.2831853071794;  // this is just pi * 2, used for making a sin wave

/* ******************************************  PWM_sin_thread functions for pulsedThread *****************************************


 *********************************** PWM_sin Hi functions (no low function) ******************************
gets the next value from the array data to be output, depending on frequency, and writes it to the FIFO register
arrayData contains a sine wave with PWM_UPDATE_FREQ points.  When output at PWM_UPDATE_FREQ, you get a 1 Hz output.  The idea is to make a sine wave at
lowest ever needed frequency, in this case 1 Hz, and instead of changing the array data when we change frequencies, we change the step
by which we jump. We can easily do any multiple of the base frequency.  With 1Hz sine wave, we add 1 each time for 1 Hz, add 2 each time
for 2 Hz, etc, using the modulo operator % so we don't get tripped up at wrap-around. 

/* *********************************** PWM Hi function when using FIFO for channel 1 ******************************
checks the FULL bit and feeds the FIFO from channel 1 array till it is full
Last Modified:
2018/09/24 - by Jamie Boyd - initial version */
void ptPWM_sin_FIFO_1 (void * taskDataP){
	ptPWMStructPtr taskData = (ptPWMStructPtr)taskDataP;
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

/* ************************ callback to turn on or off use of FIFO vs data registers *********************
modData is a pointer to an int, 0 to use dataRegisters, 1 to use FIFO for either or both channels
Last Modified:
2018/09/23 by Jamie Boyd - initial version */
int ptPWM_sin_setFIFOCallback (void * modData, taskParams * theTask){
	ptPWMStructPtr taskData = (ptPWMStructPtr) theTask->taskData;
	taskData->useFIFO = 1;
	*(PWMperi ->addr + PWM_CTL) |= PWM_USEF1;
	*(PWMperi ->addr + PWM_CTL) |= PWM_USEF2;
	if (taskData->channels ==3){ 
		theTask->hiFunc = &ptPWM_sin_FIFO_dual;
		// can't use repeat last when both channels are active
		*(PWMperi ->addr + PWM_CTL) &= ~PWM_RPTL1;
		*(PWMperi ->addr + PWM_CTL) &= ~PWM_RPTL2;
	}else{
		if (taskData->channels ==  2){
			theTask->hiFunc =  &ptPWM_sin_FIFO_2;
		}else{
			theTask->hiFunc =  &ptPWM_sin_FIFO_1;
		}
		*(PWMperi ->addr + PWM_CTL) |=  PWM_RPTL1;
		*(PWMperi ->addr + PWM_CTL) |= PWM_RPTL2;
	}
	return 0;
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
PWM_sin_thread * PWM_sin_thread::PWM_sin_threadMaker (void){
	return (PWM_sin_thread *)PWM_thread::PWM_threadMaker(PWM_UPDATE_FREQ, PWM_RANGE, 1, THREAD_UPDATE_FREQ, kINFINITETRAIN,ACC_MODE_SLEEPS_AND_SPINS);
}

/* ************************************ SetsPWM peripheral to use the FIFO  *************************** 
if both channels are being used, they either both use the FIFO, or both use their respective data registers
Last Modified:
2018/09/24 by Jamie Boyd - intial verison modified from base class to call ptPWM_sin_setFIFOCallback */
int PWM_thread::setFIFO (int FIFOstate, int isLocking){
	return modCustom (&ptPWM_sin_setFIFOCallback, nullptr, isLocking);
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