#include "CountermandPulse.h"
/*Simple tester application for a countermanable pulse. If requested to be countermanded BEFORE the delay period is up, the pulse
does not happen The output of the program should be:
pulse 1 was standard, not countermandable.
pulse 2 was countermanded = 1.
pulse 3 was countermanded = 0.
and if you have an oscilloscope or LED, you should  2 pulses  or 2 flashes, 0.25 seconds long, separated by 4 seconds
*/
int main(int argc, char **argv){
	// make a countermanable pulse on pin 23, polarity = low-to-high, delay = 2 seconds, duration = 0.25 seconds,
	CountermandPulse * cp = CountermandPulse::CountermandPulse_threadMaker(23, 0, 2000000, 250000, ACC_MODE_SLEEPS_AND_SPINS);
	// calling doTask igives the standard pulse, not countermandable
	cp->DoTask();
	cp->waitOnBusy(3);
	printf ("pulse 1 was standard, not countermandable.\n");
	
	// calling doCountermandPulse with a wait less than 2 seconds should be countermandable
	cp->doCountermandPulse();
	cp->waitOnBusy(1.99);
	cp->countermand();
	cp->waitOnBusy(2);
	printf ("pulse 2 was countermanded = %d.\n", cp->wasCountermanded());
	
	// calling doCountermandPulse with a wait greater than 2 seconds should NOT be countermandable
	cp->doCountermandPulse();
	cp->waitOnBusy(2.01);
	cp->countermand();
	cp->waitOnBusy(1);
	printf ("pulse 3 was countermanded = %d.\n", cp->wasCountermanded());
}

 //g++ -O3 -std=gnu++11 -Wall -lpulsedThread GPIOlowlevel.cpp SimpleGPIO_thread.cpp CountermandPulse.cpp CountermandPulseRunner.cpp -o cmpTester
