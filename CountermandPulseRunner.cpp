#include "CountermandPulse.h"

int main(int argc, char **argv){
	CountermandPulse * cp = CountermandPulse::CountermandPulse_threadMaker(23, 0, 2000000, 250000, 1);
	cp->DoTask();
	cp->waitOnBusy(3);
	printf ("pulse 1 was countermanded = %d.\n", cp->wasCountermanded());
	
	cp->DoTask();
	cp->waitOnBusy(1);
	cp->countermand();
	cp->waitOnBusy(2);
	printf ("pulse 2 was countermanded = %d.\n", cp->wasCountermanded());
	
	cp->DoTask();
	cp->waitOnBusy(3);
	printf ("pulse 3 was countermanded = %d.\n", cp->wasCountermanded());
}

 //g++ -O3 -std=gnu++11 -Wall -lpulsedThread GPIOlowlevel.cpp SimpleGPIO_thread.cpp CountermandPulse.cpp CountermandPulseRunner.cpp -o cmpTester
