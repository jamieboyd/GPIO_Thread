#include "PWM_thread.h"

/* **************************************** Test of PWM_thread *******************************************
Last Modified:
2018/09/18 by Jamie Boyd - updated for new way of handling channels
2018/09/12 by Jamie Boyd - initial version for testing
*/

int main(int argc, char **argv){
	// PWM settings
	float PWMfreq = 100e3; 
	unsigned int PWMrange = 1024;
	// PWM channel settings
	int channel = 1;
	int onAudio =1;
	int mode = PWM_MARK_SPACE;
	int enable = 0;
	int polarity = 0;
	int offState = 0;
	
	// make a thread for continuous output, at 5x slower frequency as thread 
	PWM_thread * myPWM  = PWM_thread::PWM_threadMaker (PWMfreq, PWMrange, (PWMfreq/5), 0, 2);
	
	if (myPWM == nullptr){
		printf ("thread maker failed to make a thread.\n");
		return 1;
	}
	printf ("thread maker made a thread.\n");
	printf ("Actual PWM update frequency = %.3f.\n", myPWM->PWMfreq);
	// make sine wave array data 
	unsigned int arraySize = (unsigned int)(PWMfreq/100);
	int * dataArray = new int [arraySize];
	const double phi = 6.2831853071794;
	double offset = PWMrange/2;
	for (unsigned int ii=0; ii< arraySize; ii +=1){
		dataArray [ii] = (unsigned int) (offset - offset * cos (phi *((double) ii/ (double) arraySize)));
	}
	printf ("data at 0 =%d, 1 = %d, 2 = %d, 3 = %d, 4 = %d\n", dataArray [0],  dataArray [1], dataArray [2], dataArray [3], dataArray [4]);      
	// add the channel to the thread
	myPWM->addChannel (channel, onAudio, mode, enable, polarity, offState, dataArray, arraySize);
	// enable the PWM to output
	myPWM->setEnable (1, channel, 1);
	// start the thread doing an infinite train for 10 seconds
	myPWM->startInfiniteTrain ();
	myPWM->waitOnBusy (10);
	myPWM->stopInfiniteTrain ();
	delete myPWM;
	delete dataArray;
	return 0;
}


// g++ -O3 -std=gnu++11 -Wall -lpulsedThread GPIOlowlevel.cpp PWM_thread.cpp PWM_tester.cpp -o PWMtester
