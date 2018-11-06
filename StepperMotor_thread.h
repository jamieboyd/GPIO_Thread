#ifndef STEPPERMOTOR_THREAD_H
#define STEPPERMOTOR_THREAD_H
#include "pulsedThread.h"
#include "GPIOlowlevel.h"

 
/***********************************************************************************************
StepperMotor_thread is a class for controlling Raspberry Pi GPIO outputs for driving a stepper motor.
The needed pulses are timed by a separate thread using the pulsedThread library.
StepperMotor_thread drives a 2 phase bipolar stepper motor, either in half step or full step mode
using 4 GPIO pins, or in full step mode using 2 GPIO pins and an inverting buffer.

A two phase bipolar stepper motor has 4 inputs, A, B, A\, and B\. A and A\ are used to drive
the same coil/phase, but with opposite polarity.  B and B\ drive the other coil/phase.

In full mode sequence, A and A\ are always in opposite states, likewise B and B\.
If using the full step motor, you have the option of only using two GPIO pins, one each for A and B,
and feeding A and B into an inverter to get A\ and B\.

In half mode sequence, sometimes only 1 coil/phase is energized, so A and A\ are not
always in opposite states. Same for B and B\. So 2 GPIO lines and an inverter will not suffice.
The half step motor always uses 4 GPIO pins.

The big advantage of half step mode is that angular resolution is also increased i.e. it
it becomes double the angular resolution in full mode. The disadvantage of half step mode is 
the loss in torque that you get relative to full steps, and the need for 4 separate GPIO lines.

Full Mode Sequence
Stp	A	B	A\	B\
0	1   1	0	0
1	0	1	1	0
2	0	0	1	1
3	1	0	0	1

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

* ********************************** Mnemonic Constants for stepper motor *******************/

const int kSTEPPERMOTORFULL = 1;
const int kSTEPPERMOTORHALF =2;
const int kSTEPPERMOTORFULL_INV =3;

const int kSTEPPERFORWARD =1;
const int kSTEPPERREVERSE = -1;

/* *********************** Forward declare functions used by thread so we can refer to them in constructors*************************/
int StepperMotor_Init (void * initDataP, void *  volatile &taskDataP);
void StepperMotor_Full_forward (void * volatile taskData);
void StepperMotor_Full_backward (void * volatile taskData);
void StepperMotor_Half_forward(void * volatile taskData);
void StepperMotor_Half_backward(void * volatile taskData);
int StepperMotor_setMoveCallback (void * modData, taskParams * theTask); // requests a movement. modData is pointer to integer for number of steps
int StepperMotorHalf_setHoldCallBack (void * modData, taskParams * theTask); // set all pins to low and free motor to spin. modData is pointer to integer, 0 to unhold, 1 to hold

/* *********************** initialization struct the custom data used by thread functions **************************/
typedef struct StepperMotorInitStruct{
	int pins [4]; // the GPIO pins used to drive motor, need 2 for full step with inverter, 4 for full step without inverter and half step
	int stepperMode; //1 for full, 2 for half, 3 for full steps with inverting buffer, only 2 pins
	volatile unsigned int * GPIOperiAddr; // base address needed when writing to registers for setting and unsetting
}StepperMotorInitStruct, *StepperMotorInitStructPtr;


/* ************** custom data used by a stepper motor **********************************************************************/
typedef struct StepperMotorStruct{
	int pinBits [4]; // the precalculated bits used for the stepper motor pins
	int curPos; // current position in the cycle of steps
	int curDir; // current direction,1 for forwards, -1 for backwards
	int stepperMode;  //1 for full steps, 2 for half steps, 3 for full steps with inverting buffer
}StepperMotorStruct, *StepperMotorStructPtr;


/* *********************StepperMotor_thread class extends pulsedThread ****************
Does pulses and trains of pulses on Raspberry Pi GPIO pins */
class StepperMotor_thread : public pulsedThread{
	public:
	StepperMotor_thread (unsigned int delayUsecs, void * initData, void (*gLoFunc)(void *), void (*gHiFunc)(void *), int accuracyLevel, int &errCode) : pulsedThread(delayUsecs, delayUsecs, (unsigned int) 2, initData, &StepperMotor_Init, &gLoFunc, &gHiFunc, accuracyLevel, errCode){

	};
	~StepperMotor_thread();
	StepperMotor_thread * StepperMotor_threadMaker(int pinA, int pinB, int pinAbar, int pinBbar, int mode, float p_scaling, float p_speed, int accuracyLevel);
	
	
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
