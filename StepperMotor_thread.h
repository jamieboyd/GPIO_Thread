#ifndef STEPPERMOTOR_THREAD_H
#define STEPPERMOTOR_THREAD_H
#include "pulsedThread.h"
#include "GPIOlowlevel.h"

 
/***********************************************************************************************
StepperMotor drives a 2 phase bipolar stepper motor, in full step mode using 2 GPIO pins, or in half step mode, 
using 4 GPIO pins.  

A two phase bipolar stepper motor has 4 inputs, A, B, A\, and B\,. A and A\ are used to drive
the same coil/phase, but with opposite polarity.  B and B\ drive the other coil/phase.

In full mode sequence, A and A\ are always in opposite states, like wise
B and B\, so 2 GPIO lines and a hex-inverter can be used. The full step motor here only uses 
two pins, so feed A and B into an inverter to get A\ and B\.

Full Mode Sequence
Stp	A	B	A\	B\
0	1   1	0	0
1	0	1	1	0
2	0	0	1	1
3	1	0	0	1

In half mode sequence, sometimes only 1 coil/phase is energized, so A and A\ are not
always in opposite states. Same for B and B\. So 2 GPIO lines and a hex
inverter will not suffice. The half step motor here uses 4 GPIO pins.

Half Mode Sequence
Step	A	B	A\	B\
0	1	0	0	0
1	1	1	0	0
2	0	1	0	0
3	0	1	1	0
4	0	0	1	0
5	0	0	1	1
6	0	0	0	1
7	1	0	0	1


The big advantage of half step mode is that angular resolution is also increased i.e. it
it becomes double the angular resolution in full mode.

The disadvantage of half step mode is the loss in torque that you get relative to full steps, 
and the need for 4 separate GPIO lines.

The code describes a class for a stepper motor, subclassed for a stepperMotor with full steps
and a stepper motor with half steps with additional Abar and Bbar pins.
*/


const int kSTEPPERMOTORFULL = 1;
const int kSTEPPERMOTORHALF =2;
const int kSTEPPERFORWARD =1;
const int kSTEPPERREVERSE = -1;


 /* ***********************************************************************************************
 Non-class structures and functions - non-class because we pass these to the thread function
 as simple pointers
 ************************************************************************************************/
// The custom data used by a stepper motor thread function
typedef struct StepperMotorStruct{
	int pinBits [4]; // the precalculated bits used for the stepper motor pins
	unsigned int *GPIOaddr; // base address of GPIO peripheral
	int curPos; // current position in the cycle of steps
	int curDir; // current direction,1 for forwards, -1 for backwards
	int stepperMode;  //1 for full steps, 2 for half steps 
}StepperMotorStruct, *StepperMotorPtr;

// Data used to initialize the custom data used by thread functions
typedef struct StepperMotorInitStruct{
	int pins [4]; // the GPIO pins used to drive motor, need 2 for full step, 4 for half step
	int stepperMode; //1 for full, 2 for half
}StepperMotorInitStruct, *StepperMotorInitPtr;

// function declarations for non-classs thread functions
void StepperMotor_Full_forward (void * volatile taskData);
void StepperMotor_Full_backward (void * volatile taskData);
void StepperMotor_Half_forward(void * volatile taskData);
void StepperMotor_Half_backward(void * volatile taskData);

// custom data mod callbacks 
int StepperMotor_setMoveCallback (void * modData, taskParams * theTask); // requests a movement from the stepper motor. modData is pointer to integer for number of steps
int StepperMotorHalf_setHoldCallBack (void * modData, taskParams * theTask); // for half step, we can set all pins to low and free motor to spin. modData is poniter to integer,  0 to unhold, 1 to hold

/* *********************SimpleGPIO_thread class extends pulsedThread ****************
Does pulses and trains of pulses on Raspberry Pi GPIO pins */
class StepperMotor_thread : public pulsedThread{
	public:
	StepperMotor_thread (int pinA, int pinB, int pinAbar, int pinBbar, int mode, float scaling, float speed, int accuracyLevel) : 
	/* constructors, similar to pulsedThread, one expects unsigned ints for pulse delay and duration times in microseconds and number of pulses */
	SimpleGPIO_thread (int pinP, int polarityP, unsigned int delayUsecs, unsigned int durUsecs, unsigned int nPulses, void * initData, int accLevel , int &errCode) : 
	pinNumber = pinP;
	polarity = polarityP;
	};

class ptStepperMotor{
	public:
		ptStepperMotor(int pinA, int pinB, int pinAbar, int pinBbar, int mode, float scaling, float speed, int accuracyLevel); // base constructor 
		~ptStepperMotor();
		void setScaling (float newScaling);
		void setSpeed (float newSpeed);
		void setZero ();
		void moveRel (float distance);
		void moveAbs (float position);
		float getScaling ();
		float getSpeed ();
		float getPosition ();
		int isBusy();
		int waitOnBusy(float timeOut);
		void setHold (int hold); // hasfstep has addition function to set all pins low to allow motor to spin freely.  hold =0 to be free, 1 to be held.
	protected:
		float scaling;	// steps/unit (calibrated for whatever your unit is in, metres, degrees)
		float speed;	// units/second
		float position;		// in units (can be positive or negative, need to have 0 set)
		pulsedThread * stepperThread;  // pointer to the pulsedThread that makes and controls the threaded task to drive stepper motor
};


#endif // STEPPERMOTOR_THREAD_H
