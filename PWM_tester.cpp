#include "PWM_sin_thread.h"

/* **************************************** Test of PWM_thread *******************************************
Last Modified:
2018/09/18 by Jamie Boyd - updated for new way of handling channels
2018/09/12 by Jamie Boyd - initial version for testing
*/

int main(int argc, char **argv){
	
	
	// PWM channel settings
	int channel = 1;

	// now with PWM sin
	printf ("let's do PWM_sin version.\n");
	PWM_sin_thread * my_sin_PWM  =  PWM_sin_thread::PWM_sin_threadMaker (channel);
	printf ("thread maker made a thread.\n");
	// set initial frequency
	my_sin_PWM->setSinFrequency (100 ,1,0);
	// enable the PWM to output
	my_sin_PWM->setEnable (1, channel, 0);
	// start train 
	my_sin_PWM->startInfiniteTrain ();
	float freq ;
	for (freq= 200; freq < 25e03; freq *= 1.12246){
		my_sin_PWM->setSinFrequency ((unsigned int)freq ,channel,0);
		printf ("Current Sine wave frequency is %dHz.\n", my_sin_PWM->getSinFrequency (channel));;
		my_sin_PWM->waitOnBusy (0.075);
	}
	my_sin_PWM->stopInfiniteTrain ();
	usleep (2000); // just to be sure train is stopped
	delete my_sin_PWM;
	usleep (2000); // just to be sure destructor is called
	
	
	return 0;
}
	
	/*
	int audioOnly =0;
	int mode = PWM_BALANCED;// PWM_BALANCED; //PWM_MARK_SPACE; //
	int enable = 0;
	int polarity = 0;
	int offState =0;
	int useFIFO=1;
	// PWM settings
	float PWMfreq = 50e03;
	unsigned int PWMrange = 1000;
	float toneFreq = 2000;  // tone, in Hz, that will play over the audio if directed to the speakers
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
	float offset = PWMrange/2;
	double phi = 6.2831853071794;
	for (unsigned int ii=0; ii< arraySize; ii +=1){
		dataArray [ii] = (unsigned int) (offset - offset * cos (phi *((double) ii/ (double) arraySize)));
	}
	//printf ("data at 0 =%d, 1 = %d, 2 = %d, 3 = %d, 4 = %d\n", dataArray [0],  dataArray [1], dataArray [2], dataArray [3], dataArray [4]);      
	// add the channel to the thread
	myPWM->addChannel (channel, audioOnly, mode, enable, polarity, offState, dataArray, arraySize);
	// start the thread doing an infinite train for 10 seconds
	myPWM->startInfiniteTrain ();
	// enable the PWM to output
	myPWM->setEnable (1, channel, 1);
	myPWM->waitOnBusy (10);
	myPWM->stopInfiniteTrain ();
	
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
	printf ("And we are done here.\n");
	//usleep (200000); // just to be sure train is stopped
	delete myPWM;
	printf ("myPWM was deleted.\n");
	delete dataArray;
	printf (" data array was deleted.\n");
	//usleep (20000); // just to be sure train is stopped

	return 0;
} */

/*
g++ -O3 -std=gnu++11 -Wall -lpulsedThread GPIOlowlevel.cpp PWM_thread.cpp PWM_sin_thread.cpp PWM_tester.cpp -o PWMtester
*/