#include <Python.h>
#include "lever_thread.h"

/* *** Makes and returns a new lever thread object. 
*/
uint8_t * positionData, unsigned int nPositionData, unsigned int nCircular, int goalCuerPin, float cuerFreq

	
static PyObject* ptLever_New (PyObject *self, PyObject *args) {
	PyObject * bufferObj;
	unsigned int nCircular;
	int goalCuerPin;
	float cuerFreq
	if (!PyArg_ParseTuple(args,"OIIif", &bufferObj, &nCircular, &goalCuerPin, &cuerFreq)) {
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
	if (strcmp (buffer.format, "b") != 0){
		PyErr_SetString (PyExc_RuntimeError, "Error for bufferObj: data type of Python array is not unsigned byte");
		return NULL;
	}
	// make a lever_thread object
	lever_thread * threadObj = lever_thread::lever_thread_threadMaker ()
	
	SimpleGPIO_thread *  threadObj= SimpleGPIO_thread::SimpleGPIO_threadMaker (pin, 1, (unsigned int) round (1e06 *delay), (unsigned int) round (1e06 *duration), (unsigned int) nPulses, accLevel);
	if (threadObj == nullptr){
		PyErr_SetString (PyExc_RuntimeError, "SimpleGPIO_threadMaker was not able to make a GPIO thread object");
		return NULL;
	}else{
		return PyCapsule_New (static_cast <void *>(threadObj), "pulsedThread", pulsedThread_del);
	}
  }