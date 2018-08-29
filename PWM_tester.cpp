#include "PWM_thread.h"

static const float PWMfreq = 100;
static const int PWMrange = 4096;
static const int PWMchan = 0;
static const int PWMmode = PWM_MARK_SPACE;

int PWM_thread::PWMchans =0;
float PWM_thread::PWMfreq = 0;
int PWM_thread::PWMrange =0;

int main(int argc, char **argv){
	

	// map peripherals for PWM controller
	int mapResult = PWM_thread::mapPeripherals ();
	if (mapResult){
		printf ("Could not map peripherals for PWM access with return code %d\n.", mapResult);
		return 1;
	}
	// set clock for PWM from constants
	float setFreq = PWM_thread::setClock (PWMfreq, PWMrange);
	if (setFreq == -1){
		printf ("Could not set clock for PWM with frequency = %.3f and range = %d.\n", PWMfreq, PWMrange);
		return 1;
	}
	printf ("clock frequency = %.3f\n", setFreq);
	// make some array data, a simple ramp
	int * arrayData = new int [PWMrange];
	for (int iData =0; iData < PWMrange; iData +=1){
		arrayData [iData] = iData;
	}
	printf ("first ArrayData = %d,%d,%d,%d,%d \n", arrayData [0], arrayData [1] , arrayData [2], arrayData [3] , arrayData [4] );
	
	// make the thread
	PWM_thread * myPWM = PWM_thread::PWM_threadMaker (PWMchan, PWMmode, 1, arrayData, PWMrange, (unsigned int) 1000, (unsigned int) PWMrange, 1);
	if (myPWM == nullptr){
		printf ("thread maker failed to make a thread.\n");
		return 1;
	}
	printf ("thread maker made a thread.\n");
	printf ("Thread channel = %d.\n", myPWM->PWM_chan);
	myPWM->DoTasks (10);
	myPWM->waitOnBusy (60);
	delete myPWM;
	
	delete arrayData;
}

// g++ -O3 -std=gnu++11 -Wall -lpulsedThread GPIOlowlevel.cpp PWM_thread.cpp PWM_tester.cpp -o PWMtester