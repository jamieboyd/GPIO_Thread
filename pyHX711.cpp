#include <Python.h>
#include "HX711.h"


void  py_HX711_del(PyObject * PyPtr){
    delete static_cast<HX711 *> (PyCapsule_GetPointer (PyPtr, "HX711"));
}

static PyObject* py_HX711_new (PyObject *self, PyObject *args) {
	int dp;
	int cp;
	float sp;
	if (!PyArg_ParseTuple(args,"iif", &dp, &cp, &sp)) {
		return NULL;
	}
	int errCode;
	HX711* HX711ptr = new HX711(dp, cp, sp, errCode);
	if (errCode ){
		PyRun_SimpleString ("print ('Making new HX711object failed to map the physical GPIO registers onto the virtual memory space.')");
		return NULL;
	}
	return PyCapsule_New(static_cast <void *>( HX711ptr), "HX711", py_HX711_del);
  }

  static PyObject* py_HX711_tare (PyObject *self, PyObject *args) {
	PyObject *PyPtr;
	int nTares;
	if (!PyArg_ParseTuple(args, "Oi", &PyPtr, &nTares)) {
		return NULL;
	}
	HX711 * HX711Ptr = static_cast<HX711 * > (PyCapsule_GetPointer(PyPtr, "HX711"));
	HX711Ptr->tare (nTares, false);
	Py_RETURN_NONE;
}

static PyObject* py_HX711_weigh (PyObject *self, PyObject *args) {
    PyObject *PyPtr;
    int nAvg;
    if (!PyArg_ParseTuple(args,"Oi", &PyPtr, &nAvg)) {
        return NULL;
    }
    HX711 * HX711Ptr = static_cast<HX711 * > (PyCapsule_GetPointer(PyPtr, "HX711"));
    return Py_BuildValue("f", HX711Ptr->weigh (nAvg, false));
}

static PyObject* py_HX711_readValue (PyObject *self, PyObject *PyPtr) {
    HX711 * HX711Ptr = static_cast<HX711 * > (PyCapsule_GetPointer(PyPtr, "HX711"));
    return Py_BuildValue("i", HX711Ptr->readValue());
}


static PyObject* py_HX711_weighThreadStart (PyObject *self, PyObject *args) {
    PyObject * PyPtr;
    PyObject * bufferObj;
    int arraySize;
	
    if (!PyArg_ParseTuple(args,"OOi", &PyPtr, &bufferObj, &arraySize)) {
        return NULL;
    }
    if (PyObject_CheckBuffer (bufferObj) == 0){
		printf ("You only thought this was a buffer object.\n");
		return NULL;
	}
	Py_buffer buffer;
	if (PyObject_GetBuffer (bufferObj, &buffer, PyBUF_WRITABLE)==-1){
		printf ("PyObject_GetBuffer failed.\n");
		return NULL;
	}
	float* arrayStart = static_cast <float *>(buffer.buf);
	HX711 * HX711Ptr = static_cast<HX711 * > (PyCapsule_GetPointer(PyPtr, "HX711"));
	HX711Ptr->weighThreadStart (arrayStart, arraySize);
	PyBuffer_Release (&buffer);
	Py_RETURN_NONE;
}

static PyObject* py_HX711_weighThreadStop  (PyObject *self, PyObject *PyPtr) {
    HX711 * HX711Ptr = static_cast<HX711 * > (PyCapsule_GetPointer(PyPtr, "HX711"));
    return Py_BuildValue("i", HX711Ptr->weighThreadStop());
}

static PyObject* py_HX711_weighThreadCheck  (PyObject *self, PyObject *PyPtr) {
    HX711 * HX711Ptr = static_cast<HX711 * > (PyCapsule_GetPointer(PyPtr, "HX711"));
    return Py_BuildValue("i", HX711Ptr->weighThreadCheck());
}

static PyObject* py_HX711_getDataPin (PyObject *self, PyObject *PyPtr) {
    HX711 * HX711Ptr = static_cast<HX711 * > (PyCapsule_GetPointer(PyPtr, "HX711"));
    return Py_BuildValue("i", HX711Ptr->weighThreadStop());
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
        return NULL;
    }
    HX711 * HX711Ptr = static_cast<HX711 * > (PyCapsule_GetPointer(PyPtr, "HX711"));
    HX711Ptr->setScaling (newScaling);
    Py_RETURN_NONE;
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
  {"readIntValue", py_HX711_readValue, METH_O, "returns a single unconverted integer value from the scale"},
  {"getScaling", py_HX711_getScaling, METH_O, "returns grams per ADC unit used with HX711"},
  {"setScaling", py_HX711_setScaling, METH_VARARGS, "sets grams per ADC unit for HX711"},
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
