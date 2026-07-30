# Project 2: Assembly-Based Text File Analysis

## Overview
An x86‑64 Linux assembly program that reads `sensor_readings.txt`, traverses its contents byte by byte, and counts:
- **Total records** (all lines, including blank lines)
- **Valid records** (only lines that contain actual data)

The program uses only Linux system calls (`sys_open`, `sys_read`, `sys_write`, `sys_close`, `sys_exit`) and avoids any C standard library. It correctly handles Unix LF and Windows CRLF line endings, missing files, empty files, and files that lack a final newline.

---

## Build Instructions

### Requirements
- **Assembler:** NASM (Netwide Assembler)  
- **Linker:** GNU ld (typically part of binutils)  
- **Architecture:** x86‑64 Linux

### Compilation & Linking
```bash
nasm -f elf64 sensor_analyzer.asm -o sensor_analyzer.o
ld sensor_analyzer.o -o sensor_analyzer
