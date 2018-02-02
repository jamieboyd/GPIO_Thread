#ifndef SIMPLEGPIO_THREAD
#define SIMPLEGPIO_THREAD

#include <pulsedThread.h>
#include "GPIOlowlevel.h"

/* ****************** Initialization Data ********************************
 memory mapped addresses to write to on HI and Lo, and GPIO pin bit */
typedef struct SimpleGPIOInitStruct{
	unsigned int * GPIOperiHi;	// memory mapped addresss of GPIO register to write to on HI
	unsigned int * GPIOperiLo;	// memory mapped address of GPIO register to write to on LO
	unsigned int pinBit;		// pin number translated to bit position in register
}SimpleGPIOInitStruct, *SimpleGPIOInitStructPtr;

/* ******************** Custom Data Struct for Simple GPIO***************************
includes a pointer for end Function data */
typedef struct SimpleGPIOStruct{
	unsigned int * GPIOperiHi; // address of register to write pin bit to on Hi
	unsigned int * GPIOperiLo; // address of register to write pin bit to on Lo
	unsigned int pinBit;	// pin number translated to bit position in register
	void * endFuncData;		// pointer to data for end function
}SimpleGPIOStruct, *SimpleGPIOStructPtr;

/* ******************* A Custom struct for endFunc Data **************************
for the two provided endFuncs that change frequency and dutyCycle for trains */
typedef struct SimpleGPIOArrayStruct{
	float * arrayData;		// pointer to an array of floats
	unsigned int nData;		// size of array, or at least size of initialized data in array
	unsigned int arrayPos;	// current position in array, as it is iterated through
}SimpleGPIOArrayStruct, *SimpleGPIOArrayStructPtr;

/* *********************SimpleGPIO_thread class extends pulsedThread ****************
Does pulses and trains of pulses on Raspberry Pi GPIO pins */
class SimpleGPIO_thread : public pulsedThread{
	public:
	/* constructors, same as pulsedThread, one expecting unsigned ints based on pulse time in microseconds, 
	the other expecting floats based on frequency, duty cycle, and train duration */
	SimpleGPIO_thread  (unsigned int delayUsecs, unsigned int durUsecs, unsigned int nPulses, void * initData, int (*initFunc)(void *, void *  &), void (* loFunc)(void *), void (*hiFunc)(void *), int accLevel , int &errCode) : pulsedThread (delayUsecs, durUsecs, nPulses, initData, initFunc, loFunc, hiFunc, accLevel,errCode) {};
	SimpleGPIO_thread  (float frequency, float dutyCycle, float trainDuration, void * initData, int (*initFunc)(void *, void *  &), void (* loFunc)(void *), void (*hiFunc)(void *), int accLevel , int &errCode) : pulsedThread (frequency, dutyCycle, trainDuration, initData, initFunc, loFunc, hiFunc, accLevel,errCode) {};
	/* Static ThreadMakers call constructors after making initStruct and return a pointer to a SimpleGPIO_thread */
	static SimpleGPIO_thread * SimpleGPIO_threadMaker (int thePin, int polarity, unsigned int delayUsecs, unsigned int  durUsecs, unsigned int nPulses, int accuracyLevel);
	static SimpleGPIO_thread * SimpleGPIO_threadMaker (int thePin, int polarity, float frequency, float dutyCycle, float trainDuration, int accuracyLevel);
	/* static variables to map GPIO peripheral and track usage */
	static bcm_peripheralPtr GPIOperi ;
	static int GPIOperi_users;
	// destructor
	~SimpleGPIO_thread ();
	private:
	int pinNumber;
	int polarity;
	// utility functions
	/*
	void setPin (int pin);
	int getPin (void );
	void setLevel (int level);
	protected:
	 */
};



#endif