#ifndef PWM_SIN_THREAD_H
#define PWM_SIN_THREAD_H
#include "PWM_thread.h"


/* *********************** PWM_sin_thread ********************************************************
Subclasses PWM_thread with goal of outputting a cleanish sine wave at user-settable frequency.
To that end, range is set to 1000 and PWM update frequency is 100 kHz.

The pulsedThread is configured as an infinite train. Because we use the FIFO for PWM data,
we can set the thread update rate 10X slower than PWM update frequency, and we can use the 
less intensive ACC_MODE_SLEEPS_AND_SPINS thread timing mode.

 *********************** Forward declare new functions used by thread *************************/
void ptPWM_sin_FIFO_1 (void * taskDataP);
void ptPWM_sin_FIFO_2 (void * taskDataP);
void ptPWM_sin_FIFO_dual (void * taskDataP);
int ptPWM_sin_setFreqCallback (void * modData, taskParams * theTask);


/* ********************************************* PWM_sin_thread class *********************************************
superclass is PWM_thread. Modified to continuously output a sine wave of user-set frequency, from 1Hz to 25 KHz, in steps of 1 Hz
last modified:
2018/09/21 by Jamie Boyd - updated for separate channels, and using FIFO 
2018/09/12 by Jamie Boyd - initial verison */
class PWM_sin_thread : public PWM_thread{
	public:
	~PWM_sin_thread (void);
	void clearDataArray(void);
	/* Static thread maker makes and fill an init struct, calls constructor, and return a pointer to a new PWM_sin_thread */
	static PWM_sin_thread * PWM_sin_threadMaker (int channels);
	int setFIFO (int FIFOstate, int isLocking);
	// sets the frequency to output
	int setSinFrequency (unsigned int newFrequency, int channel, int isLocking);
	// gets the frequency being output
	unsigned int getSinFrequency (int channel);
	protected:
	unsigned int sinFrequency1 ;
	unsigned int sinFrequency2 ;
	int * dataArray ;
	
};

#endif
