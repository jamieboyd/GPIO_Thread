#include "PWM_thread.h"

/* **************************************** Test of PWM_thread *******************************************
Last Modified:
2018/09/18 by Jamie Boyd - updated for new way of handling channels
2018/09/12 by Jamie Boyd - initial version for testing
*/

int main(int argc, char **argv){
	
	/*
	// PWM channel settings
	int channel = 1;
	
	// now with PWM sin
	printf ("let's do PWM_sin version.\n");
	PWM_sin_thread * my_sin_PWM  =  PWM_sin_thread::PWM_sin_threadMaker (channel);
	printf ("thread maker made a thread.\n");
	// set initial frequency
	my_sin_PWM->setSinFrequency (100 ,channel,1);
	// enable the PWM to output
	my_sin_PWM->setEnable (1, channel, 1);
	// start train 
	my_sin_PWM->startInfiniteTrain ();
	float freq ;
	for (freq= 200; freq < 12e03; freq *= 1.12246){
		printf ("Current Sine wave frequency is %dHz.\n", my_sin_PWM->getSinFrequency (channel));;
		my_sin_PWM->waitOnBusy (0.1);
		my_sin_PWM->setSinFrequency ((unsigned int)freq ,channel,1);
	}
	my_sin_PWM->stopInfiniteTrain ();
	my_sin_PWM->setEnable (0, channel, 1);
	delete my_sin_PWM;
	
	*/
	int audioOnly =0;
	int mode = PWM_BALANCED;// PWM_BALANCED; //PWM_MARK_SPACE; //
	int enable = 0;
	int polarity = 0;
	int offState =0;
	int useFIFO=1;
	// PWM settings
	float PWMfreq = 10000;
	unsigned int PWMrange = 1024;//;
	float toneFreq = 200;  // tone, in Hz, that will play over the audio if directed to the speakers
	float pulsedThreadFreq;
	int accMode;
	
	
	// make a thread for continuous output. Do thread at same speed as PWM frequency when writing to the data register
	// WHen using a FIFO, thread can go 10x slower because as long as we get back before the FIFO is empty, all is good
	// When using a FIFO, the data output rate is set by PWMfreq, not by the thread frequency, so we can use
	// a less demanding but lower precision thread timing mode with the FIFO.  
	if (useFIFO){
		pulsedThreadFreq = PWMfreq/4;
		accMode = ACC_MODE_SLEEPS_AND_SPINS; //ACC_MODE_SLEEPS_AND_SPINS
	}else{
		pulsedThreadFreq = PWMfreq;
		accMode = ACC_MODE_SLEEPS_AND_OR_SPINS;
	}
	PWM_thread * myPWM  = PWM_thread::PWM_threadMaker (PWMfreq, PWMrange, useFIFO,  pulsedThreadFreq, 0, accMode);
	if (myPWM == nullptr){
		printf ("thread maker failed to make a thread.\n");
		return 1;
	}
	// make sine wave array data for channel 1
	unsigned int arraySize = (unsigned int)(PWMfreq/toneFreq);
	int * dataArray1 = new int [arraySize];
	float offset = PWMrange/2;
	double phi = 6.2831853071794;
	for (unsigned int ii=0; ii< arraySize; ii +=1){
		dataArray1 [ii] = (unsigned int) (offset - offset * cos (phi *((double) ii/ (double) arraySize)));
	}
	int * dataArray2 = new int [arraySize];
	for (unsigned int ii=0; ii< arraySize; ii +=1){
		dataArray2 [ii] = (unsigned int) (offset - offset * cos (phi *((double) (ii*2)/ (double) arraySize)));
	}
	
	
	
//	dataArray2 [0] = offset;
//	dataArray2 [1] = offset;
//	dataArray2 [2] = offset;
	myPWM->addChannel (1, audioOnly, mode, enable, polarity, offState, dataArray1, arraySize);
	while (myPWM->getModCustomStatus());
	myPWM->addChannel (2, audioOnly, mode, enable, polarity, offState, dataArray2, arraySize);
	while (myPWM->getModCustomStatus());
	// do channel 2
	printf ("channel 2\n");
	myPWM->setEnable (1, 2, 1);
	while (myPWM->getModCustomStatus());
	myPWM->startInfiniteTrain ();
	myPWM->waitOnBusy (5);
	myPWM->stopInfiniteTrain ();
	while (myPWM->getModCustomStatus());
	myPWM->setEnable (0, 2, 1);
	while (myPWM->getModCustomStatus());
/*
	// do channel 1
	printf ("channel 1\n");
	myPWM->startInfiniteTrain ();
		while (myPWM->getModCustomStatus());
	myPWM->setEnable (1, 1, 1);
		while (myPWM->getModCustomStatus());

	myPWM->waitOnBusy (5);
	printf ("add channel 2\n");
	myPWM->setEnable (1, 2, 1);
		while (myPWM->getModCustomStatus());

	myPWM->waitOnBusy (5);
	myPWM->setEnable (0, 3, 1);
		while (myPWM->getModCustomStatus());

	myPWM->stopInfiniteTrain ();


	
		// do channel 1
	printf ("channel 1\n");
	myPWM->setEnable (1, 1, 0);
	myPWM->startInfiniteTrain ();
	myPWM->waitOnBusy (5);
	printf ("add channel 2\n");
	//myPWM->stopInfiniteTrain ();
	myPWM->setEnable (1, 2, 1);
	myPWM->waitOnBusy (3);


	usleep (20000);
	
	
		// channel 2
	printf ("channel 2\n");
	myPWM->setEnable (1, 2, 0);
	myPWM->startInfiniteTrain ();
	myPWM->waitOnBusy (5);
	myPWM->stopInfiniteTrain ();
	myPWM->setEnable (0, 2, 0);
	usleep (20000);
	
	

	
	
	// both channels
	printf ("channels 1 and 2\n");
	myPWM->startInfiniteTrain ();
	myPWM->setEnable (1, 3, 1);
	myPWM->waitOnBusy (5);
	myPWM->stopInfiniteTrain ();
	myPWM->setEnable (0, 3, 1);
	usleep (20000);
	
	
	

	

	*/
	
	
	
	

	return 0;
}
	
	
	
	/*
	// both channels
	printf ("channels 1 and 2\n");
	myPWM->setEnable (1, 3, 0);
	myPWM->startInfiniteTrain ();
	myPWM->waitOnBusy (5);
	myPWM->stopInfiniteTrain ();
	myPWM->setEnable (0, 3, 0);
	usleep (2000000);
	
	//printf ("data at 0 =%d, 1 = %d, 2 = %d, 3 = %d, 4 = %d\n", dataArray [0],  dataArray [1], dataArray [2], dataArray [3], dataArray [4]);      
	// add the channel to the thread
	if (channel &2){
		myPWM->addChannel (2, audioOnly, mode, enable, polarity, offState, dataArray, arraySize);
	}
	if (channel &1){
		myPWM->addChannel (1, audioOnly, mode, enable, polarity, offState, dataArray, arraySize);
	}
	unsigned int status = myPWM->getStatusRegister();
	printf ("status reg = 0x%x. chan1 state = %d and chan2 state = %d.\n", status, (status & PWM_STA1), (status & PWM_STA2));
	
	
	
	
	
	//myPWM->addChannel (2, audioOnly, mode, enable, polarity, offState, dataArray, arraySize);
	// start the thread doing an infinite train for 10 seconds
	myPWM->startInfiniteTrain ();
	// enable the PWM to output
	myPWM->setEnable (1, channel, 1);

	myPWM->waitOnBusy (1);
	status = myPWM->getStatusRegister();
	printf ("status reg = 0x%x. chan1 state = %d and chan2 state = %d.\n", status, (status & PWM_STA1), (status & PWM_STA2));
	myPWM->waitOnBusy (1);
	status = myPWM->getStatusRegister();
	printf ("status reg = 0x%x. chan1 state = %d and chan2 state = %d.\n", status, (status & PWM_STA1), (status & PWM_STA2));
	myPWM->waitOnBusy (20);
	myPWM->stopInfiniteTrain ();
	myPWM->setEnable (0, 	2, 0);
	myPWM->setEnable (0, 	1, 0);
	printf ("turned off.\n");
	status = myPWM->getStatusRegister();
	printf ("status reg = 0x%x. chan1 state = %d and chan2 state = %d.\n", status, (status & PWM_STA1), (status & PWM_STA2));

	//usleep (2000000);
	//myPWM->setOffState (1,channel,0);
	//printf ("Set offstate high.\n");
	//usleep (2000000);
	//myPWM->setOffState (0,channel,0);
	printf ("Set offstate low.\n");
	usleep (2000000);
	myPWM->setArraySubrange ((unsigned int) (arraySize/3), (unsigned int) (2 * arraySize/3), channel, 1);
	myPWM->setEnable (1, channel, 0);
	myPWM->startInfiniteTrain ();	
	myPWM->waitOnBusy (4);
	status = myPWM->getStatusRegister();
	printf ("status reg = 0x%x. chan1 state = %d and chan2 state = %d.\n", status, (status & PWM_STA1), (status & PWM_STA2));
	myPWM->stopInfiniteTrain ();
	myPWM->setEnable (0, channel, 0);
	printf ("And we are done here.\n");
	//usleep (200000); // just to be sure train is stopped
	
	delete myPWM;
	printf ("myPWM was deleted.\n");
	delete dataArray;
	printf ("data array was deleted.\n");
	//usleep (20000); // just to be sure train is stopped
	return 0;
}*/
 /*
 g++ -O3 -std=gnu++11 -Wall -lpulsedThread GPIOlowlevel.cpp PWM_thread.cpp PWM_thread_test.cpp -o PWMtester
g++ -O3 -std=gnu++11 -Wall -lpulsedThread GPIOlowlevel.cpp PWM_thread.cpp PWM_sin_thread.cpp PWM_tester.cpp -o PWMtester
*/
