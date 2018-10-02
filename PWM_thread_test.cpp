#include "PWM_thread.h"

/* **************************************** Test of PWM_thread *******************************************
To test PWM_thread, pass 6 parameters, channels (1, 2, or 3 for both channels), useFIFO, audioOnly, mode, polarity, and offState
To test PWM_Sin_thread, pass 1 parameter for channels (1, 2, or 3 for both channels) 
Last Modified:
2018/10/01 by Jamie Boyd - added command line arguments
2018/09/18 by Jamie Boyd - updated for new way of handling channels
2018/09/12 by Jamie Boyd - initial version for testing
*/
int main(int argc, char **argv){
	
	int channel;
	int isSin;
	int audioOnly;
	int mode;// PWM_BALANCED; //PWM_MARK_SPACE; //
	int polarity;
	int offState;
	int useFIFO;
	// parse input paramaters. If not correct number, exit
	if (argc == 7){
		isSin =0;
		channel = atoi (argv[1]);
		useFIFO = atoi (argv[2]);
		audioOnly = atoi (argv[3]);
		mode = atoi (argv[4]);
		polarity = atoi (argv[5]);
		offState = atoi (argv[6]);
	}else{
		if (argc=2){
			isSin =1;
			channel = atoi (argv[1]);
		}else
			printf ("To test PWM_thread, pass 6 arguments, channels (1, 2, or 3 for both channels), useFIFO, audioOnly, mode, polarity, and offState.\n");
			printf ("To test PWM_sin_thread, pass 1 argument for channels (1, 2, or 3 for both channels).\n");
			return 0;
		}
	}
	if (isSin){
		printf ("******************* PWM_sin_thread *******************\n");
		PWM_sin_thread * my_sin_PWM  =  PWM_sin_thread::PWM_sin_threadMaker (channel);
		if (channel & 1){
			printf ("************** Testing Channel 1 **************\n")
			// set initial frequency
			my_sin_PWM->setSinFrequency (100 ,1,1);
			// enable the PWM to output
			my_sin_PWM->setEnable (1, 1, 0);
			// start train 
			my_sin_PWM->startInfiniteTrain ();
			float freq ;
			for (freq= 200; freq < 12e03; freq *= 1.12246){
				printf ("Current Sine wave frequency is %dHz.\n", my_sin_PWM->getSinFrequency (1));
				my_sin_PWM->waitOnBusy (0.2);
				my_sin_PWM->setSinFrequency ((unsigned int)freq ,channel,1);
			}
			my_sin_PWM->stopInfiniteTrain ();
			my_sin_PWM->setEnable (0, 1, 0);
		}
		if (channel & 2){
			printf ("************** Testing Channel 2 **************\n")
			// set initial frequency
			my_sin_PWM->setSinFrequency (100 ,2,1);
			// enable the PWM to output
			my_sin_PWM->setEnable (1, 2, 0);
			// start train 
			my_sin_PWM->startInfiniteTrain ();
			float freq ;
			for (freq= 200; freq < 12e03; freq *= 1.12246){
				printf ("Current Sine wave frequency is %dHz.\n", my_sin_PWM->getSinFrequency (2));
				my_sin_PWM->waitOnBusy (0.2);
				my_sin_PWM->setSinFrequency ((unsigned int)freq ,2,1);
			}
			my_sin_PWM->stopInfiniteTrain ();
			my_sin_PWM->setEnable (0, 2, 0);
		}
		if ((channel & 3) == 3){
			printf ("********** Testing Both Channels Simultaneously **********\n")
			// set initial frequency
			my_sin_PWM->setSinFrequency (100 ,1,1);
			my_sin_PWM->setSinFrequency (1e04 ,2,1);
			// enable the PWM to output
			my_sin_PWM->setEnable (1, 3, 1);
			// start train 
			my_sin_PWM->startInfiniteTrain ();
			float freq1, freq2 ;
			for (freq= 200, freq2 = 1e04; freq1 < 10e03; freq1 *= 1.12246, freq2 /= 1.12246){
				printf ("Frequency of channel 1 is %dHz, and of channel 2 is %dHz.\n", my_sin_PWM->getSinFrequency (1), my_sin_PWM->getSinFrequency (2));
				my_sin_PWM->waitOnBusy (0.2);
				my_sin_PWM->setSinFrequency ((unsigned int)freq1 ,1,1);
				my_sin_PWM->setSinFrequency ((unsigned int)freq2 ,2,1);
			}
			my_sin_PWM->stopInfiniteTrain ();
			my_sin_PWM->setEnable (0, 3, 1);
		}
	}else{
		printf ("******************* PWM_thread *******************\n");
		printf ("Parameters: channel = %d, useFIFO = %d, audioOnly = %d, PWM mode = %d, output polarity = %d, output off state = %d.\n", channel, useFIFO, audioOnly, mode, polarity, offState);
		// PWM thread settings
		float PWMfreq = 10000;
		unsigned int PWMrange = 1024;//;
		float toneFreq = 200;  // tone, in Hz, that will play over the audio if directed to the speakers
		float pulsedThreadFreq;
		int accMode;
		unsigned int arraySize = (unsigned int)(PWMfreq/toneFreq);
		float offset = PWMrange/2;
		double phi = 6.2831853071794;
		// make a thread for continuous output. Do thread at same speed as PWM frequency when writing to the data register
		// WHen using a FIFO, thread can go slower because as long as we get back before the FIFO is empty, all is good
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
		if (channel & 1){
			printf ("*************** Testing Channel 1 ***************\n");
			// make sine wave array data for channel 1
			int * dataArray1 = new int [arraySize];
			for (unsigned int ii=0; ii< arraySize; ii +=1){
				dataArray1 [ii] = (unsigned int) (offset - offset * cos (phi *((double) ii/ (double) arraySize)));
			}
			myPWM->addChannel (1, audioOnly, mode, enable, polarity, offState, dataArray1, arraySize);
			while (myPWM->getModCustomStatus());
			// do channel 1
			myPWM->setEnable (1, 1, 1);
			myPWM->startInfiniteTrain ();
			myPWM->waitOnBusy (1);
			PWM_thread::getStatusRegister (1);
			myPWM->waitOnBusy (4);
			myPWM->stopInfiniteTrain ();
			myPWM->setEnable (0, 1, 1);
		}
		if (channel & 2){
			printf ("*************** Testing Channel 2 ***************\n");
			// make sine wave array data for channel 2
			int * dataArray2 = new int [arraySize];
			for (unsigned int ii=0; ii< arraySize; ii +=1){
				dataArray2 [ii] = (unsigned int) (offset - offset * cos (phi *((double) (ii*2)/ (double) arraySize)));
			}
			myPWM->addChannel (2, audioOnly, mode, enable, polarity, offState, dataArray1, arraySize);
			while (myPWM->getModCustomStatus());
			// do channel 2
			myPWM->setEnable (1, 2, 1);
			myPWM->startInfiniteTrain ();
			myPWM->waitOnBusy (1);
			PWM_thread::getStatusRegister (1);
			myPWM->waitOnBusy (4);
			myPWM->stopInfiniteTrain ();
			myPWM->setEnable (0, 2, 1);
		}
		if ((channel & 3) == 3){
			printf ("*************** testing Both Channels Simultaneously ****************\n");
			myPWM->setEnable (1, 3, 1);
			myPWM->startInfiniteTrain ();
			myPWM->waitOnBusy (1);
			PWM_thread::getStatusRegister (1);
			myPWM->waitOnBusy (4);
			myPWM->stopInfiniteTrain ();
			myPWM->setEnable (0, 3, 1);
			while (myPWM->getModCustomStatus());
		}
	}
	return 0;
}
/*
g++ -O3 -std=gnu++11 -Wall -lpulsedThread GPIOlowlevel.cpp PWM_thread.cpp PWM_sin_thread.cpp PWM_tester.cpp -o PWMtester
*/
