#include <Python.h>
#include "lever_thread.h"

/* *** Makes and returns a new lever thread object. 


******************* Function called automatically when PyCapsule object is deleted in Python *****************************************
Last Modified:
2018/03/12 by Jamie Boyd - initial version */
void  py_Lever_del(PyObject * PyPtr){
	delete static_cast<lever_thread*> (PyCapsule_GetPointer (PyPtr, "leverThread"));
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
	leverThread * leverThreadPtr = leverThread::leverThreadMaker (static_cast <uint8_t *>(buffer.buf), (unsigned int) buffer.len/buffer.itemsize, nCircular, goalCuerPin,cuerFreq );
	
	if (leverThreadPtr == nullptr){
		PyErr_SetString (PyExc_RuntimeError, "leverThreadMaker was not able to make a leverThread object");
		return NULL;
	}else{
		return PyCapsule_New (static_cast <void *>(leverThreadPtr), "leverThread", py_Lever_del);
	}
  }
  
  
/* Module method table */
static PyMethodDef leverThreadMethods[] = {
  {"new", py_LeverThread_New, METH_VARARGS, "Creates a new instance of leverThread"},
  
  /*
  {"tare", py_leverThread_tare, METH_VARARGS, "Tares the leverThread load cell amplifier"},
  {"weigh", py_leverThread_weigh, METH_VARARGS, "Weighs with the leverThread load cell amplifier"},
  {"weighThreadStart",py_leverThread_weighThreadStart, METH_VARARGS, "Starts the thread weighing into the array"},
  {"weighThreadStop", py_leverThread_weighThreadStop, METH_O, "Stops the thread weighing, returns number of weights so far"},
  {"weighThreadCheck", py_leverThread_weighThreadCheck, METH_O, "returns number of weights so far, but does not stop thread"},
  {"getDataPin", py_leverThread_getDataPin, METH_O, "returns data pin used with the leverThread"},
  {"getClockPin", py_leverThread_getClockPin, METH_O, "returns clock pin used with the leverThread"},
  {"getTareValue", py_leverThread_getTareValue, METH_O, "returns current tare value used with leverThread"},
  {"getScaling", py_leverThread_getScaling, METH_O, "returns grams per ADC unit used with leverThread"},
  {"setScaling", py_leverThread_setScaling, METH_VARARGS, "sets grams per ADC unit for leverThread"},
  {"turnOn", py_leverThread_turnON, METH_O, "wakes leverThread from low power state"},
  {"turnOff", py_leverThread_turnOFF, METH_O, "sets leverThread to low power state"},
  */
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
