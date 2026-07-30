# Linux Programming Assignments

This repository contains a set of Linux programming assignments covering C, assembly, Python, and systems programming topics.

## Summative Project

### Project 1: ELF Analysis and Memory Concepts

The first project demonstrates a simple C program that uses:

- a global variable,
- local stack variables,
- dynamic heap allocation,
- a loop over an integer array,
- function calls for processing and classification,
- standard library output with `printf`, `malloc`, and `free`.

### What the program does

The program:

1. creates an integer array on the heap,
2. initializes it with values `1, 2, 3, 4`,
3. multiplies each value by a global multiplier,
4. checks whether each value is even or odd,
5. prints the result,
6. frees the allocated memory before exiting.

### Build and run

```bash
cd Summative-Project/1st-Project
gcc -Wall -O0 -fno-inline -o program program.c
./program
```

### Expected output

```text
Processing Array:
Value 3 is Odd
Value 6 is Even
Value 9 is Odd
Value 12 is Even
```

### Project files

- `program.c` — source code for the assignment
- `program` — compiled executable

> Note: if you intend to remove the unstripped executable `program`/`program.c`, delete or replace the file in the repository as needed.

## Repository contents

The repository also includes other assignments related to:

- x86 assembly analysis,
- Python C extensions,
- multithreaded producer-consumer systems,
- TCP client-server programming.
