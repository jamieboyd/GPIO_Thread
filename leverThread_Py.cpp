#include <Python.h>
#include "leverThread.h"

/* *** Makes and returns a new lever thread object. 


******************* Function called automatically when PyCapsule object is deleted in Python *****************************************
Last Modified:
2018/03/12 by Jamie Boyd - initial version */
void  py_Lever_del(PyObject * PyPtr){
	delete static_cast<leverThread*> (PyCapsule_GetPointer (PyPtr, "pulsedThread"));
}

static PyObject* py_LeverThread_New (PyObject *self, PyObject *args) {
	PyObject * bufferObj;
	int isCued;
	unsigned int nCircularOrToGoal;
	int isReversed;
	int goalCuerPin;
	float cuerFreq;
	if (!PyArg_ParseTuple(args,"OiIiif", &bufferObj, &isCued,&nCircularOrToGoal, &isReversed, &goalCuerPin, &cuerFreq)) {
		PyErr_SetString (PyExc_RuntimeError, "Could not parse input for lever position buffer, isCued, number for circular buffer or goal pos, isReversed, goal cuer pin, and cuer frequency.");
		return NULL;
	}
	// check that buffer is valid and that it is writable floating point buffer
	 if (PyObject_CheckBuffer (bufferObj) == 0){
		PyErr_SetString (PyExc_RuntimeError, "Error getting bufferObj from Python array.");
		return NULL;
	}
	Py_buffer buffer;
	if (PyObject_GetBuffer (bufferObj, &buffer, PyBUF_FORMAT)==-1){
		PyErr_SetString (PyExc_RuntimeError,"Error getting C array from bufferObj from Python array");
		return NULL;
	}
	if (strcmp (buffer.format, "B") != 0){
		PyErr_SetString (PyExc_RuntimeError, "Error for bufferObj: data type of Python array is not unsigned byte");
		return NULL;
	}
	// make a leverThread object
	leverThread * leverThreadPtr = leverThread::leverThreadMaker (static_cast <uint8_t *>(buffer.buf), (unsigned int) (buffer.len/buffer.itemsize), isCued,nCircularOrToGoal, isReversed, goalCuerPin,cuerFreq );
	
	if (leverThreadPtr == nullptr){
		PyErr_SetString (PyExc_RuntimeError, "leverThreadMaker was not able to make a leverThread object");
		return NULL;
	}else{
		return PyCapsule_New (static_cast <void *>(leverThreadPtr), "pulsedThread", py_Lever_del);
	}
  }
  
 /* *************************************** Sets value of the constant force that is stored in the lever struct *************************************/
static PyObject* py_leverThread_setConstForce (PyObject *self, PyObject *args){
	  PyObject *PyPtr;
	  int newForce;
	  if (!PyArg_ParseTuple(args,"Oi", &PyPtr, &newForce)) {
		PyErr_SetString (PyExc_RuntimeError, "Could not parse input for thread object and constant force");
		return NULL;
	}
	leverThread * leverThreadPtr = static_cast<leverThread * > (PyCapsule_GetPointer(PyPtr, "pulsedThread"));
	leverThreadPtr->setConstForce (newForce);
	Py_RETURN_NONE;
}


static PyObject* py_leverThread_getConstForce (PyObject *self, PyObject *PyPtr) {
	leverThread * leverThreadPtr = static_cast<leverThread * > (PyCapsule_GetPointer(PyPtr, "pulsedThread"));
	return Py_BuildValue("i", leverThreadPtr->getConstForce());
}

static PyObject* py_leverThread_applyForce (PyObject *self, PyObject *args){
	  PyObject *PyPtr;
	  int newForce;
	  if (!PyArg_ParseTuple(args,"Oi", &PyPtr, &newForce)) {
		PyErr_SetString (PyExc_RuntimeError, "Could not parse input for thread object and force");
		return NULL;
	}
	leverThread * leverThreadPtr = static_cast<leverThread * > (PyCapsule_GetPointer(PyPtr, "pulsedThread"));
	leverThreadPtr->applyForce (newForce);
	Py_RETURN_NONE;
}
static PyObject* py_leverThread_zeroLever (PyObject *self, PyObject *args){
	  PyObject *PyPtr;
	  int zeroMode;
	  int isLocking;
	  if (!PyArg_ParseTuple(args,"Oii", &PyPtr, &zeroMode, &isLocking)) {
		PyErr_SetString (PyExc_RuntimeError, "Could not parse input for thread object, zero Mode and isLocking");
		return NULL;
	}
	leverThread * leverThreadPtr = static_cast<leverThread * > (PyCapsule_GetPointer(PyPtr, "pulsedThread"));
	return Py_BuildValue("i", leverThreadPtr-> zeroLever (zeroMode, isLocking));
}

static PyObject* py_leverThread_setPerturbForce (PyObject *self, PyObject *args){
	  PyObject *PyPtr;
	  int perturbForce;
	  if (!PyArg_ParseTuple(args,"Oi", &PyPtr, &perturbForce)) {
		PyErr_SetString (PyExc_RuntimeError, "Could not parse input for thread object and perturbForce");
		return NULL;
	}
	leverThread * leverThreadPtr = static_cast<leverThread * > (PyCapsule_GetPointer(PyPtr, "pulsedThread"));
	leverThreadPtr->setPerturbForce (perturbForce);
	Py_RETURN_NONE;
}

static PyObject* py_leverThread_setPerturbStartPos (PyObject *self, PyObject *args){
	  PyObject *PyPtr;
	  unsigned int perturbStartPos;
	  if (!PyArg_ParseTuple(args,"OI", &PyPtr, &perturbStartPos)) {
		PyErr_SetString (PyExc_RuntimeError, "Could not parse input for thread object and perturb start position");
		return NULL;
	}
	leverThread * leverThreadPtr = static_cast<leverThread * > (PyCapsule_GetPointer(PyPtr, "pulsedThread"));
	leverThreadPtr->setPerturbStartPos (perturbStartPos);
	Py_RETURN_NONE;
}


static PyObject* py_leverThread_startTrial (PyObject *self, PyObject *PyPtr) {
	leverThread * leverThreadPtr = static_cast<leverThread * > (PyCapsule_GetPointer(PyPtr, "pulsedThread"));
	leverThreadPtr->startTrial();
	Py_RETURN_NONE;
}

static PyObject* py_leverThread_checkTrial (PyObject *self, PyObject *PyPtr) {
	leverThread * leverThreadPtr = static_cast<leverThread * > (PyCapsule_GetPointer(PyPtr, "pulsedThread"));
	int trialCode;
	unsigned int goalEntryPos;
	bool isDone=leverThreadPtr->checkTrial(trialCode, goalEntryPos);
	PyObject *returnTuple;
	if (isDone){
		returnTuple = Py_BuildValue("(OiI)", Py_True, trialCode,goalEntryPos );
		Py_INCREF (Py_True);
	}else{
		returnTuple = Py_BuildValue("(OiI)", Py_False, trialCode, goalEntryPos);
		Py_INCREF (Py_False);
	}
	return returnTuple;
}

static PyObject* py_leverThread_doGoalCue (PyObject *self, PyObject *args){
	  PyObject *PyPtr;
	  int offOn;
	  if (!PyArg_ParseTuple(args,"Oi", &PyPtr, &offOn)) {
		PyErr_SetString (PyExc_RuntimeError, "Could not parse input for thread object and offOn");
		return NULL;
	}
	leverThread * leverThreadPtr = static_cast<leverThread * > (PyCapsule_GetPointer(PyPtr, "pulsedThread"));
	leverThreadPtr->doGoalCue (offOn);
	Py_RETURN_NONE;
}

static PyObject* py_leverThread_setHoldParams (PyObject *self, PyObject *args){
	PyObject *PyPtr;
	uint8_t goalBottom;
	uint8_t goalTop;
	unsigned int nHoldTicks;
	if (!PyArg_ParseTuple(args,"OBBI", &PyPtr, &goalBottom, &goalTop, &nHoldTicks)) {
		PyErr_SetString (PyExc_RuntimeError, "Could not parse input for thread object, goalBottom, goalTop, and holdTicks");
		return NULL;
	}
	leverThread * leverThreadPtr = static_cast<leverThread * > (PyCapsule_GetPointer(PyPtr, "pulsedThread"));
	leverThreadPtr->setHoldParams (goalBottom, goalTop, nHoldTicks);
	Py_RETURN_NONE;
}

static PyObject* py_leverThread_getLeverPos(PyObject *self, PyObject *PyPtr) {
	leverThread * leverThreadPtr = static_cast<leverThread * > (PyCapsule_GetPointer(PyPtr, "pulsedThread"));
	return Py_BuildValue("B", leverThreadPtr->getLeverPos());
}
static PyObject* py_leverThread_abortUncuedTrial(PyObject *self, PyObject *PyPtr) {
	leverThread * leverThreadPtr = static_cast<leverThread * > (PyCapsule_GetPointer(PyPtr, "pulsedThread"));
	leverThreadPtr->abortUncuedTrial();
	Py_RETURN_NONE;
}
/* Module method table */
static PyMethodDef leverThreadMethods[] = {
  {"new", py_LeverThread_New, METH_VARARGS, "(lever position buffer, circular buffer num, isReversed, goal cuer pin, cuer frequency) Creates a new instance of leverThread"},
  {"setConstForce", py_leverThread_setConstForce, METH_VARARGS, "(PyPtr, newForce) Sets constant force to be used for leverThread"},
  {"getConstForce", py_leverThread_getConstForce, METH_O, "(PyPtr) Returns constant force used for leverThread"},
  {"applyForce", py_leverThread_applyForce, METH_VARARGS, "(PyPtr, force) Sets physical force on lever for leverThread"},
  {"zeroLever", py_leverThread_zeroLever, METH_VARARGS, "(PyPtr, zeroMode, isLocking) Returns lever to front rail, optionally zeroing encoder"},
  {"setPerturbForce", py_leverThread_setPerturbForce, METH_VARARGS, "(PyPtr, perturbForce) Fills perturbation force array with sigmoid ramp"},
  {"setPerturbStartPos", py_leverThread_setPerturbStartPos, METH_VARARGS, " (PyPtr, perturbPos) sets start position of perturb force"},
  {"startTrial", py_leverThread_startTrial, METH_O, "(PyPtr) starts a trial as currently confgured"},
  {"checkTrial", py_leverThread_checkTrial, METH_O, "(PyPtr) returns a three tuple of a boolean for trial completion and integers for trial result and goal position"},
  {"doGoalCue", py_leverThread_doGoalCue, METH_VARARGS, "(PyPtr, OffOn) turns In-Goal Cue on or off"},
  {"setHoldParams", py_leverThread_setHoldParams,  METH_VARARGS, "(PyPtr, goalBottom, goalTop, nHoldTicks) sets lever hold params for next trial."},
  {"getLeverPos",py_leverThread_getLeverPos, METH_O, "(PyPtr) returns the current lever position"},
  {"abortUncuedTrial", py_leverThread_abortUncuedTrial, METH_O, "(PyPtr) aborts an uncued trial."},
  { NULL, NULL, 0, NULL}
};

/* Module structure */
static struct PyModuleDef leverThreadmodule = {
  PyModuleDef_HEAD_INIT,
  "ptLeverThread",           /* name of module */
  "Controls a leverThread for Auto Head Fix",  /* Doc string (may be NULL) */
  -1,                 /* Size of per-interpreter state or -1 */
  leverThreadMethods       /* Method table */
};

/* Module initialization function */
PyMODINIT_FUNC
PyInit_ptLeverThread (void) {
  return PyModule_Create(&leverThreadmodule);
}
