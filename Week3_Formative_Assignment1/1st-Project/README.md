# Project 1: Investigating a Suspicious Binary

## 📝 Objective
Simulate a forensic investigation of an unknown executable (`data_sync`). The goal is to analyze the binary purely through static reverse-engineering techniques to determine its safety and functionality without executing potentially malicious code.

## 📂 Files Included
* `data_sync.c`: The simulated source code for a file synchronization tool.
* `data_sync`: The compiled ELF64 binary.
* `README.md`: Project documentation.

## 🚀 Build & Analysis Instructions
Navigate to this directory in your terminal and compile the program (if not already compiled):
```bash
gcc -o data_sync data_sync.c

To safely analyze the binary without running it, use the following GNU Binutils commands:

1. Identify the execution entry point and architecture:

Bash
objdump -f data_sync
2. Extract dynamically linked shared library functions (e.g., fopen, fwrite):

Bash
nm -u data_sync
3. Analyze the ELF segment headers (.text, .rodata, .bss):

Bash
objdump -h data_sync

-------------
Note: Refer to the main PDF report for the complete forensic breakdown of these outputs.
-------------
