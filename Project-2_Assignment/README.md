# Project 2: System Calls, I/O Performance, and Multithreading

[![C](https://img.shields.io/badge/C-17-blue.svg)](https://en.wikipedia.org/wiki/C_(programming_language))
[![Linux](https://img.shields.io/badge/Linux-Ubuntu%2024.04-orange.svg)](https://ubuntu.com/)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)

A comprehensive exploration of **process communication**, **I/O system calls vs. standard library**, and **POSIX multithreading with synchronization**, implemented in C and traced with `strace`.

---

## 📂 Project Structure

```text
Project-2_Assignment/
├── q1/
│   ├── q1_pipeline.c            # Question 1: pipe(), fork(), execvp() pipeline
│   └── pipeline_output.txt      # Captured output (generated)
├── q2/
│   ├── copy_syscall.c           # Question 2a: low-level read/write copy
│   ├── copy_stdio.c             # Question 2b: stdio fread/fwrite copy
│   └── testfile_100mb.bin       # 100 MB test file (generate with dd)
├── q3/
│   └── prime_count.c            # Question 3: 16-thread prime counter
├── q4/
│   ├── search.c                 # Question 4: multithreaded keyword search
│   ├── generate_testfiles.py    # Script to create sample log files
│   └── testfiles/               # Pre-generated test files (log1.txt ... log6.txt)
└── README.md
```

---

## 🛠 Prerequisites

- **GCC** (tested with 13.3.0)
- **GNU Make** (optional)
- **Linux** with `strace` installed  
  ```bash
  sudo apt install strace
  ```
- **Python 3** (only for regenerating Q4 test files)

---

## ❓ Question 1 — Process Communication with `fork()`, `execvp()`, and `pipe()`

### Objective
Implement the shell pipeline `ps aux | grep root` using raw system calls, capture the output to a file, and display part of it.

### Build & Run
```bash
cd q1
gcc -Wall -Wextra -o q1_pipeline q1_pipeline.c
./q1_pipeline
```

### `strace` Tracing
```bash
# Full trace following child processes
strace -f -tt -o q1_full_trace.log ./q1_pipeline

# Filtered trace of relevant syscalls
strace -f \
  -e trace=clone,fork,vfork,execve,pipe,pipe2,dup2,close,open,openat,read,write,wait4 \
  -o q1_filtered_trace.log ./q1_pipeline
```

Inspect `q1_filtered_trace.log` to see the sequence of `pipe2`, child creation, `dup2` redirections, `execve` calls, and critical `close` operations.

---

## ❓ Question 2 — System Calls vs. Standard I/O

### Objective
Copy a 100 MB file using two methods and compare execution times and system call counts:

1. Low-level: `open` / `read` / `write` / `close`
2. Standard I/O: `fopen` / `fread` / `fwrite` / `fclose`

### Generate Test File
```bash
cd q2
dd if=/dev/urandom of=testfile_100mb.bin bs=1M count=100
```

### Build & Run
```bash
gcc -O2 -o copy_syscall copy_syscall.c
gcc -O2 -o copy_stdio   copy_stdio.c

# Low-level copy
./copy_syscall testfile_100mb.bin out_syscall.bin

# Standard I/O copy
./copy_stdio testfile_100mb.bin out_stdio.bin

# Verify correctness
sha256sum testfile_100mb.bin out_syscall.bin out_stdio.bin
```

### Performance Measurement with `strace`
Use `strace -c` to count syscalls. Use your program's own timing output for execution time.

```bash
strace -c -o q2_syscall_summary.txt ./copy_syscall testfile_100mb.bin out_syscall.bin
strace -c -o q2_stdio_summary.txt   ./copy_stdio   testfile_100mb.bin out_stdio.bin
```

Then compare `read`/`write` call counts and explain the impact of stdio buffering.

---

## ❓ Question 3 — 16-Thread Prime Counting with `pthread_mutex_t`

### Objective
Count primes in `[1, 200000]` using exactly **16 threads**. Split work evenly and protect updates to a shared total with a mutex.

### Build & Run
```bash
cd q3
gcc -O2 -pthread -o prime_count prime_count.c -lm
./prime_count
```

### Expected Output Format
```text
The synchronized total number of prime numbers between 1 and 200000 is 17984
```

The program also prints per-thread prime counts and elapsed time.

---

## ❓ Question 4 — Multithreaded Keyword Search Across Files

### Objective
Search for a keyword across multiple files using a thread pool, synchronize writes to a shared output file, and compare performance across thread counts.

### Build
```bash
cd q4
gcc -O2 -pthread -o search search.c
```

### Generate Test Files (Optional)
Pre-generated files are already included.

```bash
python3 generate_testfiles.py
```

This creates `testfiles/log1.txt` ... `testfiles/log6.txt` (~1.1 MB each, 20k lines each).

### Usage
```text
./search <keyword> <output.txt> <file1> [file2 ...] <number_of_threads>
```

### Examples
```bash
# 2 threads
./search error results_2.txt testfiles/log*.txt 2

# Average CPU core count
nproc
./search error results_avg.txt testfiles/log*.txt $(nproc)

# Maximum threads (one per file = 6)
./search error results_max.txt testfiles/log*.txt 6
```

### Correctness Check
Ground truth for keyword `error` across supplied files is **101581**:

```bash
grep -o error testfiles/log*.txt | wc -l
```

---

## 📊 Performance Analysis

Detailed timing tables, `strace` logs, and interpretations are provided in the accompanying Project 2 report (`report.pdf` or `report.md`).

The report covers:

- Syscall sequence walk-through (Q1)
- System call count vs. wall-clock time (Q2)
- Mutex minimal critical section design (Q3)
- Dynamic work-queue scalability (Q4)

---

## 📄 License

This project is submitted as part of an academic assignment. You may freely use the code for learning purposes.

Built with care by **Aman**
