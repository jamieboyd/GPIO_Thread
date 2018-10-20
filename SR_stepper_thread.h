#ifndef SR_STEPPER_THREAD_H
#define SR_STEPPER_THREAD_H
#include "pulsedThread.h"
#include "GPIOlowlevel.h"

/***********************************************************************************************
SR_stepper_thread uses a 8 bit Serial in/Parallel out shift register (595, e.g.) to drive a pair
of 2 phase bipolar stepper motors in half step mode using 3 GPIO pins instead of the 8 it
would require without the shift register.

A two phase bipolar stepper motor has 4 inputs, A, B, A\, and B\,. A and A\ are used to drive
the same coil/phase, but with opposite polarity.  B and B\ drive the other coil/phase.

In half-step mode sequence, sometimes only 1 coil/phase is energized, so A and A\ are not
always in opposite states. Same for B and B\. The half step motors use 4 GPIO pins each.

Half-step Mode Sequence
Step	A	B	A\	B\
0	1	0	0	0
1	1	1	0	0
2	0	1	0	0
3	0	1	1	0
4	0	0	1	0
5	0	0	1	1
6	0	0	0	1
7	1	0	0	1

Using the shift register as a serial to parallel converter, only 3 GPIO lines, serial data, 
shift register clock, and storage register clock are needed.  moreover, the 595 can
be daisy chained, so that this approach could be used to drive many more motors
at once, still only using 3 GPIO lines.

Pinout for 595 IC
Q1 -	1	16 - VCC
Q2 -	2	15 - Q0
Q3 -	3	14 - DS (serial data INPUT)
Q4 -	4	13 - OE (Output Enable, active low, tie to ground)
Q5 -	5	12 - STCP - Storage Register clock input,  data shifted from serial register to parallel ouputs on low-to-high transitions
Q6 -	6	11 - SHCP Shift register clock input, serial data shifted one stage on low-to-high transition
Q7 -	7	10 - Master Reset, active low, tie to 5V
GND	8	9 - QS7 - serial data output, use for daisy-chaining multiple shift registers together, connect to DS of next 595



/* *********************SR_stepper_thread class extends pulsedThread ****************
Does pulses and trains of pulses on Raspberry Pi GPIO pins */
class SR_stepper_thread : public pulsedThread{
	public:
	SR_stepper_thread (int pinA, int pinB, int pinAbar, int pinBbar, int mode, float scaling, float speed, int accuracyLevel) : 
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


#endif