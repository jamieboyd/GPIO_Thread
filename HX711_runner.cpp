#include "HX711.h"




/*
 g++ -O3 -std=gnu++11 -Wall -lpulsedThread GPIOlowlevel.cpp HX711.cpp HX711_runner.cpp -o HX711_Scale
*/

const int kDATAPIN=22;
const int kCLOCKPIN = 27;
const float kSCALING = 7.15e-05;
const int kNUM_WEIGHTS=200;

// **************************************************************************************************************
//  Gets a line of input from stdin
// Last modified 2016/06/01 by Jamie Boyd

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
	int dataPin;
	int clockPin; 
	// parse input paramaters for dataPin and clockPin. If not present, use defaults
	if (argc == 3){
		dataPin = atoi (argv[1]);
		clockPin = atoi (argv[2]);
		if (((dataPin > 0) && (dataPin < 30)) && ((clockPin > 0) && (clockPin < 30))){
			printf ("Initializing HX711 from command line arguments dataPin= %d and clockPin = %d.\n", dataPin, clockPin);
		}else{
			printf ("Bad value for dataPin= %d or clockPin = %d, now exiting\n", dataPin, clockPin);
			return 1;
		}
	}else{
		dataPin = kDATAPIN;
		clockPin = kCLOCKPIN;
		printf ("Initializing HX711 with default values dataPin= %d and clockPin = %d.\n", dataPin, clockPin);
	}
	// make a floating point array to hold weights
	float weightData [kNUM_WEIGHTS];
	// make a HX711 thread object
	HX711 * scale = HX711_threadMaker (dataPin, clockPin, kSCALING, &weightData, kNUM_WEIGHTS);
	if (scale == nullptr){
		printf("Could not create HX711_thread object.\n");
		return 1;
	}
	// make a temp buffer to hold a line of text to use with myGetLine
	int maxChars = 20;
	char * line = new char [maxChars];
	// some variables to handle menu selections
	signed char menuSelect;
	float newScaling;
	// Present menu and do selection, repeat until asked to quit
	for (;;){
		printf ("********** Enter a number from the menu below **********\n");
		printf ("-1:\tQuit the program.\n");
		printf ("0:\tTare the scale with average of 10 readings.\n");
		printf ("1:\tPrint current Tare value.\n");
		printf ("2:\tSet new scaling factor in grams per A/D unit\n");
		printf ("3:\tPrint current scaling factor.\n");
		printf ("4:\tWeigh something with a single reading\n");
		printf ("5\tWeigh something with an average of 10 readings\n");
		printf ("6\tStart a threaded read.\n"); 
		printf ("7\tSet scale to low power mode\n");
		printf ("8\tWake scale from low power mode\n");
		// scan the input into a string buffer
		if (myGetline(line, maxChars, 1)  == false)
			continue;
		// Get the menu selection, and check it
		sscanf (line, "%hhd\n", &menuSelect);
		if ((menuSelect < -1) || (menuSelect > 8)){
			printf ("You entered a selection, %hhd, outside the range of menu items (-1-8)\n", menuSelect);
			continue;
		} 
		switch (menuSelect){
			case -1:
				printf ("Quitting...\n");
				return 0;
				break;
			case 0:
				scale.tare (10, true);
			case 1:
				printf ("Tare Value is %.2f\n", scale.getTareValue());
				break;
			case 2:
				printf ("Enter new value for scaling factor in grams per A/D unit:");
				if (myGetline(line, maxChars, 1)  == false)
					break;
				sscanf (line, "%f\n", &newScaling);
				scale.setScaling (newScaling);
			case 3:
				printf ("Scaling factor is %.2f grams/unit)=",scale.getScaling());
				break;
			case 4:
				printf ("Measured Weight was %.2f grams.\n", scale.weigh (1,true));
				break;
			case 5:
				printf ("Measured Weight was %.2f grams.\n", scale.weigh (10,false));
				break;
			
			case 6:
				
				
				break;
			case 7:
				scale.turnOFF();
				break;
			case 8:
				scale.turnON ();
				break;
		}
	}
}