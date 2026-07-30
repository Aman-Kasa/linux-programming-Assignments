# Technical Report: Python C Extension for High-Performance Sensor Data Analysis

**Module:** `sensor_analysis`  
**Author:** Your Name  
**Date:** July 2026

---

## 1. Introduction

A smart agriculture platform collects large volumes of environmental sensor readings (soil moisture, temperature, humidity) from IoT devices. Pure Python processing can become a bottleneck at scale. This project implements a small Python C extension module, `sensor_analysis`, that provides several high-performance statistical routines for numeric sequences while minimizing memory allocations and Python/C overhead.

The goals are:
- Reduce per-element Python overhead for tight numeric loops.
- Keep auxiliary memory usage minimal (O(1) extra space).
- Preserve numerical accuracy where important.
- Expose a simple, Pythonic API.

---

## 2. Design overview

### 2.1 Python C API usage

The extension uses the Python C API (`Python.h`) with the following patterns:
- Parse arguments with `PyArg_ParseTuple`.
- Use the fast sequence protocol (`PySequence_Fast`) to treat input lists/tuples as contiguous sequences.
- Convert elements to `double` via `PyNumber_Float` and `PyFloat_AsDouble`.
- Construct return values using `PyFloat_FromDouble`, `PyLong_FromLong`, and Python dictionary helpers (`PyDict_New`, `PyDict_SetItemString`).
- Raise Python exceptions (`PyExc_TypeError`, `PyExc_ValueError`) for invalid inputs.
- Manage reference counts carefully (`Py_INCREF` / `Py_DECREF`) to avoid leaks.

### 2.2 No unnecessary dynamic memory allocation

The implementation avoids heap allocation for the data processing itself (no `malloc`/`calloc` for element buffers). It traverses the input sequence directly (via `PySequence_Fast_GET_ITEM`) and converts each element on-the-fly. Advantages:
- Eliminates allocation/deallocation overhead.
- Avoids accumulation of memory leaks.
- Keeps extra space complexity at O(1).

### 2.3 Reference counting and object lifecycle

Temporary Python objects (for example, the result of `PyNumber_Float`) are immediately decref'd after extracting their C `double` value. Returned Python objects are created and returned with the appropriate reference ownership. All API entry points validate inputs and raise informative Python exceptions on error.

---

## 3. Functions and behavior

All functions accept a Python sequence (list/tuple/other sequence) of numeric values. For non-sequence or non-numeric inputs the functions raise `TypeError`. For insufficient-length inputs (when a minimum sample size is required) the functions raise `ValueError`.

### 3.1 average(data)

Purpose: Compute the arithmetic mean.

Mathematical formula:
x̄ = (1/N) ∑_{i=0}^{N-1} x_i

Time complexity: O(N) — single pass.  
Space complexity: O(1).

Notes:
- Requires N >= 1 (raises `ValueError` if empty).
- Uses a local `double` accumulator.

### 3.2 range_value(data)

Purpose: Difference between maximum and minimum readings.

Mathematical formula:
Range = x_max - x_min

Time complexity: O(N).  
Space complexity: O(1).

Notes:
- Initializes `min_val` to `DBL_MAX` and `max_val` to `-DBL_MAX` (from `<float.h>`).
- Single-pass update of extremes; returns a float.

### 3.3 variance(data)

Purpose: Sample variance (unbiased estimator using N−1 degrees of freedom).

Mathematical formula:
s^2 = (1 / (N - 1)) ∑_{i=0}^{N-1} (x_i - x̄)^2

Time complexity: O(N) — two passes (one for mean, one for squared differences).  
Space complexity: O(1).

Numerical accuracy:
- Uses a two-pass algorithm (compute mean, then sum squared deviations) to reduce catastrophic cancellation versus the naive single-pass formula.
- Requires at least 2 data points (raises `ValueError` otherwise).

### 3.4 count_above(data, limit)

Purpose: Count of readings strictly greater than a given threshold.

Mathematical formula:
C = ∑_{i=0}^{N-1} 1_{x_i > limit}

Time complexity: O(N).  
Space complexity: O(1).

Notes:
- Accepts a second argument `limit` parsed as a Python float.
- Returns an integer (`PyLong`) count.

### 3.5 statistics(data)

Purpose: Convenience function returning a dictionary with common summary statistics.

Output format:
{
    "samples": N,
    "average": mean,
    "minimum": min_val,
    "maximum": max_val
}

Time complexity: O(N).  
Space complexity: O(1) (aside from the resulting Python dict).

---

## 4. Input validation and errors

- All functions check that the first argument is a sequence and raise `TypeError` otherwise.
- Numeric conversion is attempted for each element; non-convertible elements raise `TypeError`.
- Functions that require a minimum number of samples raise `ValueError` with a meaningful message.

---

## 5. Building and installing

Typical build using setuptools (example `setup.py` approach):

1. Ensure you have a compatible Python development environment (headers and a C compiler).
2. From the project root run:
   ```bash
   python3 setup.py build_ext --inplace
   ```
   or to install:
   ```bash
   python3 setup.py install
   ```

If this repository uses a different build system (CMake, meson, or pyproject.toml), follow the project-specific instructions.

---

## 6. Usage example

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

## 7. Tests and validation

- Add small unit tests that compare the C extension output against Python's `statistics` or NumPy for randomized inputs, including edge cases (empty sequence, single-element, large values, NaNs/infs if you intend to support them).
- Measure performance with realistic dataset sizes to verify the expected speedup over pure Python implementations.

---

## 8. License

Add your preferred license here (e.g., MIT, Apache-2.0) or leave as project default.
