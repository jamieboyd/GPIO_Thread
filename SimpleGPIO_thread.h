#ifndef SIMPLEGPIO_THREAD
#define SIMPLEGPIO_THREAD

#include <pulsedThread.h>
#include "GPIOlowlevel.h"

/* **************************************************
 Init data are mapped addresses to write to on HI and Lo, GPIO pin number and pulse polarity, */
typedef struct SimpleGPIOInitStruct{
	unsigned int * GPIOperiHi;	// memory mapped addresss of GPIO register to write to on HI
	unsigned int * GPIOperiLo;	// memory mapped address of GPIO register to write to on LO
	int pin;					// GPIO pin number in Broadcom numbering, always
	unsigned int pinBit;		// pin number translated to bit position in register
	int polarity;				// initialization state, 0 for low-to-high, 1 for high-to-low.
}SimpleGPIOInitStruct, *SimpleGPIOInitStructPtr;


/* ************************************************/
typedef struct SimpleGPIOStruct{
	unsigned int * GPIOperiHi; // address of register to write pin bit to on Hi
	unsigned int * GPIOperiLo; // address of register to write pin bit to on Lo
	int pin;				// number of the GPIO pin, in Broadcom numbering, always
	unsigned int pinBit;	// pin number translated to bit position in register
	void * endFuncData;
}SimpleGPIOStruct, *SimpleGPIOStructPtr;

/*CustomData struct for the endFuncData for the two provided endFuncs that change frequency and dutyCycle for trains */
typedef struct SimpleGPIOArrayStruct{
	float * arrayData;
	unsigned int nData;
	unsigned int arrayPos;
}SimpleGPIOArrayStruct, *SimpleGPIOArrayStructPtr;


class SimpleGPIO_thread : public pulsedThread{
	public:
	// constructors
	SimpleGPIO_thread  (unsigned int  delayUsecs, unsigned int durUsecs, unsigned int nPulses, void *  initData, int (*initFunc)(void *, void *  &), void (* loFunc)(void *), void (*hiFunc)(void *), int accLevel , int &errCode) : pulsedThread (delayUsecs, durUsecs, nPulses, initData, initFunc, loFunc, hiFunc, accLevel,errCode) {};
	// ThreadMaker calls  constructor after making initStruct 
	static SimpleGPIO_thread * SimpleGPIO_threadMaker (int thePin, int polarity, unsigned int delayUsecs, unsigned int  durUsecs, unsigned int nPulses, int accuracyLevel) ;
	
	static bcm_peripheralPtr GPIOperi ;
	static int GPIOperi_users ;
	
	// destructor
	~SimpleGPIO_thread ();
	// utility functions
	/*
	void setPin (int pin);
	int getPin (void );
	void setLevel (int level);
	protected:
	int pinNumber;
	int polarity; */
};



#endif