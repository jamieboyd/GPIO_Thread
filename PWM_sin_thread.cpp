#include "PWM_sin_thread.h"

/* ******************************************  PWM_sin_thread  functions for pulsedThread *****************************************


 *********************************** PWM_sin Hi function (no low function) ******************************
gets the next value from the array to be output, depending on frequency, and writes it to the data register arrayData contains a sine wave with
PWM_SIN_UPDATE_FREQ points.  When output at PWM_SIN_UPDATE_FREQ, you get a 1 Hz output.  The idea is to make a sine wave at
lowest ever needed frequency, in this case 1 Hz, and instead of changing the array data when we change frequencies, we change the step
by which we jump. We can easily do any multiple of the base frequency.  With 1Hz sine wave, we add 1 each time for 1 Hz, add 2 each time
for 2 Hz, etc, using the modulo operator % so we don't get tripped up at wrap-around. 
Last Modified:
2018/09/13 by Jamie Boyd - original verison*/
void ptPWM_sin_func (void * taskDataP){
	ptPWMStructPtr taskData = (ptPWMStructPtr)taskDataP;
	*(taskData->dataRegister) = taskData-> arrayData[taskData->arrayPos];
	taskData->arrayPos += taskData->startPos;  // startPos is hijacked to use  for frequency, so we can use same structs as superclass. 
	if (taskData->arrayPos >= taskData->nData){
		taskData->arrayPos = taskData->arrayPos % (taskData->nData);
	}
}

/* *************************** Sets frequency of sine wave to be output ***************
modData is a pointer to an unsigned int, the frequency in Hz, to be next output, and outputs it
last modified:
2018/09/13 by Jamie Boyd - initial version*/
int ptPWM_setFrequencyCallback (void * modData, taskParams * theTask){
	unsigned int * newFreq= (unsigned int *)modData;
	ptPWMStructPtr taskData = (ptPWMStructPtr) theTask->taskData;
	taskData->startPos = *newFreq;  // startPos is hijacked to use  for frequency, so we can use same structs as superclass. 
	delete newFreq;
	return 0;
}


/* **********************************************  PWM_sin_thread class functions ************************************************


 ********************************************* PWM_sin_threadMaker***********************************************************
returns a pointer to a new PWM_sin_thread object.  */
PWM_sin_thread * PWM_sin_thread::PWM_sin_threadMaker (int channel, int enable, unsigned int initialFreq){
	
	
	// ensure peripherals for PWM controller are mapped
	int mapResult = PWM_thread::mapPeripherals ();
	if (mapResult){
		printf ("Could not map peripherals for PWM access with return code %d\n.", mapResult);
		return nullptr;
	}
	
	// set clock for PWM from variables
	if (PWM_thread::PWMfreq != PWM_SIN_UPDATE_FREQ){
		PWM_thread::setClock (PWM_SIN_UPDATE_FREQ);
		if (PWM_thread::PWMfreq == -1){
			printf ("Could not set clock for PWM with frequency = %d and range = %d.\n", PWM_SIN_UPDATE_FREQ, PWM_thread::PWMrange);
			return nullptr;
		}
		printf ("PWM update frequency = %.3f\n", PWM_thread::PWMfreq);
	}
	
	
	// set channel bit in the class field for channels in use, or exit if already in use
	unsigned int chanBit = channel & 1; // will be 0 or 1
	if (PWM_thread::PWMchans & chanBit){
#if beVerbose
		printf ("PWM channel %d is already in use.\n", chanBit);
#endif
		return nullptr;
	}
	PWM_thread::PWMchans |= chanBit;
	// make and fill an init struct
	ptPWMinitStructPtr initStruct = new ptPWMinitStruct;
	initStruct->channel = channel;
	initStruct->mode = PWM_BALANCED;
	initStruct -> enable = enable;
	// arrayData contains a sine wave with PWM_SIN_UPDATE_FREQ points. 
	int * arrayData = new int [PWM_SIN_UPDATE_FREQ];
	double arraySize = (double)PWM_SIN_UPDATE_FREQ;
	double offset = PWM_SIN_RANGE/2;
	for (double ii=0; ii< arraySize; ii +=1){
		arrayData [(unsigned int)ii] = (unsigned int) round (offset - offset * sin (PHI *(ii/ arraySize)));
	}
	initStruct->arrayData = arrayData;
	initStruct->nData = PWM_SIN_UPDATE_FREQ;
	initStruct ->range =  PWM_thread::PWMrange;
	// call PWM_sin_thread constructor, which calls PWM_thread constructor
	int errCode =0;
	PWM_sin_thread * new_pwm_sin = new PWM_sin_thread ((void *) initStruct, errCode) ;
	if (errCode){
#if beVerbose
		printf ("PWM_sin_threadMaker failed to make PWM_sin_thread.\n");
#endif
		return nullptr;
	}
	// set custom task delete function
	new_pwm_sin->setTaskDataDelFunc (&ptPWM_delTask);
	// set object variables 
	new_pwm_sin ->PWM_chan = channel;
	new_pwm_sin ->offState =0; // 0 for low when not enabled, 1 for high when enabled
	new_pwm_sin->polarity = 0; // 0 for normal polarity, 1 for reversed
	new_pwm_sin ->enabled=enable; // 0 for not enabled, 1 for enabled
	PWM_sin_thread::PWMchans |= chanBit; // set channel bit in supclass static field (subclass inherits this, right?)
	// install PWM_sin_thread custom function to replace PWM_Thread
	new_pwm_sin->setHighFunc (&ptPWM_sin_func); // sets the function that is called on high part of cycle
	// set initial frequency
	new_pwm_sin->setFrequency (initialFreq, 0);
	return new_pwm_sin;
}


/* ****************************** sets Polarity ************************************
Last Modified:
2018/08/08 by Jamie Boyd - Initial Version  */
int PWM_sin_thread::setFrequency (unsigned int newFrequency, int isLocking){
	
	frequency = newFrequency;
	int * newFrequencyVal = new int;
	* newFrequencyVal =  newFrequency;
	int returnVal = modCustom (&ptPWM_setFrequencyCallback, (void *) newFrequencyVal, isLocking);
	return returnVal;
}


