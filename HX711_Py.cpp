#include <Python.h>
#include "HX711.h"

/* ************ Python bindings for HX711 Scale******************************
Last Modifed:

2018/03/15 by Jamie Boyd - updates for pulsedThread subclass
2018/03/05 by Jamie Boyd - updates for pulsedThread class/HX711subclass


******************* Function called automatically when PyCapsule object is deleted in Python *****************************************
Last Modified:
2018/02/20 by Jamie Boyd - initial version */
void  py_HX711_del(PyObject * PyPtr){
	delete static_cast<HX711*> (PyCapsule_GetPointer (PyPtr, "HX711"));
}

/* ******************** Make a new HX711 object and return a pointer to it, wrapped in a PyCapsule *****************************
Last Modified:
2018/03/05 by Jamie Boyd - get array at object init and hold a pointer to it */
static PyObject* py_HX711_new (PyObject *self, PyObject *args) {
	// parse inputs for dataPin, clockPin, grams per A/D unit scaling, and floating point buffer object, 
	int dataPin;
	int clockPin;
	float scaling;
	PyObject * bufferObj;
	if (!PyArg_ParseTuple(args,"iifO", &dataPin, &clockPin, &scaling, &bufferObj)) {
		PyErr_SetString (PyExc_RuntimeError, "Could not parse input for dataPin, clockPin, scaling, and floating point buffer.");
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
	if (strcmp (buffer.format, "f") != 0){
		PyErr_SetString (PyExc_RuntimeError, "Error for bufferObj: data type of Python array is not float");
		return NULL;
	}
	// make a new HX711 object
	HX711* HX711ptr = HX711::HX711_threadMaker (dataPin, clockPin, scaling, static_cast <float *>(buffer.buf), (unsigned int) buffer.len/buffer.itemsize);
	if (HX711ptr == nullptr ){
		PyErr_SetString (PyExc_RuntimeError, "Failed to make HX711object.");
		return NULL;
	}
	PyBuffer_Release (&buffer); // we don't need the buffer object after extracting a pointer to start of buffer
	return PyCapsule_New(static_cast <void *>( HX711ptr), "HX711", py_HX711_del);
  }

  /* ******************** Tares the HX711 by weighing a set number of times *****************************
Last Modified:
2018/03/05 by Jamie Boyd - modified for new pulsedThread subclassed verson */
  static PyObject* py_HX711_tare (PyObject *self, PyObject *args) {
	PyObject *PyPtr;
	unsigned int nTares;
	if (!PyArg_ParseTuple(args, "OI", &PyPtr, &nTares)) {
		PyErr_SetString (PyExc_RuntimeError, "Could not parse input for nTares.");
		return NULL;
	}
	HX711 * HX711Ptr = static_cast<HX711 * > (PyCapsule_GetPointer(PyPtr, "HX711"));
	return Py_BuildValue ("f", HX711Ptr->tare (nTares, false)); // printing is always false; print from Python if you want, you have the array
}

/* ******************** Weighs a set number of times, not returning until done *****************************
Last Modified:
2018/03/05 by Jamie Boyd - modified for new pulsedThread subclassed verson */
static PyObject* py_HX711_weigh (PyObject *self, PyObject *args) {
	PyObject *PyPtr;
	unsigned int nAvg;
	if (!PyArg_ParseTuple(args,"OI", &PyPtr, &nAvg)) {
		PyErr_SetString (PyExc_RuntimeError, "Could not parse input for nAvg.");
		return NULL;
	}
	HX711 * HX711Ptr = static_cast<HX711 * > (PyCapsule_GetPointer(PyPtr, "HX711"));
	return Py_BuildValue("f", HX711Ptr->weigh (nAvg, false));
}

/* ******************** starts thread weighing, returning immediately*****************************
Last Modified:
2018/03/05 by Jamie Boyd - modified for new pulsedThread subclassed verson */
static PyObject* py_HX711_weighThreadStart (PyObject *self, PyObject *args) {
	PyObject * PyPtr;
	unsigned int weighSize;
	
	if (!PyArg_ParseTuple(args,"OI", &PyPtr, &weighSize)) {
		PyErr_SetString (PyExc_RuntimeError, "Could not parse input for number to weigh.");
		return NULL;
	}
	HX711 * HX711Ptr = static_cast<HX711 * > (PyCapsule_GetPointer(PyPtr, "HX711"));
	HX711Ptr->weighThreadStart (weighSize);
	Py_RETURN_NONE;
}

/* ******************** stops thread weighing, returning number of weights alreay taken *****************************
Last Modified:
2018/03/05 by Jamie Boyd - modified for new pulsedThread subclassed verson */
static PyObject* py_HX711_weighThreadStop  (PyObject *self, PyObject *PyPtr) {
	HX711 * HX711Ptr = static_cast<HX711 * > (PyCapsule_GetPointer(PyPtr, "HX711"));
	return Py_BuildValue("I", HX711Ptr->weighThreadStop());
}

/* ******************** returning number of weights alreay taken, does not stop thread *****************************
Last Modified:
2018/03/05 by Jamie Boyd - modified for new pulsedThread subclassed verson */
static PyObject* py_HX711_weighThreadCheck  (PyObject *self, PyObject *PyPtr) {
	HX711 * HX711Ptr = static_cast<HX711 * > (PyCapsule_GetPointer(PyPtr, "HX711"));
	return Py_BuildValue("I", HX711Ptr->weighThreadCheck());
}

static PyObject* py_HX711_getDataPin (PyObject *self, PyObject *PyPtr) {
	HX711 * HX711Ptr = static_cast<HX711 * > (PyCapsule_GetPointer(PyPtr, "HX711"));
	return Py_BuildValue("i", HX711Ptr->getDataPin());
}
 
static PyObject* py_HX711_getClockPin (PyObject *self, PyObject *PyPtr) {
	HX711 * HX711Ptr = static_cast<HX711 * > (PyCapsule_GetPointer(PyPtr, "HX711"));
	return Py_BuildValue("i", HX711Ptr->getClockPin());
}

static PyObject* py_HX711_getScaling (PyObject *self, PyObject *PyPtr) {
	HX711 * HX711Ptr = static_cast<HX711 * > (PyCapsule_GetPointer(PyPtr, "HX711"));
	return Py_BuildValue("f", HX711Ptr->getScaling());
}

static PyObject* py_HX711_getTareValue (PyObject *self, PyObject *PyPtr) {
	HX711 * HX711Ptr = static_cast<HX711 * > (PyCapsule_GetPointer(PyPtr, "HX711"));
	return Py_BuildValue("f", HX711Ptr->getTareValue());
}

static PyObject* py_HX711_setScaling (PyObject *self, PyObject *args) {
	PyObject *PyPtr;
	float newScaling;
	if (!PyArg_ParseTuple(args, "Of", &PyPtr, &newScaling)) {
		PyErr_SetString (PyExc_RuntimeError, "Could not parse input for new scaling.");
		return NULL;
	}
	HX711 * HX711Ptr = static_cast<HX711 * > (PyCapsule_GetPointer(PyPtr, "HX711"));
	HX711Ptr->setScaling (newScaling);
	Py_RETURN_NONE;
}


static PyObject* py_HX711_scalingFromStd (PyObject *self, PyObject *args){
	PyObject *PyPtr;
	float standardGrams;
	unsigned int nAvg;
	if (!PyArg_ParseTuple(args, "OfI", &PyPtr, &standardGrams, &nAvg)) {
		PyErr_SetString (PyExc_RuntimeError, "Could not parse input for new scaling.");
		return NULL;
	}
	HX711 * HX711Ptr = static_cast<HX711 * > (PyCapsule_GetPointer(PyPtr, "HX711"));
	return Py_BuildValue("f",HX711Ptr->scalingFromStd(standardGrams, nAvg));
}


static PyObject* py_HX711_turnON (PyObject *self, PyObject *PyPtr) {
	HX711 * HX711Ptr = static_cast<HX711 * > (PyCapsule_GetPointer(PyPtr, "HX711"));
	HX711Ptr->turnON();
	Py_RETURN_NONE;
}

static PyObject* py_HX711_turnOFF (PyObject *self, PyObject *PyPtr) {
	HX711 * HX711Ptr = static_cast<HX711 * > (PyCapsule_GetPointer(PyPtr, "HX711"));
	HX711Ptr->turnOFF();
	Py_RETURN_NONE;
}

/* Module method table */
static PyMethodDef HX711Methods[] = {
  {"new", py_HX711_new, METH_VARARGS, "Creates a new instance of HX711 load cell amplifier"},
  {"tare", py_HX711_tare, METH_VARARGS, "Tares the HX711 load cell amplifier"},
  {"weigh", py_HX711_weigh, METH_VARARGS, "Weighs with the HX711 load cell amplifier"},
  {"weighThreadStart",py_HX711_weighThreadStart, METH_VARARGS, "Starts the thread weighing into the array"},
  {"weighThreadStop", py_HX711_weighThreadStop, METH_O, "Stops the thread weighing, returns number of weights so far"},
  {"weighThreadCheck", py_HX711_weighThreadCheck, METH_O, "returns number of weights so far, but does not stop thread"},
  {"getDataPin", py_HX711_getDataPin, METH_O, "returns data pin used with the HX711"},
  {"getClockPin", py_HX711_getClockPin, METH_O, "returns clock pin used with the HX711"},
  {"getTareValue", py_HX711_getTareValue, METH_O, "returns current tare value used with HX711"},
  {"getScaling", py_HX711_getScaling, METH_O, "returns grams per ADC unit used with HX711"},
  {"setScaling", py_HX711_setScaling, METH_VARARGS, "sets grams per ADC unit for HX711"},
  {"scalingFromStd", py_HX711_scalingFromStd, METH_VARARGS, "calculates scaling by weighing a standard"},
  {"turnOn", py_HX711_turnON, METH_O, "wakes HX711 from low power state"},
  {"turnOff", py_HX711_turnOFF, METH_O, "sets HX711 to low power state"},
  { NULL, NULL, 0, NULL}
};

/* Module structure */
static struct PyModuleDef HX711module = {
  PyModuleDef_HEAD_INIT,
  "HX711",           /* name of module */
  "Controls an HX711 Load Cell Amplifier",  /* Doc string (may be NULL) */
  -1,                 /* Size of per-interpreter state or -1 */
  HX711Methods       /* Method table */
};

/* Module initialization function */
PyMODINIT_FUNC
PyInit_HX711(void) {
  return PyModule_Create(&HX711module);
}
