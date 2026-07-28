import math
import time
import sensor_analysis


def run_tests():
    print("=" * 60)
    print("      SENSOR_ANALYSIS C EXTENSION TEST SUITE")
    print("=" * 60)

    # 1. Normal Dataset Testing (List and Tuple)
    sample_list = [22.4, 25.1, 19.8, 28.3, 24.0, 21.5, 26.7]
    sample_tuple = (22.4, 25.1, 19.8, 28.3, 24.0, 21.5, 26.7)

    print("\n--- 1. Testing Standard Operations (List input) ---")
    avg = sensor_analysis.average(sample_list)
    rng = sensor_analysis.range_value(sample_list)
    var = sensor_analysis.variance(sample_list)
    cnt = sensor_analysis.count_above(sample_list, 23.0)
    stats = sensor_analysis.statistics(sample_list)

    print(f"Dataset:         {sample_list}")
    print(f"average():       {avg:.4f}")
    print(f"range_value():   {rng:.4f}")
    print(f"variance():      {var:.4f}")
    print(f"count_above(23): {cnt}")
    print(f"statistics():    {stats}")

    # Verify tuple compatibility
    assert math.isclose(avg, sensor_analysis.average(sample_tuple)), "Tuple test failed!"
    print("✓ Tuple conversion verified successfully.")

    # 2. Boundary & Edge Cases
    print("\n--- 2. Testing Boundary & Edge Cases ---")

    # Empty list handling
    try:
        sensor_analysis.average([])
        print("✗ FAIL: Empty list average did not raise ValueError")
    except ValueError as e:
        print(f"✓ Empty list average caught ValueError: {e}")

    # Single item variance handling
    try:
        sensor_analysis.variance([25.0])
        print("✗ FAIL: Single element variance did not raise ValueError")
    except ValueError as e:
        print(f"✓ Single element variance caught ValueError: {e}")

    # Invalid type inside sequence
    try:
        sensor_analysis.average([22.1, "invalid_reading", 24.5])
        print("✗ FAIL: Non-numeric element did not raise TypeError")
    except TypeError as e:
        print(f"✓ Non-numeric sequence item caught TypeError: {e}")

    # Non-sequence argument
    try:
        sensor_analysis.range_value(12345)
        print("✗ FAIL: Non-sequence data did not raise TypeError")
    except TypeError as e:
        print(f"✓ Non-sequence argument caught TypeError: {e}")

    # 3. Performance Benchmark vs Pure Python
    print("\n--- 3. Performance Benchmark (1,000,000 Sensor Samples) ---")
    large_dataset = [20.0 + (i % 100) * 0.1 for i in range(1_000_000)]

    # Pure Python implementation for timing
    t0 = time.perf_counter()
    py_avg = sum(large_dataset) / len(large_dataset)
    py_time = time.perf_counter() - t0

    # C Extension implementation
    t0 = time.perf_counter()
    c_avg = sensor_analysis.average(large_dataset)
    c_time = time.perf_counter() - t0

    speedup = py_time / c_time if c_time > 0 else float("inf")
    print(f"Pure Python Average Time: {py_time * 1000:.2f} ms")
    print(f"C Extension Average Time: {c_time * 1000:.2f} ms")
    print(f"Speedup Factor:           {speedup:.2f}x faster")
    assert math.isclose(py_avg, c_avg), "Benchmark calculation discrepancy!"

    print("\n" + "=" * 60)
    print("      ALL TESTS PASSED SUCCESSFULLY!")
    print("=" * 60)


if __name__ == "__main__":
    run_tests()
