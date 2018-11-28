#ifndef SIMPLEGPIO_THREAD
#define SIMPLEGPIO_THREAD

#include <pulsedThread.h>
#include "GPIOlowlevel.h"


/* *********************** Forward declare functions used by thread so we can refer to them in constructors*************************/
void SimpleGPIO_Lo (void *  taskData);
void SimpleGPIO_Hi (void *  taskData);
int SimpleGPIO_Init (void * initDataP, void *  &taskDataP);
int SimpleGPIO_setPinCallback (void * modData, taskParams * theTask);
int SimpleGPIO_setLevelCallBack (void * modData, taskParams * theTask);
void SimpleGPIO_delTask (void * taskData);

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
	/* constructors, similar to pulsedThread, one expects unsigned ints for pulse delay and duration times in microseconds and number of pulses */
	SimpleGPIO_thread (int pinP, int polarityP, unsigned int delayUsecs, unsigned int durUsecs, unsigned int nPulses, void * initData, int accLevel , int &errCode) : pulsedThread (delayUsecs, durUsecs, nPulses, initData, &SimpleGPIO_Init, &SimpleGPIO_Lo, &SimpleGPIO_Hi, accLevel, errCode) {
	pinNumber = pinP;
	polarity = polarityP;
	};
	
	/* the other constructor expects floats for frequency, duty cycle, and train duration */
	SimpleGPIO_thread (int pinP, int polarityP, float frequency, float dutyCycle, float trainDuration, void * initData, int accLevel, int &errCode) : pulsedThread (frequency, dutyCycle, trainDuration, initData, &SimpleGPIO_Init, &SimpleGPIO_Lo, &SimpleGPIO_Hi, accLevel,errCode) {
	pinNumber = pinP;
	polarity = polarityP;
	};
	
	/* Static ThreadMakers make an initStruct and call a constructor with it, returning a pointer to a SimpleGPIO_thread */
	static SimpleGPIO_thread * SimpleGPIO_threadMaker (int pin, int polarity, unsigned int delayUsecs, unsigned int  durUsecs, unsigned int nPulses, int accuracyLevel);
	static SimpleGPIO_thread * SimpleGPIO_threadMaker (int pin, int polarity, float frequency, float dutyCycle, float trainDuration, int accuracyLevel);
	// destructor
	~SimpleGPIO_thread ();
	// utility functions
	int setPin (int newPin, int isLocking);
	int getPin (void);
	int getPolarity (void);
	int setLevel (int level, int isLocking);
	// data members
protected:
	int pinNumber;
	int polarity;
};
#endif