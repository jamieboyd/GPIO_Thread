#include "SimpleGPIO_thread.h"
#include <stdio.h>

/* ******************************** makes a GPIO output that does software pulse width modulation *********************************************/

static const int GPIO_PIN =17;
static const int POLARITY = 0;

int main(int argc, char **argv){
	
	// Use threadMaker function to make a simpleGPIO_thread, an infinite train with frequency = 1kHz, and duty cycle = 0.001
	// SimpleGPIO_threadMaker calls pulsedThread constructor
	SimpleGPIO_thread *  myGPIO= SimpleGPIO_thread::SimpleGPIO_threadMaker (GPIO_PIN, POLARITY, (float)100,(float)0.001, (float)0, ACC_MODE_SLEEPS_AND_OR_SPINS);
	printf ("GPIO peri users = %d.\n", GPIOperi_users);
	if (myGPIO == nullptr){
		printf ("SimpleGPIO_thread object was not created. Now exiting...\n");
		return 1;
	}
	// make an array of floats and fill it with values that vary from 0 to 1 in a sinusoidal fashion
	float * endFuncArrayData = new float [100];
	myGPIO->cosineDutyCycleArray (endFuncArrayData, 100, 100, 0.5, 0.5);
	// Set a pointer to this array as endFunc data 
	myGPIO->setUpEndFuncArray (endFuncArrayData, 100, 1);
	// set the endFunc to modify pulse dutycycle from the array  
	//myGPIO->setEndFunc (&pulsedThreadDutyCycleFromArrayEndFunc);
	myGPIO->chooseArrayEndFunc (kDUTY_CYCLE);
	/*pulsedThreadArrayModStructPtr myModder = new pulsedThreadArrayModStruct;
	myModder->modBits =3 ; // to mod array start and end position
	myModder-> startPos = 64; // start outputting in the middle
	myModder-> endPos = 96;
	int result = myGPIO->modCustom (&pulsedThreadSetArrayLimitsCallback, myModder, 1); */
	// request thread to output 128 trains, as this is a multiple of the period, we start low and end low, should take 32 seconds
	
	myGPIO ->startInfiniteTrain ();
	myGPIO->waitOnBusy (20);
	int result = myGPIO->setEndFuncArrayLimits (24, 75, 1);
		if (result){
		printf ("modCustom calling setEndFuncArrayLimits reported an error. Now exiting...\n");
		return 1;
	}
	myGPIO->waitOnBusy (20);
	myGPIO ->stopInfiniteTrain();
	myGPIO->waitOnBusy (100);
	/*
	myGPIO ->DoTasks (12800);
	// wait til the trains are all done, or 60 seconds, which is lots
	printf ("Trains left = %d.\n", myGPIO->waitOnBusy (60));
	// test use of pulsedThreadSetArrayLimitsCallback from modCustom to set array poition
	int result = myGPIO->setEndFuncArrayLimits (64, 96, 0);
		if (result){
		printf ("modCustom calling setEndFuncArrayLimits reported an error. Now exiting...\n");
		return 1;
	}
	myGPIO ->DoTasks (12800);
	printf ("Trains left = %d.\n", myGPIO->waitOnBusy (60));
	*/
	// clean up
	delete (myGPIO);
	printf ("GPIO peri users = %d.\n", GPIOperi_users);
	delete (endFuncArrayData);
	return 0;
}


/*
 g++ -O3 -std=gnu++11 -Wall -lpulsedThread GPIOlowlevel.cpp SimpleGPIO_thread.cpp SimpleGPIO_tester.cpp -o SimpleGPIOtester
*/
