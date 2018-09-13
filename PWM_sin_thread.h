#ifndef PWM_SIN_THREAD_H
#define PWM_SIN_THREAD_H

#include <PWM_thread.h>


/* *********************** PWM_sin_thread ********************************************************
Subclasses PWM_thread with goal of outputting a sine wave at user-settable frequency.
To that end, range is set to 1000 and frequency is 250k. This is about as fast as you can go; 
the clock it uses is the 500MHz PLL D with the integer divider being 2.  Could get faster update with
a smaller range; these values work well for audio frequencies
*/

const float PWM_SIN_UPDATE_FREQ = 250e3;
const unsigned int PWM_SIN_RANGE = 1000;
const double PHI = 6.2831853071794;




/* *********************** Forward declare functions used by thread *************************/
void ptPWM_sin_func (void *  taskData);
int ptPWM_sin_Init (void * initData, void *  &taskData);





/* ********************************************* PWM_sin_thread class *********************************************
superclass is PWM_thread
last modified:
2018/09/12 by Jamie Boyd - initial verison */
class PWM_sin_thread : public PWM_thread{
	public:
	/* constructor, we will call the superclass constructor with float  times in microseconds and number of pulses */
	PWM_sin_thread (void * initData, int &errCode) : PWM_thread (PWM_SIN_UPDATE_FREQ, 0, initData, ACC_MODE_SLEEPS_AND_OR_SPINS, errCode) {
	};
	/* Static thread maker makes and fill an init struct, calls constructor, and return a pointer to a new PWM_sin_thread */
	static PWM_thread * PWM_sin_threadMaker (int channel, int mode, int enable);
