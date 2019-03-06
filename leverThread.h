
#ifndef LEVER_THREAD_H
#define LEVER_THREAD_H
/* ********************* libraries needed for lever thread ***************************** */
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include <math.h>
#include <wiringPi.h>	// used for spi (quad decoder) and i2c (analog out)
#include <wiringPiSPI.h>
#include <pulsedThread.h> // used for thread timing
#include "SimpleGPIO_thread.h" // used for goal cue 

/* ******************************************** Output Force Control*******************************
 *  can be set by PWM from the Pi's PWM controller, channel 1 on GPIO 18, 
 * or analog output from MCP4725 analog out chip on i2c bus.
 * Set FORCEMODE defined below to either AOUT or PWM and recompile the code
 * If using a Maxon motor driver, it must be configured appropriately for analog or PWM control
 * The mcp4725 ranges from 0 to 4095, scaled from 0 to 5V, so if using PWM, use a range 
 * of 4095 to match it. PWM frequency should be set to 1kHz. */
#define AOUT 0
#define PWM 1
/* ************************************ define FORCEMODE to PWM or AOUT *************************************************/
#define  FORCEMODE PWM
#if FORCEMODE == PWM
#include "PWM_thread.h"
#elif FORCEMODE == AOUT
#include <wiringPiI2C.h>
#endif

/* *********************** Lever Recording Frequency *******************************************
 * we use kLEVER_FREQ to calculate length of array needed for however long we want to record lever position
 * we make an array of force values for sigmoidal force transition for perturbation
 * kFORCE_ARRAY_SIZE times kLEVER_FREQ sets the duration of the force ramp */
const float kLEVER_FREQ = 250;
const unsigned int kFORCE_ARRAY_SIZE = 100;

/* ************************************************ mnemonic defines for setting goal cue **********************************/
#define kGOALMODE_NONE   0	// no goal cuer
#define kGOALMODE_HILO   1	// goal cuer  that sets hi and lo, as for an LED
#define kGOALMODE_TRAIN  2	// goal cuer that turns an infinite train ON and OFF, as for a tone
#define kLEVER_DIR_NORMAL    0
#define kLEVER_DIR_REVERSED  1
#define kTRIAL_UNCUED    0
#define kTRIAL_CUED      1
/* ******************************* LS7366R quadrature decoder constants ******************************/
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

#if FORCEMODE == AOUT
/* *************************** MCP4725 DAC constants  *******************************************/
const int kDAC_WRITEDAC = 0x040; // write to the DAC 
const int kDAC_WRITEEPROM = 0x60; // to the EPROM
const int kDAC_ADDRESS = 0x62; 	// i2c address to use
#elif FORCEMODE == PWM
const unsigned int PWM_RANGE = 4095;
const float PWM_FREQ = 1000; 
#endif



/* *********************** Forward declaration of non-class functions used by thread *************************/
int lever_init (void * initDataP, void *  &taskDataP);
void lever_Hi (void * taskData);
void leverThread_delTask (void * taskData);
int leverThread_zeroLeverCallback (void * modData, taskParams * theTask);


/* ***************** Init Data for lever Task ********************************
 * Last Modified:
 * 2019/03/05 by Jamie Boyd - made quad decoder array 2 bytes, signed integers */
typedef struct leverThreadInitStruct{
	int16_t * positionData;			// array for signed two byte values to hold data from quadrature decoder.
	unsigned int nPositionData; 	// number of points in positionData array,
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
	int16_t * positionData;		// array for inputs from quadrature decoder, array passed in from calling function
	unsigned int nPositionData; // number of points in lever position array, this limits maximum amount of time we can set nHoldTicks
	unsigned int iPosition; 		// current place in position array
	bool isReversed;			// true if polarity of quadrature decoder is reversed
	int16_t leverPosition; 		// current lever position in ticks of the lever, 0 -255, dumped here for easy access during a trial
	// Task control
	bool isCued;				// true if we are running in cued mode, false for uncued mode
	unsigned int breakPos;		// iPosition where lever crossed into goal area. Important for uncued trials
	unsigned int nToFinish;		// tick position where trial is finished, set by pulsedThread according to nCircularOrToGoal and nHoldTicks
	unsigned int nToGoalOrCircular;	// max points to get into goal area for cued trials, number of points for circular buffer at start, for uncued trials
	// fields for task difficulty, lever position and time
	int16_t goalBottom;			// bottom of Goal area
	int16_t goalTop;			// top of Goal area
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
	int * forceData;			// array for output to DAC or to PWM for force output
	unsigned int nForceData;	// number of points in force data array, we always use all of them, so transition time is constant
	unsigned int iForce;		// current position in force array
	unsigned int forceStartPos;	// position in decoder input array to start force output - set forceStartPos to end of decoder array for no force output and we never reach it
	// hardware access
	uint8_t spi_wpData [5];		 // buffer for spi read/write - 5 bytes is as big as we need it to be
	SimpleGPIO_thread * goalCuer;// simpleGPIO thread to give a cue when in goal range. 
#if FORCEMODE == AOUT
	int i2c_fd; 				// file descriptor for i2c used by mcp4725 DAC
#elif FORCEMODE == PWM
	// calculated register addresses for direct PWM control
	volatile unsigned int * ctlRegister; // address of PWM control register
	volatile unsigned int * dataRegister1; // address of register to write data to for channel 1
	float setFrequency;
	unsigned int pwmRange;
#endif
	int goalMode;				// for goal cuer, 1 for setHigh, setLow, 2 for startTrain, stopTrain
	
}leverThreadStruct, *leverThreadStructPtr;


/* ********************* leverThread class extends pulsedThread ****************
Works the motorized lever for the leverPulling task 
last modified:
2019/03/05 by Jamie Boyd - modifying for PWM force control, maiking lever position data 2 byte signed ionts instead of 1 bytse unsigned
2018/03/28 by Jamie Boyd - cleaning things up, adding comments, testing
2018/02/08 by Jamie Boyd - initial verison */
class leverThread : public pulsedThread{
	public:
	/* integer param constructor: delay =0, duration = 5000 (200 hz), nThreadPulseOrZero = 0 for infinite train for uncued, with circular buffer, or the size of the array, for cued trials */
	leverThread (void * initData, unsigned int nThreadPulsesOrZero, int &errCode) : pulsedThread ((unsigned int) 0, (unsigned int)(1E06/kLEVER_FREQ), (unsigned int) nThreadPulsesOrZero, initData, &lever_init, nullptr, &lever_Hi, 2, errCode) {
	};
	static leverThread * leverThreadMaker (int16_t * positionData, unsigned int nPositionData, bool isCued, unsigned int nCircularOrToGoal,  int isReversed, int goalCuerPinOrZero, float cuerFreqOrZero);
	// constant force, setting, geting, applying a force
	void setConstForce (int theForce);
	int getConstForce (void);
	void applyForce (int theForce);
	void applyConstForce (void);
	// setting perturb force and start positon
	void setPerturbForce(int perturbForce);
	void setPerturbStartPos(unsigned int perturbStartPos);
	void setPerturbOff (void);
	void setHoldParams (int16_t goalBottomP, int16_t goalTopP, unsigned int nHoldTicksP);
	int zeroLever (int checkZero, int isLocking); // if checkZero is set, returns 0 if new lever zero is same as old lever zero, else 1 
	void startTrial(void);
	bool checkTrial(int &trialCode, unsigned int &goalEntryPos);
	void doGoalCue (int offOn);
	void abortUncuedTrial(void);
	bool isCued (void);
	void setCue (bool isCuedP);

	int16_t getLeverPos (void);
	protected:
	leverThreadStructPtr taskPtr;
};

#endif
