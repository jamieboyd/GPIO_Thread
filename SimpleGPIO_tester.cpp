#include "SimpleGPIO_thread.h"
#include <stdio.h>

int main(int argc, char **argv){
	SimpleGPIO_thread * myGPIO= SimpleGPIO_thread::SimpleGPIO_threadMaker (23, 0, 2e05, 2e05, 20, 1) ;
	printf ("GPIO peri users = %d.\n", SimpleGPIO_thread::GPIOperi_users);
	SimpleGPIO_thread * myGPIO2= SimpleGPIO_thread::SimpleGPIO_threadMaker (22, 1, 2e05, 2e05, 20, 1);
	printf ("GPIO peri users = %d.\n", SimpleGPIO_thread::GPIOperi_users);
	if ((myGPIO == nullptr) || (myGPIO2 == nullptr)){
		printf ("SimpleGPIO_thread object was not created. Now exiting...\n");
		return 1;
	}
	myGPIO->DoTask();
	myGPIO2->DoTask();
	myGPIO->waitOnBusy (60);
	delete (myGPIO);
	printf ("GPIO peri users = %d.\n", SimpleGPIO_thread::GPIOperi_users);
	delete (myGPIO2);
	printf ("GPIO peri users = %d.\n", SimpleGPIO_thread::GPIOperi_users);
	return 0;
}


/*
g++ -O3 -std=gnu++11 -Wall -lpulsedThread SimpleGPIO_thread.cpp SimpleGPIO_tester.cpp -o Tester
*/