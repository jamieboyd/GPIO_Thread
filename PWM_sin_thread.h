#ifndef PWM_SIN_THREAD_H
#define PWM_SIN_THREAD_H
#include "PWM_thread.h"

/* *********************** PWM_sin_thread ********************************************************
Subclasses PWM_thread with goal of outputting a cleanish sine wave at user-settable frequency without completely 
maximizing a processor core. To that end, set PWM update frequency to 50 - 100 kHz and set PWM range to
1000.

The pulsedThread is configured as an infinite train. Because we use the FIFO for PWM data,
we can set the thread update rate  around 10X slower than PWM update frequency, and we can use the 
less intensive ACC_MODE_SLEEPS_AND_SPINS thread timing mode. 

******************************************** Constants for PWM_sin_thread ***************************************************/
static const unsigned int PWM_UPDATE_FREQ = 40E03; // 				the PWM output is updated at this frequency
static const float THREAD_UPDATE_FREQ = (PWM_UPDATE_FREQ/9); //		pulsed thread update frequency, slower than PWM update, try 8-10X slower
static const unsigned int PWM_RANGE = 1000; //					data for sine wave ranges from 0 to 999
static const double PHI = 6.2831853071794;  //						this is just pi * 2, used for making a sin wave

 /* ********************** Forward declare new functions used by thread *************************/
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
	PWM_sin_thread (int &errCode) : PWM_thread (THREAD_UPDATE_FREQ, 0.0, ACC_MODE_SLEEPS_AND_SPINS, errCode) {};
	~PWM_sin_thread (void);
	/* Static thread maker calls constructor, and return a pointer to a new PWM_sin_thread */
	static PWM_sin_thread * PWM_sin_threadMaker (int channels);
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
