#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include <float.h>
#include <math.h>

/* =========================================================================
 * HELPER FUNCTIONS & CONVERSION LOGIC
 * ========================================================================= */

/**
 * Extracts a numeric element from a fast sequence and converts it to a double.
 *
 * Object Conversion Mechanism:
 * - Uses PySequence_Fast_GET_ITEM to borrow an object reference at `index`.
 * - Calls PyNumber_Float() to attempt converting integers/floats into PyFloat.
 * - Calls PyFloat_AsDouble() to extract the native IEEE 754 64-bit double value.
 *
 * Memory & Safety Management:
 * - PyNumber_Float creates a NEW reference. We must call Py_DECREF() before
 *   returning to prevent memory leaks.
 * - If conversion fails (e.g., element is a string or invalid type), sets a 
 *   PyExc_TypeError and returns 0.
 */
static int get_double_item(PyObject *fast_seq, Py_ssize_t index, double *out_val) {
    PyObject *item = PySequence_Fast_GET_ITEM(fast_seq, index); // Borrowed ref
    if (!item) {
        PyErr_SetString(PyExc_TypeError, "Unable to access sequence item.");
        return 0;
    }

    PyObject *py_float = PyNumber_Float(item); // New ref
    if (!py_float) {
        PyErr_SetString(PyExc_TypeError, "All elements in dataset must be numeric (int or float).");
        return 0;
    }

    *out_val = PyFloat_AsDouble(py_float);
    Py_DECREF(py_float); // Clean up new reference

    if (PyErr_Occurred()) {
        return 0;
    }
    return 1;
}

/* =========================================================================
 * MODULE METHOD IMPLEMENTATIONS
 * ========================================================================= */

/**
 * average(data)
 * Calculates the arithmetic mean of sensor readings.
 *
 * Mathematical Formula:
 *   \bar{x} = \frac{1}{N} \sum_{i=0}^{N-1} x_i
 *
 * Time Complexity: O(N) where N is sequence length.
 * Space Complexity: O(1) auxiliary memory.
 * Zero-Allocation Rationale: Iterates directly through sequence items via
 * index pointers without malloc()-ing C arrays.
 */
static PyObject* sensor_analysis_average(PyObject *self, PyObject *args) {
    PyObject *data;
    if (!PyArg_ParseTuple(args, "O", &data)) {
        return NULL;
    }

    PyObject *fast_seq = PySequence_Fast(data, "Argument must be a list or tuple.");
    if (!fast_seq) {
        return NULL; // TypeError raised automatically by PySequence_Fast
    }

    Py_ssize_t len = PySequence_Fast_GET_SIZE(fast_seq);
    if (len == 0) {
        Py_DECREF(fast_seq);
        PyErr_SetString(PyExc_ValueError, "Cannot calculate average of an empty dataset.");
        return NULL;
    }

    double sum = 0.0;
    for (Py_ssize_t i = 0; i < len; i++) {
        double val;
        if (!get_double_item(fast_seq, i, &val)) {
            Py_DECREF(fast_seq);
            return NULL;
        }
        sum += val;
    }

    Py_DECREF(fast_seq);
    return PyFloat_FromDouble(sum / (double)len);
}

/**
 * range_value(data)
 * Returns the difference between maximum and minimum sensor readings.
 *
 * Mathematical Formula:
 *   \text{Range} = x_{\max} - x_{\min}
 *
 * Time Complexity: O(N)
 * Space Complexity: O(1)
 * Numerical Accuracy Consideration: Initialized with DBL_MAX and -DBL_MAX from
 * <float.h> to ensure proper boundary tracking across extreme datasets.
 */
static PyObject* sensor_analysis_range_value(PyObject *self, PyObject *args) {
    PyObject *data;
    if (!PyArg_ParseTuple(args, "O", &data)) {
        return NULL;
    }

    PyObject *fast_seq = PySequence_Fast(data, "Argument must be a list or tuple.");
    if (!fast_seq) {
        return NULL;
    }

    Py_ssize_t len = PySequence_Fast_GET_SIZE(fast_seq);
    if (len == 0) {
        Py_DECREF(fast_seq);
        PyErr_SetString(PyExc_ValueError, "Cannot calculate range of an empty dataset.");
        return NULL;
    }

    double min_val = DBL_MAX;
    double max_val = -DBL_MAX;

    for (Py_ssize_t i = 0; i < len; i++) {
        double val;
        if (!get_double_item(fast_seq, i, &val)) {
            Py_DECREF(fast_seq);
            return NULL;
        }
        if (val < min_val) min_val = val;
        if (val > max_val) max_val = val;
    }

    Py_DECREF(fast_seq);
    return PyFloat_FromDouble(max_val - min_val);
}

/**
 * variance(data)
 * Calculates the sample variance (N - 1 degrees of freedom).
 *
 * Mathematical Formula:
 *   s^2 = \frac{1}{N - 1} \sum_{i=0}^{N-1} (x_i - \bar{x})^2
 *
 * Time Complexity: O(N) (Two-pass algorithm)
 * Space Complexity: O(1)
 * Numerical Stability: Uses a two-pass algorithm to compute mean first, then 
 * accumulate squared differences. This prevents catastrophic cancellation that 
 * occurs in naive single-pass \sum x^2 - (\sum x)^2 / N formulas.
 */
static PyObject* sensor_analysis_variance(PyObject *self, PyObject *args) {
    PyObject *data;
    if (!PyArg_ParseTuple(args, "O", &data)) {
        return NULL;
    }

    PyObject *fast_seq = PySequence_Fast(data, "Argument must be a list or tuple.");
    if (!fast_seq) {
        return NULL;
    }

    Py_ssize_t len = PySequence_Fast_GET_SIZE(fast_seq);
    if (len < 2) {
        Py_DECREF(fast_seq);
        PyErr_SetString(PyExc_ValueError, "Sample variance requires at least 2 data points.");
        return NULL;
    }

    // Pass 1: Mean calculation
    double sum = 0.0;
    for (Py_ssize_t i = 0; i < len; i++) {
        double val;
        if (!get_double_item(fast_seq, i, &val)) {
            Py_DECREF(fast_seq);
            return NULL;
        }
        sum += val;
    }
    double mean = sum / (double)len;

    // Pass 2: Squared differences sum
    double sq_diff_sum = 0.0;
    for (Py_ssize_t i = 0; i < len; i++) {
        double val;
        if (!get_double_item(fast_seq, i, &val)) {
            Py_DECREF(fast_seq);
            return NULL;
        }
        double diff = val - mean;
        sq_diff_sum += diff * diff;
    }

    Py_DECREF(fast_seq);
    return PyFloat_FromDouble(sq_diff_sum / (double)(len - 1));
}

/**
 * count_above(data, limit)
 * Counts the number of readings strictly greater than specified limit threshold.
 *
 * Mathematical Formula:
 *   C = \sum_{i=0}^{N-1} \mathbb{I}(x_i > \text{limit})
 *
 * Time Complexity: O(N)
 * Space Complexity: O(1)
 */
static PyObject* sensor_analysis_count_above(PyObject *self, PyObject *args) {
    PyObject *data;
    double limit;
    if (!PyArg_ParseTuple(args, "Od", &data, &limit)) {
        return NULL;
    }

    PyObject *fast_seq = PySequence_Fast(data, "First argument must be a list or tuple.");
    if (!fast_seq) {
        return NULL;
    }

    Py_ssize_t len = PySequence_Fast_GET_SIZE(fast_seq);
    long count = 0;

    for (Py_ssize_t i = 0; i < len; i++) {
        double val;
        if (!get_double_item(fast_seq, i, &val)) {
            Py_DECREF(fast_seq);
            return NULL;
        }
        if (val > limit) {
            count++;
        }
    }

    Py_DECREF(fast_seq);
    return PyLong_FromLong(count);
}

/**
 * statistics(data)
 * Returns a Python dictionary with dataset metrics:
 * { "samples": N, "average": mean, "minimum": min, "maximum": max }
 *
 * Time Complexity: O(N) computed in a single pass.
 * Space Complexity: O(1) auxiliary C memory.
 */
static PyObject* sensor_analysis_statistics(PyObject *self, PyObject *args) {
    PyObject *data;
    if (!PyArg_ParseTuple(args, "O", &data)) {
        return NULL;
    }

    PyObject *fast_seq = PySequence_Fast(data, "Argument must be a list or tuple.");
    if (!fast_seq) {
        return NULL;
    }

    Py_ssize_t len = PySequence_Fast_GET_SIZE(fast_seq);
    if (len == 0) {
        Py_DECREF(fast_seq);
        PyErr_SetString(PyExc_ValueError, "Cannot calculate statistics on an empty dataset.");
        return NULL;
    }

    double sum = 0.0;
    double min_val = DBL_MAX;
    double max_val = -DBL_MAX;

    for (Py_ssize_t i = 0; i < len; i++) {
        double val;
        if (!get_double_item(fast_seq, i, &val)) {
            Py_DECREF(fast_seq);
            return NULL;
        }
        sum += val;
        if (val < min_val) min_val = val;
        if (val > max_val) max_val = val;
    }

    Py_DECREF(fast_seq);

    double mean = sum / (double)len;

    // Dictionary Construction
    PyObject *dict = PyDict_New();
    if (!dict) {
        return NULL;
    }

    PyObject *py_samples = PyLong_FromSsize_t(len);
    PyObject *py_avg = PyFloat_FromDouble(mean);
    PyObject *py_min = PyFloat_FromDouble(min_val);
    PyObject *py_max = PyFloat_FromDouble(max_val);

    if (!py_samples || !py_avg || !py_min || !py_max) {
        Py_XDECREF(py_samples);
        Py_XDECREF(py_avg);
        Py_XDECREF(py_min);
        Py_XDECREF(py_max);
        Py_DECREF(dict);
        return NULL;
    }

    // PyDict_SetItemString steals no references, so we DECREF temporary primitives
    PyDict_SetItemString(dict, "samples", py_samples);
    PyDict_SetItemString(dict, "average", py_avg);
    PyDict_SetItemString(dict, "minimum", py_min);
    PyDict_SetItemString(dict, "maximum", py_max);

    Py_DECREF(py_samples);
    Py_DECREF(py_avg);
    Py_DECREF(py_min);
    Py_DECREF(py_max);

    return dict;
}

/* =========================================================================
 * MODULE INITIALIZATION DEFINITIONS
 * ========================================================================= */

static PyMethodDef SensorAnalysisMethods[] = {
    {"average",     sensor_analysis_average,     METH_VARARGS, "Calculate arithmetic mean of sensor readings."},
    {"range_value", sensor_analysis_range_value, METH_VARARGS, "Calculate difference between max and min readings."},
    {"variance",    sensor_analysis_variance,    METH_VARARGS, "Calculate sample variance of sensor readings."},
    {"count_above", sensor_analysis_count_above, METH_VARARGS, "Count readings strictly greater than a threshold limit."},
    {"statistics",  sensor_analysis_statistics,  METH_VARARGS, "Return dictionary of summary statistics."},
    {NULL, NULL, 0, NULL} /* Sentinel */
};

static struct PyModuleDef sensor_analysis_module = {
    PyModuleDef_HEAD_INIT,
    "sensor_analysis",
    "High-performance C extension for IoT environmental sensor data analytics.",
    -1,
    SensorAnalysisMethods
};

PyMODINIT_FUNC PyInit_sensor_analysis(void) {
    return PyModule_Create(&sensor_analysis_module);
}
