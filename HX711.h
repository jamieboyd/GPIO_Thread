#ifndef HX711_H
#define HX711_H
#include "SimpleGPIO_thread.h"
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
	
*/

/* ******************** Initialization struct for HX711 *******************************
 pin numbers, address base for memory mapped addresses, scaling constant (grams per A/D unit), pointer to array for weigh data, size of array */
typedef struct HX711InitStruct{
	int theClockPin; // pin to use for the GPIO output for clock
	int theDataPin; //pin to use for the GPIO input for data
	volatile unsigned int * GPIOperiAddr; // base address needed when writing to registers for setting and unsetting
	float scaling;
	float * weightData;         // pointer to the array to be filled with data, an array of floats
	unsigned int nWeights;
}HX711InitStruct, *HX711InitStructPtr;


// this C-style struct contains all the relevant thread variables and task variables, and is shared with the pulsedThread pthread
typedef struct HX711struct {
	unsigned int * GPIOperiHi; // address of register to WRITE clockPinBit bit to on Hi for clock
	unsigned int * GPIOperiLo; // address of register to WRITE clockPinBit bit to on Lo for clock
	unsigned int clockPinBit;	// clock pin number translated to bit position in register
	unsigned int * GPIOperiData; // address of register to READ from to get the data	
	unsigned int dataPinBit;	// data pin number translated to bit position in register 
	int * dataArray;			// an array of 24 integers used to hold the set bits for a single weighing
	unsigned int dataBitPos;	// tracks where we are in the 24 data bit positions
	int * pow2;				// precomputed array of powers of 2 used to translate bits that are set into data
	int thisWeight;         		// computed value for a single weight
	float scaling;				// grams per A/D unit
	float * weightData;         	// pointer to the array to be filled with data, an array of floats
	unsigned int nWeights;		// number of weights to get, must be less than the size of the array
	unsigned int iWeight;		// used as we iterate through the array
	float tareVal;				// tare scale value, in raw A/D units
}HX711struct, * HX711structPtr;

typedef struct HX711EndFuncStruct{
	
}

class HX711: SimpleGPIO_thread{
	public:
	HX711 (int dataPinP, int clockPinP, unsigned int delayUsecs, unsigned int durUsecs, unsigned int nPulses, void * initData, int (*initFunc)(void *, void *  &), void (* loFunc)(void *), void (*hiFunc)(void *), int accLevel , int &errCode) : pulsedThread (delayUsecs, durUsecs, nPulses, initData, initFunc, loFunc, hiFunc, accLevel,errCode) {
		dataPin = dataPinP;
		clockPin = clockPinP;
		isPoweredUp = true;
	};

	static HX711* HX711_threadMaker  (int dataPin, int clockPin, float scaling, float* weightData, unsigned int nWeights);
	void tare (int nAvg, bool printVals);
	float weigh (int nAvg, bool printVals);
	void weighThreadStart (float * weights, int nWeights);
	int weighThreadStop (void);
	int weighThreadCheck (void);
	int readValue (void);
	int getDataPin (void);
	int getClockPin(void);
	float getScaling (void);
	float getTareValue (void);
	void setScaling (float newScaling);
	void turnON(void);
	void turnOFF (void);

	private:
	int dataPin;
	int clockPin;
	float tareValue;
	bool isPoweredUp;

};

#endif // HX711_H
