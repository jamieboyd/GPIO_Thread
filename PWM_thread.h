#ifndef PWM_THREAD_H
#define PWM_THREAD_H

#include <pulsedThread.h>
#include "GPIOlowlevel.h"

/* *********************** Forward declare functions used by thread *************************/
void ptPWM_Hi (void * taskData);
int ptPWM_Init (void * initData, void * &taskData);

/*
task data for PWM is:
channel (0 or 1)
mode (Mark/space or balanced)
range
*/
typedef struct ptPWMStruct{
	// variables copied over from init struct
	// pwm init settings
	int channel; // 0 done on GPIO 18 or 1 done on GPIO 19
	int mode; //MARK_SPACE for servos or BALANCED for analog
	int polarity; // 0 for normal polarity, 1 for reversed
	unsigned int *  ctlRegister; // address of PWM control register
	unsigned int * dataRegister; // address of register to write data to
	// peripheral base adresses
	volatile unsigned int * GPIOperiAddr;  // base address needed when writing to registers for setting and unsetting
	volatile unsigned int * PWMperiAddr; // base address for PWM peripheral
	volatile unsigned int * PWMClockperiAddr; // base address for PWM clock stuff
	// data for outputting
	int * arrayData; // array of PWM values for thread to cycle through, start to end, 0 to range-1
	unsigned int nData; // number of points in data array
	unsigned int arrayPos; // position in data array we are currently outputting
	unsigned int startPos;
	unsigned int endPos;
	
	// these variables depend on channel (0 or 1), and will be set at intiialization
	//unsigned int rangeRegisterOffset;
	
	//unsigned int modeBit;
	//unsigned int enableBit;
} ptPWMStruct, *ptPWMStructPtr;

	
	

/* ***********************************************
 
 last modified:
2018/08/06 byJamie Boyd - updating for pulsedThread subclass threading
2017/02/17 by Jamie Boyd - initial version
*************************************************/
typedef struct ptPWMinitStruct{
	int channel; // 0 done on GPIO 18 or 1 done on GPIO 19
	int mode; //MARK_SPACE for servos or BALANCED for analog
	int polarity; // 0 for normal polarity, 1 for reversed
	int range ; // PWM clock counts per output value, sets precision of output
	int waitForEnable; // 
	volatile unsigned int * GPIOperiAddr;  // base address needed when writing to registers for setting and unsetting
	volatile unsigned int * PWMperiAddr; // base address for PWM peripheral
	unsigned int nData; // number of points in data array
	int * arrayData; // array of PWM values for thread to cycle through, 0 to range
}ptPWMinitStruct, *ptPWMinitStructPtr;

// function declarations
void ptPWM_delChan (char bcm_PWM_chan);
void ptPWM_cleanUp(bcm_peripheralPtr &GPIOperi, bcm_peripheralPtr &PWMperi, bcm_peripheralPtr &PWMClockperi);
//int ptPWM_mapPeripherals (bcm_peripheralPtr &GPIOperi, bcm_peripheralPtr &PWMperi, bcm_peripheralPtr &PWMClockperi);
//float ptPWM_SetClock (float newFreq, int PWMrange, bcm_peripheralPtr PWMperi, bcm_peripheralPtr PWMClockperi);
int ptPWM_enableCallback (void * modData, taskParams * theTask);
int ptPWM_setNumDataCallback (void * modData, taskParams * theTask);
int ptPWM_setArrayPosCallback (void * modData, taskParams * theTask);
int ptPWM_setArrayCallback (void * modData, taskParams * theTask);


class PWM_thread : public pulsedThread{
	public:
	/* constructors, one expects unsigned ints for pulse delay and duration times in microseconds and number of pulses 
	 PWM has duration but no delay, has a channel (0 on pin 18, 1 on pin 19) instead of a GPIO pin, pulses is always 0 */
	PWM_thread (unsigned int durUsecs, unsigned int nPulses, void * initData, int accLevel , int &errCode) : pulsedThread (0, durUsecs, nPulses, initData, &ptPWM_Init, nullptr, &ptPWM_Hi, accLevel, errCode) {
	};
	/* the other constructor expects floats for frequency, duty cycle, and train duration */
	PWM_thread (float frequency, float trainDuration, void * initData, int accLevel, int &errCode) : pulsedThread (frequency, 1, trainDuration, initData, &ptPWM_Init, nullptr, &ptPWM_Hi, accLevel,errCode) {
	};
	/* Static ThreadMakers make an initStruct and call a constructor with it, returning a pointer to a PWM_thread */
	static PWM_thread * PWM_threadMaker (int channel, int mode, int polarity, int enable, int * arrayData, unsigned int nData, unsigned int  durUsecs, unsigned int nPulses, int accuracyLevel);
	static PWM_thread * PWM_threadMaker (int channel, int mode, int polarity,  int enable, int * arrayData, unsigned int nData, float frequency, float trainDuration, int accuracyLevel);
	// maps the GPIO, PWM, and PWMclock perip[herals
	static int mapPeripherals ();
	// sets PWM clock for given frequency and range, do this before enabling either PWM channel to start output
	static float setClock (float PWMFreq, int PWMrange);
	// data members
	static float PWMfreq;
	static int PWMrange;
	//protected:

	
	
};

#endif
