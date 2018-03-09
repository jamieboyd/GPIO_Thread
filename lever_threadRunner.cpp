/* ************************ Runs the lever_thread for testing purposes ***********************************

Compile like this:
g++ -O3 -std=gnu++11 -Wall -lpulsedThread -lwiringPi GPIOlowlevel.cpp SimpleGPIO_thread.cpp lever_thread.cpp lever_threadRunner.cpp -o leverThread

last modified:
2018/03/08 by Jamie Boyd - started lever_threadRunner */

#include "lever_thread.h"


/* ************************ Gets a line of input from stdin into a passed-in char buffer ************************************************************************************
returns false if there are too many characters on the line to fit into the passed in buffer
Last modified :
2016/06/01 by Jamie Boyd  - initial version */
 bool myGetline(char * line, int lenMax, int keepNewLine) {
	
	int len;
	int readVal;
	char readChar;
	for(len=0; len < lenMax; len +=1) {
		readVal = getchar(); // read next value into an int (need bigger than a char to account for EOF  -which we should never see anyways
		readChar = (char) readVal;
		line[len] =readChar;
		if ( readChar == '\n'){
			if (keepNewLine)
				len +=1;
			break;
		}
	}
	if (len == lenMax ){
		printf ("You entered too many characters on the line; the limit is %d\n", lenMax);
		return false;
	}else{
		line[len] = '\0';
		return true;
	}
} 



int main(int argc, char **argv){
	
	printf ("leverThread reporting\n");
	uint8_t * positionData = new uint8_t [200];
		for (unsigned int i =0; i < 200; i +=1){ 
			positionData [i] = i;
	}
	
	lever_thread * myLeverThread= lever_thread::lever_thread_threadMaker (positionData, 200, 201, 0, 0);
	printf ("leverThread made thread\n");
	myLeverThread->modTrainLength(200);
	myLeverThread->DoTask();
	printf ("leverThread called doTask\n");
	myLeverThread->waitOnBusy (1.0);
	printf ("leverThread waited on busy.\n");
	printf ("data=");
	for (unsigned int i =0; i < 200; i +=1){ 
		printf ("%i, ", positionData [i]);
	}
	printf ("\n");
}
	