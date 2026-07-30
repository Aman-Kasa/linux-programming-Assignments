# sensor_analysis — C extension for fast sensor stats

A small Python C-extension providing high-performance summary statistics for numeric sequences from IoT sensors.

## Provided functions
- `average(data)` — arithmetic mean (requires N ≥ 1)
- `range_value(data)` — max − min (requires N ≥ 1)
- `variance(data)` — unbiased sample variance (requires N ≥ 2)
- `count_above(data, limit)` — count values > limit
- `statistics(data)` — dict: {"samples", "average", "minimum", "maximum"}

All numeric work is done in C (double precision). Inputs are validated and errors raise `TypeError`/`ValueError`.

## Build / Install
Ensure Python dev headers and a C compiler are installed, then from project root:

```bash
python3 setup.py build_ext --inplace
python3 setup.py install
```

## Quick usage
```python
import sensor_analysis as sa

data = [12.3, 13.0, 11.8, 12.7]
print(sa.average(data))
print(sa.statistics(data))
```

## Tests
Add unit tests comparing results to Python's `statistics` or NumPy; include edge cases (empty, single, NaN/Inf).
