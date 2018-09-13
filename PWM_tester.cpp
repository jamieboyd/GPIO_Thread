#include "PWM_thread.h"

// need to initialize C++ static class fields in this way
float PWM_thread::PWMfreq =0; // this will be set when clock is initialized
int PWM_thread::PWMchans=0; // this will track channels active, bitwise, 0,1,2, or 3
int PWM_thread::PWMrange =1000;  // PWM clock counts per output value, sets precision of output, we keep same for both channels


/* ************************************ custom thread func *******************************************************************
The method is to make a cosine wave at lowest ever needed frequency, in this case 1 Hz, and instead of changing the wave when we change frequencies,
we change the step by which we jump. We can easily do any multiple of the base frequency.   With 1Hz cosine wave, we add 1 each time for 1 Hz,
add 2 each time for 2 Hz, etc, using the % operator so we don't get tripped up at wrap-around. 
WIth
Last Modified:
2018/09/12 by Jamie Boyd - initial version for testing
*/
unsigned int OutFreq = 110;  // we will pass this as a paramater to a modFunction when we do it for real 
void freqFunc (void * taskDataP){
	ptPWMStructPtr taskData = (ptPWMStructPtr)taskDataP;
	*(taskData->dataRegister) = taskData-> arrayData[taskData->arrayPos];
	taskData->arrayPos += OutFreq ; //taskDataP->freq;
	if (taskData->arrayPos >= taskData->nData){
		taskData->arrayPos = taskData->arrayPos % (taskData->nData);
	}
}


int main(int argc, char **argv){
	
	// PWM settings - this is about as fast as you can go; the clock it uses is the 500MHz PLL D with the integer divider being 2.
	float threadFreq = 250e3;
	int PWMchan = 2; // channel to use, 0 or 1for channel, plus 2 to play over audio
	int PWMmode = PWM_BALANCED; //PWM_BALANCED for LEDs/Analog out or PWM_MARK_SPACE for servos	
	
	// map peripherals for PWM controller
	int mapResult = PWM_thread::mapPeripherals ();
	if (mapResult){
		printf ("Could not map peripherals for PWM access with return code %d\n.", mapResult);
		return 1;
	}
	
	// set clock for PWM from variables
	PWM_thread::setClock (threadFreq);
	if (PWM_thread::PWMfreq == -1){
		printf ("Could not set clock for PWM with frequency = %.3f and range = %d.\n", threadFreq, PWM_thread::PWMrange);
		return 1;
	}
	printf ("PWM update frequency = %.3f\n", PWM_thread::PWMfreq);
	
	// make sine wave array data for 1 Hz sine wave
	unsigned int arraySize = (unsigned int)(threadFreq);
	int * dataArray = new int [arraySize];
	const double phi = 6.2831853071794;
	double offset = PWM_thread::PWMrange/2;
	for (unsigned int ii=0; ii< arraySize; ii +=1){
		dataArray [ii] = (unsigned int) (offset - offset * cos (phi *((double) ii/ (double) arraySize)));
	}
	
	// make the thread to do infinite train
	PWM_thread * myPWM = PWM_thread::PWM_threadMaker (PWMchan, PWMmode, 1, dataArray, arraySize, threadFreq, (float) 0, 2);
	if (myPWM == nullptr){
		printf ("thread maker failed to make a thread.\n");
		return 1;
	}
	// install a custom function for frequency control
	myPWM->setHighFunc (&freqFunc); // sets the function that is called on high part of cycle
	
	//myPWM->setEnable (0, 1);
	myPWM->startInfiniteTrain ();
	myPWM->waitOnBusy (10);
	myPWM->stopInfiniteTrain ();
	
	return 0; // 
}


// g++ -O3 -std=gnu++11 -Wall -lpulsedThread GPIOlowlevel.cpp PWM_thread.cpp PWM_tester.cpp -o PWMtester
