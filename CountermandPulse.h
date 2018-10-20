#ifndef COUNTERMAND_PULSE
#define COUNTERMAND_PULSE

#include <pulsedThread.h>
#include "GPIOlowlevel.h"
#include "SimpleGPIO_thread.h"

 #include <stdio.h>
 
/* ************************************** declaration of non-class thread functions *********************************/
void Countermand_Hi (void *  taskData);
void Countermand_Lo (void *  taskData);
int countermand_Init (void * initDataP, void *  &taskDataP);

/* ******************** Custom Data Struct is same as Simple GPIO with added bool for countermanding ****************************/
typedef struct CountermandPulseStruct{
	unsigned int * GPIOperiHi; // address of register to write pin bit to on Hi
	unsigned int * GPIOperiLo; // address of register to write pin bit to on Lo
	unsigned int pinBit;	// pin number translated to bit position in register
	int countermand;		 // 0 for regular pulse, 1 to request countermand, 2 if in middle of a countermand
	bool wasCountermanded; // true if last pulse was countermanded
	pthread_mutex_t * taskMutexPtr; // pointer to task mutex, so we can lock and unlock it from hi func
}CountermandPulseStruct, *CountermandPulseStructPtr;


/* ***************************** CountermandPulse subclasses SimpleGPIO_thread **************************************************/
class CountermandPulse : public SimpleGPIO_thread{
public:
	CountermandPulse (int pinP, int polarityP, unsigned int delayUsecs, unsigned int durUsecs, void * initData, int accLevel , int &errCode) : SimpleGPIO_thread (pinP, polarityP, (unsigned int) delayUsecs, (unsigned int) durUsecs, (unsigned int) 1, initData, &countermand_Init, accLevel, errCode) {};
	static CountermandPulse * CountermandPulse_threadMaker (int pin, int polarity, unsigned int delayUsecs, unsigned int durUsecs, int accuracyLevel);
    bool doCountermandPulse (void);
	bool countermand(void);
	bool wasCountermanded (void);
protected:
	CountermandPulseStructPtr taskStructPtr;
};
#endif
