#ifndef PWM_SIN_THREAD_H
#define PWM_SIN_THREAD_H

#include "PWM_thread.h"


/* *********************** PWM_sin_thread ********************************************************
Subclasses PWM_thread with goal of outputting a sine wave at user-settable frequency.
To that end, range is set to 1000 and frequency is 250k. This is about as fast as you can go; 
the clock it uses is the 500MHz PLL D with the integer divider being 2.  Could get faster update with
a smaller range; these values work well for audio frequencies
*/

const unsigned int  PWM_SIN_UPDATE_FREQ = 21.6e3;
const unsigned int PWM_SIN_RANGE = 1000;
const double PHI = 6.2831853071794;


/* *********************** Forward declare functions used by thread *************************/
void ptPWM_sin_func (void *  taskData);   // the hiFunc run by pulsedThreadFunc, ouputs next PWM value
int ptPWM_setFrequencyCallback (void * modData, taskParams * theTask);


/* ********************************************* PWM_sin_thread class *********************************************
superclass is PWM_thread. Modified to continuously output a sine wave of user-set frequency, from 1Hz to 25 KHz, in steps of 1 Hz
last modified:
2018/09/12 by Jamie Boyd - initial verison */
class PWM_sin_thread : public PWM_thread{
	public:
	/* constructor, we will call the superclass constructor with float  times in microseconds and number of pulses */
	PWM_sin_thread (void * initData, int &errCode) : PWM_thread ((float)PWM_SIN_UPDATE_FREQ, 0, initData, ACC_MODE_SLEEPS_AND_OR_SPINS, errCode) {
	};
	/* Static thread maker makes and fill an init struct, calls constructor, and return a pointer to a new PWM_sin_thread */
	static PWM_sin_thread * PWM_sin_threadMaker (int channel, int enable, unsigned int initialFreq);
	// sets the frequency to output
	int setFrequency (unsigned int newFrequency, int isLocking);
	protected:
	int PWM_chan; // 0 or 1
	int polarity; // 0 for normal polarity, 1 for reversed
	int offState; // 0 for low when not enabled, 1 for high when enabled
	int enabled; // 0 for not enabled, 1 for enabled
	unsigned int frequency ;
	
};

#endif
