#ifndef COUNTERMAND_PULSE
#define COUNTERMAND_PULSE

#include <pulsedThread.h>
#include "GPIOlowlevel.h"
#include "SimpleGPIO_thread.h"

void SimpleGPIO_Hi_C (void *  taskData);

/* ******************** Custom Data Struct for Simple GPIO with countermanding ***************************
 memory mapped addresses to write to on HI and Lo, and GPIO pin bit */
typedef struct CountermandPulseStruct{
	unsigned int * GPIOperiHi; // address of register to write pin bit to on Hi
	unsigned int * GPIOperiLo; // address of register to write pin bit to on Lo
	unsigned int pinBit;	// pin number translated to bit position in register
	volatile bool countermand; // volatile because we modify it without benifit of mutex
	
}CountermandPulseStruct, *CountermandPulseStructPtr;


class CountermandPulse : public SimpleGPIO_thread{
	public:
		CountermandPulse (int pinP, int polarityP, unsigned int delayUsecs, unsigned int durUsecs, void * initData, int accLevel , int &errCode) : SimpleGPIO_thread ((unsigned int) delayUsecs, (unsigned int) durUsecs, (unsigned int) 1, initData, &SimpleGPIO_Init, &SimpleGPIO_Lo, &SimpleGPIO_Hi_C, accLevel,errCode) {
	pinNumber = pinP;
	polarity = polarityP;
	
	static CountermandPulse * CountermandPulse_threadMaker (int pin, int polarity, unsigned int delayUsecs, unsigned int  durUsecs, int accuracyLevel);
	};
	void countermand(void);
protected:
	CountermandPulseStructPtr taskStructPtr;
};
#endif