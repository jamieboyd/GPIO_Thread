#include "Python.h"
#include "StepperMotor_thread.h"


/******************** Function called automatically when PyCapsule object is deleted in Python *****************************************
Last Modified:
2018/02/20 by Jamie Boyd - initial version */
static void  ptSR_stepper_del(PyObject * PyPtr){
    delete static_cast<SR_Stepper_thread *> (PyCapsule_GetPointer (PyPtr, "pulsedThread"));
}

/* ****************************************  Constructor ******************************************************
Last Modified:
2018/10/23 by Jamie Boyd - initial version */
static PyObject* ptSR_stepper_New (PyObject *self, PyObject *args) {
	int data_pin;
	int shift_reg_pin;
	int stor_reg_pin;
	int nMotors;
	float steps_per_sec;
	int accLevel;

	if (!PyArg_ParseTuple(args,"iiiifi", &data_pin, &shift_reg_pin, &stor_reg_pin, &nMotors, &steps_per_sec, &accLevel)) {
		PyErr_SetString (PyExc_RuntimeError, "Could not parse input for data, shift, and storage pin numbers, number of motors, steps per second, and accuracy level.");
		return NULL;
	}
	SR_Stepper_thread *  threadObj= SR_Stepper_thread::SR_Stepper_threadMaker (data_pin, shift_reg_pin,  stor_reg_pin, nMotors, steps_per_sec, accLevel);
	if (threadObj == nullptr){
		PyErr_SetString (PyExc_RuntimeError, "SR_Stepper_threadMaker was not able to make a SR_Stepper_thread object");
		return NULL;
	}else{
		return PyCapsule_New (static_cast <void *>(threadObj), "pulsedThread",ptSR_stepper_del);
	}
  }
  
  
  
  
  
  void moveSteps (int mDists [MAX_MOTORS]); // an array of number of steps you want travelled for each motor.  negative values are for negative directions
	void Free (int mFree [MAX_MOTORS]); // an array where a 1 means unhold the motor by setting all 4 outputs to 0, and a 0 means to leave the motor as it is
	void Hold (int mHold [MAX_MOTORS]); // an array where a 1 means to hold the motor firm by setting to position 7 where both coils are energized, and 0 is to leave the motor as is
	int emergStop (); // stops all motors ASAP
  
  
  
  
  /* ************************************************************* Module method table  ***************************************************************************
   those starting with ptSR_stepper are defined here, those starting with pulsedThread are defined in pyPulsedThread.h*/
static PyMethodDef ptSR_stepperMethods[] = {
	{"isBusy", pulsedThread_isBusy, METH_O, "(PyCapsule) returns number of tasks a thread has left to do, 0 means finished all tasks"},
	{"waitOnBusy", pulsedThread_waitOnBusy, METH_VARARGS, " (PyCapsule, timeOutSecs) Returns when a thread is no longer busy, or after timeOutSecs"},
	{"doTask", pulsedThread_doTask, METH_O, "(PyCapsule) Tells the pulsedThread object to do whatever task it was configured for"},
	{"doTasks", pulsedThread_doTasks, METH_VARARGS, "(PyCapsule, nTasks) Tells the pulsedThread object to do whatever task it was configured for nTasks times"},
	{"unDoTasks", pulsedThread_unDoTasks, METH_O, "(PyCapsule) Tells the pulsedThread object to stop doing however many task it was asked to do"},
	{"startTrain", pulsedThread_startTrain, METH_O, "(PyCapsule) Tells a pulsedThread object configured as an infinite train to start"},
	{"stopTrain", pulsedThread_stopTrain, METH_O, "(PyCapsule) Tells a pulsedThread object configured as an infinite train to stop"},
	{"modDelay", pulsedThread_modDelay, METH_VARARGS, "(PyCapsule, newDelaySecs) changes the delay period of a pulse or LOW period of a train"},
	{"modDur", pulsedThread_modDur, METH_VARARGS, "(PyCapsule, newDurationSecs) changes the delay period of a pulse or HIGH period of a train"},
	{"modTrainLength", pulsedThread_modTrainLength, METH_VARARGS, "(PyCapsule, newTrainLength) changes the number of pulses of a train"},
	{"modTrainDur", pulsedThread_modTrainDur, METH_VARARGS, "(PyCapsule, newTrainDurSecs) changes the total duration of a train"},
	{"modTrainFreq", pulsedThread_modFreq, METH_VARARGS, "(PyCapsule, newTrainFequency) changes the frequency of a train"},
	{"modTrainDuty", pulsedThread_modDutyCycle, METH_VARARGS, "(PyCapsule, newTrainDutyCycle) changes the duty cycle of a train"},
	{"getPulseDelay", pulsedThread_getPulseDelay, METH_O, "(PyCapsule) returns pulse delay, in seconds"},
	{"getPulseDuration", pulsedThread_getPulseDuration, METH_O, "(PyCapsule) returns pulse duration, in seconds"},
	{"getPulseNumber", pulsedThread_getPulseNumber, METH_O, "(PyCapsule) returns number of pulses in a train, 1 for a single pulse, or 0 for an infinite train"},
	{"getTrainDuration", pulsedThread_getTrainDuration, METH_O, "(PyCapsule) returns duration of a train, in seconds"},
	{"getTrainFrequency", pulsedThread_getTrainFrequency, METH_O, "(PyCapsule) returns frequency of a train, in Hz"},
	{"getTrainDutyCycle", pulsedThread_getTrainDutyCycle, METH_O, "(PyCapsule) returns duty cycle of a train, between 0 and 1"},
	{"unsetEndFunc", pulsedThread_UnSetEndFunc, METH_O, "(PyCapsule) un-sets any end function set for this pulsed thread"},
	{"hasEndFunc", pulsedThread_hasEndFunc, METH_O, "(PyCapsule) Returns the endFunc status (installed or not installed) for this pulsed thread"},
	{"setEndFuncObj", pulsedThread_SetPythonEndFuncObj, METH_VARARGS, "(PyCapsule, PythonObj, int mode) sets a Python object to provide endFunction for pulsedThread"},
	//{"setTaskFuncObj", pulsedThread_SetPythonTaskObj, METH_VARARGS, "(PyCapsule, PythonObj) sets a Python object to provide LoFunc and HiFunc for pulsedThread"},
	{"setArrayEndFunc", pulsedThread_setArrayFunc, METH_VARARGS, "(PyCapsule, Python float array, endFuncType, isLocking) sets pulsedThread endFunc to set frequency (type 0) or duty cycle (type 1) from a Python float array"},
	{"cosDutyCycleArray", pulsedThread_cosineDutyCycleArray, METH_VARARGS, "(Python float array, pointsPerCycle, offset, scaling) fills passed-in array with cosine values of given period, with applied scaling and offset expected to range between 0 and 1"},
	{"getModFuncStatus", pulsedThread_modCustomStatus, METH_O, "(PyCapsule) Returns 1 if the pulsedThread object is waiting for the thread to call a modFunction, else 0"},

		
	{"newDelayDur", ptSimpleGPIO_DelayDur, METH_VARARGS, "(pin, polarity, delay, dur, nPulses, accLevel) Creates and configures new SimpleGPIOthread"},
	{"newFreqDuty", ptSimpleGPIO_FreqDuty, METH_VARARGS, "(pin, polarity, frequency, dutyCycle, trainTime, accLevel) Creates and configures new  SimpleGPIOthread"},
	{"setLevel", ptSimpleGPIO_setLevel, METH_VARARGS, "(PyCapsule, int level, int isLocking) Sets the output level of a ptSimpleGPIO task to lo or hi when thread is not busy"},
	{"setPin", ptSimpleGPIO_setPin, METH_VARARGS, "(PyCapsule, int pin, int isLocking) Sets the GPIO pin used by the SimpleGPIO task"},
	{"getPin", ptSimpleGPIO_getPin, METH_O, "(PyCapsule) returns GPIO pin used by a SimpleGPIOthread, in Brodcomm numbering"},
	{"getPolarity", ptSimpleGPIO_getPolarity, METH_O, "(PyCapsule) returns pulse polarity (0 = low-to-high, 1 = high-to-low) of a SimpleGPIOthread"},
	
	{"simpleOutput", simpleGPIO_output_new, METH_VARARGS, "(int pinNumber, int initial level) Creates a simple output on a single pin, with no associated thread"},
	{"setOutput", simpleGPIO_output_set, METH_VARARGS, "(PyCapsule, int level) sets level of a simple output with no associated thread"},
	{ NULL, NULL, 0, NULL}
  };
  
  /* Module structure */
  static struct PyModuleDef ptSR_steppermodule = {
    PyModuleDef_HEAD_INIT,
    "ptSR_stepper",           /* name of module */
    "An external module for precise timing of Raspberry Pi GPIO pulses and trains with pulsedThreads",  /* Doc string (may be NULL) */
    -1,                 /* Size of per-interpreter state or -1 */
    ptSR_stepperMethods       /* Method table */
  };

  /* Module initialization function */
  PyMODINIT_FUNC
  PyInit_ptSR_stepper(void) {
    return PyModule_Create(&ptSR_steppermodule);
  }  