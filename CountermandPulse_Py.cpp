#include <pyPulsedThread.h>
#include "CountermandPulse.h"

/* ************ Python C module using CountermandPulse Sublcass of SimpleGPIO_thread ******************************
The first part of the file is pretty much the same as SimpleGPIO_Py.cpp, mutatis mutandi, with a few functions deleted
Two new functions for countermanding a pulse and asking if the last pulse was countermanded are added

Last Modifed:
2018/04/04 by Jamie Boyd - initial version


******************** Function called automatically when PyCapsule object is deleted in Python *****************************************
Last Modified:
2018/04/04 by Jamie Boyd - initial version */
static void  cmp_del(PyObject * PyPtr){
    delete static_cast<CountermandPulse *> (PyCapsule_GetPointer (PyPtr, "pulsedThread"));
}

/* ****************************************  Constructor modified from SimpleGPIO_Py.cpp ******************************************************
no need for number of pulses because it is always 1, that is, it is never a train
Creates and configures a countermandablePulse object to output a pulse on a GPIO pin
Parameters for pulse timing are delay in seconds, and duration in seconds
pin: number of the gpio pin used for the task. 
polarity: 0 for normally low; i.e., at start and end of train, line is set low,  1 for normally high
Last Modified;
2018/04/04 by Jamie Boyd - initial version */
static PyObject* cmp_DelayDur (PyObject *self, PyObject *args) {
	float delay; // in seconds from Python, but translated to microseconds when calling constructor
	float dur;   // in seconds from Python, but translated to microseconds when calling constructor
	int polarity;
	int pin;
	int accLevel;
	if (!PyArg_ParseTuple(args,"iiffi", &pin, &polarity, &delay, &dur,  &accLevel)) {
		PyErr_SetString (PyExc_RuntimeError, "Could not parse input for pin number, polarity, delay, duration, number of Pulses, and accuracy level.");
		return NULL;
	}
	CountermandPulse *  threadObj= CountermandPulse::CountermandPulse_threadMaker (pin, polarity, (unsigned int) round (1e06 *delay), (unsigned int) round (1e06 *dur), accLevel);
	if (threadObj == nullptr){
		PyErr_SetString (PyExc_RuntimeError, "CountermandPulse_threadMaker was not able to make a countermandPulse object");
		return NULL;
	}else{
		return PyCapsule_New (static_cast <void *>(threadObj), "pulsedThread",cmp_del);
	}
  }
 
  
  /* ****************** Setters and getters for Class Fields pin and polarity modified from SimpleGPIO_Py.cpp ***********************************************/
 static PyObject* cmp_setPin (PyObject *self, PyObject *args){
	  PyObject *PyPtr;
	  int newPin;
	  int isLocking;
	  if (!PyArg_ParseTuple(args,"Oii", &PyPtr, &newPin, &isLocking)) {
		PyErr_SetString (PyExc_RuntimeError, "Could not parse input for thread object, newPin, and isLocking");
		return NULL;
	}
	CountermandPulse * threadPtr = static_cast<CountermandPulse* > (PyCapsule_GetPointer(PyPtr, "pulsedThread"));
	int returnVal =threadPtr->setPin (newPin, isLocking);
	return  Py_BuildValue("i", returnVal);
}


static PyObject* cmp_getPin (PyObject *self, PyObject *PyPtr) {
	CountermandPulse* threadPtr = static_cast<CountermandPulse* > (PyCapsule_GetPointer(PyPtr, "pulsedThread"));
    return Py_BuildValue("i", threadPtr -> getPin());
}

static PyObject* cmp_getPolarity (PyObject *self, PyObject *PyPtr) {
	CountermandPulse* threadPtr = static_cast<CountermandPulse* > (PyCapsule_GetPointer(PyPtr, "pulsedThread"));
    return Py_BuildValue("i", threadPtr -> getPolarity());
}


/* *********************** Mod functon to set the level of the GPIO pin to hi or lo directly, modified from SimpleGPIO_Py.cpp *********************************
Really, there is no need to even substitute CountermandPulse *  for SimpleGPIO_thread * , as they are the same except for 2 added fields in the taskData
Last Modified:
2018/04/04 by Jamie Boyd - initial version */
static PyObject* cmp_setLevel (PyObject *self, PyObject *args){
	  PyObject *PyPtr;
	  int theLevel;
	  int isLocking;
	  if (!PyArg_ParseTuple(args,"Oii", &PyPtr, &theLevel, &isLocking)) {
		PyErr_SetString (PyExc_RuntimeError, "Could not parse input for thread object, level, and isLocking");
		return NULL;
	}
	CountermandPulse * threadPtr = static_cast<CountermandPulse * > (PyCapsule_GetPointer(PyPtr, "pulsedThread"));
	int returnVal =threadPtr->setLevel (theLevel, isLocking);
	return  Py_BuildValue("i", returnVal);
}

/* ********************************** Requests a countermanable pulse to be initiated **************************
 Returns true if the pulse was started (no pulse currently in progress), else false
 Last Modified:
 2018/04/04 by Jamie Boyd - initial version */
static PyObject* cmp_doCountermandPulse (PyObject *self, PyObject *PyPtr) {
    CountermandPulse* threadPtr = static_cast<CountermandPulse* > (PyCapsule_GetPointer(PyPtr, "pulsedThread"));
    if (threadPtr -> doCountermandPulse()){
        Py_RETURN_TRUE;
    }else{
        Py_RETURN_FALSE;
    }
}

/* ********************************** Function for actually Conutermanding a pulse ****************************************
Last Modified:
2018/04/04 by Jamie Boyd - initial version */
static PyObject* cmp_countermand (PyObject *self, PyObject *PyPtr) {
	CountermandPulse* threadPtr = static_cast<CountermandPulse* > (PyCapsule_GetPointer(PyPtr, "pulsedThread"));
	if (threadPtr -> countermand()){
		Py_RETURN_TRUE;
	}else{
		Py_RETURN_FALSE;
	}
}

/* ********************************** Function for seeing if previous pulse was countermanded ****************************************
returns True if the pulse was countermanded, else False
Last Modified:
2018/04/04 by Jamie Boyd - initial version */
static PyObject* cmp_wasCountermanded  (PyObject *self, PyObject *PyPtr) {
	CountermandPulse* threadPtr = static_cast<CountermandPulse* > (PyCapsule_GetPointer(PyPtr, "pulsedThread"));
	if ( threadPtr -> wasCountermanded()){
		Py_RETURN_TRUE;
	}else{
		Py_RETURN_FALSE;
	}
}
	
/* Module method table - those starting with cmp_ are defined here, those starting with pulsedThread are defined in pyPulsedThread.h*/
static PyMethodDef ptCountermandPulseMethods[] = {
	{"isBusy", pulsedThread_isBusy, METH_O, "(PyCapsule) returns number of tasks a thread has left to do, 0 means finished all tasks"},
	{"waitOnBusy", pulsedThread_waitOnBusy, METH_VARARGS, " (PyCapsule, timeOutSecs) Returns when a thread is no longer busy, or after timeOutSecs"},
	{"doTask", pulsedThread_doTask, METH_O, "(PyCapsule) Tells the pulsedThread object to do whatever task it was configured for"},
	{"doTasks", pulsedThread_doTasks, METH_VARARGS, "(PyCapsule, nTasks) Tells the pulsedThread object to do whatever task it was configured for nTasks times"},
	{"unDoTasks", pulsedThread_unDoTasks, METH_O, "(PyCapsule) Tells the pulsedThread object to stop doing however many task it was asked to do"},
	{"modDelay", pulsedThread_modDelay, METH_VARARGS, "(PyCapsule, newDelaySecs) changes the delay period of a pulse or LOW period of a train"},
	{"modDur", pulsedThread_modDur, METH_VARARGS, "(PyCapsule, newDurationSecs) changes the delay period of a pulse or HIGH period of a train"},
	{"getPulseDelay", pulsedThread_getPulseDelay, METH_O, "(PyCapsule) returns pulse delay, in seconds"},
	{"getPulseDuration", pulsedThread_getPulseDuration, METH_O, "(PyCapsule) returns pulse duration, in seconds"},
	{"unsetEndFunc", pulsedThread_UnSetEndFunc, METH_O, "(PyCapsule) un-sets any end function set for this pulsed thread"},
	{"hasEndFunc", pulsedThread_hasEndFunc, METH_O, "(PyCapsule) Returns the endFunc status (installed or not installed) for this pulsed thread"},
	{"setEndFuncObj", pulsedThread_SetPythonEndFuncObj, METH_VARARGS, "(PyCapsule, PythonObj, int mode) sets a Python object to provide endFunction for pulsedThread"},
	{"setArrayEndFunc", pulsedThread_setArrayFunc, METH_VARARGS, "(PyCapsule, Python float array, endFuncType, isLocking) sets pulsedThread endFunc to set frequency (type 0) or duty cycle (type 1) from a Python float array"},
	{"cosDutyCycleArray", pulsedThread_cosineDutyCycleArray, METH_VARARGS, "(Python float array, pointsPerCycle, offset, scaling) fills passed-in array with cosine values of given period, with applied scaling and offset expected to range between 0 and 1"},
	
	{"newDelayDur", cmp_DelayDur, METH_VARARGS, "(pin, polarity, delay, dur, accLevel) Creates and configures new CountermandPulse"},
	{"setLevel", cmp_setLevel, METH_VARARGS, "(PyCapsule, int level, int isLocking) Sets the output level of a CountermandPulse task to lo or hi when thread is not busy"},
	{"setPin", cmp_setPin, METH_VARARGS, "(PyCapsule, int pin, int isLocking) Sets the GPIO pin used by the CountermandPulse task"},
	{"getPin", cmp_getPin, METH_O, "(PyCapsule) returns GPIO pin used by a CountermandPulse, in Brodcomm numbering"},
	{"getPolarity", cmp_getPolarity, METH_O, "(PyCapsule) returns pulse polarity (0 = low-to-high, 1 = high-to-low) of a CountermandPulse"},
    {"doCountermandPulse", cmp_doCountermandPulse, METH_O, "(PyCapsule) starts a pulse that can be countermanded before the delay is finished"},
    {"countermand", cmp_countermand, METH_O, "(PyCapsule) tells a CountermandPulse to rescind just requested pulse"},
	{"wasCountermanded", cmp_wasCountermanded, METH_O, "(PyCapsule) returns truth that last requested pulse was stopped beforeit started"},
	{ NULL, NULL, 0, NULL}
  };
  
  /* Module structure */
  static struct PyModuleDef ptCountermandPulsemodule = {
    PyModuleDef_HEAD_INIT,
    "ptCountermandPulse",           /* name of module */
    "An external module for a countermandable Pulse from a Raspberry Pi GPIO pin using pulsedThread",  /* Doc string (may be NULL) */
    -1,                 /* Size of per-interpreter state or -1 */
    ptCountermandPulseMethods       /* Method table */
  };

  /* Module initialization function */
  PyMODINIT_FUNC
  PyInit_ptCountermandPulse(void) {
    return PyModule_Create(&ptCountermandPulsemodule);
  }  
