#ifndef PWM_THREAD_H
#define PWM_THREAD_H

#include <pulsedThread.h>
#include <math.h>
#include "GPIOlowlevel.h"

/* ************************************** PWM Thread ***************************************************
Subclasses pulsedThread to output data from an array of values on the Raspberry Pi PWM (Pulse Width Modulator) peripheral
There should only ever be 1 PWM thread, and both channels of the pWM will be controlled on the same thread.
Could implement the whole singleton design pattern thing to ensure a single instance

****************************** Declare non-class functions used by pulsed thread ******************************/
void ptPWM_Hi (void *  taskData);
int ptPWM_Init (void * initData, void *  &taskData);
void ptPWM_delTask (void * taskData);
int ptPWM_addChannelCallback (void * modData, taskParams * theTask);
int ptPWM_setEnableCallback (void * modData, taskParams * theTask);
int ptPWM_reversePolarityCallback (void * modData, taskParams * theTask);
int ptPWM_setOffStateCallback (void * modData, taskParams * theTask);
int ptPWM_ArrayModCalback  (void * modData, taskParams * theTask);


/* ******************** Custom Data Struct for pulsed Thread used by PWM ***************************
last modified:
2018/09/18 by Jamie Boyd - channels and outputs modifications
2018/09/12 by Jamie Boyd - added note that channel includes info on which pins to use
2018/08/07 byJamie Boyd - updating for pulsedThread subclass threading
2017/02/17 by Jamie Boyd - initial version */
typedef struct ptPWMStruct{
	int channels; // 1 for channel 1, 2 for channel 2
	// setings inited as default
	int mode1; //MARK_SPACE for servos or BALANCED for analog
	int enable1;  // 1 if channel is enabled, 0 if not enabled
	int polarity1; // 1 for reversed output polarity, 0 for normal, default is 0
	int offState1; // 0 for low level when PWM is not enabled, 1 for high level when PWM is enabled
	int onAudio1; // set if output is directed to audio GPIO 40, not GPIO 18
	int mode2; //MARK_SPACE for servos or BALANCED for analog
	int enable2;  // 1 if channel is enabled, 0 if not enabled
	int polarity2; // 1 for reversed output polarity, 0 for normal, default is 0
	int offState2; // 0 for low level when PWM is not enabled, 1 for high level when PWM is enabled
	int onAudio2; // set if output is directed to audio GPIO 41, not GPIO 19
	// calculated register addresses, used for customDataMod functions
	volatile unsigned int * ctlRegister; // address of PWM control register
	volatile unsigned int * statusRegister; // address of status register
	volatile unsigned int * FIFOregister; // address of FIFO
	volatile unsigned int * dataRegister1; // address of register to write data to
	volatile unsigned int * dataRegister2; // address of register to write data to
	// data for outputting
	int * arrayData1; // array of PWM values for thread to cycle through, start to end, 0 to range-1
	unsigned int nData1; // number of points in data array
	unsigned int arrayPos1; // position in data array we are currently outputting
	unsigned int startPos1;
	unsigned int stopPos1;
	int * arrayData2; // array of PWM values for thread to cycle through, start to end, 0 to range-1
	unsigned int nData2; // number of points in data array
	unsigned int arrayPos2; // position in data array we are currently outputting
	unsigned int startPos2;
	unsigned int stopPos2;
} ptPWMStruct, *ptPWMStructPtr;

/* **************custom struct for callback configuring a PWM channel *****************************
Contains data for channel configurtation and pointer to data to output 
last modified:
2018/09/19 by Jamie Boyd - initial version */
typedef struct ptPWMchanAddStruct{
	int channel; // 1 for channel 1, or 2 for channel 2
	int onAudio; // set to do outputs on audio pins, not GPIO 18 or 19
	int mode; //MARK_SPACE for servos or BALANCED for analog
	int enable; // 1 to start PWMing immediately, 0 to start in un-enabled state
	int polarity; 
	int offState;
	int * arrayData; // array of PWM values for thread to cycle through, 0 to range, for channel
	unsigned int nData; // number of points in data array for channel 
}ptPWMchanAddStruct, *ptPWMchanAddStructPtr;


/* **************custom struct for callback changing PWM array settings *****************************
Used to modify which section of the array to currently use, or set current position in the array, or change array 
last modified:
2018/09/18 by Jamie Boyd - channels and outputs modifications
2018/08/08 by Jamie Boyd - initial verison */
typedef struct ptPWMArrayModStruct{
	int channel;			// 1 for channel 0, 2 for channel 1
	int modBits;			// bit-wise for which param to modify, 1 for startPos, 2 for stopPos, 4 for arrayPos, 8 for array data
	unsigned int startPos; // where to start in the array when out putting data
	unsigned int stopPos;	// where to end in the array
	unsigned int arrayPos;	// current position in array, as it is iterated through
	int * arrayData; //data for the array
	unsigned int nData; // size of data array
}ptPWMArrayModStruct, *ptPWMArrayModStructPtr;


/* ********************************************* PWM_thread class *********************************************
last modified:
2018/09/19 by Jamie Boyd - channels and outputs modifications
2018/08/08 by Jamie Boyd - initial verison */
class PWM_thread : public pulsedThread{
	public:
	/* constructors, one with unsigned ints for pulse delay and duration times in microseconds and number of pulses */
	PWM_thread (unsigned int durUsecs, unsigned int nPulses, int accLevel , int &errCode) : pulsedThread (0, durUsecs, nPulses, nullptr, &ptPWM_Init, nullptr, &ptPWM_Hi, accLevel, errCode) {
	};
	/* and the the other constructor with floats for frequency, duty cycle, and train duration */
	PWM_thread (float frequency, float trainDuration, int accLevel, int &errCode) : pulsedThread (frequency, 1, trainDuration, nullptr, &ptPWM_Init, nullptr, &ptPWM_Hi, accLevel, errCode){
	};
	~PWM_thread ();
	// maps the GPIO, PWM, and PWMclock peripherals.  Do this before doing anything else
	static int mapPeripherals ();
	// sets PWM clock for given frequency and range. Do this before making a PWM_thread, because thread makers need to kknow range and freq
	static float setClock (float PWMFreq, unsigned int PWMrange);
	// Static ThreadMakers make an initStruct and call a constructor with it, returning a pointer to a new PWM_thread
	static PWM_thread * PWM_threadMaker (float pwmFreq, unsigned int pwmRange, unsigned int durUsecs, unsigned int nPulses, int accuracyLevel);
	static PWM_thread * PWM_threadMaker (float pwmFreq, unsigned int pwmRange,  float frequency, float trainDuration, int accuracyLevel);
	// configures one of the channels, 1 or 2, for output on the PWM. returns 0 for success, 1 for failure
	int addChannel (int channel, int onAudio, int mode, int enable, int polarity, int offState, int * arrayData, unsigned int nData);
	// mod functions for enabling PWM output, setting polarity, and 
	int setEnable (int enableState, int channel, int isLocking);
	int setPolarity (int polarityP, int channel, int isLocking);
	int setOffState (int offStateP, int channel, int isLocking);
	int setArraySubrange (unsigned int startPos, unsigned int stopPos, int channel, int isLocking);
	int setArrayPos (unsigned int arrayPos, int channel, int isLocking);
	int setNewArray (int * arrayData, unsigned int nData, int channel, int isLocking);
	// data members
	float PWMfreq;
	int PWMrange;
	int PWMchans; // bitwise pwm channels in use, 1 for channel 0, 2 for channel 1
	protected:
	int mode1; // 0 for PWM_BALANCED, 1 for MARK_SPACE
	int polarity1; // 0 for normal polarity, 1 for reversed
	int offState1; // 0 for low when not enabled, 1 for high when enabled
	int enabled1; // 0 for not enabled, 1 for enabled
	int onAudio1; // 0 for GPIO 18, 1 for default output over audio
	int mode2;
	int polarity2; // 0 for normal polarity, 1 for reversed
	int offState2; // 0 for low when not enabled, 1 for high when enabled
	int enabled2; // 0 for not enabled, 1 for enabled
	int onAudio2; // 0 for GPIO 19, 1 for default output over audio	
};

#endif
