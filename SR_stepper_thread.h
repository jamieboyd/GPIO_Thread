#ifndef SR_STEPPER_THREAD_H
#define SR_STEPPER_THREAD_H
#include "pulsedThread.h"
#include "GPIOlowlevel.h"

/* **********************************************************************************************
SR_stepper_thread uses a 8 bit Serial in/Parallel out Shift register (595, e.g.) to drive a pair (or more)
of 2 phase bipolar stepper motors in half step mode using 3 GPIO pins instead of the 8 (or more) it
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
be daisy chained, so that this approach can be used to drive many more motors
at once, still only using 3 GPIO lines.

Pinout for 595 IC
Q1 -	1	16 - VCC
Q2 -	2	15 - Q0
Q3 -	3	14 - DS (serial data INPUT)
Q4 -	4	13 - OE (Output Enable, active low, tie to ground)
Q5 -	5	12 - STCP - Storage Register clock input,  data shifted from serial register to parallel ouputs on low-to-high transitions
Q6 -	6	11 - SHCP Shift register clock input, serial data shifted one stage on low-to-high transition
Q7 -	7	10 - Master Reset, active low, tie to 5V
GND -	8	9 - QS7 - serial data output, use for daisy-chaining multiple shift registers together, connect to DS of next 595


atttach the outputs to the stepper motor wires as follows, 
q0	q1	q2	q3	q4	q5	q6	q7
A	B	A\	B\	A	B 	A\	B\
____motor 1______    _____motor 2______


concatenate multiple 595 together as follows to drive multiple motors (this arrangement means we iterate backwards through nMotors when moving)
___________________595 #1______________   ________________595 #2_____________________
q0	q1	q2	q3	q4	q5	q6	q7		q0	q1	q2	q3	q4	q5	q6	q7
A	B	A\	B\	A	B 	A\	B\		A	B	A\	B\	A	B 	A\	B\
____motor 1______    _____motor 2____		_____motor 3______    _____motor 4______  


     __    __    __    __    __    __    __    __
 |__|  |__|  |__|  |__|  |__|  |__|  |__|  |__|     shift reg clock, starts high, set low in SR_stepper_Hi
                                                                                  set high in SR_stepper_Lo, data shifted on low-to-high
  __1__   0   __1_____1__   0   __1_____1__   0
 |     |_____|           |_____|           |_____    serial data, written on high-to-low of shift register clock,in SR_stepper_Hi

____                                             
    |____________________________________________|    storage reg clock, set low on first low-to-high transition of shift register clock, in SR_stepper_Lo
                                                                         set Hi at end of each train, from endFunc

*/
/* ********************************** constant for maximum number of motors we wish to drive at once ********************************/
const int MAX_MOTORS = 16 ; //  "should be enough for anybody" but if you want more, make it bigger


/* *********************************** Declare Non-class methods used by thread ***************************************************/
int SR_stepper_init (void * initDataP, void *  &taskDataP);
void SR_stepper_Lo (void *  taskData);
void SR_stepper_Hi (void *  taskData);
void SR_stepper_End (void * endFuncData, taskParams * theTask);


/* ******************** Initialization struct for SR_Stepper *******************************
 pin, polarity, and address base for memory mapped addresses to write to on HI and Lo */
typedef struct SR_StepperInitStruct{
	int data_pin; // pin to use for the data output
	int shift_reg_pin; // pin to use for shift register clock
	int stor_reg_pin; // pin to use for storage register clock (toggles at end of a train, could do it in an end-function)
	int nMotors; // how many motors are hooked up to this thing
	volatile unsigned int * GPIOperiAddr; // base address needed when writing to registers for setting and unsetting
}SR_StepperInitStruct, *SR_StepperInitStructPtr;


/* ******************** Custom Data Struct for SR_Stepper***************************
 memory mapped addresses to write to on HI and Lo, GPIO pin bits, shiftPos and direction data for each motor */
typedef struct SR_StepperStruct{
	unsigned int * GPIOperiHi; // address of register to write pin bit to on Hi
	unsigned int * GPIOperiLo; // address of register to write pin bit to on Lo
	unsigned int data_bit;	// pin number translated to bit position in register
	unsigned int shift_reg_bit; // pin number translated to bit position in register
	unsigned int stor_reg_bit; // pin number translated to bit position in register
	int shiftData [32] = {0,0,0,1,  0,0,1,1,  0,0,1,0,  0,1,1,0,  0,1,0,0,  1,1,0,0,  1,0,0,0,  1,0,0,1};  // the half step sequence, 8 groups of 4
	int motorDataPos [MAX_MOTORS]; // position of each motor output in the array of shiftData, increment for each motor as we pass through the array
	int nMotors; // number of motors hooked up in series, needs to be no bigger than MAX_MOTORS
	int iMotor; // the motor for which we are currently shifting out bits from the Data array, 4 at a time
	int iMotorAB; // we send out data for each motor 4 in a row, one for each of A,B, A\, B\, iterating iMotorAB from 0 to 3
	int motorDir [MAX_MOTORS]; // direction each motor is traveling, 1 for forward, -1 for negative, 0 for not moving (send same data)
	int iTrain; // number of train we are doing, each train is nMotors * 4 pulses long
	int nTrains [MAX_MOTORS]; // Not each motor will be moving the same number of steps, so will stop progressing through shiftData at different times
}SR_StepperStruct, *SR_StepperStructPtr;

//myGPIO->setEndFunc (&pulsedThreadDutyCycleFromArrayEndFunc);


/* *********************SR_stepper_thread class extends pulsedThread ****************
Each step is defined by a single train. The length of the train is  nMotors * 4.  
Last modified:
2018/10/22 by jamie Boyd - initial version modified from un-shift registered code */
class SR_stepper_thread : public pulsedThread{
	public:
	SR_stepper_thread (int data_pinP, int shift_reg_pinP, int stor_reg_pinP, int nMotorsP, unsigned int durUsecs, void * initData, int accuracyLevel, int &errCode) : pulsedThread (durUsecs, durUsecs, (nMotorsP * 4), initData, &SR_stepper_init, &SR_stepper_Lo, &SR_stepper_Hi, accLevel, errCode) {
	data_pin = data_pinP;
	shift_reg_pin=shift_reg_pinP;
	stor_reg_pin = stor_reg_pinP;
	nMotors = nMotorsP;
	};
	~SR_stepper_thread();
	static SR_stepper_thread * SR_stepper_threadMaker (int data_pinP, int shift_reg_pinP, int stor_reg_pinP, int nMotorsP, float steps_per_secP, int accuracyLevel) ; // static thread maker
	void moveSteps (int mDists [MAX_MOTORS]);
	protected:
	data_pin;
	shift_reg_pin;
	stor_reg_pin;
	nMotors ;
	
};

/*
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
}; */


#endif