#include "PWM_thread.h"

// need to initialize C++ static class fields in this way
float PWM_thread::PWMfreq =0; // this will be set when clock is initialized
int PWM_thread::PWMchans=0; // this will track channels active, bitwise, 0,1,2, or 3
int PWM_thread::PWMrange =4096;  // PWM clock counts per output value, sets precision of output, we keep same for both channels

//float pwmFreq = 100; // desired frequency that PWM output value is updated, should be >= frequency requested for pulsed thread output
//unsigned int arraySizeBase = 2048; // size of the array, must contain at least one period of output. A bigger array can put out a lower frequency 

int main(int argc, char **argv){
	
	// PWM settings
	float threadFreq = 1000;
	float PWMoversampling = 5; // make PWM frequency updating this many times faster than thread frequency
	int PWMchan = 0; // channel to use, 0 or 1
	int PWMmode = PWM_MARK_SPACE; //PWM_BALANCED for LEDs/Analog out or PWM_MARK_SPACE for servos
	float sin_frequency = 5; // requested sine wave frequency in Hz
	
	
	// map peripherals for PWM controller
	int mapResult = PWM_thread::mapPeripherals ();
	if (mapResult){
		printf ("Could not map peripherals for PWM access with return code %d\n.", mapResult);
		return 1;
	}
	
	// set clock for PWM from variables
	float reqPWMfreq = threadFreq * PWMoversampling; // requested PWM frequency
	PWM_thread::setClock (reqPWMfreq);
	if (PWM_thread::PWMfreq == -1){
		printf ("Could not set clock for PWM with frequency = %.3f and range = %d.\n", reqPWMfreq, PWM_thread::PWMrange);
		return 1;
	}
	printf ("PWM update frequency = %.3f\n", PWM_thread::PWMfreq);
	
	// make sine wave array data
	unsigned int arraySize = (unsigned int)(threadFreq/sin_frequency);
	int * dataArray = new int [arraySize];
	const double phi = 6.2831853071794;
	double offset = PWM_thread::PWMrange/2;
	for (unsigned int ii=0; ii< arraySize; ii +=1){
		dataArray [ii] = (unsigned int) (offset - offset * cos (phi *((double) ii/ (double) arraySize)));
#ifdef beVerbose
		printf ("data at %u = %u.\n", ii, dataArray [ii]); 
#endif
	}
	
	// make the thread to do infinite train
	PWM_thread * myPWM = PWM_thread::PWM_threadMaker (PWMchan, PWMmode, 1, dataArray, arraySize, threadFreq, (float) 0, 1);
	if (myPWM == nullptr){
		printf ("thread maker failed to make a thread.\n");
		return 1;
	}
	//myPWM->setEnable (0, 1);
	myPWM->startInfiniteTrain ();
	myPWM->waitOnBusy (30);
	myPWM->stopInfiniteTrain ();
	/*
	INP_GPIO(GPIOperi ->addr,18);           // Set GPIO 18 to input to clear bits
	SET_GPIO_ALT(GPIOperi ->addr,18,5);     // Set GPIO 18 to Alt5 function PWM0
	unsigned int rangeRegisterOffset = PWM0_RNG;
	unsigned int dataRegisterOffset = PWM0_DAT;
	unsigned int modeBit = 0x80;
	unsigned int enableBit = 0x1;
	
	*(PWMperi ->addr  + rangeRegisterOffset) = PWMrange; // set range
	if (PWMmode ==PWM_MARK_SPACE){
		*(PWMperi ->addr + PWM_CTL) |= modeBit; // put PWM in MS Mode
	}else{
		*(PWMperi ->addr  + PWM_CTL) &= ~(modeBit);  // clear MS mode bit for balanced mode
	}
	// set initial PWM value first so we have something to put out
	*(PWMperi ->addr  + dataRegisterOffset) = 500;
	*(PWMperi ->addr  + PWM_CTL) |= enableBit;
	*/
	return 0; // 
}


	
	

/*

	
	
	
	
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
	PWM_thread * myPWM = PWM_thread::PWM_threadMaker (PWMchan, PWMmode, 0, arrayData, PWMrange, (unsigned int) 1000, (unsigned int) PWMrange, 1);
	if (myPWM == nullptr){
		printf ("thread maker failed to make a thread.\n");
		return 1;
	}
	printf ("Wait on busy returned %d\n", myPWM->waitOnBusy(1));
	printf ("thread maker made a thread.\n");
	printf ("Thread channel = %d.\n", myPWM->PWM_chan);
	myPWM->setEnable (0, 1);
	myPWM->DoTasks (2);
	myPWM->waitOnBusy (60);
	myPWM->setEnable (1, 1);
	myPWM->DoTasks (2);
	myPWM->waitOnBusy (60);
	delete myPWM;
	
	delete arrayData;
}*/


// g++ -O3 -std=gnu++11 -Wall -lpulsedThread GPIOlowlevel.cpp PWM_thread.cpp PWM_tester.cpp -o PWMtester
