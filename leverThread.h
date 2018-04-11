#ifndef LEVER_THREAD_H
#define LEVER_THREAD_H
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include <math.h>
#include <wiringPi.h>
#include <wiringPiSPI.h>
#include <wiringPiI2C.h>
#include <pulsedThread.h>
#include "SimpleGPIO_thread.h"

/* The 3 parts to running the lever pulling task  
1) The program calling the leverThread object 
	- may be in Python through a Python C++ module that provides an interface to a leverThread object
	- makes an array of unsigned bytes to hold lever position data
	- makes the leverThread object, passing it a pointer to the lever position data, plus some size info
	- sets constant force, sets lever hold params, sets force params if doing force
	-starts a trial, first doing a cue for cued trials. Suppose we could have the leverthread do the cuing, as it does for goal cuing
	- checks a trial to see if it is done. For an uncued trial, this could be while. 
	- is responsible for saving the lever position data in the array before starting another trial
	
2) the leverThread object
	- makes a leverThread struct 
	- receives data from calling program
	- makes the array for leverForce data and passes a pointer to that data to the leverThread struct shared with the thread function
	- writes configuration data to leverThreadStruct, uses pulsedThread functions to signal thread

	
3) the threaded function that works with leverThreadStruct
	- timing controlled by pulsedThread superclass. Can be Trian or Infinite train with circular buffer
	- does the hardware stuff, reading the encoder and outputting force
	- saves lever position data in buffer
	- turns gioal cue on and off, sets trialPosition and breakPos for entering goal area
*/

/* *********************** Forward declare functions used by thread so we can refer to them in constructor *************************/
int lever_init (void * initDataP, void *  &taskDataP);
void lever_Hi (void * taskData);
void leverThread_delTask (void * taskData);
int leverThread_zeroLeverCallback (void * modData, taskParams * theTask);


/* ***************** Init Data for lever Task ********************************/
typedef struct leverThreadInitStruct{
	uint8_t * positionData;			// array for inputs from quadrature decoder. 
	unsigned int nPositionData; 	// number of points in array,
	bool isCued;					// true if lever task is started after an external cue
	unsigned int nToGoalOrCircular;	// max points to get into goal area for cued trials, number of points for circular buffer at start, for uncued trials
	bool isReversed;				// true if polarity of quadrature decoder is reversed. Motor force is not reversed.  reverse it with Escon studio
	int goalCuerPin;				// number of a GPIO pin to use for a cue that lever is in rewarded position, else 0 for no cue
	float cuerFreq;					// if a tone, frequency of tone to play. duty cycle is assumed to 0.5. If a simple on/off, pass 0
	unsigned int nForceData;		// size of force data array - sets time it takes to switch on perturbation with sigmoidal ramp
}leverThreadInitStruct, *leverThreadInitStructPtr;


/* ********************************************** Custom Data Struct for Lever Task********************************************************/
typedef struct leverThreadStruct{
	// lever position data
	uint8_t * positionData;		// array for inputs from quadrature decoder, array passed in from calling function
	unsigned int nPositionData; // number of points in lever position array, this limits maximum amount of time we can set nHoldTicks
	unsigned int iPosition; 		// current place in position array
	bool isReversed;			// true if polarity of quadrature decoder is reversed
	uint8_t leverPosition; 		// current lever position in ticks of the lever, 0 -255, dumped here for easy access during a trial
	// Task control
	bool isCued;				// true if we are running in cued mode, false for uncued mode
	unsigned int breakPos;		// iPosition where lever crossed into goal area. Important for uncued trials
	unsigned int nToFinish;		// tick position where trial is finished, set by pulsedThread according to nCircularOrToGoal and nHoldTicks
	unsigned int nToGoalOrCircular;	// max points to get into goal area for cued trials, number of points for circular buffer at start, for uncued trials
	// fields for task difficulty, lever position and time
	uint8_t goalBottom;			// bottom of Goal area
	uint8_t goalTop;			// top of Goal area
	unsigned int nHoldTicks;	// number of ticks lever needs to be held, after getting to goal area. nToGoalOrCircular + nHoldTicks must be less than nPositionData
	// tracking trial progress. trialPos is positive for good trials, negative when the mouse fails
	// trialPos is 1 when trial starts
	// thread sets trialPos to 2 when lever first crosses into goal position. For cued trial, this must be before nToGoal, or trialPos is set to -1 
	// thread sets trialpos to -2 if lever then leaves goal area before nHoldTicks has elapsed. trialComplete = true and trialPos=2 indicates success
	bool inGoal;				// thread sets this to true if lever is in goal position, false when not in goalPos, and uses this for doing the cue
	int trialPos;				// 1 when trial starts, 2 or -2 when trial ends
	bool trialComplete;			// set to false when trial begins, thread sets this to true when trial is complete.
	// fields for force data
	int constForce;				// value for constant force applied to lever when no force perturbation is happening
	int perturbForce;			// additional force used for perturbing
	int * forceData;			// array for output to DAC for force output
	unsigned int nForceData;	// number of points in force data array, we always use all of them, so transition time is constant
	unsigned int iForce;		// current position in force array
	unsigned int forceStartPos;	// position in decoder input array to start force output - set forceStartPos to end of decoder array for no force output and we never reach it
	// hardware access
	int i2c_fd; 				// file descriptor for i2c used by mcp4725 DAC
	uint8_t spi_wpData [5];		 // buffer for spi read/write - 5 bytes is as big as we need it to be
	SimpleGPIO_thread * goalCuer;// simpleGPIO thread to give a cue when in goal range. 
	int goalMode;				// for goal cuer, 1 for setHigh, setLow, 2 for startTrain, stopTrain
	
}leverThreadStruct, *leverThreadStructPtr;


/* ******************************* LS7366R quadrature decoder constants******************************/
// SPI settings
const int kQD_CS_LINE  =  0;  // CS0 on pi
const int kQD_CLOCK_FREQ = 10000000;  // 10Mhz
// LS7366R device-specific constants 
const uint8_t kQD_CLEAR_COUNTER = 0x20;
const uint8_t kQD_CLEAR_STATUS = 0x30;
const uint8_t kQD_READ_COUNTER = 0x60;
const uint8_t kQD_READ_STATUS = 0x70;
const uint8_t kQD_WRITE_MODE0 = 0x88;
const uint8_t kQD_WRITE_MODE1 = 0x90;
const uint8_t kQD_ONEX_COUNT = 0x01;
const uint8_t kQD_TWOX_COUNT = 0x02;
const uint8_t kQD_FOURX_COUNT = 0x03;
const uint8_t kQD_FOURBYTE_COUNTER = 0x00;
const uint8_t kQD_THREEBYTE_COUNTER = 0x01;
const uint8_t kQD_TWOBYTE_COUNTER = 0x02;
const uint8_t kQD_ONEBYTE_COUNTER = 0x03;

/* *************************** MCP4725 DAC constants  *******************************************/
const int kDAC_WRITEDAC = 0x040; // write to the DAC 
const int kDAC_WRITEEPROM = 0x60; // to the EPROM
const int kDAC_ADDRESS = 0x62; 	// i2c address to use


/* Lever recording frequency, we use this to calculate length of array needed for however long we want to record*/
const float kLEVER_FREQ = 250;
const unsigned int kFORCE_ARRAY_SIZE = 100;

/* ************************************************constants for settings **********************************/
const int kGOALMODE_NONE =0;	// no goal cuer
const int kGOALMODE_HILO =1;	// goal cuer  that sets hi and lo, as for an LED
const int kGOALMODE_TRAIN =2;	// goal cuer that turns an infinite train ON and OFF, as for a tone

/* ********************* leverThread class extends pulsedThread ****************
Works the motorized lever for the leverPulling task 
last modified:
2018/03/28 by Jamie Boyd - cleaning things up, adding comments, testing
2018/02/08 by Jamie Boyd - initial verison */
class leverThread : public pulsedThread{
	public:
	/* integer param constructor: delay =0, duration = 5000 (200 hz), nThreadPulseOrZero = 0 for infinite train for uncued, with circular buffer, or the size of the array, for cued trials */
	leverThread (void * initData, unsigned int nThreadPulsesOrZero, int &errCode) : pulsedThread ((unsigned int) 0, (unsigned int)(1E06/kLEVER_FREQ), (unsigned int) nThreadPulsesOrZero, initData, &lever_init, nullptr, &lever_Hi, 2, errCode) {
	};
	static leverThread * leverThreadMaker (uint8_t * positionData, unsigned int nPositionData, bool isCued, unsigned int nCircularOrToGoal,  int isReversed, int goalCuerPinOrZero, float cuerFreqOrZero) ;
	// constant force, setting, geting, applying a force
	void setConstForce (int theForce);
	int getConstForce (void);
	void applyForce (int theForce);
	void applyConstForce (void);
	// setting perturb force and start positon
	void setPerturbForce(int perturbForce);
	void setPerturbStartPos(unsigned int perturbStartPos);
	void setPerturbOff (void);
	void setHoldParams (uint8_t goalBottomP, uint8_t goalTopP, unsigned int nHoldTicksP);
	int zeroLever (int mode, int isLocking);
	
	void startTrial(void);
	bool checkTrial(int &trialCode, unsigned int &goalEntryPos);
	void doGoalCue (int offOn);
	void abortUncuedTrial(void);
	uint8_t getLeverPos (void);
	protected:
	leverThreadStructPtr taskPtr;
};

#endif