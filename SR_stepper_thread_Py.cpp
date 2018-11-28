#include "Python.h"
#include "StepperMotor_thread.h"


/******************** Function called automatically when PyCapsule object is deleted in Python *****************************************
Last Modified:
2018/02/20 by Jamie Boyd - initial version */
static void  ptSR_stepper_del(PyObject * PyPtr){
    delete static_cast<SR_Stepper_thread *> (PyCapsule_GetPointer (PyPtr, "pulsedThread"));
}

/* ****************************************  Constructor ******************************************************
makes a new SR_stepper_thread object and returns a pointer to it, wrapped in a PyCapsule
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
	SR_Stepper_thread * threadObj= SR_Stepper_thread::SR_Stepper_threadMaker (data_pin, shift_reg_pin, stor_reg_pin, nMotors, steps_per_sec, accLevel);
	if (threadObj == nullptr){
		PyErr_SetString (PyExc_RuntimeError, "SR_Stepper_threadMaker was not able to make a SR_Stepper_thread object");
		return NULL;
	}else{
		return PyCapsule_New (static_cast <void *>(threadObj), "pulsedThread", ptSR_stepper_del);
	}
}

/* ************************************* move funtion for stepper motors ******************************
takes an array of requested steps for each stepper motor
Last Modified:
2018/10/23 by Jamie Boyd - initial version */
static PyObject* ptSR_stepper_move (PyObject *self, PyObject *args) {
	PyObject *PyPtr;
	PyObject * bufferObj;
  	if (!PyArg_ParseTuple(args, "OO", &PyPtr, &bufferObj)) {
		PyErr_SetString (PyExc_RuntimeError, "Could not parse input for PyCapsule object and buffer object with array for steps to move");
		return NULL;
	}
	// parse buffer object
	int nData;
	int * arrayData;
	if (PyObject_CheckBuffer (bufferObj) == 0){
		PyErr_SetString (PyExc_RuntimeError, "Error getting bufferObj from Python array.");
		return NULL;
	}
	Py_buffer buffer;
	if (PyObject_GetBuffer (bufferObj, &buffer, PyBUF_FORMAT)==-1){
		PyErr_SetString (PyExc_RuntimeError,"Error getting C array from bufferObj from Python array");
		return NULL;
	}
#if beVerbose
	printf ("Buffer type is %s, length is %d bytes, and item size is %d.\n", buffer.format, buffer.len, buffer.itemsize);
#endif
	if (strcmp (buffer.format, "i") != 0){
		PyErr_SetString (PyExc_RuntimeError, "Error for bufferObj: data type of Python array is not integer");
		return NULL;
	}
	arrayData = static_cast <int *>(buffer.buf); // Now we have a pointer to the array from the passed in buffer
	nData = (int) buffer.len/buffer.itemsize;
	PyBuffer_Release (&buffer); // we don't need the buffer object as we have a pointer to the array start, which is all we care about
	// get pointer to SR_Stepper
	SR_Stepper_thread * stepperThread = static_cast<SR_Stepper_thread * > (PyCapsule_GetPointer(PyPtr, "pulsedThread"));
	stepperThread->moveSteps (arrayData);
	Py_RETURN_NONE;
}

/* ************************************* Free funtion for stepper motors ******************************
takes an array where a 1 frees the stepper motor, and a 0 leaves it alone
Last Modified:
2018/10/23 by Jamie Boyd - initial version */
static PyObject* ptSR_stepper_free (PyObject *self, PyObject *args) {
	PyObject *PyPtr;
	PyObject * bufferObj;
  	if (!PyArg_ParseTuple(args, "OO", &PyPtr, &bufferObj)) {
		PyErr_SetString (PyExc_RuntimeError, "Could not parse input for PyCapsule object and buffer object with array for steps to move");
		return NULL;
	}
	// parse buffer object
	int nData;
	int * arrayData;
	if (PyObject_CheckBuffer (bufferObj) == 0){
		PyErr_SetString (PyExc_RuntimeError, "Error getting bufferObj from Python array.");
		return NULL;
	}
	Py_buffer buffer;
	if (PyObject_GetBuffer (bufferObj, &buffer, PyBUF_FORMAT)==-1){
		PyErr_SetString (PyExc_RuntimeError,"Error getting C array from bufferObj from Python array");
		return NULL;
	}
#if beVerbose
	printf ("Buffer type is %s, length is %d bytes, and item size is %d.\n", buffer.format, buffer.len, buffer.itemsize);
#endif
	if (strcmp (buffer.format, "i") != 0){
		PyErr_SetString (PyExc_RuntimeError, "Error for bufferObj: data type of Python array is not integer");
		return NULL;
	}
	arrayData = static_cast <int *>(buffer.buf); // Now we have a pointer to the array from the passed in buffer
	nData = (int) buffer.len/buffer.itemsize;
	PyBuffer_Release (&buffer); // we don't need the buffer object as we have a pointer to the array start, which is all we care about
	// get pointer to SR_Stepper
	SR_Stepper_thread * stepperThread = static_cast<SR_Stepper_thread * > (PyCapsule_GetPointer(PyPtr, "pulsedThread"));
	return Py_BuildValue("i", stepperThread->Free(arrayData));
}

/* ************************************* Hold funtion for stepper motors ******************************
takes an array where a 1 holds the stepper motor, and a 0 leaves it alone
Last Modified:
2018/10/23 by Jamie Boyd - initial version */
static PyObject* ptSR_stepper_hold (PyObject *self, PyObject *args) {
	PyObject *PyPtr;
	PyObject * bufferObj;
  	if (!PyArg_ParseTuple(args, "OO", &PyPtr, &bufferObj)) {
		PyErr_SetString (PyExc_RuntimeError, "Could not parse input for PyCapsule object and buffer object with array for steps to move");
		return NULL;
	}
	// parse buffer object
	int nData;
	int * arrayData;
	if (PyObject_CheckBuffer (bufferObj) == 0){
		PyErr_SetString (PyExc_RuntimeError, "Error getting bufferObj from Python array.");
		return NULL;
	}
	Py_buffer buffer;
	if (PyObject_GetBuffer (bufferObj, &buffer, PyBUF_FORMAT)==-1){
		PyErr_SetString (PyExc_RuntimeError,"Error getting C array from bufferObj from Python array");
		return NULL;
	}
#if beVerbose
	printf ("Buffer type is %s, length is %d bytes, and item size is %d.\n", buffer.format, buffer.len, buffer.itemsize);
#endif
	if (strcmp (buffer.format, "i") != 0){
		PyErr_SetString (PyExc_RuntimeError, "Error for bufferObj: data type of Python array is not integer");
		return NULL;
	}
	arrayData = static_cast <int *>(buffer.buf); // Now we have a pointer to the array from the passed in buffer
	nData = (int) buffer.len/buffer.itemsize;
	PyBuffer_Release (&buffer); // we don't need the buffer object as we have a pointer to the array start, which is all we care about
	// get pointer to SR_Stepper
	SR_Stepper_thread * stepperThread = static_cast<SR_Stepper_thread * > (PyCapsule_GetPointer(PyPtr, "pulsedThread"));
	return Py_BuildValue("i", stepperThread->Hold(arrayData));
}

/* ************************************* Emergency Stop for stepper motors ******************************
Stops all stepper motors, ASAP
Last Modified:
2018/10/23 by Jamie Boyd - initial version */
static PyObject* ptSR_stepper_stop (PyObject *self, PyObject *PyPtr) {
	SR_Stepper_thread * stepperThread = static_cast<SR_Stepper_thread * > (PyCapsule_GetPointer(PyPtr, "pulsedThread"));
	return Py_BuildValue("i", stepperThread->emergStop());
}

/* **************************************** gets steps/second for all stepper motors ************************
Last Modified:
2018/10/24 by Jamie Boyd - initial version */
static PyObject* ptSR_stepper_get_speed (PyObject *self, PyObject *PyPtr) {
	SR_Stepper_thread * stepperThread = static_cast<SR_Stepper_thread * > (PyCapsule_GetPointer(PyPtr, "pulsedThread"));
	return Py_BuildValue("f", stepperThread->getStepsPerSec());
}

/* **************************************** sets steps/second for all stepper motors ************************
Last Modified:
2018/10/24 by Jamie Boyd - initial version */
static PyObject* ptSR_stepper_set_speed (PyObject *self, PyObject *args) {
	PyObject *PyPtr;
	float steps_per_sec;
  	if (!PyArg_ParseTuple(args, "Of", &PyPtr, &steps_per_sec)) {
		PyErr_SetString (PyExc_RuntimeError, "Could not parse input for PyCapsule object and requested speed");
		return NULL;
	}
	SR_Stepper_thread * stepperThread = static_cast<SR_Stepper_thread * > (PyCapsule_GetPointer(PyPtr, "pulsedThread");
	return Py_BuildValue("i", stepperThread->setStepsPerSec(steps_per_sec));
}

/* ************************************************** Module method table  ******************************************************************
those starting with ptSR_stepper are defined here, those starting with pulsedThread are defined in pyPulsedThread.h  
here, we only use isBusy and waitOnBusy from pyPulsedThread.h, the rest of the functions are not appropriate */
static PyMethodDef ptSR_stepperMethods[] = {
	{"isBusy", pulsedThread_isBusy, METH_O, "(PyCapsule) returns number of tasks a thread has left to do, 0 means finished all tasks"},
	{"waitOnBusy", pulsedThread_waitOnBusy, METH_VARARGS, " (PyCapsule, timeOutSecs) Returns when a thread is no longer busy, or after timeOutSecs"},

	{"new", ptSR_stepper_New, METH_VARARGS, "(data pin, shift reg pin, storage reg pin, num motors, steps/sec, accLevel) Creates and configures new Shift Register stepper."},
	{"move", ptSR_stepper_move, METH_VARARGS, "(PyCapsule, array of requested moves) Moves the stepper motors the requested amounts"},
	{"free", ptSR_stepper_free, METH_VARARGS, "(PyCapsule, array of requested frees) frees selected stepper motors to spin freely"},
	{"hold", ptSR_stepper_hold, METH_VARARGS, "(PyCapsule, array of requested holds) sets selected stepper motors to hold their position"},
	{"stop", ptSR_stepper_stop, METH_O, "(PyCapsule) stops ALL connected stepper motors, ASAP"},
	{"getSpeed", ptSR_stepper_get_speed, METH_O, "(PyCapsule) gets speed of stepper motors in steps/second"},
	{"setSpeed", ptSR_stepper_set_speed, METH_VARARGS, "(PyCapsule, steps/second) sets stepper motors to move at requested speed"},
	{ NULL, NULL, 0, NULL}
  };
  int data_pinP, int shift_reg_pinP, int stor_reg_pinP, int nMotorsP, float steps_per_secP, int accuracyLevel
  /* Module structure */
  static struct PyModuleDef ptSR_steppermodule = {
    PyModuleDef_HEAD_INIT,
    "ptSR_stepper",           /* name of module */
    "An external module for using Raspberry Pi GPIO outputs to drive stepper motors via shift registers",  /* Doc string (may be NULL) */
    -1,                 /* Size of per-interpreter state or -1 */
    ptSR_stepperMethods       /* Method table */
  };

  /* Module initialization function */
  PyMODINIT_FUNC
  PyInit_ptSR_stepper(void) {
    return PyModule_Create(&ptSR_steppermodule);
  }  