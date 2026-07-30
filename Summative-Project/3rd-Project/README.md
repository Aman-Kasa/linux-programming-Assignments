# Technical Report: Python C Extension for High-Performance Sensor Data Analysis

**Module:** `sensor_analysis`  
**Author:** Your Name  
**Date:** July 2026

---

## 1. Introduction

A smart agriculture platform collects large volumes of environmental sensor readings (soil moisture, temperature, humidity) from IoT devices. Pure Python processing can become a bottleneck at scale. This project implements a native C extension module named `sensor_analysis` that performs essential statistical computations directly in C and exposes them as standard Python callables.

The module provides five functions:
- `average`
- `range_value`
- `variance`
- `count_above`
- `statistics`

All numerical work is executed in C using double-precision floating-point arithmetic, with input validation, exception handling, and memory-safe design.

---

## 2. Design Overview

### 2.1 Python C API Usage

The module uses the Python C API (`Python.h`) to:
- Parse input arguments with `PyArg_ParseTuple`.
- Treat input sequences (list/tuple) via the fast sequence protocol (`PySequence_Fast`).
- Convert sequence elements to C `double` using `PyNumber_Float` and `PyFloat_AsDouble`.
- Construct return values with `PyFloat_FromDouble`, `PyLong_FromLong`, `PyDict_New`, and `PyDict_SetItemString`.
- Raise exceptions (`PyExc_TypeError`, `PyExc_ValueError`) on invalid input.
- Manage reference counts with `Py_INCREF` and `Py_DECREF` to avoid memory leaks.

### 2.2 No Unnecessary Dynamic Memory Allocation

The module does not call `malloc`, `calloc`, or other heap allocation functions for data storage. Instead, it traverses the input sequence directly through `PySequence_Fast_GET_ITEM`, extracting one number at a time. Statistics are accumulated in local C variables such as `sum`, `min`, and `max`.

This design:
- Eliminates allocation/deallocation overhead.
- Avoids memory leaks.
- Keeps auxiliary space complexity at **O(1)**.

### 2.3 Reference Counting and Object Lifecycle

Temporary Python objects created during numeric conversion are immediately decremented after their values are extracted. Returned objects are created with the correct reference count, ensuring proper ownership and garbage collection.

---

## 3. Function Descriptions

### 3.1 `average(data)`

**Purpose:** Compute the arithmetic mean.

**Mathematical formula:**  
\[
\bar{x} = \frac{1}{N} \sum_{i=0}^{N-1} x_i
\]

**Time Complexity:** O(N) — single pass over the input sequence.  
**Space Complexity:** O(1).

**Implementation notes:**
- Converts input to a fast sequence and checks that its length is greater than 0.
- Iterates with a helper that validates every element and extracts its `double` value.
- Accumulates the sum in a local `double`, then returns `PyFloat_FromDouble(sum / len)`.

### 3.2 `range_value(data)`

**Purpose:** Compute the difference between the maximum and minimum readings.

**Mathematical formula:**  
\[
\text{Range} = x_{\max} - x_{\min}
\]

**Time Complexity:** O(N).  
**Space Complexity:** O(1).

**Implementation notes:**
- Initializes `min_val` to `DBL_MAX` and `max_val` to `-DBL_MAX`.
- Performs a single-pass update of the extremes.
- Returns the result as a Python float.

### 3.3 `variance(data)`

**Purpose:** Compute the sample variance using the unbiased estimator.

**Mathematical formula:**  
\[
s^2 = \frac{1}{N - 1} \sum_{i=0}^{N-1} (x_i - \bar{x})^2
\]

**Time Complexity:** O(N) — two passes.  
**Space Complexity:** O(1).

**Numerical accuracy:**
- Uses a two-pass algorithm: first compute the mean, then compute the sum of squared deviations.
- This reduces catastrophic cancellation compared to a naive one-pass formula.
- Requires at least 2 data points.

### 3.4 `count_above(data, limit)`

**Purpose:** Count readings strictly greater than a given threshold.

**Mathematical formula:**  
\[
C = \sum_{i=0}^{N-1} 1_{x_i > limit}
\]

**Time Complexity:** O(N).  
**Space Complexity:** O(1).

**Implementation notes:**
- Accepts a second argument `limit` parsed as a Python float.
- Returns an integer count.

### 3.5 `statistics(data)`

**Purpose:** Return a dictionary with common summary statistics.

**Output format:**
```python
{
    "samples": N,
    "average": mean,
    "minimum": min_val,
    "maximum": max_val
}
```

**Time Complexity:** O(N).  
**Space Complexity:** O(1), aside from the resulting Python dictionary.

---

## 4. Input Validation and Errors

- All functions verify that the first argument is a sequence and raise `TypeError` otherwise.
- Each element must be numeric or convertible to a number; otherwise `TypeError` is raised.
- Functions requiring a minimum number of samples raise `ValueError` with a meaningful message.

---

## 5. Building and Installing

Example build using `setuptools`:

1. Ensure a compatible Python development environment is installed, including headers and a C compiler.
2. From the project root, run:
   ```bash
   python3 setup.py build_ext --inplace
   ```
3. To install the module globally or into the active environment:
   ```bash
   python3 setup.py install
   ```

If the repository uses a different build system such as CMake, Meson, or `pyproject.toml`, follow the project-specific instructions.

---

## 6. Usage Example

Once built and importable:

```python
import sensor_analysis as sa

data = [12.3, 13.0, 11.8, 12.7]
print("Average:", sa.average(data))
print("Range:", sa.range_value(data))
print("Variance:", sa.variance(data))
print("Count above 12.5:", sa.count_above(data, 12.5))
print("Summary:", sa.statistics(data))
```

---

## 7. Tests and Validation

- Add unit tests that compare the extension output against Python’s `statistics` module or NumPy for randomized inputs.
- Include edge cases such as empty sequences, single-element sequences, large values, and NaNs/Infs if supported.
- Measure performance with realistic dataset sizes to verify the expected speedup over pure Python implementations.

---

## 8. License

Add your preferred license here (for example, MIT or Apache-2.0), or leave this section as the project default.
