#include "SimpleGPIO_thread.h"
#include <stdio.h>

/* ******************************** makes a GPIO output that does software pulse width modulation *********************************************/

static const int GPIO_PIN = 23;
static const int POLARITY = 0;

int main(int argc, char **argv){
	
	// Use threadMaker function to make a simpleGPIO_thread (580 + 20 = 500 microseconds per pulse times 50 pulses = 0.025 seconds per train
	SimpleGPIO_thread *  myGPIO= SimpleGPIO_thread::SimpleGPIO_threadMaker (GPIO_PIN, POLARITY, (unsigned int)480,(unsigned int)20, (unsigned int)50, 1);
	printf ("GPIO peri users = %d.\n", GPIOperi_users);
	if (myGPIO == nullptr){
		printf ("SimpleGPIO_thread object was not created. Now exiting...\n");
		return 1;
	}
	// make an array of floats and fill it with values that vary from 0.2 to 1 in a sinusoidal fashion
	float * endFuncArrayData = new float [128];
	myGPIO->cosineDutyCycleArray (endFuncArrayData, 128, 64, 0.75, 0.1);
	// Set a ponter to this array as endFunc data 
	myGPIO->setUpEndFuncArray (endFuncArrayData, 128, 1);
	// set the endFunc to modify pulse dutycycle from the array  
	myGPIO->setEndFunc (&pulsedThreadDutyCycleFromArrayEndFunc);
	// request thread to output 1280 trains, as this is a multiple of the period, we start low and end low, should take 32 seconds
	myGPIO ->DoTasks (1280);
	// wait til the trains are all done, or 60 seconds, which is lots
	myGPIO->waitOnBusy (60);
	// clean up
	delete (myGPIO);
	printf ("GPIO peri users = %d.\n", GPIOperi_users);
	delete (endFuncArrayData);
	return 0;
}


/*
 g++ -O3 -std=gnu++11 -Wall -lpulsedThread GPIOlowlevel.cpp SimpleGPIO_thread.cpp SimpleGPIO_tester.cpp -o Tester
*/