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
	- makes the leverThread object, passing it a pointer to the lever position data, plus some size info,
	
2) the leverThread object
	- makes a leverThread struct 
	- receives data from calling program
	- writes to leverThreadStruct shared with the thread function to signal thread
	- makes the array for leverForce data and passes a pointer to that data to the leverThread struct
3) the threaded function that works with leverThreadStruct
*/

/* *********************** Forward declare functions used by thread so we can refer to them in constructor *************************/
int lever_init (void * initDataP, void *  &taskDataP);
void lever_Hi (void * taskData);

/* ***************** Init Data for lever Task ********************************/
typedef struct leverThreadInitStruct{
	uint8_t * positionData;			// array for inputs from quadrature decoder. 
	unsigned int nPositionData; 	// number of points in array,
	unsigned int nCircular;			// number of points at start to reserve for circular buffer at start, use nCircular for finite length trials
	int goalCuerPin;				// number of a GPIO pin to use for a cue that lever is in rewarded position, else 0 for no cue
	float cuerFreq;				// if a tone, frequency of tone to play. duty cycle is assumed to 0.5. If a simple on/off, pass 0
	unsigned int nForceData;		//
}leverThreadInitStruct, *leverThreadInitStructPtr;


/* ******************** Custom Data Struct for Lever Task***************************
*/
typedef struct leverThreadStruct{
	uint8_t * positionData;		// array for inputs from quadrature decoder, array passed in from calling function
	unsigned int nPositionData; // number of points in lever position array,
	unsigned int iPosition; 		// current place in position array
	uint8_t leverPosition; 		// current lever position
	unsigned int nCircular;		// number of points at start of position array to use for a circular buffer for uncued trials set to nPosition data for no circular buffer
	uint8_t leverTrialStartPos;	// when lever crosses here, we break out of circular buffer and start a trial. set to nPosition when not circular
	unsigned int circularBreak;	// where we broke out of circular buffer and started a trial.
	uint8_t goalBottom;			// bottom of goal area
	uint8_t goalTop;			// top of Goal area
	bool inGoal;				// thread sets this to true if lever is in goal position
	int * forceData;			// array for output to DAC for force output
	int constForce;			// value for constant force applied to lever
	unsigned int nForceData;	// number of points in force data array
	unsigned int iForce;		// current position in array
	int leverForce;				// current force on lever
	unsigned int forceStartPos;	// position in input array to start force output - set to end of array for no force output
	bool doForce;				// thread sets this to true when outputting force
	int i2c_fd; 				// file descriptor for i2c used by mcp4725 DAC
	uint8_t spi_wpData [5];		 // buffer for spi read/write - 5 bytes is as big as we need it to be
	SimpleGPIO_thread * goalCuer;// pre-made GPIO thread to give a cue when in goal range
	int goalMode;				// 1 for setHigh, setLow, 2 for startTrain, stopTrain
	
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



/* ********************* lever_thread class extends pulsedThread ****************
Works the motorized lever for the leverPulling task 
last modified:
2018/02/08 by Jamie Boyd - initial verison */
class lever_thread : public pulsedThread{
	public:
	/* integer param constructor: delay =0, duration = 5000 (200 hz), pulses = 0 for infinite train, can change 
	*/
	lever_thread (void * initData, int &errCode) : pulsedThread ((unsigned int) 0, (unsigned int)5000, (unsigned int) 200, initData, &lever_init, nullptr, &lever_Hi, 1, errCode) {
	
	};
	static lever_thread * lever_thread_threadMaker (uint8_t * positionData, unsigned int nPositionData, unsigned int nCircular, int goalCuerPin, float cuerFreq) ;

	
	protected:
};

#endif