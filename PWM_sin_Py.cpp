#include <pyPulsedThread.h>
#include "PWM_sin_thread.h"

/* ************ Python C mosule using PWM_sin_thread to do PWM sine wave output from Python ******************************
Last Modifed:
2018/09/13 by Jamie Boyd - initial version

******************** Function called automatically when PyCapsule object is deleted in Python *****************************************
Last Modified:
2018/09/13 by Jamie Boyd - initial version */
static void  ptPWMsin_del(PyObject * PyPtr){
    delete static_cast<PWM_sin_thread *> (PyCapsule_GetPointer (PyPtr, "pulsedThread"));
}


/* ****************************************  Constructor ******************************************************
Creates and configures a PWM_sin_thread object to output a sin wave from PWM channel 0 GPIO 18 or from GPIO 40
appearing on the on audio Right channel , or from PWM channel 1, appearing on GPIO 19 or GPIO 41 appearing on
the audio Left channel
channel:  0 or 1 for PWM channel, plus 2 if outputting channel to audio instead of GPIO
enable: 1 to start the PWM channel running when thread is initialized, 0 to not initially enable the PWM output
initial freq: the frequency to start with
Last Modified;
2019/09/13 by Jamie Boyd - initial version*/
 static PyObject* ptPWMsin_New (PyObject *self, PyObject *args) {
	int channel;
	int enable;
	unsigned int initialFreq;
	if (!PyArg_ParseTuple(args,"iiI", &channel, &enable, &initialFreq)) {
		PyErr_SetString (PyExc_RuntimeError, "Could not parse input for channel, enable, and initial frequency.");
		return NULL;
	}
	PWM_sin_thread * threadObj = PWM_sin_thread::PWM_sin_threadMaker (channel, enable, initialFreq);
	if (threadObj == nullptr){
		PyErr_SetString (PyExc_RuntimeError, "PWM_sin_threadMaker was not able to make a PWM_sin object");
		return NULL;
	}else{
		return PyCapsule_New (static_cast <void *>(threadObj), "pulsedThread", ptPWMsin_del);
	}
  }
  
static PyObject* ptPWMsin__setFrequency (PyObject *self, PyObject *args){
	PyObject *PyPtr;
	unsigned int newlFreq;
	int isLocking;
	if (!PyArg_ParseTuple(args,"OIi",  &PyPtr, &newlFreq, &isLocking)) {
		PyErr_SetString (PyExc_RuntimeError, "Could not parse input for thread object, new frequency, and isLocking.");
		return NULL;
	}
	PWM_sin_thread * threadPtr = static_cast<PWM_sin_thread* > (PyCapsule_GetPointer(PyPtr, "pulsedThread"));
	int returnVal =threadPtr->setFrequency(newlFreq, isLocking);
	return  Py_BuildValue("i", returnVal);
}

static PyObject* ptPWMsin__setEnable(PyObject *self, PyObject *args){
	PyObject *PyPtr;
	int enableState;
	int isLocking;
	if (!PyArg_ParseTuple(args,"Oii",  &PyPtr, &enableState, &isLocking)) {
		PyErr_SetString (PyExc_RuntimeError, "Could not parse input for thread object, enable state, and isLocking.");
		return NULL;
	}
	PWM_sin_thread * threadPtr = static_cast<PWM_sin_thread* > (PyCapsule_GetPointer(PyPtr, "pulsedThread"));
	int returnVal =threadPtr->setEnable (enableState, isLocking);
	return  Py_BuildValue("i", returnVal);
}


/* Module method table - those starting with ptPWMsin are defined here, those starting with pulsedThread are defined in pyPulsedThread.h*/
static PyMethodDef ptPWM_sinMethods[] = {
	{"isBusy", pulsedThread_isBusy, METH_O, "(PyCapsule) returns number of tasks a thread has left to do, 0 means finished all tasks"},
	{"waitOnBusy", pulsedThread_waitOnBusy, METH_VARARGS, " (PyCapsule, timeOutSecs) Returns when a thread is no longer busy, or after timeOutSecs"},
	//{"doTask", pulsedThread_doTask, METH_O, "(PyCapsule) Tells the pulsedThread object to do whatever task it was configured for"},
	//{"doTasks", pulsedThread_doTasks, METH_VARARGS, "(PyCapsule, nTasks) Tells the pulsedThread object to do whatever task it was configured for nTasks times"},
	//{"unDoTasks", pulsedThread_unDoTasks, METH_O, "(PyCapsule) Tells the pulsedThread object to stop doing however many task it was asked to do"},
	//{"modDelay", pulsedThread_modDelay, METH_VARARGS, "(PyCapsule, newDelaySecs) changes the delay period of a pulse or LOW period of a train"},
	//{"modDur", pulsedThread_modDur, METH_VARARGS, "(PyCapsule, newDurationSecs) changes the delay period of a pulse or HIGH period of a train"},
	//{"getPulseDelay", pulsedThread_getPulseDelay, METH_O, "(PyCapsule) returns pulse delay, in seconds"},
	//{"getPulseDuration", pulsedThread_getPulseDuration, METH_O, "(PyCapsule) returns pulse duration, in seconds"},
	{"unsetEndFunc", pulsedThread_UnSetEndFunc, METH_O, "(PyCapsule) un-sets any end function set for this pulsed thread"},
	{"hasEndFunc", pulsedThread_hasEndFunc, METH_O, "(PyCapsule) Returns the endFunc status (installed or not installed) for this pulsed thread"},
	{"setEndFuncObj", pulsedThread_SetPythonEndFuncObj, METH_VARARGS, "(PyCapsule, PythonObj, int mode) sets a Python object to provide endFunction for pulsedThread"},
	{"setArrayEndFunc", pulsedThread_setArrayFunc, METH_VARARGS, "(PyCapsule, Python float array, endFuncType, isLocking) sets pulsedThread endFunc to set frequency (type 0) or duty cycle (type 1) from a Python float array"},
	{"cosDutyCycleArray", pulsedThread_cosineDutyCycleArray, METH_VARARGS, "(Python float array, pointsPerCycle, offset, scaling) fills passed-in array with cosine values of given period, with applied scaling and offset expected to range between 0 and 1"},
	
	{"new", ptPWMsin_New, METH_VARARGS, "(channel, enable, initial frequency) Creates and configures new PWM_sin task"},
	{"setFrequency", ptPWMsin__setFrequency, METH_VARARGS, "(PyCapsule, unsigned int frequency int isLocking) Sets the frequency of the sine output by PWM_sin"},
	{"setEnable", ptPWMsin__setEnable, METH_VARARGS, "(PyCapsule, int enableState, int isLocking)  Enables or disables the PWM_sin task"},
	{ NULL, NULL, 0, NULL}
  };
  
  /* Module structure */
  static struct PyModuleDef ptPWM_sinmodule = {
    PyModuleDef_HEAD_INIT,
    "ptPWM_sin",           /* name of module */
    "An external module for a sine wave made from a Raspberry Pi PWM output using pulsedThread",  /* Doc string (may be NULL) */
    -1,                 /* Size of per-interpreter state or -1 */
    ptPWM_sinMethods       /* Method table */
  };

  /* Module initialization function */
  PyMODINIT_FUNC
  PyInit_ptPWM_sin(void) {
    return PyModule_Create(&ptPWM_sinmodule);
  }  
