#include "PWM_thread.h"

/* **************************************** Test of PWM_thread *******************************************
Last Modified:
2018/09/18 by Jamie Boyd - updated for new way of handling channels
2018/09/12 by Jamie Boyd - initial version for testing
*/

int main(int argc, char **argv){
	// PWM settings
	float PWMfreq = 80e03;
	unsigned int PWMrange = 1000;
	float toneFreq = 20000;  // tone, in Hz, that will play over the audio if directed to the speakers
	// PWM channel settings
	int channel = 1;
	int audioOnly =0;
	int mode = PWM_BALANCED; //PWM_MARK_SPACE; //
	int enable = 0;
	int polarity = 0;
	int offState =0;
	int useFIFO=1;
	float pulsedThreadFreq;
	int accMode;
	// make a thread for continuous output. Do thread at same speed as PWM frequency when writing to the data register
	// WHen using a FIFO, thread can go 10x slower because as long as we get back before the FIFO is empty, all is good
	// When using a FIFO, the data output rate is set by PWMfreq, not by the thread frequency, so we can use
	// a less demanding but lower precision thread timing mode with the FIFO.  
	if (useFIFO){
		pulsedThreadFreq = PWMfreq/10;
		accMode = ACC_MODE_SLEEPS_AND_SPINS;
	}else{
		pulsedThreadFreq = PWMfreq;
		accMode = ACC_MODE_SLEEPS_AND_OR_SPINS;
	}
	PWM_thread * myPWM  = PWM_thread::PWM_threadMaker (PWMfreq, PWMrange, useFIFO,  pulsedThreadFreq, 0, accMode);
	
	if (myPWM == nullptr){
		printf ("thread maker failed to make a thread.\n");
		return 1;
	}
	printf ("thread maker made a thread.\n");
	//printf ("Actual PWM update frequency = %.3f.\n", myPWM->PWMfreq);
	// make sine wave array data 
	unsigned int arraySize = (unsigned int)(PWMfreq/toneFreq);
	int * dataArray = new int [arraySize];
	const double phi = 6.2831853071794;
	double offset = PWMrange/2;
	for (unsigned int ii=0; ii< arraySize; ii +=1){
		dataArray [ii] = (unsigned int) (offset - offset * cos (phi *((double) ii/ (double) arraySize)));
	}
	//printf ("data at 0 =%d, 1 = %d, 2 = %d, 3 = %d, 4 = %d\n", dataArray [0],  dataArray [1], dataArray [2], dataArray [3], dataArray [4]);      
	// add the channel to the thread
	myPWM->addChannel (channel, audioOnly, mode, enable, polarity, offState, dataArray, arraySize);
	// enable the PWM to output
	myPWM->setEnable (1, channel, 0);
	//myPWM->setPolarity (1, 1,0);
	//myPWM->setOffState (1,1,0);
	// start the thread doing an infinite train for 10 seconds
	myPWM->startInfiniteTrain ();
	myPWM->waitOnBusy (15);
	myPWM->stopInfiniteTrain ();
	/*
	myPWM->setEnable (0, channel, 0);
	printf ("turned off.\n");
	usleep (2000000);
	myPWM->setOffState (1,channel,0);
	printf ("Set offstate high.\n");
	usleep (2000000);
	myPWM->setOffState (0,channel,0);
	printf ("Set offstate low.\n");
	myPWM->setArraySubrange ((unsigned int) (arraySize/3), (unsigned int) (2 * arraySize/3), channel, 1);
	myPWM->setEnable (1, channel, 0);
	myPWM->startInfiniteTrain ();	
	myPWM->waitOnBusy (4);
	myPWM->stopInfiniteTrain ();
	myPWM->setEnable (0, channel, 0);
	*/
	usleep (200);
	delete myPWM;
	delete dataArray;
	return 0;
}

/*
g++ -O3 -std=gnu++11 -Wall -lpulsedThread GPIOlowlevel.cpp PWM_thread.cpp PWM_tester.cpp -o PWMtester
*/