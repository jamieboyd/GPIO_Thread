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
GND -8	9 - QS7 - serial data output, use for daisy-chaining multiple shift registers together, connect to DS of next 595


atttach the outputs to the stepper motor wires as follows, 
q0	q1	q2	q3	q4	q5	q6	q7
A	B	A\	B\	A	B 	A\	B\
____motor 1______    _____motor 2______


concatenate multiple 595 together as follows to drive multiple motors (this arrangement means we iterate backwards through nMotors when moving)
___________________595 #1______________   ________________595 #2_____________________
q0	q1	q2	q3	q4	q5	q6	q7		q0	q1	q2	q3	q4	q5	q6	q7
A	B	A\	B\	A	B 	A\	B\		A	B	A\	B\	A	B 	A\	B\
____motor 1______    _____motor 2____		_____motor 3______    _____motor 4______  


The pulsedThread is configured as a finite train.  The length of the train is  nMotors * 4.  All the motors are stepped together once in a single train
Movements of multiple steps are made by calling for the required number of trains. The HIgh function sets the shift_reg_pin low and sets the serial
data.  The LOw function sets the shift_reg_pin high and, only on the first pulse of the train, sets the storage register pin low. The End function sets the 
storage register pin high.

      __    __    __    __    __    __    __    __
 |__|  |__|  |__|  |__|  |__|  |__|  |__|  |__|     	shift reg clock, starts high, set low in SR_stepper_Hi
								set high in SR_stepper_Lo, data shifted on low-to-high
  __1_    0   __1___1   0    _1____1_   0
 |      |_____|           |_____|            |_____  	serial data, written on high-to-low of shift register clock,in SR_stepper_Hi

_____                                           
       |__________________________________| 	storage reg clock, starts high, set low on first low-to-high transition of shift register clock, in SR_stepper_Lo
								set Hi at end of each train, from endFunc





 ********************************** constant for maximum number of motors we wish to drive at once ********************************/
const int MAX_MOTORS = 16 ; //  "should be enough for anybody" but if you want more, make it bigger


/* *********************************** Declare Non-class methods used by thread ***************************************************/
int SR_stepper_init (void * initDataP, void *  &taskDataP);
void SR_stepper_Lo (void *  taskData);
void SR_stepper_Hi (void *  taskData);
void SR_stepper_End (void * endFuncData, taskParams * theTask);
void SR_stepper_delTask (void * taskData);


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
	int nSteps [MAX_MOTORS]; // Not each motor will be moving the same number of steps, so will stop progressing through shiftData at different times
}SR_StepperStruct, *SR_StepperStructPtr;


/* *********************SR_stepper_thread class extends pulsedThread ****************
Last modified:
2018/10/22 by jamie Boyd - initial version modified from un-shift registered code */
class SR_stepper_thread : public pulsedThread{
	public:
	SR_stepper_thread (unsigned int durUsecs, void * initData, int accuracyLevel, int &errCode) : pulsedThread (durUsecs, durUsecs, (unsigned int) (nMotorsP * 4), initData, &SR_stepper_init, &SR_stepper_Lo, &SR_stepper_Hi, accLevel, errCode) {
	};
	~SR_stepper_thread();
	static SR_stepper_thread * SR_stepper_threadMaker (int data_pinP, int shift_reg_pinP, int stor_reg_pinP, int nMotorsP, float steps_per_secP, int accuracyLevel) ; // static thread maker
	void moveSteps (int mDists [MAX_MOTORS]); // an array of number of steps you want travelled for each motor.  negative values are for negative directions
	void Free (int mFree [MAX_MOTORS]); // an array where a 1 means unhold the motor by setting all 4 outputs to 0, and a 0 means to leave the motor as it is
	void Hold (int mHold [MAX_MOTORS]); // an array where a 1 means to hold the motor firm by setting to position 7 where both coils are energized, and 0 is to leave the motor as is
	int emergStop (); // stops all motors ASAP
	float getStepsPerSec ();
	int setStepsPerSec (float stepsPerSec);
	protected:
	// an SR_StepperStructPtr for easy direct access to thread's task data 
	SR_StepperStructPtr taskPtr;
};



#endif