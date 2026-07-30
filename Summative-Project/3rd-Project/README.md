# Technical Report: Python C Extension for High-Performance Sensor Data Analysis

**Module:** `sensor_analysis`  
**Author:** [Your Name]  
**Date:** July 2026  

---

## 1. Introduction

A smart agriculture platform collects large volumes of environmental sensor readings (soil moisture, temperature, humidity) from IoT devices. Pure Python processing becomes a bottleneck at scale. This project implements a native C extension module named `sensor_analysis` that performs essential statistical computations directly in C, exposing them as standard Python callables. The module provides five functions: `average`, `range_value`, `variance`, `count_above`, and `statistics`. All numerical work is executed in C using double-precision floating-point arithmetic, with thorough input validation, exception handling, and memory-safe design.

---

## 2. Design Overview

### 2.1 Python C API Usage

The module uses the Python C API (`Python.h`) to:
- Parse input arguments with `PyArg_ParseTuple`.
- Treat input sequences (list/tuple) via the fast sequence protocol (`PySequence_Fast`).
- Convert sequence elements to C `double` using `PyNumber_Float` and `PyFloat_AsDouble`.
- Construct return values: `PyFloat_FromDouble`, `PyLong_FromLong`, `PyDict_New`, and `PyDict_SetItemString`.
- Raise exceptions (`PyExc_TypeError`, `PyExc_ValueError`) on invalid input.
- Manage reference counts (`Py_INCREF`/`Py_DECREF`) to avoid memory leaks.

### 2.2 No Unnecessary Dynamic Memory Allocation

The module **never** calls `malloc`, `calloc`, or any other heap allocation for data storage. Instead, it traverses the input sequence directly through `PySequence_Fast_GET_ITEM`, extracting one number at a time. Statistics are accumulated in local C variables (scalars like `sum`, `min`, `max`). This design:
- Eliminates allocation/deallocation overhead.
- Avoids memory leaks entirely.
- Keeps space complexity at **O(1)** auxiliary memory per function.

### 2.3 Reference Counting and Object Lifecycle

The only Python objects created temporarily are those returned by `PyNumber_Float` during element conversion. Each such object is immediately `Py_DECREF`‑ed after its value is extracted. Output objects (floats, integers, dictionaries) are returned to the caller with the correct reference count, ensuring no leaks and proper garbage collection.

---

## 3. Function Descriptions

### 3.1 `average(data)`

**Purpose:** Compute the arithmetic mean.

**Mathematical formula:**  
\[
\bar{x} = \frac{1}{N} \sum_{i=0}^{N-1} x_i
\]

**Time Complexity:** O(N) – single pass over the input sequence.  
**Space Complexity:** O(1).

**Implementation notes:**
- Converts input to a fast sequence and checks length > 0 (raises `ValueError` otherwise).
- Iterates with `get_double_item()`, which validates every element and extracts its double value.
- Accumulates sum in a local `double`, then returns `PyFloat_FromDouble(sum / len)`.

### 3.2 `range_value(data)`

**Purpose:** Difference between maximum and minimum readings.

**Mathematical formula:**  
\[
\text{Range} = x_{\max} - x_{\min}
\]

**Time Complexity:** O(N).  
**Space Complexity:** O(1).

**Implementation notes:**
- Initialises `min_val` to `DBL_MAX` and `max_val` to `-DBL_MAX` from `<float.h>`, ensuring any valid reading updates the extremes.
- Updates min and max in a single pass.
- Returns `PyFloat_FromDouble(max_val - min_val)`.

### 3.3 `variance(data)`

**Purpose:** Sample variance (unbiased estimator using N−1 degrees of freedom).

**Mathematical formula:**  
\[
s^2 = \frac{1}{N-1} \sum_{i=0}^{N-1} (x_i - \bar{x})^2
\]

**Time Complexity:** O(N) – two passes (mean first, then squared differences).  
**Space Complexity:** O(1).

**Numerical accuracy considerations:**
- Uses the **two‑pass algorithm** rather than the naive single‑pass \(\sum x^2 - (\sum x)^2/N\) formula, which is prone to catastrophic cancellation when numbers are large and nearly equal.
- Requires at least 2 data points; otherwise raises `ValueError`.

### 3.4 `count_above(data, limit)`

**Purpose:** Number of readings strictly greater than a given threshold.

**Mathematical formula:**  
\[
C = \sum_{i=0}^{N-1} \mathbb{I}(x_i > \text{limit})
\]

**Time Complexity:** O(N).  
**Space Complexity:** O(1).

**Implementation notes:**
- Accepts a second argument, `limit`, parsed as a Python double.
- Increments a `long` counter for each element satisfying `val > limit`.
- Returns `PyLong_FromLong(count)`.

### 3.5 `statistics(data)`

**Purpose:** Returns a Python dictionary with sample count, mean, minimum, and maximum.

**Output format:**  
```python
{
    "samples": N,
    "average": mean,
    "minimum": min_val,
    "maximum": max_val
}
