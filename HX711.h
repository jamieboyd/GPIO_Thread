#ifndef HX711_H
#define HX711_H


#include "GPIOlowlevel.h"
#include <pulsedThread.h>

#include <stdio.h>
#include <sys/time.h>
#include <math.h>

/*   
	Class to get values from a HX711 Load Cell amplifier with scaling and taring
	Pin PD_SCK (clockPin) and DOUT (dataPin) are used for data retrieval, input selection, 
	gain selection and power down controls.  The HX711 outputs data on the the dataPin
	and the Pi controls the clock signal on the clock pin. 
	
	When the Pi holds the clockPin high for greater than 60 ms, the HX711 goes into low power mode, 
	holding the dataPin high.
	
	When the Pi sets the dataPin low, the HX711 wakes up, but does not have new data ready until 0.5 seconds
	after turning on. 
	
	
	When output data are not ready for retrieval, digital output pin DOUT is high.
	       _                                                 _
	____| |________________________________| |_________________________
	with clock pin held low, (not reading any data) data pin pulses are 100 usec, frequency is 11.2 Hz (90 msec spacing)
	each high-to-low transution indicates a new value is available from the HX711
	
	The data pin goes high when data has been read, and only goes low when new data is available
	__        _   _         _    __________________
	   |____| |_| |_____| |__|                          |__Data Pin
	
	
	___|||||||||||||||||||||||||____________________|| Clock Pin
	
	
	By applying 25~27 positive clock pulses at the clock pin, data is shifted out from the data output pin.
	Each clock pulse shifts out one bit, starting with the MSB bit first, until all 24 bits are shifted out.
	The 25th pulse at clock input will pull data pin back to high.
	Input and gain selection is controlled by adding a number of extra input clock pulses to the train
	after the data is collected

	clock Pulses   	Input channel   Gain
	25               		A              	128
	26               		B              	32
	27               		A              	64

	This code always runs the HX711 with high gain, input channel A, by using 25 pulses. 
	This maps onto a pulsedThread train with 25 pulses with 1 us delay and duration time
		
	Data is 24 bit two's-complement differential signal
	min value is -8388608, max value is 8388607
	
************ Forward declaration of functions used by thread so we can refer to them in constructor *********************************/

int HX711_Init (void * initDataP, void *  &taskDataP);
void HX711_Hi (void *  taskData);
void HX711_Lo (void *  taskData);

/* *************************** Constants for control codes passed to thread in controlCode *********************************************/
static const int kCTRL_WEIGH =0;
static const int kCTRL_TARE=1;

/* ******************** Initialization struct for HX711 *******************************
 pin numbers, address base for memory mapped addresses, scaling constant (grams per A/D unit), pointer to array for weigh data, size of array */
typedef struct HX711InitStruct{
	int theClockPin; // pin to use for the GPIO output for clock
	int theDataPin; //pin to use for the GPIO input for data
	volatile unsigned int * GPIOperiAddr; // base address needed when writing to registers for setting and unsetting
	float scaling;			// scaling in grams per A/D unit
	float * weightData;         // pointer to the array to be filled with data, an array of floats
	unsigned int nWeightData; // size of the array
}HX711InitStruct, *HX711InitStructPtr;

// this C-style struct contains all the relevant thread variables and task variables, and is shared with the pulsedThread pthread
typedef struct HX711struct {
	unsigned int * GPIOperiHi; // address of register to WRITE clockPinBit bit to on Hi for clock
	unsigned int * GPIOperiLo; // address of register to WRITE clockPinBit bit to on Lo for clock
	unsigned int clockPinBit;	// clock pin number translated to bit position in register
	volatile unsigned int * GPIOperiData; // address of register to READ from to get the data	THIS NEEDS TO BE VOLATILE FFS
	unsigned int dataPinBit;	// data pin number translated to bit position in register 
	int pow2 [24] ;			// precomputed array of powers of 2 used to translate bits that are set into data
	int dataBitPos;			// tracks where we are in the 24 data bit positions
	float * weightData;         	// pointer to the array to be filled with data, an array of floats
	unsigned int iWeight;		// used as we iterate through the array
	int controlCode;			// set to indicate weighing or taring. 
	float tareVal;				// tare scale value, in raw A/D units, but we need a float becaue it is an average of multiple readings
	float scaling;				// grams per A/D unit

}HX711struct, * HX711structPtr; 


class HX711: public pulsedThread{
	public:
	HX711 (int dataPinP, int clockPinP, unsigned int nDataP, void * initData,  int &errCode) : pulsedThread ((unsigned int)1, (unsigned int)1, (unsigned int) 25, initData, &HX711_Init, &HX711_Lo, &HX711_Hi, 1, errCode) {
		dataPin = dataPinP;
		clockPin = clockPinP;
		nWeightData = nDataP;
		isPoweredUp = true;
	};
	// destructor
	~HX711(void);
	static HX711* HX711_threadMaker (int dataPin, int clockPin, float scaling, float * weightData, unsigned int nWeights);
	float tare (int nAvg, bool printVals);
	float getTareValue (void);
	float weigh (unsigned int nAvg, bool printVals);
	void turnON (void);
	void turnOFF (void);
	void weighThreadStart (unsigned int nWeights);
	unsigned int weighThreadStop (void);
	unsigned int weighThreadCheck (void);
	
	int getDataPin (void);
	int getClockPin(void);
	float getScaling (void);
	void setScaling (float newScaling);
	unsigned int getNweights (void);
	private:
	float readSynchronous (unsigned int nAvg, bool printVals, int weighMode);
	int dataPin;
	int clockPin;
	bool isPoweredUp;
	unsigned int nWeightData;
	HX711structPtr HX711TaskPtr;

};

#endif // HX711_H
