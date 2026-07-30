# Linux Programming Summative Project

**Course:** Linux Programming Assignments (May Term 2026)  
**Student:** Aman Abraha Kasa  
**Repository:** `Aman-Kasa/linux-programming-Assignments`  
**Project Folder:** `Summative-Project`  

---

## Project Overview

This summative project demonstrates core Linux systems programming skills across five integrated tasks:

1. **ELF executable development and reverse analysis (C)**
2. **x86 assembly file analysis**
3. **Python C extension for high-performance sensor statistics**
4. **Multithreaded producer-consumer order processing system (C + pthreads)**
5. **Concurrent TCP client-server equipment reservation system (Linux sockets)**

The work emphasizes:
- program compilation and execution in Linux,
- memory and process-level understanding,
- low-level file/system interactions,
- safe concurrency and synchronization,
- performance-oriented C/Python integration.

---

## Repository Structure

```text
Summative-Project/
├── project1-elf/
│   ├── program.c
│   ├── program                 # stripped ELF executable
│   ├── analysis_report.md
│   └── outputs/
│       ├── readelf.txt
│       ├── objdump.txt
│       ├── strace.txt
│       └── gdb_notes.txt
│
├── project2-assembly/
│   ├── sensor_analyzer.asm
│   ├── sensor_readings.txt
│   ├── Makefile
│   └── output.txt
│
├── project3-c-extension/
│   ├── sensor_analysis.c
│   ├── setup.py
│   ├── test_sensor_analysis.py
│   └── README_notes.md
│
├── project4-producer-consumer/
│   ├── order_processing.c
│   ├── Makefile
│   └── sample_output.txt
│
├── project5-tcp-client-server/
│   ├── server.c
│   ├── client.c
│   ├── users.txt
│   ├── equipment.txt
│   ├── Makefile
│   └── demo_logs/
│
└── README.md
```

> If your actual file names differ, keep the same section content but update paths accordingly.

---

## General Build Environment

- **OS:** Ubuntu/Linux
- **Compiler:** `gcc`
- **Assembler:** `nasm` (or GNU assembler if used)
- **Python:** 3.x
- **Libraries/Tools:** `pthread`, Linux socket API, Python C API, `readelf`, `objdump`, `strace`, `gdb`

---

## Project 1: Investigating an ELF Executable

### Objective
Develop a C program, compile/strip it, then perform static and dynamic analysis of the ELF binary.

### Build
```bash
cd project1-elf
gcc -Wall -O0 -fno-inline -o program program.c
strip program
```

### Run
```bash
./program
```

### Static Analysis Commands
```bash
readelf -h program
readelf -S program
objdump -d program
objdump -Mintel -d program
```

### Dynamic Analysis
```bash
strace -o outputs/strace.txt ./program
```

### Debugging / Memory Inspection
```bash
gdb ./program
```

### Expected Deliverables
- `program.c`
- stripped executable `program`
- concise analysis report with:
  - architecture & entry point,
  - section purposes (`.text`, `.data`, `.bss`, `.plt`, `.got`),
  - linking mode (dynamic/static),
  - reconstructed function behavior,
  - branch/loop explanation from assembly,
  - syscall categorization,
  - stack/heap/global memory explanation.

---

## Project 2: Assembly-Based Text File Analysis

### Objective
Read `sensor_readings.txt`, count:
- total lines,
- non-empty valid lines.

Handle both:
- Unix endings (`\n`),
- Windows endings (`\r\n`).

### Build (NASM + LD example)
```bash
cd project2-assembly
nasm -f elf64 sensor_analyzer.asm -o sensor_analyzer.o
ld sensor_analyzer.o -o sensor_analyzer
```

### Run
```bash
./sensor_analyzer
```

### Expected Console Output Format
```text
Total records: X
Valid records: Y
```

### Notes
- Includes file open/read error handling.
- Uses loop traversal and conditional logic for line detection and validation.

---

## Project 3: Python C Extension (`sensor_analysis`)

### Objective
Implement a high-performance C extension callable from Python.

### Implemented Functions
- `average(data)`
- `range_value(data)`
- `variance(data)` *(sample variance)*
- `count_above(data, limit)`
- `statistics(data)` → dictionary:
  ```python
  {"samples": ..., "average": ..., "minimum": ..., "maximum": ...}
  ```

### Build
```bash
cd project3-c-extension
python3 setup.py build
python3 setup.py install --user
```

### Test
```bash
python3 test_sensor_analysis.py
```

### Requirements Satisfied
- Python C API usage
- numeric computation in C (`double`)
- accepts list/tuple numeric input
- input validation and Python exceptions
- safe empty-data handling
- minimal/unnecessary dynamic allocation avoided

---

## Project 4: Multithreaded Order Processing System

### Objective
Simulate online food order processing using:
- producer (kitchen),
- consumer (delivery),
- monitor thread.

### Concurrency Controls
- `pthread_mutex_t`
- `pthread_cond_t`
- fixed shared queue capacity = `5`

### Build
```bash
cd project4-producer-consumer
gcc -Wall -pthread -o order_processing order_processing.c
```

### Run
```bash
./order_processing
```

### Behavior
- Producer creates orders every 2 seconds.
- Consumer delivers every 4 seconds.
- Monitor prints status every 5 seconds:
  - Orders prepared
  - Orders delivered
  - Current queue size

---

## Project 5: Concurrent TCP Client–Server Monitoring System

### Objective
Implement centralized reservation system with concurrent TCP clients.

### Server Responsibilities
- accepts multiple clients (>= 5),
- authenticates valid users,
- shares equipment list after auth,
- handles reservation requests safely,
- prevents duplicate reservations,
- handles abrupt client disconnects.

### Build
```bash
cd project5-tcp-client-server
gcc -Wall -pthread -o server server.c
gcc -Wall -pthread -o client client.c
```

### Run (Terminal 1: Server)
```bash
./server
```

### Run (Terminal 2+ : Clients)
```bash
./client
```

### Example Session End
```text
Session closed. Goodbye, USER_ID
```

---

## Sample Validation Checklist

- [x] Project 1 compiles with required flags and is stripped
- [x] ELF static analysis artifacts generated
- [x] `strace` syscall log generated and categorized
- [x] `gdb` breakpoints + memory inspection demonstrated
- [x] Assembly file parser counts total/valid lines correctly
- [x] C extension builds and all required APIs function
- [x] Producer-consumer synchronization avoids race conditions
- [x] TCP server handles concurrent reservations safely

---

## How to Reproduce Quickly

From `Summative-Project/`, run each project in order:

```bash
# Project 1
cd project1-elf && gcc -Wall -O0 -fno-inline -o program program.c && strip program && ./program && cd ..

# Project 2
cd project2-assembly && nasm -f elf64 sensor_analyzer.asm -o sensor_analyzer.o && ld sensor_analyzer.o -o sensor_analyzer && ./sensor_analyzer && cd ..

# Project 3
cd project3-c-extension && python3 setup.py build && python3 test_sensor_analysis.py && cd ..

# Project 4
cd project4-producer-consumer && gcc -Wall -pthread -o order_processing order_processing.c && ./order_processing && cd ..

# Project 5
cd project5-tcp-client-server && gcc -Wall -pthread -o server server.c && gcc -Wall -pthread -o client client.c
```

---

## Key Learning Outcomes Demonstrated

- ELF internals and reverse-oriented reasoning
- Linux syscall-level observation and debugging
- x86 assembly control-flow and file parsing
- C/Python interoperability for performance
- thread-safe producer-consumer design
- concurrent networked system design with shared-state protection

---

## Academic Integrity Statement

This submission represents my individual work for the summative assessment.  
All implementation, testing, and documentation were prepared in accordance with course requirements.

---

## Author

**Aman Abraha Kasa**  
Linux Programming – Summative Project (July 2026)
