#include <Python.h>
#include "leverThread.h"

/* *** Makes and returns a new lever thread object. 


******************* Function called automatically when PyCapsule object is deleted in Python *****************************************
Last Modified:
2018/03/12 by Jamie Boyd - initial version */
void  py_Lever_del(PyObject * PyPtr){
	delete static_cast<leverThread*> (PyCapsule_GetPointer (PyPtr, "leverThread"));
}

static PyObject* py_LeverThread_New (PyObject *self, PyObject *args) {
	PyObject * bufferObj;
	unsigned int nCircular;
	int goalCuerPin;
	float cuerFreq;
	if (!PyArg_ParseTuple(args,"OIif", &bufferObj, &nCircular, &goalCuerPin, &cuerFreq)) {
		PyErr_SetString (PyExc_RuntimeError, "Could not parse input for lever position buffer, number for circular buffer, goal cuer pin, and cuer frequency.");
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
	leverThread * leverThreadPtr = leverThread::leverThreadMaker (static_cast <uint8_t *>(buffer.buf), (unsigned int) (buffer.len/buffer.itemsize), nCircular, goalCuerPin,cuerFreq );
	
	if (leverThreadPtr == nullptr){
		PyErr_SetString (PyExc_RuntimeError, "leverThreadMaker was not able to make a leverThread object");
		return NULL;
	}else{
		return PyCapsule_New (static_cast <void *>(leverThreadPtr), "leverThread", py_Lever_del);
	}
  }
  
  
static PyObject* py_leverThread_setConstForce (PyObject *self, PyObject *args){
	  PyObject *PyPtr;
	  int newForce;
	  if (!PyArg_ParseTuple(args,"Oi", &PyPtr, &newForce)) {
		PyErr_SetString (PyExc_RuntimeError, "Could not parse input for thread object and constant force");
		return NULL;
	}
	leverThread * leverThreadPtr = static_cast<leverThread * > (PyCapsule_GetPointer(PyPtr, "leverThread"));
	leverThreadPtr->setConstForce (newForce);
	Py_RETURN_NONE;
}

static PyObject* py_leverThread_getConstForce (PyObject *self, PyObject *PyPtr) {
	leverThread * leverThreadPtr = static_cast<leverThread * > (PyCapsule_GetPointer(PyPtr, "leverThread"));
	return Py_BuildValue("i", leverThreadPtr->getConstForce());
}

static PyObject* py_leverThread_applyForce (PyObject *self, PyObject *args){
	  PyObject *PyPtr;
	  int newForce;
	  if (!PyArg_ParseTuple(args,"Oi", &PyPtr, &newForce)) {
		PyErr_SetString (PyExc_RuntimeError, "Could not parse input for thread object and force");
		return NULL;
	}
	leverThread * leverThreadPtr = static_cast<leverThread * > (PyCapsule_GetPointer(PyPtr, "leverThread"));
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
	leverThread * leverThreadPtr = static_cast<leverThread * > (PyCapsule_GetPointer(PyPtr, "leverThread"));
	return Py_BuildValue("i", leverThreadPtr-> zeroLever (zeroMode, isLocking));
}

static PyObject* py_leverThread_setPerturbForce (PyObject *self, PyObject *args){
	  PyObject *PyPtr;
	  int perturbForce;
	  if (!PyArg_ParseTuple(args,"Oi", &PyPtr, &perturbForce)) {
		PyErr_SetString (PyExc_RuntimeError, "Could not parse input for thread object and perturbForce");
		return NULL;
	}
	leverThread * leverThreadPtr = static_cast<leverThread * > (PyCapsule_GetPointer(PyPtr, "leverThread"));
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
	leverThread * leverThreadPtr = static_cast<leverThread * > (PyCapsule_GetPointer(PyPtr, "leverThread"));
	leverThreadPtr->setPerturbStartPos (perturbStartPos);
	Py_RETURN_NONE;
}


static PyObject* py_leverThread_startTrial (PyObject *self, PyObject *PyPtr) {
	leverThread * leverThreadPtr = static_cast<leverThread * > (PyCapsule_GetPointer(PyPtr, "leverThread"));
	leverThreadPtr->startTrial();
	Py_RETURN_NONE;
}

static PyObject* py_leverThread_checkTrial (PyObject *self, PyObject *PyPtr) {
	leverThread * leverThreadPtr = static_cast<leverThread * > (PyCapsule_GetPointer(PyPtr, "leverThread"));
	int trialCode;
	bool isDone=leverThreadPtr->checkTrial(trialCode);
	PyObject *returnTuple;
	if (isDone){
		returnTuple = Py_BuildValue("(Oi)", Py_True, trialCode);
		Py_INCREF (Py_True);
	}else{
		returnTuple = Py_BuildValue("(Oi)", Py_False, trialCode);
		Py_INCREF (Py_False);
	}
	return returnTuple;
}





/* Module method table */
static PyMethodDef leverThreadMethods[] = {
  {"new", py_LeverThread_New, METH_VARARGS, "Creates a new instance of leverThread"},
  {"setConstForce", py_leverThread_setConstForce, METH_VARARGS, "Sets constant force field for leverThread"},
  {"getConstForce", py_leverThread_getConstForce, METH_O, "Returns constant force for leverThread"},
  {"applyForce", py_leverThread_applyForce, METH_VARARGS, "Sets force on lever for leverThread"},
  {"zeroLever", py_leverThread_zeroLever, METH_VARARGS, "Returns lever to front rail, optionally zeroing encoder"},
  {"setPerturbForce", py_leverThread_setPerturbForce, METH_VARARGS, "Fills perturbation force array with sigmoid ramp"},
  {"setPerturbStartPos", py_leverThread_setPerturbStartPos, METH_VARARGS, "sets start position of perturb force"},
  {"startTrial", py_leverThread_startTrial, METH_O, "starts a trial"},
  {"checkTrial", py_leverThread_checkTrial, METH_O, "returns a tuple of a boolean for trial completion and an integer for trial result"},
  { NULL, NULL, 0, NULL}
};

/* Module structure */
static struct PyModuleDef leverThreadmodule = {
  PyModuleDef_HEAD_INIT,
  "leverThread",           /* name of module */
  "Controls a leverThread for Auto Head Fix",  /* Doc string (may be NULL) */
  -1,                 /* Size of per-interpreter state or -1 */
  leverThreadMethods       /* Method table */
};

/* Module initialization function */
PyMODINIT_FUNC
PyInit_leverThread(void) {
  return PyModule_Create(&leverThreadmodule);
}
