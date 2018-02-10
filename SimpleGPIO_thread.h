#ifndef SIMPLEGPIO_THREAD
#define SIMPLEGPIO_THREAD

#include <pulsedThread.h>
#include "GPIOlowlevel.h"


/* ******************** Initialization struct for SimpleGPIO *******************************
 pin, polarity, and address base for memory mapped addresses to write to on HI and Lo */
typedef struct SimpleGPIOInitStruct{
	int thePin; // pin to use for the GPIO output
	int thePolarity; // polarity, 0 for low-to-high, 1 for high-to-low
	volatile unsigned int * GPIOperiAddr; // base address needed when writing to registers for setting and unsetting
}SimpleGPIOInitStruct, *SimpleGPIOInitStructPtr;

/* ******************** Custom Data Struct for Simple GPIO***************************
 memory mapped addresses to write to on HI and Lo, and GPIO pin bit */
typedef struct SimpleGPIOStruct{
	unsigned int * GPIOperiHi; // address of register to write pin bit to on Hi
	unsigned int * GPIOperiLo; // address of register to write pin bit to on Lo
	unsigned int pinBit;	// pin number translated to bit position in register
}SimpleGPIOStruct, *SimpleGPIOStructPtr;

/* *********************SimpleGPIO_thread class extends pulsedThread ****************
Does pulses and trains of pulses on Raspberry Pi GPIO pins */
class SimpleGPIO_thread : public pulsedThread{
	public:
	/* constructors, similar to pulsedThread, one expects unsigned ints based on pulse delay and duration times in microseconds */
	SimpleGPIO_thread  (int pinP, int polarityP, unsigned int delayUsecs, unsigned int durUsecs, unsigned int nPulses, void * initData, int (*initFunc)(void *, void *  &), void (* loFunc)(void *), void (*hiFunc)(void *), int accLevel , int &errCode) : pulsedThread (delayUsecs, durUsecs, nPulses, initData, initFunc, loFunc, hiFunc, accLevel,errCode) {
	pinNumber = pinP;
	polarity = polarityP;
	GPIOperi_users +=1;};
	
	/* the other constructor expects floats based on frequency, duty cycle, and train duration */
	SimpleGPIO_thread  (int pinP, int polarityP, float frequency, float dutyCycle, float trainDuration, void * initData, int (*initFunc)(void *, void *  &), void (* loFunc)(void *), void (*hiFunc)(void *), int accLevel , int &errCode) : pulsedThread (frequency, dutyCycle, trainDuration, initData, initFunc, loFunc, hiFunc, accLevel,errCode) {
	pinNumber = pinP;
	polarity = polarityP;
	GPIOperi_users +=1;};
	
	/* Static ThreadMakers call constructors after making initStruct and return a pointer to a SimpleGPIO_thread */
	static SimpleGPIO_thread * SimpleGPIO_threadMaker (int pin, int polarity, unsigned int delayUsecs, unsigned int  durUsecs, unsigned int nPulses, int accuracyLevel);
	static SimpleGPIO_thread * SimpleGPIO_threadMaker (int pin, int polarity, float frequency, float dutyCycle, float trainDuration, int accuracyLevel);
	/* static variables to map GPIO peripheral and track usage */
	static bcm_peripheralPtr GPIOperi ;
	static int GPIOperi_users;
	static int mapGPIOperi(void);
	// destructor
	~SimpleGPIO_thread ();
	// utility functions
	int setPin (int newPin, int isLocking);
	int getPin (void);
	int getPolarity (void);
	int setLevel (int level, int isLocking);
	// data members
	float * endFuncArrayData; // a pointer for endFunc array data, because we use it a lot
	private:
	int pinNumber;
	int polarity;
};
#endif
