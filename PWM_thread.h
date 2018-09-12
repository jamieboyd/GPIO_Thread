#ifndef PWM_THREAD_H
#define PWM_THREAD_H

#include <pulsedThread.h>
#include <math.h>
#include "GPIOlowlevel.h"

/* *********************** Forward declare functions used by thread *************************/
void ptPWM_Hi (void *  taskData);
int ptPWM_Init (void * initData, void *  &taskData);
void ptPWM_delTask (char bcm_PWM_chan);
int ptPWM_setEnableCallback (void * modData, taskParams * theTask);
int ptPWM_reversePolarityCallback (void * modData, taskParams * theTask);
int ptPWM_setOffStateCallback (void * modData, taskParams * theTask);
int ptPWM_setArrayPosCallback (void * modData, taskParams * theTask);
int ptPWM_ArrayModCalback  (void * modData, taskParams * theTask);

/* ********************************* Initialization struct for PWM ******************************
last modified:
2018/08/07 byJamie Boyd - updating for pulsedThread subclass threading
2017/02/17 by Jamie Boyd - initial version */
typedef struct ptPWMinitStruct{
	int channel; // 0 done on GPIO 18 or 1 done on GPIO 19
	int mode; //MARK_SPACE for servos or BALANCED for analog
	int range ; // PWM clock counts per output value, sets precision of output, from static class variable
	int enable; // 1 to start PWMing immediatley, 0 to start in unenabled state
	int * arrayData; // array of PWM values for thread to cycle through, 0 to range
	unsigned int nData; // number of points in data array
}ptPWMinitStruct, *ptPWMinitStructPtr;


/* ******************** Custom Data Struct for PWM ***************************
last modified:
2018/08/07 byJamie Boyd - updating for pulsedThread subclass threading
2017/02/17 by Jamie Boyd - initial version */
typedef struct ptPWMStruct{
	// copied from pwm init settings
	int channel; // 0 done on GPIO 18 or 1 done on GPIO 19, 2 for channel 0 through audio on pin 40, 3 for channel 1 on audio through pin 41
	int mode; //MARK_SPACE for servos or BALANCED for analog
	// setings inited as default
	int enable;  // 1 if channel is enabled, 0 if not enabled
	int polarity; // 1 for reversed output polarity, 0 for normal, default is 0
	int offState; // 0 for low level when PWM is not enabled, 1 for high level when PWM is enabled
	// calculated register addresses, used for customDataMod functions
	volatile unsigned int *  ctlRegister; // address of PWM control register
	volatile unsigned int * dataRegister; // address of register to write data to
	// data for outputting
	int * arrayData; // array of PWM values for thread to cycle through, start to end, 0 to range-1
	unsigned int nData; // number of points in data array
	unsigned int arrayPos; // position in data array we are currently outputting
	unsigned int startPos;
	unsigned int endPos;
} ptPWMStruct, *ptPWMStructPtr;


/* **************custom struct for changing PWM array settings *****************************
Used to modify which section of the array to currently use, or set current position in the array, or change array 
last modified:
2018/08/08 by Jamie Boyd - initial verison */
typedef struct ptPWMArrayModStruct{
	int modBits;			// bit-wise for which param to modify, 1 for startPos, 2 for endPos, 4 for arrayPos, 8 for array data
	unsigned int startPos; // where to start in the array when out putting data
	unsigned int endPos;		// where to end in the array
	unsigned int arrayPos;	// current position in array, as it is iterated through
	int * arrayData; //data for the array
}ptPWMArrayModStruct, *ptPWMArrayModStructPtr;


/* ********************************************* PWM_thread class *********************************************
last modified:
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
	static PWM_thread * PWM_threadMaker (int channel, int mode, int enable, int * arrayData, unsigned int nData, unsigned int  durUsecs, unsigned int nPulses, int accuracyLevel);
	static PWM_thread * PWM_threadMaker (int channel, int mode,  int enable, int * arrayData, unsigned int nData, float frequency, float trainDuration, int accuracyLevel);
	// maps the GPIO, PWM, and PWMclock perip[herals. DO this before anything else
	static int mapPeripherals ();
	// sets PWM clock for given frequency and range, do this before enabling either PWM channel to start output
	static void setClock (float PWMFreq);
	int setEnable (int enableState, int isLocking);
	int setPolarity (int polarityP, int isLocking);
	int setOffState (int offStateP, int isLocking);
	// data members
	static float PWMfreq;
	static int PWMrange;
	static int PWMchans; // bitwise pwm channels in use init at 0
	int PWM_chan;
	int polarity; // 0 for normal polarity, 1 for reversed
	int offState; // 0 for low when not enabled, 1 for high when enabled
	int enabled; // 0 for not enabled, 1 for enabled
	
	
};

#endif
