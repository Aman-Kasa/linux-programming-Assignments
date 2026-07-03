#define PY_SSIZE_T_CLEAN
#include <Python.h>

// Accelerated execution calculation function
static PyObject* method_compute_sum(PyObject* self, PyObject* args) {
    long long size;
    if (!PyArg_ParseTuple(args, "L", &size)) {
        return NULL;
    }

    long long total = 0;
    for (long long i = 0; i < size; i++) {
        total += i * 2;
    }

    return PyLong_FromLongLong(total);
}

// Module structural method layout configuration map
static PyMethodDef ExtMethods[] = {
    {"compute_sum", method_compute_sum, METH_VARARGS, "Compute heavy numerical sum loop natively"},
    {NULL, NULL, 0, NULL}
};

// Module structural definition mapping block
static struct PyModuleDef my_ext_module = {
    PyModuleDef_HEAD_INIT,
    "my_ext",
    "CPython optimization metrics extension module.",
    -1,
    ExtMethods
};

// Initialization routine hook triggered upon explicit python import
PyMODINIT_FUNC PyInit_my_ext(void) {
    return PyModule_Create(&my_ext_module);
}
