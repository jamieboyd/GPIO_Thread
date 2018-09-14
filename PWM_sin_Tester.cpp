#include "PWM_sin_thread.h"

// need to initialize C++ static class fields in this way
float PWM_thread::PWMfreq =0; // this will be changed when clock is initialized
int PWM_thread::PWMchans=0; // this will track channels active, bitwise, 0,1,2, or 3
int PWM_thread::PWMrange =PWM_SIN_RANGE;  // PWM clock counts per output value, sets precision of output, we keep same for both channels


int main(int argc, char **argv){
	
	int PWMchan = 3; // channel to use, 0 or 1for channel, plus 2 to play over audio
	int enabledAtStart = 1;
	unsigned int initialFreq = 100;
	/*
	// map peripherals for PWM controller
	int mapResult = PWM_thread::mapPeripherals ();
	if (mapResult){
		printf ("Could not map peripherals for PWM access with return code %d\n.", mapResult);
		return 1;
	}
	
	// set clock for PWM from variables
	PWM_thread::setClock (PWM_SIN_UPDATE_FREQ);
	if (PWM_thread::PWMfreq == -1){
		printf ("Could not set clock for PWM with frequency = %d and range = %d.\n", PWM_SIN_UPDATE_FREQ, PWM_thread::PWMrange);
		return 1;
	}
	printf ("PWM update frequency = %.3f\n", PWM_thread::PWMfreq);
	*/
	// make the thread 
	PWM_sin_thread * myPWM = PWM_sin_thread::PWM_sin_threadMaker (PWMchan, enabledAtStart, initialFreq);
	if (myPWM == nullptr){
		printf ("thread maker failed to make a thread.\n");
		return 1;
	}
	
	myPWM->startInfiniteTrain ();
	for (double frequency = 100; frequency < 18000; frequency *= 1.122462048309373){
		myPWM->waitOnBusy (0.1);
		myPWM->setFrequency ((unsigned int)frequency, 1);
		printf ("frequency = %.3f\n", frequency);
	}
	myPWM->waitOnBusy (0.1);
	myPWM->stopInfiniteTrain ();
	/*
	myPWM->waitOnBusy (2);
	myPWM->setFrequency (400, 01;
	myPWM->waitOnBusy (2);
	myPWM->setFrequency (800, 1);
	myPWM->waitOnBusy (2);
	myPWM->setFrequency (1600, 1);
	myPWM->waitOnBusy (2);
	myPWM->setEnable (0, 1);
	myPWM->waitOnBusy (2);
	myPWM->setEnable (1, 1);
	myPWM->waitOnBusy (2);
	myPWM->setFrequency (3200, 1);
	myPWM->waitOnBusy (2);
	myPWM->setFrequency (6400, 1);
	myPWM->waitOnBusy (2);
	myPWM->setFrequency (12800, 1);
	myPWM->waitOnBusy (2);
	myPWM->setFrequency (25600, 1);
	myPWM->waitOnBusy (2);
	myPWM->stopInfiniteTrain ();
	*/
	
	return 0; // 
}


// g++ -O3 -std=gnu++11 -Wall -lpulsedThread GPIOlowlevel.cpp PWM_thread.cpp PWM_sin_thread.cpp PWM_sin_Tester.cpp -o PWM_sin_tester
