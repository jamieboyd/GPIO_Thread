#include "SimpleGPIO_thread.h"
#include <stdio.h>

int main(int argc, char **argv){
	
	/*SimpleGPIO_thread * myGPIO= SimpleGPIO_thread::SimpleGPIO_threadMaker (23, 0, (unsigned int)2e04, (unsigned int)2e04, (unsigned int)20, 2) ;
	printf ("GPIO peri users = %d.\n", SimpleGPIO_thread::GPIOperi_users);
	SimpleGPIO_thread * myGPIO2= SimpleGPIO_thread::SimpleGPIO_threadMaker (22, 1, (float)25, (float)0.5, (float)8.0, 2) ;
	printf ("GPIO peri users = %d.\n", SimpleGPIO_thread::GPIOperi_users);
	if ((myGPIO == nullptr) || (myGPIO2 == nullptr)){
		printf ("SimpleGPIO_thread object was not created. Now exiting...\n");
		return 1;
	}
	myGPIO->DoTasks(5);
	myGPIO2->DoTasks(5);
	myGPIO->waitOnBusy (60);
	delete (myGPIO);
	printf ("GPIO peri users = %d.\n", SimpleGPIO_thread::GPIOperi_users);
	myGPIO2 ->setLevel (1, 0);
	delete (myGPIO2);
	printf ("GPIO peri users = %d.\n", SimpleGPIO_thread::GPIOperi_users);
	*/
	SimpleGPIO_thread *  myGPIO3= SimpleGPIO_thread::SimpleGPIO_threadMaker (23, 0, (float)50, (float)0.01, (float)0.02, 2) ;
	printf ("GPIO peri users = %d.\n", SimpleGPIO_thread::GPIOperi_users);
	if (myGPIO3 == nullptr){
		printf ("SimpleGPIO_thread object was not created the second time. Now exiting...\n");
		return 1;
	}
	//myGPIO3 ->setLevel (1, 0);
	myGPIO3->endFuncArrayData = myGPIO3->cosineDutyCycleArray (128, 128, 0.5, 0.3);
	myGPIO3->setUpEndFuncArray (myGPIO3->endFuncArrayData, 128, 1);
	myGPIO3->setEndFunc (&pulsedThreadDutyCycleFromArrayEndFunc);
	myGPIO3->DoTasks(1280);
	myGPIO3->waitOnBusy (600);
	delete (myGPIO3);
	printf ("GPIO peri users = %d.\n", SimpleGPIO_thread::GPIOperi_users);
	
	return 0;
}


/*
g++ -O3 -std=gnu++11 -Wall -lpulsedThread SimpleGPIO_thread.cpp SimpleGPIO_tester.cpp -o Tester
*/