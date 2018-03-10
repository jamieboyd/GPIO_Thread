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
	gain selection and power down controls.
	When output data is not ready for retrieval, digital output pin DOUT is high.
	Serial clock input PD_SCK should be low. When DOUT goes to low, it indicates data is ready for retrieval.
	By applying 25~27 positive clock pulses at the PD_SCK pin, data is shifted out from the DOUT output pin.
	Each PD_SCK pulse shifts out one bit, starting with the MSB bit first, until all 24 bits are shifted out.
	The 25th pulse at PD_SCK input will pull DOUT pin back to high.
	Input and gain selection is controlled by adding a number of extra input PD_SCK pulses to the train
	after the data is collected

	PD_SCK Pulses   	Input channel   Gain
	25               		A              	128
	26               		B              	32
	27               		A              	64

	This code always runs the HX711 with high gain, input channel A, by using 25 pulses
		
	Data is 24 bit two's-complement differential signal
	min value is -8388608, max value is 8388607
	
	This maps onto a pulsedThread train with 25 pulses with 1 us delay and duration time
	
	
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
	unsigned int nWeightData;
}HX711InitStruct, *HX711InitStructPtr;

// this C-style struct contains all the relevant thread variables and task variables, and is shared with the pulsedThread pthread
typedef struct HX711struct {
	unsigned int * GPIOperiHi; // address of register to WRITE clockPinBit bit to on Hi for clock
	unsigned int * GPIOperiLo; // address of register to WRITE clockPinBit bit to on Lo for clock
	unsigned int clockPinBit;	// clock pin number translated to bit position in register
	unsigned int * GPIOperiData; // address of register to READ from to get the data	
	unsigned int dataPinBit;	// data pin number translated to bit position in register 
	int pow2 [24] ;			// precomputed array of powers of 2 used to translate bits that are set into data
	int dataBitPos;			// tracks where we are in the 24 data bit positions
	float * weightData;         	// pointer to the array to be filled with data, an array of floats
	unsigned int nWeightData;	// number of points in array of weight data
	unsigned int iWeight;		// used as we iterate through the array
	int controlCode;			// set to indicate weighing or taring. 
	float tareVal;				// tare scale value, in raw A/D units, but we need a float becaue it is an average of multiple readings
	float scaling;				// grams per A/D unit

}HX711struct, * HX711structPtr; 


class HX711: public pulsedThread{
	public:
	HX711 (int dataPinP, int clockPinP,  void * initData, int &errCode) : pulsedThread ((unsigned int)1, (unsigned int)1, (unsigned int) 25, initData, &HX711_Init, &HX711_Hi, &HX711_Lo, 1, errCode) {
		dataPin = dataPinP;
		clockPin = clockPinP;
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

	protected:
	float readSynchronous (unsigned int nAvg, bool printVals, int weighMode);
	int dataPin;
	int clockPin;
	bool isPoweredUp;
	HX711structPtr HX711TaskPtr;

};

#endif // HX711_H
