# Project 3: Building a Python Performance Extension

## 📝 Objective
Demonstrate the performance limitations of the CPython bytecode interpreter and resolve them by offloading heavy computational math loops to a natively compiled C extension using the CPython API.

## 📂 Files Included
* `compute.py`: The Python benchmarking script.
* `my_ext.c`: The native C code implementing the high-speed loop.
* `setup.py`: The setuptools configuration for building the shared object library.
* `README.md`: Project documentation.

## 🚀 Build & Benchmark Instructions
To build the native C extension in-place, ensure you have Python 3 development headers installed, then run:
```bash
python3 setup.py build_ext --inplace
(This will generate a .so shared object file in the directory).

Run the performance benchmark to compare pure Python execution against the native C extension:

Bash
python3 compute.py
-------------------------------------------------------------------------------------------------
Expected Outcome:                                                                               |
The terminal will display the execution times, typically showing the C extension performing over|
~190x faster than the pure Python implementation due to                                         |
the elimination of dynamic type checking and interpreter overhead.                              |
-------------------------------------------------------------------------------------------------
