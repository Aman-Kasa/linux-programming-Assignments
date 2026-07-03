import time
import my_ext

def pure_python_sum(size):
    # Simulating heavy numeric computation using structural python elements
    total = 0
    for i in range(size):
        total += i * 2
    return total

def main():
    array_size = 10_000_000
    print(f"Running benchmarks with array size: {array_size:,}\n")

    # Benchmark Pure Python
    start_time = time.time()
    python_res = pure_python_sum(array_size)
    python_duration = time.time() - start_time
    print(f"[Pure Python] Result: {python_res}")
    print(f"[Pure Python] Duration: {python_duration:.4f} seconds\n")

    # Benchmark C Extension
    start_time = time.time()
    c_res = my_ext.compute_sum(array_size)
    c_duration = time.time() - start_time
    print(f"[C Extension] Result: {c_res}")
    print(f"[C Extension] Duration: {c_duration:.4f} seconds\n")

    # Performance Delta calculation
    speedup = python_duration / c_duration
    print(f"--> C Extension is {speedup:.2f}x faster than Pure Python.")

if __name__ == "__main__":
    main()
