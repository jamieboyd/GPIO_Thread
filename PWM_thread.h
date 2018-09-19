#ifndef PWM_THREAD_H
#define PWM_THREAD_H

#include <pulsedThread.h>
#include <math.h>
#include "GPIOlowlevel.h"

/* *********************** Forward declare functions used by thread *************************/
void ptPWM_Hi (void *  taskData);
int ptPWM_Init (void * initData, void *  &taskData);
void ptPWM_delTask (void * taskData);
int ptPWM_setEnableCallback (void * modData, taskParams * theTask);
int ptPWM_reversePolarityCallback (void * modData, taskParams * theTask);
int ptPWM_setOffStateCallback (void * modData, taskParams * theTask);
int ptPWM_setArrayPosCallback (void * modData, taskParams * theTask);
int ptPWM_ArrayModCalback  (void * modData, taskParams * theTask);

/* ********************************* Initialization struct for PWM ******************************
last modified:
2018/09/18 by Jamie Boyd - channels and outputs modifications
2018/08/07 byJamie Boyd - updating for pulsedThread subclass threading
2017/02/17 by Jamie Boyd - initial version */
typedef struct ptPWMinitStruct{
	int channel; // 1 for channel 0, 2  for channel 1, plus 4 to do outputs on audio, not GPIO 18 and 19
	int mode; //MARK_SPACE for servos or BALANCED for analog
	int range ; // PWM clock counts per output value, sets precision of output, from static class variable
	int enable; // 1 to start PWMing immediately, 0 to start in un-enabled state
	int * arrayData0; // array of PWM values for thread to cycle through, 0 to range, for channel 0
	int * arrayData1; // array of PWM values for thread to cycle through, 0 to range, for channel 1
	unsigned int nData0; // number of points in data array for channel  0
	unsigned int nData1; // number of points in data array for channel  1
}ptPWMinitStruct, *ptPWMinitStructPtr;


/* ******************** Custom Data Struct for PWM ***************************
last modified:
2018/09/18 by Jamie Boyd - channels and outputs modifications
2018/09/12 by Jamie Boyd - added note that channel includes info on which pins to use
2018/08/07 byJamie Boyd - updating for pulsedThread subclass threading
2017/02/17 by Jamie Boyd - initial version */
typedef struct ptPWMStruct{
	// copied from pwm init settings
	int channel; // 1 for channel 0, 2 for channel 1, plus 4 to do output through audio on pin 40 for channel 0, pin 41 for channel 1
	int mode; //MARK_SPACE for servos or BALANCED for analog
	// setings inited as default
	int enable0;  // 1 if channel is enabled, 0 if not enabled
	int enable1;  // 1 if channel is enabled, 0 if not enabled
	int polarity0; // 1 for reversed output polarity, 0 for normal, default is 0
	int polarity1; // 1 for reversed output polarity, 0 for normal, default is 0
	int offState0; // 0 for low level when PWM is not enabled, 1 for high level when PWM is enabled
	int offState1; // 0 for low level when PWM is not enabled, 1 for high level when PWM is enabled
	// calculated register addresses, used for customDataMod functions
	volatile unsigned int *  ctlRegister0; // address of PWM control register
	volatile unsigned int * dataRegister0; // address of register to write data to
	volatile unsigned int *  ctlRegister1; // address of PWM control register
	volatile unsigned int * dataRegister1; // address of register to write data to
	// data for outputting
	int * arrayData0; // array of PWM values for thread to cycle through, start to end, 0 to range-1
	unsigned int nData0; // number of points in data array
	unsigned int arrayPos0; // position in data array we are currently outputting
	unsigned int startPos0;
	unsigned int stopPos0;
	int * arrayData1; // array of PWM values for thread to cycle through, start to end, 0 to range-1
	unsigned int nData1; // number of points in data array
	unsigned int arrayPos1; // position in data array we are currently outputting
	unsigned int startPos1;
	unsigned int stopPos1;
} ptPWMStruct, *ptPWMStructPtr;


/* **************custom struct for changing PWM array settings *****************************
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
	
}ptPWMArrayModStruct, *ptPWMArrayModStructPtr;


/* ********************************************* PWM_thread class *********************************************
last modified:
2018/09/18 by Jamie Boyd - channels and outputs modifications
2018/08/08 by Jamie Boyd - initial verison */
class PWM_thread : public pulsedThread{
	public:
	/* constructors, one with unsigned ints for pulse delay and duration times in microseconds and number of pulses */
	PWM_thread (unsigned int durUsecs, unsigned int nPulses, void *  initData, int accLevel , int &errCode) : pulsedThread (0, durUsecs, nPulses, initData, &ptPWM_Init, nullptr, &ptPWM_Hi, accLevel, errCode) {
	};
	/* and the the other constructor with floats for frequency, duty cycle, and train duration */
	PWM_thread (float frequency, float trainDuration, void *  initData, int accLevel, int &errCode) : pulsedThread (frequency, 1, trainDuration, initData, &ptPWM_Init, nullptr, &ptPWM_Hi, accLevel,errCode) {
	};
	~PWM_thread ();
	/* Static ThreadMakers make an initStruct and call a constructor with it, returning a pointer to a new PWM_thread */
	static PWM_thread * PWM_threadMaker (int channel, int mode, int enable, unsigned int updateFreq, int * arrayData, unsigned int nData, unsigned int  durUsecs, unsigned int nPulses, int accuracyLevel);
	static PWM_thread * PWM_threadMaker (int channel, int mode,  int enable, unsigned int updateFreq, int * arrayData, unsigned int nData, float frequency, float trainDuration, int accuracyLevel);
	// maps the GPIO, PWM, and PWMclock perip[herals. DO this before anything else
	static int mapPeripherals ();
	// sets PWM clock for given frequency and range, do this before enabling either PWM channel to start output
	static void setClock (float PWMFreq, int PWMrange);
	int setEnable (int enableState, int channel, int isLocking);
	int setPolarity (int polarityP, int channel, int isLocking);
	int setOffState (int offStateP, int channel, int isLocking);
	// data members
	float PWMfreq;
	int PWMrange;
	int PWMchans; // bitwise pwm channels in use, 1 for channel 0, 2 for channel 1
	protected:
	int polarity0; // 0 for normal polarity, 1 for reversed
	int offState0; // 0 for low when not enabled, 1 for high when enabled
	int enabled0; // 0 for not enabled, 1 for enabled
	int polarity1; // 0 for normal polarity, 1 for reversed
	int offState1; // 0 for low when not enabled, 1 for high when enabled
	int enabled1; // 0 for not enabled, 1 for enabled
	
	
};

#endif
