
#ifndef LEVER_THREAD_H
#define LEVER_THREAD_H
/* ********** leverThread is for use with the corresponding Stimulator class for AutoHeadFix program *************
 *  The stimulator class AHF_Stimulator_leverThread will use the leverThread class from the Python module made from
 * the wrappers in leverThread_Py.cpp. But you could include the leverThread class in a C++ program if you wanted.
 */


/* ********************* libraries needed for lever thread ***************************** */
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include <math.h>
#include <unistd.h>
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
#define  FORCEMODE AOUT
#if FORCEMODE == PWM
#include "PWM_thread.h"
#elif FORCEMODE == AOUT
#include <wiringPiI2C.h>
#endif

/* *********************** Lever Recording Frequency *******************************************
 * we use kLEVER_FREQ to calculate length of array needed for however long we want to record lever position
 * we make an array of force values for sigmoidal force transition for perturbation
 * force array size divided by kLEVER_FREQ sets the duration of the force ramp. kMAX_FORCE_ARRAY_SIZE
 * sets the size of the array used, and thus the maximum time for perturbation sigmoid ramp */
const float kLEVER_FREQ = 250;
const unsigned int kMAX_FORCE_ARRAY_SIZE = 125;

/* ************************************************ mnemonic defines for setting goal cue **********************************/
#define kGOALMODE_NONE   0	// no goal cuer
#define kGOALMODE_HILO   1	// goal cuer  that sets hi and lo, as for an LED
#define kGOALMODE_TRAIN  2	// goal cuer that turns an infinite train ON and OFF, as for a tone
#define kLEVER_DIR_NORMAL    0 // read lever position in normal direction, lower values are nearer to the hold position, higher values means mouse has moved lever.
#define kLEVER_DIR_REVERSED  1 // read lever posiiton in reversed direction
#define kLEVER_BACKWARDS 0     // lever force to move lever backwards towards start position
#define kLEVER_FORWARDS 1     // lever force to move lever forwards towards mouse, away from start position
#define kTRIAL_UNCUED    0 // a trial runs as an infinite train until lever enters goal
#define kTRIAL_CUED      1 // a trial runs in one-shot mode. The calling code should make a start cue
/* ******************************* LS7366R quadrature decoder constants ******************************/
// SPI settings
const int kQD_CS_LINE  =  0;  // CS0 on pi
const int kQD_CLOCK_FREQ = 10000000;  // 10Mhz
// LS7366R device-specific constants  (we only use a few of these)
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
/* *************************** MCP4725 DAC constants *******************************************/
const int kDAC_WRITEDAC = 0x040; // write to the DAC
const int kDAC_WRITEEPROM = 0x60; // to the EPROM
const int kDAC_ADDRESS = 0x62; 	// i2c address to use
#elif FORCEMODE == PWM
/* ******************************** PWM Range and Frequency **********************************/
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
	bool isReversed;				// true if polarity of quadrature decoder is reversed. Motor force is not reversed.  reverse it with motorIsReversed or in Escon studio
	int goalCuerPin;				// number of a GPIO pin to use for a cue that lever is in rewarded position, else 0 for no cue
	int motorDirPin;				// if lever force direction is controlled by GPIO pin, the number of the pin. 0 if force is mapped symetrically around 2047
	bool motorIsReversed;		// if false, lower values of motor output (or motorDir set LOW, if motorDir is used)bring lever back to start position,
								// if true, higher values of motor output (or motorDir set high) bring lever back to start position
	float cuerFreq;				// if a tone, frequency of tone to play. duty cycle is assumed to 0.5. If a simple on/off, pass 0
	int startCuerPin;
	float startCuerFreq;
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
	// trialPos is 0 when trial starts
	// thread sets trialPos to 1 when lever first crosses into goal position. For cued trial, this must be before nToGoal, or trialPos is set to -1
	// thread sets trialpos to -1 if lever then leaves goal area before nHoldTicks has elapsed. trialComplete = true and trialPos=2 indicates success
	bool inGoal;				// thread sets this to true if lever is in goal position, false when not in goalPos, and uses this for doing the cue
	int trialPos;				// 1 when trial starts, 2 or -2 when trial ends
	bool trialComplete;			// set to false when trial begins, thread sets this to true when trial is complete.
	// fields for force control
	bool motorIsReversed;		// true if force on motor is reversed from usual low numbers/low level of motorDirPin output move lever closer to starting position
	bool motorHasDirPin;		// set to true if motor direction is controlled by GPIO pin and 0-4095 force input is scaled from 0 to max
								// set to false if 0-4095 force input is scaled symmetrically with 2048 = no force. if not motorIsReversed lower numbers move lever towards start pos
	SimpleGPIOStructPtr motorDir; // structure for controlling GPIO pin setting motor direction, if motor is not Bi. if motorIsReversed, GPIO High moves motor towards start position
	// fields for force data
	int constForce;			// value for constant force applied to lever when no force perturbation is happening
	int * forceData;			// array for output to DAC or to PWM for force output
	unsigned int nForceData;	// number of points in force data array, we always use all of them, so transition time is constant
	unsigned int iForce;		// current position in force array
	unsigned int forceStartPos;	// position in decoder input array to start force output - set forceStartPos to end of decoder array for no force output and we never reach it
	// hardware access
	uint8_t spi_wpData [5];		 // buffer for spi read/write - 5 bytes is as big as we need it to be
	SimpleGPIO_thread * goalCuer;// simpleGPIO thread to give a cue when in goal range. Use thread even if not a train
	SimpleGPIO_thread * startCuer;
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
	int startMode;

}leverThreadStruct, *leverThreadStructPtr;


/* ********************* leverThread class extends pulsedThread ****************
Works the motorized lever for the leverPulling task
last modified:
2019/03/05 by Jamie Boyd - modifying for PWM force control, maiking lever position data 2 byte signed ionts instead of 1 bytse unsigned
2018/03/28 by Jamie Boyd - cleaning things up, adding comments, testing
2018/02/08 by Jamie Boyd - initial verison */
class leverThread : public pulsedThread{
	public:
	//integer param constructor: delay =0, duration = 5000 (200 hz), nThreadPulseOrZero = 0 for infinite train for uncued, with circular buffer, or the size of the array, for cued trials
	leverThread (void * initData, unsigned int nThreadPulsesOrZero, int &errCode) : pulsedThread ((unsigned int) 0, (unsigned int)(1E06/kLEVER_FREQ), (unsigned int) nThreadPulsesOrZero, initData, &lever_init, nullptr, &lever_Hi, 2, errCode) {
	};
	static leverThread * leverThreadMaker (int16_t * positionData, unsigned int nPositionData, bool isCued, unsigned int nCircularOrToGoal,  int isReversed, int goalCuerPinOrZero, float cuerFreqOrZero, int MotorDirPinOrZero, int motorIsReversed);
	// lever position utilities
	int zeroLever (int resetZero, int isLocking); // returns position of lever when raoiled against post, before resetting the zero
	int16_t getLeverPos (void); // returns current lever position, from position array if task in progress, else reads the quad decoder itself
	void doGoalCue (int offOn); // turns ON or OFF the task used for signalling the lever is in goal position, for testing, mostly
	// constant force, setting, geting, applying
	void setConstForce (float theForce); // sets the force that will be applied at start of trial for mouse to pull against. from 0 to 1. Direction is always backward
	float getConstForce (void); // returns current value for constant force
	void applyConstForce (void); // applies the force currently set as constant force to the lever
	void  applyForce (float theForce, int direction); // applies an arbitrary force to the lever, 0 direction means towards starting position, 1 means towards mouse
	// seeting trial performance parameters for lever goal area, time to get to goal pos, hold time, and force perturbations
	void setHoldParams (int16_t goalBottomP, int16_t goalTopP, unsigned int nHoldTicksP); // sets lever goal area and hold time before each trial
	void setTicksToGoal (unsigned int ticksToGoal); // sets ticks mouse is given to get the lever into goal posiiton before trial aborts, also used for circular buffer size
	void setPerturbLength (unsigned int perturbLength); //sets length of portion of the force array used to generate the sigmoid of perturbation force
	unsigned int getPerturbLength (void); // returns length of the portion of force array used to genberate sigmidal ramp for perturbation force
	void setPerturbForce(float perturbForce); // calculates a sigmoid over force array from constant force to constant force plus perturb force, which can be negative
	void setPerturbStartPos(unsigned int perturbStartPos); // sets position in lever position array corresponding to point where perturb force will be applied to lever
	void setPerturbOff (void); // turns off perturb force application for upcoming trials
	// trial control, starting, stopping, checking progress and results
	void startTrial(void); // starts a trial, either cued or ucued trial, depending on settings
	bool checkTrial(int &trialCode, unsigned int &goalEntryPos); // Returns truth that a trial is completed, sets trial code to trial code, which will be 2 at end of a successful trial
	void setCue (bool isCuedP); // sets trials to be run in one-shot cued mode or in uncued mode with circular buffer until lever enters goal area
	bool isCued (void); // returns the truth that trials are set to be run in cued mode
	void abortUncuedTrial(void); // aborts an uncued trial, which could go on forever if lever is never moved into goal area. does nothing for a cued trial
	float constantForce; // scaled from 0 to 1, force applied to lever when not perturbing or zeroing
	float perturbForce; // force added to lever when perturbing. Scrunched to -constantForce < perturbForce < 1-constantForce
	protected:
	leverThreadStructPtr taskPtr;
};

#endif
