#include <pyPulsedThread.h>
#include "SimpleGPIO_thread.h"

/* ************Simple GPIO Pulses and Trains from Python using SimpleGPIO_thread ******************************
Last Modifed:
2018/02/21 by Jamie Boyd
2018/02/19 by jamie Boyd
2018/02/05 by Jamie Boyd - updates for pulsedThread class/SimpleGPIO_thread subclass


******************** Function called automatically when PyCapsule object is deleted in Python *****************************************
Last Modified:
2018/02/20 by Jamie Boyd - initial version */
static void  ptSimpleGPIO_del(PyObject * PyPtr){
    delete static_cast<SimpleGPIO_thread *> (PyCapsule_GetPointer (PyPtr, "pulsedThread"));
}

/* ****************************************  Constructors ******************************************************
Creates and configures a SimpleGPIO_thread object to output a pulse or train of pulses on a GPIO pin
Two functions, one taking parameters of pulse timing and number, (delay in seconds, duration in seconds, and number of puses)
the other taking paramaters of train frequency,  train duration , and train duty cycle. 0 for either nPulses or trainDuration
requests an infinite train.
pin: number of the gpio pin used for the task. 
polarity: 0 for normally low; i.e., at start and end of train, line is set low,  1 for normally high
Last Modified;
2018/02/20 by Jamie Boyd - replaced calls to pulsedThread_del with  ptSimpleGPIO_del
2018/02/19 by Jamie Boyd - decided to not have separate constructors for pulses and trains, just for delay/duration and frequency/duty cycle
2018/02/05 by Jamie Boyd - updates for pulsedThread class/SimpleGPIO_thread subclass */
static PyObject* ptSimpleGPIO_DelayDur (PyObject *self, PyObject *args) {
	float delay; // in seconds from Python, but translated to microseconds when calling constructor
	float dur;   // in seconds from Python, but translated to microseconds when calling constructor
	int nPulses;
	int polarity;
	int pin;
	int accLevel;
	if (!PyArg_ParseTuple(args,"iiffii", &pin, &polarity, &delay, &dur, &nPulses, &accLevel)) {
		PyErr_SetString (PyExc_RuntimeError, "Could not parse input for pin number, polarity, delay, duration, number of Pulses, and accuracy level.");
		return NULL;
	}
	SimpleGPIO_thread *  threadObj= SimpleGPIO_thread::SimpleGPIO_threadMaker (pin, polarity, (unsigned int) round (1e06 *delay), (unsigned int) round (1e06 *dur), (unsigned int) nPulses, accLevel);
	if (threadObj == nullptr){
		PyErr_SetString (PyExc_RuntimeError, "SimpleGPIO_threadMaker was not able to make a GPIO thread object");
		return NULL;
	}else{
		return PyCapsule_New (static_cast <void *>(threadObj), "pulsedThread",ptSimpleGPIO_del);
	}
  }
  
  static PyObject* ptSimpleGPIO_FreqDuty (PyObject *self, PyObject *args) {
	float trainFrequency;
	float trainDuration;
	float trainDutyCycle;
	int polarity;
	int pin;
	int accLevel;
	if (!PyArg_ParseTuple(args,"iifffi", &pin, &polarity, &trainFrequency, &trainDutyCycle, &trainDuration,  &accLevel)) {
		PyErr_SetString (PyExc_RuntimeError, "Could not parse input for pin number, polarity, frequency, duty cycle, train duration, and accuracy level.");
		return NULL;
	}
	SimpleGPIO_thread *  threadObj= SimpleGPIO_thread::SimpleGPIO_threadMaker (pin, polarity, (float) trainFrequency, (float) trainDutyCycle, (float) trainDuration, accLevel); 
	if (threadObj == nullptr){
		PyErr_SetString (PyExc_RuntimeError, "SimpleGPIO_threadMaker was not able to make a GPIO thread object");
		return NULL;
	}else{
		return PyCapsule_New (static_cast <void *>(threadObj), "pulsedThread", ptSimpleGPIO_del);
	}
  }
  
 
  /* ****************** Setters and getters for Class Fields ***********************************************/
 static PyObject* ptSimpleGPIO_setPin (PyObject *self, PyObject *args){
	  PyObject *PyPtr;
	  int newPin;
	  int isLocking;
	  if (!PyArg_ParseTuple(args,"Oii", &PyPtr, &newPin, &isLocking)) {
		PyErr_SetString (PyExc_RuntimeError, "Could not parse input for thread object, newPin, and isLocking");
		return NULL;
	}
	SimpleGPIO_thread * threadPtr = static_cast<SimpleGPIO_thread * > (PyCapsule_GetPointer(PyPtr, "pulsedThread"));
	int returnVal =threadPtr->setPin (newPin, isLocking);
	return  Py_BuildValue("i", returnVal);
}


static PyObject* ptSimpleGPIO_getPin (PyObject *self, PyObject *PyPtr) {
	SimpleGPIO_thread * threadPtr = static_cast<SimpleGPIO_thread * > (PyCapsule_GetPointer(PyPtr, "pulsedThread"));
    return Py_BuildValue("i", threadPtr -> getPin());
}

static PyObject* ptSimpleGPIO_getPolarity (PyObject *self, PyObject *PyPtr) {
	SimpleGPIO_thread * threadPtr = static_cast<SimpleGPIO_thread * > (PyCapsule_GetPointer(PyPtr, "pulsedThread"));
    return Py_BuildValue("i", threadPtr -> getPolarity());
}


/* *********************** Mod functon to set the level of the GPIO pin to hi or lo directly *********************************
Last Modified:
2018/02/20 by jamie Boyd - added the isLocking parameter */
 static PyObject* ptSimpleGPIO_setLevel (PyObject *self, PyObject *args){
	  PyObject *PyPtr;
	  int theLevel;
	  int isLocking;
	  if (!PyArg_ParseTuple(args,"Oii", &PyPtr, &theLevel, &isLocking)) {
		PyErr_SetString (PyExc_RuntimeError, "Could not parse input for thread object, level, and isLocking");
		return NULL;
	}
	SimpleGPIO_thread* threadPtr = static_cast<SimpleGPIO_thread * > (PyCapsule_GetPointer(PyPtr, "pulsedThread"));
	int returnVal =threadPtr->setLevel (theLevel, isLocking);
	return  Py_BuildValue("i", returnVal);
}


/* ***************************************** code for a simple output without a thread**************************************
so we can set pin levels high and low directly from Python, without making a thread, and without loading wiringPi or other library 
************************************************************************************************************************************

**************************** Custom delete for simpleGPIO_output ****************************************************
Last Modified:
2018/02/20 by Jamie Boyd - harmonized naming */
static void  simpleGPIO_output_del(PyObject * PyPtr){
	unUseGPIOperi ();
	delete static_cast<SimpleGPIOStructPtr *> (PyCapsule_GetPointer (PyPtr, "simpleGPIO_output"));
}

/* **************Makes a new simpleGPIO_output and sets intial state *****************************
Last Modfied:
2018/02/20 by Jamie Boyd - harmonized naming */
static PyObject* simpleGPIO_output_new (PyObject *self, PyObject *args) {
	int pin;
	int initialState; // 0 for low, 1for high
	if (!PyArg_ParseTuple(args,"ii", &pin, &initialState)) {
		return NULL;
	}
	volatile unsigned int * GPIOperiAddr  = useGpioPeri ();
	if (GPIOperiAddr == nullptr){
		PyErr_SetString (PyExc_RuntimeError, " failed to map GPIO peripheral");
		return NULL;
	}
	SimpleGPIOStructPtr structPtr = new SimpleGPIOStruct;
	// calculate address to ON and OFF register as Hi or Lo as appropriate to save a level of indirection later
	structPtr->GPIOperiHi = (unsigned int *) (GPIOperiAddr + 7);
	structPtr->GPIOperiLo = (unsigned int *) (GPIOperiAddr + 10);
	// initialize pin for output
	*(GPIOperiAddr + ((pin) /10)) &= ~(7<<(((pin) %10)*3));
	*(GPIOperiAddr + ((pin)/10)) |=  (1<<(((pin)%10)*3));
	// calculate pinBit
	structPtr->pinBit =  1 << pin;
	// put pin in selected start state
	if (initialState ==0){
		*(structPtr->GPIOperiLo ) = structPtr->pinBit ;
	}else{
		*(structPtr->GPIOperiHi) = structPtr->pinBit ;
	}
	return PyCapsule_New (static_cast <void *>(structPtr), "simpleGPIO_output", simpleGPIO_output_del);
}

/* **************Sets level, hi or lo, of a simpleGPIO_output *****************************
Last Modfied:
2018/02/20 by Jamie Boyd - harmonized naming */
  static PyObject* simpleGPIO_output_set (PyObject *self, PyObject *args){
	  PyObject *PyPtr;
	  int level;
	  if (!PyArg_ParseTuple(args,"Oi", &PyPtr, &level)) {
		return NULL;
	}
	SimpleGPIOStructPtr structPtr = static_cast<SimpleGPIOStructPtr > (PyCapsule_GetPointer(PyPtr, "simpleGPIO_output"));
	if (level == 0){ // set high
		*(structPtr->GPIOperiLo) = structPtr->pinBit;
	}else{
		*(structPtr->GPIOperiHi) = structPtr->pinBit;
	}
	Py_RETURN_NONE;
}

	
/* Module method table - those starting with ptSimpleGPIO are defined here, those starting with pulsedThread are defined in pyPulsedThread.h*/
static PyMethodDef ptSimpleGPIOMethods[] = {
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
  static struct PyModuleDef ptSimpleGPIOmodule = {
    PyModuleDef_HEAD_INIT,
    "ptSimpleGPIO",           /* name of module */
    "An external module for precise timing of Raspberry Pi GPIO pulses and trains with pulsedThreads",  /* Doc string (may be NULL) */
    -1,                 /* Size of per-interpreter state or -1 */
    ptSimpleGPIOMethods       /* Method table */
  };

  /* Module initialization function */
  PyMODINIT_FUNC
  PyInit_ptSimpleGPIO(void) {
    return PyModule_Create(&ptSimpleGPIOmodule);
  }  
