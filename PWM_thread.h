#ifndef PWM_THREAD_H
#define PWM_THREAD_H

#include <pulsedThread.h>
#include "GPIOlowlevel.h"

#define PWM_MARK_SPACE 0
#define PWM_BALANCED 1

/* *********************** Forward declare functions used by thread so we can refer to them in constructors*************************/
void ptPWM_Hi (void * volatile taskData);


/*
task data for PWM is:
channel (0 or 1)
mode (Mark/space or balanced)
range
*/
typedef struct PWMStruct{
	// variables copied over from init struct
	// pwm init settings
	int channel; // 0 done on GPIO 18 or 1 done on GPIO 19
	int mode; //MARK_SPACE for servos or BALANCED for analog
	int range; // counter counts per cycle for this channel
	volatile unsigned int * GPIOaddr;
	volatile unsigned int * PWMaddr;
	// data for outputting
	unsigned int dataPos; // starting position in data array we are currently outputting
	unsigned int nData; // number of points in data array
	int * pwmData; // array of PWM values for thread to cycle through, 0 to range
	// these variables depend on channel, can be set at intiialization
	unsigned int rangeRegisterOffset;
	unsigned int dataRegisterOffset;
	unsigned int modeBit;
	unsigned int enableBit;
} PWMStruct, *PWMStructPtr;


/* ***********************************************
 
 last modified:
 2017/02/17 by Jamie Boyd - initial version
*************************************************/
typedef struct PWMInitStruct{
	int channel; //0 done on GPIO 18 or 1 done on GPIO 19
	int mode;
	int range;
	volatile unsigned int * GPIOaddr;
	volatile unsigned int * PWMaddr;
	unsigned int dataPos; // starting position in data array we are currently outputting
	unsigned int nData; // number of points in data array
	int * pwmData; // array of PWM values for thread to cycle through, 0 to range
	int waitForEnable; // set this value to not start PWming immediately, but to wait for enable signal
}PWMInitStruct, *PWMInitStructPtr;

// function declarations
void ptPWM_delChan (char bcm_PWM_chan);
void ptPWM_cleanUp(bcm_peripheralPtr &GPIOperi, bcm_peripheralPtr &PWMperi, bcm_peripheralPtr &PWMClockperi);
int ptPWM_mapPeripherals (bcm_peripheralPtr &GPIOperi, bcm_peripheralPtr &PWMperi, bcm_peripheralPtr &PWMClockperi);
float ptPWM_SetClock (float newFreq, int PWMrange, bcm_peripheralPtr PWMperi, bcm_peripheralPtr PWMClockperi);
int ptPWM_enableCallback (void * modData, taskParams * theTask);
int ptPWM_setNumDataCallback (void * modData, taskParams * theTask);
int ptPWM_setArrayPosCallback (void * modData, taskParams * theTask);
int ptPWM_setArrayCallback (void * modData, taskParams * theTask);
int ptPWM_Init (void * initData, void *  volatile &taskData);
void ptPWM_Lo (void * volatile taskData); // blank, as we don't have HI and LO, just need HI


class PWM_thread : public pulsedThread{
	public:
	/* constructors, similar to pulsedThread, one expects unsigned ints for pulse delay and duration times in microseconds and number of pulses */
	PWM_thread (int pinP, int polarityP, unsigned int delayUsecs, unsigned int durUsecs, unsigned int nPulses, void * initData, int accLevel , int &errCode) : pulsedThread (delayUsecs, durUsecs, nPulses, initData, &SimpleGPIO_Init, &SimpleGPIO_Lo, &SimpleGPIO_Hi, accLevel, errCode) {
	pinNumber = pinP;
	polarity = polarityP;
	};



#endif