# sensor_analysis – Python C Extension for Sensor Statistics

## Build Instructions
1. Ensure Python 3 and GCC are installed.
2. Run:
   python setup.py build_ext --inplace
   This creates a shared object (`sensor_analysis*.so`) in the current directory.

## Execution
Run the test script:
   python test_sensor_analysis.py

## Expected Output
The test suite prints computed statistics, edge‑case error messages, and a performance comparison.
Example:
   average(): 24.0286
   range_value(): 8.5000
   variance(): 9.1157
   count_above(23): 3
   statistics(): {'samples': 7, 'average': 24.0286, 'minimum': 19.8, 'maximum': 28.3}
   ...
   ALL TESTS PASSED SUCCESSFULLY!

## Inputs
All functions accept a Python list or tuple of numeric values (int or float).

## Notes
- Sample variance uses N-1 degrees of freedom.
- All calculations are performed in C for high performance.
- No additional heap memory is allocated during processing.
