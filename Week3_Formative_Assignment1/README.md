# Linux Programming — Week 3 Formative Assignment 1

<div align="center">

**Aman Abraha Kasa**  
African Leadership University (ALU)  
Software Engineering — Linux Programming

</div>

---

## Overview

This repository contains my submissions for **Week 3 Formative Assignment 1** in Linux Programming.  
The projects focus on practical exploration of Linux internals, including binary analysis, system-call tracing, Python/C integration, and process signaling.

---

## Project Structure

| Directory | Project Focus | Technical Area |
|-----------|----------------|----------------|
| `1st-Project/` | Investigating a Suspicious Binary | Static ELF analysis |
| `2nd-Project/` | System Call Monitoring Tool | VFS tracing with `strace` |
| `3rd-Project/` | Python Performance Extension | CPython C-API |
| `4th-Project/` | Signal-Based Server Controller | POSIX `sigaction` IPC |

---

## Environment & Prerequisites

To compile and run the programs, use a Linux environment with the following tools installed:

- **GCC** (GNU Compiler Collection)
- **Python 3**
- **Python 3 Development Headers** (`python3-dev`)
- **GNU Binutils** (`objdump`, `nm`)
- **strace** (system-call tracing utility)

### Suggested installation (Debian/Ubuntu)

```bash
sudo apt update
sudo apt install -y build-essential python3 python3-dev binutils strace
```

---

## Usage

1. Navigate to one of the project directories:
   ```bash
   cd 1st-Project
   ```

2. Read the local `README.md` in that folder.

3. Follow the provided instructions for:
   - compilation
   - execution
   - testing and validation

---

## Notes

- Each project is self-contained and includes detailed setup instructions.
- Outputs may vary slightly depending on Linux distribution and kernel version.
- This work is intended for academic and educational purposes.

---

## Author

**Aman Abraha Kasa**  
African Leadership University (ALU)
