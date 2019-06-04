/* ************************ Runs the lever_thread for testing purposes ***********************************

Compile like this:
g++ -O3 -std=gnu++11 -Wall -lpulsedThread -lwiringPi GPIOlowlevel.cpp SimpleGPIO_thread.cpp PWM_thread.cpp leverThread.cpp leverThread_Runner.cpp -o leverThread

last modified:
2019/03/05 by Jamie Boyd - lever positions now 2 byte signed
2018/03/08 by Jamie Boyd - started lever_threadRunner */

#include "leverThread.h"


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
	
	int16_t * positionData = new int16_t [6000];

	leverThread * myLeverThread= leverThread::leverThreadMaker (positionData,1000, kTRIAL_CUED, 500, kLEVER_DIR_REVERSED, 22, 0);
	
	myLeverThread->zeroLever (0,0);	
	myLeverThread->setHoldParams (25, 175, 250);
	myLeverThread->startTrial();
	printf ("leverThread GO\n");
	fflush (stdout);
	
	int trialCode;
	unsigned int goalEntryPos;
	
	unsigned int ii =0;
	while (myLeverThread->checkTrial(trialCode, goalEntryPos) == false){
		printf ("LEVERPOS = %d; trialCode = %d.\n", myLeverThread->getLeverPos(), trialCode);
	}
	printf("goal entry pos = %d; trial code = %i\n", goalEntryPos, trialCode);
	for (ii =0; ii < 1000; ii +=1){ 
		printf ("%i, ", positionData[ii]);
	}
	printf ("\n");
}
