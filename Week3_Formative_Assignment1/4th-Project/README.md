# Project 4: Signal-Based Server Controller

## 📝 Objective
Deploy a continuous background daemon that listens for and properly handles asynchronous operating system signals (`SIGINT`, `SIGUSR1`, `SIGTERM`). The implementation utilizes the robust POSIX `sigaction` API rather than legacy signal handlers to ensure safe inter-process communication (IPC).

## 📂 Files Included
* `monitor_service.c`: The source code containing the background daemon loop and custom signal routing handlers.
* `monitor_service`: The compiled executable.
* `README.md`: Project documentation.

## 🚀 Build & Execution Instructions

**Step 1: Start the Service**
Open your terminal, compile the program, and run it:
```bash
gcc -o monitor_service monitor_service.c
./monitor_service


Note the Process ID (PID) printed to the console (e.g., PID: 23410). Leave this terminal open.

**Step 2: Send Administrative Signals
Open a second terminal window to act as the administrator. Use the Linux kill command to send signals to the running daemon (replace <PID> with your actual Process ID):

Request a system status update (SIGUSR1):

Bash
kill -SIGUSR1 <PID>
Send an emergency shutdown command (SIGTERM):

```Bash
#kill -SIGTERM <PID>

**Step 3: Test Keyboard Interrupt (SIGINT)
            Alternatively, in the first terminal where the service is running, press Ctrl+C to trigger a graceful shutdown sequence.
