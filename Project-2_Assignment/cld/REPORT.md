# Project 2 Report — System Calls, I/O Performance, and Multithreading

## 0. Methodology note (read this first)

All four programs below were written, compiled, executed, and correctness-checked
in a sandboxed Linux container (Ubuntu 24.04, gcc 13.3.0, 1 vCPU). Every output,
checksum, and timing number in this report is real, not invented — you can
reproduce all of it yourself with `make all` (see the accompanying source tree).

**One limitation:** the sandbox this was built in has no network access, so
`strace` could not be installed there (`apt-get install strace` fails with no
route to the package mirror — you'll see the same if you ever work in a locked
-down container). That means the `strace` *logs* quoted below are illustrative
examples of the syscall shapes you will see, built from how these exact code
paths are documented to behave (glibc/Linux syscall semantics), not a literal
paste of a captured trace. **You need to run the exact `strace` commands given
in each section yourself** (any normal Linux VM/lab machine/WSL will have
`strace`, or `sudo apt install strace`) and drop your real output into this
report where indicated — that's also what your grader will expect to see
matched against your own machine's process IDs and timings. Everything else
(the code, the correctness verification, the Q2/Q3/Q4 timing data) is genuine,
measured output from actually running these programs.

---

## Question 1 — Process Creation, `execvp()`, and `pipe()` IPC

### What it does
`q1/q1_pipeline.c` reproduces the shell pipeline `ps aux | grep root` using raw
system calls instead of the shell:

1. `pipe(pipefd)` creates one pipe (`pipefd[0]` = read end, `pipefd[1]` = write end).
2. `fork()` #1 creates a child that `dup2()`s the pipe's write end onto its
   `stdout`, closes the now-redundant fd, and `execvp("ps", ["ps","aux",NULL])`s.
3. `fork()` #2 creates a second child that `dup2()`s the pipe's read end onto
   its `stdin`, opens `pipeline_output.txt` and `dup2()`s it onto `stdout`, then
   `execvp("grep", ["grep","root",NULL])`s.
4. The **parent** closes both of its own copies of the pipe fds (critical —
   otherwise `grep` blocks forever because a writable copy of the pipe stays
   open in the parent and it never sees end-of-file), `waitpid()`s on both
   children, then itself `fopen()`s the result file and prints the first 5
   lines to the terminal — this is the "capture to a file" + "read and
   display part of the output" requirement, achieved through fd redirection
   exactly the way a real shell builds a pipeline.

### Build & run
```
gcc -Wall -Wextra -o q1_pipeline q1_pipeline.c
./q1_pipeline
```

### Real captured output (from this sandbox)
```
[parent] ps  exited with status 0
[parent] grep exited with status 0
[parent] pipeline complete -> pipeline_output.txt

--- First 5 line(s) of pipeline_output.txt ---
root         1  2.4  0.1  17216  5696 ?        Sl   18:53   0:01 /process_api ...
root         2  0.0  0.0      0     0 ?        S    18:53   0:00 [kthreadd]
root         3  0.0  0.0      0     0 ?        S    18:53   0:00 [pool_workqueue_release]
root         4  0.0  0.0      0     0 ?        I<   18:53   0:00 [kworker/R-rcu_gp]
root         5  0.0  0.0      0     0 ?        I<   18:53   0:00 [kworker/R-sync_wq]
```
57 total matching lines were written to `pipeline_output.txt` — every line
independently confirmed to contain `root` and to be a real line of `ps aux`
output for this machine.

### Tracing it — commands to run yourself
Full trace, following child processes (`-f` is mandatory here — without it
you only see the parent):
```
strace -f -tt -o q1_full_trace.log ./q1_pipeline
```
Focused trace on exactly the syscall categories the assignment asks about:
```
strace -f -e trace=clone,fork,vfork,execve,pipe,pipe2,dup2,close,open,openat,read,write,wait4 \
       -o q1_filtered_trace.log ./q1_pipeline
```

### How to read and explain the trace (for your write-up)
| Syscall you'll see | Why it's there |
|---|---|
| `pipe([3,4])` (or `pipe2`) | Parent creates the IPC channel before forking, so both children inherit the same fds. |
| `clone(...)` ×2 | glibc's `fork()` is implemented on Linux via the `clone()` syscall, not a syscall literally named `fork` — don't be surprised if you never see the word "fork" in the log. Each call spawns one child. |
| `dup2(4,1)` in child A / `dup2(3,0)` in child B | Redirect the pipe onto `stdout`/`stdin` so the exec'd program's normal I/O calls transparently go through the pipe. |
| `close(...)` (several) | Each process closes the pipe end(s) it doesn't use. This is not just cleanup — if a writable fd to the pipe is left open anywhere, the reader **never sees EOF** and blocks forever. This is the single most common bug in pipeline code and worth a full paragraph in your analysis. |
| `execve("/usr/bin/ps", ["ps","aux"], ...)` / `execve("/usr/bin/grep", ...)` | The actual program replacement. Note: `execvp()` searches `$PATH` **in user space**, so you may see one or more failed `execve()` attempts (`ENOENT`) against earlier `$PATH` directories before the one that succeeds — that's `execvp` doing its job, not an error in your program. |
| `read`/`write` on fd 3/4 | The pipe traffic itself: `ps` writing its listing, `grep` reading it. |
| `openat(..., "pipeline_output.txt", ...)` | Child B creating the capture file. |
| `wait4(-1, ...)` ×2 | The parent blocking on each child via `waitpid()`. |

Your report should walk through the trace roughly in this order (creation →
IPC setup → redirection → exec → data flow → cleanup/wait) and explicitly
call out the close-before-EOF point above — that's the "deeply analyzed...
accurate explanation of system calls" language the rubric is asking for.

### Bonus (ties to the stated "Analyze ELF binaries" goal)
```
file ./q1_pipeline          # confirms it's a dynamically linked ELF64 executable
ldd ./q1_pipeline            # shows libc.so and the dynamic linker it depends on
readelf -h ./q1_pipeline     # ELF header: entry point, type, etc.
```

---

## Question 2 — Raw System Calls vs. Standard I/O

### What it does
Two independent copy utilities, both correctness-checked byte-for-byte
against the source with `sha256sum`:

- **`q2/copy_syscall.c`** — `open`/`read`/`write`/`close` only, 8192-byte
  chunks, no library buffering layer at all.
- **`q2/copy_stdio.c`** — `fopen`/`fread`/`fwrite`/`fclose`, same 8192-byte
  request size, but glibc adds its own internal `FILE*` buffer underneath.

### Build & run
```
gcc -O2 -o copy_syscall copy_syscall.c
gcc -O2 -o copy_stdio   copy_stdio.c
dd if=/dev/urandom of=testfile_100mb.bin bs=1M count=100   # 100 MB test file
./copy_syscall testfile_100mb.bin out_syscall.bin
./copy_stdio   testfile_100mb.bin out_stdio.bin
sha256sum testfile_100mb.bin out_syscall.bin out_stdio.bin  # must all match
```

### Correctness (real output from this sandbox)
```
8c029c6863b43d18ea971a0878cfbf15a006ebbab5af2d3296700fc7907d69e6  testfile_100mb.bin
8c029c6863b43d18ea971a0878cfbf15a006ebbab5af2d3296700fc7907d69e6  out_syscall.bin
8c029c6863b43d18ea971a0878cfbf15a006ebbab5af2d3296700fc7907d69e6  out_stdio.bin
```
All three checksums are identical — both copies are byte-perfect.

### Real measured timing (this sandbox, 100 MB, 5 warm-cache trials each)
| Trial | `copy_syscall` (s) | `copy_stdio` (s) |
|---|---|---|
| 1 | 0.184954 | 0.245824 |
| 2 | 0.173496 | 0.123462 |
| 3 | 0.108063 | 0.212455 |
| 4 | 0.133221 | 0.119757 |
| 5 | 0.109899 | 0.210878 |
| **Average** | **0.1419 s** | **0.1825 s** |

On this particular machine, the raw-syscall version was modestly faster on
average, with high run-to-run variance in both (a single shared vCPU
sandbox is a noisy environment — page-cache state and scheduler jitter
matter more here than on a quiet lab machine). **Run this same table on your
own grading machine and use your own numbers** — the direction can
legitimately differ by system, and that's fine to report honestly; what
matters for the rubric is that your numbers are real and your explanation of
*why* is grounded in the mechanism below, not a guess.

### Tracing it — commands to run yourself
```
strace -c -o q2_syscall_summary.txt ./copy_syscall testfile_100mb.bin out_syscall.bin
strace -c -o q2_stdio_summary.txt   ./copy_stdio   testfile_100mb.bin out_stdio.bin
```
`-c` gives you a summary table (calls, errors, seconds, % time per syscall) —
exactly the "count number of system calls" the rubric wants, without a wall
of per-call lines to wade through.

**Important methodology point for your report:** `strace` itself adds
overhead to every syscall it intercepts (it's implemented via `ptrace`,
which traps the process at every syscall boundary). So the "seconds" column
`strace -c` reports is **not** a fair number to use for your execution-time
comparison — use your program's own internal timer (already built in, printed
as `Elapsed time (s)`) for that, and use `strace -c` **only** for the syscall
*count*, not the timing. Conflating the two is the most common mistake on
this kind of assignment.

### What to look for and explain
Both versions issue the same number of **application-level** calls in this
sandbox run (12,800 each, since 100 MB / 8192 bytes divides evenly) — but the
`fread`/`fwrite` numbers above are calls to a **library function**, not
necessarily 1:1 with the underlying `read`/`write` **syscalls**. Underneath,
glibc's `FILE*` keeps its own internal buffer (sized from the file's block
size via `fstat`, commonly 4096 bytes) and copies data out of it in
user-space to satisfy your `fread()` request; whether that produces fewer,
equal, or more real `read`/`write` syscalls than the raw version depends on
how that internal buffer size interacts with your request size. **This is
exactly what your own `strace -c` counts will reveal** — compare the `calls`
column for `read`/`write` between the two summary files and explain the
actual ratio you observe (don't guess a number you haven't seen; whatever it
is, explain it in terms of the buffering layer above). This empirical,
mechanism-grounded explanation is what separates a "deep and accurate"
comparison from a shallow one in the rubric.

---

## Question 3 — 16-Thread Prime Counting with `pthread_mutex_t`

### What it does
`q3/prime_count.c` splits `[1, 200000]` into 16 **equal** contiguous ranges
(200000 / 16 = 12500 numbers each, dividing perfectly evenly, satisfying
"divide the workload equally"). Each thread trial-divides its own range up
to `sqrt(n)` and accumulates a **local** count — no locking during the actual
work. Only once, at the very end of each thread's work, does it take
`counter_mutex` to add its local subtotal into the shared `shared_counter`.
This is deliberate: locking around the *entire* counting loop would
serialize all 16 threads and defeat the purpose of using threads at all;
locking only around the final addition gives correct synchronization with
essentially zero contention.

### Build & run
```
gcc -O2 -pthread -o prime_count prime_count.c -lm
./prime_count
```

### Real output (this sandbox)
```
Thread   Range Start  Range End    Primes Found
0        1            12500        1492
1        12501        25000        1270
2        25001        37500        1206
3        37501        50000        1165
4        50001        62500        1142
5        62501        75000        1118
6        75001        87500        1099
7        87501        100000       1100
8        100001       112500       1070
9        112501       125000       1072
10       125001       137500       1068
11       137501       150000       1046
12       150001       162500       1037
13       162501       175000       1031
14       175001       187500       1048
15       187501       200000       1020

The synchronized total number of prime numbers between 1 and 200000 is 17984
Elapsed time (s): 0.012320
```
This matches the assignment's required output format exactly:
`The synchronized total number of prime numbers between 1 and 200000 is <total>`.

### Correctness verification
17984 is the well-known value of π(200000) (the prime-counting function).
It was independently cross-checked here with a completely separate
single-threaded Sieve of Eratosthenes implementation:
```
Sieve of Eratosthenes independent check: pi(200000) = 17984
```
Both methods agree, and the multithreaded program produced the identical
total across multiple repeated runs (no race condition — a broken mutex or
missing `pthread_join()` would tend to show up as an inconsistent total
across runs).

### Report talking points
- Why `pthread_mutex_t` is correct here: `shared_counter += local` is a
  read-modify-write; without a mutex, two threads' updates can interleave
  and lose one increment (a classic race condition).
- Why locking only around the addition (not the whole loop) matters for
  performance: it minimizes the critical section, so the 16 threads spend
  almost all their time doing independent, unsynchronized work.
- If you want a performance number to report: time the sequential sieve
  above vs. this program and compare — on a real multi-core grading machine
  you should see a meaningful speedup from parallelism (this sandbox only
  exposes 1 vCPU, so it won't show real parallel speedup here — run this
  comparison on your own multi-core machine for a number worth reporting).

---

## Question 4 — Multithreaded Keyword Search Across Files

### Design
```
./search keyword output.txt file1.txt file2.txt ... <number_of_threads>
```
The assignment asks for "each thread processes one file" **and** a tunable
`<number_of_threads>` argument independent of file count — so `q4/search.c`
uses a small **thread pool with a shared work queue**: `num_threads` workers
each loop, grabbing the next unclaimed file index under `file_index_mutex`,
fully processing that one file (one thread : one file, per unit of work),
then looping back for another file if any remain. This reconciles both
requirements: if `num_threads >= num_files` it behaves like classic
one-thread-per-file; if `num_threads < num_files`, work is load-balanced
across the pool instead of leaving files unprocessed.

Three separate mutexes protect three independent pieces of shared state
(this is deliberate — a single global lock would serialize file-picking,
output-writing, and total-updating unnecessarily):
- `file_index_mutex` — who gets which file next.
- `output_mutex` — serializes writes to the shared results file (each
  thread's `fprintf` + `fflush` happens atomically with respect to others).
- `total_mutex` — protects the grand-total occurrence counter.

### Build & run
```
gcc -O2 -pthread -o search search.c
./search error results.txt testfiles/log1.txt testfiles/log2.txt testfiles/log3.txt \
                testfiles/log4.txt testfiles/log5.txt testfiles/log6.txt <N>
```
`testfiles/` ships with 6 pre-generated sample log files (~1.1 MB each,
20,000 lines, mixed system-log-style vocabulary); `generate_testfiles.py`
reproduces them (or resize/reword the set) if you want different test data.

### Correctness verification
Ground truth was computed independently with `grep -o keyword file | wc -l`
summed across all 6 files:
```
testfiles/log1.txt: 17032
testfiles/log2.txt: 17160
testfiles/log3.txt: 16871
testfiles/log4.txt: 16782
testfiles/log5.txt: 16973
testfiles/log6.txt: 16763
GRAND TOTAL (ground truth): 101581
```
`search`'s output file total matched **101581** exactly, at every thread
count tested below — confirming both the per-file counting logic and the
synchronization around the shared total and shared output file are correct.

### Real runs at the three required configurations
This sandbox exposes only 1 vCPU (`nproc` → 1), so "2 threads" and "average
cores" collapse to the same hardware here and you won't see real parallel
speedup in these particular numbers — **re-run this exact test on your own
multi-core machine** and report your own timings; the assignment wants to
see a genuine core-count effect, which requires genuine cores.

**2 threads:**
```
Search complete using 2 thread(s) across 6 file(s).
Total occurrences of 'error': 101581
Elapsed time (s): 0.017499
```
**Average number of CPU cores (`$(nproc)` = 1 in this sandbox):**
```
Search complete using 1 thread(s) across 6 file(s).
Total occurrences of 'error': 101581
Elapsed time (s): 0.023454
```
**Maximum threads (interpreted here as one thread per file = 6; the
assignment doesn't pin down an exact number, so state your own
interpretation and justify it — one-per-file is the natural "maximum
useful" count for this workload since extra threads beyond the file count
sit idle):**
```
Search complete using 6 thread(s) across 6 file(s).
Total occurrences of 'error': 101581
Elapsed time (s): 0.013566
```

### Report talking points
- On a real multi-core machine, expect: 1 thread = purely sequential
  baseline; 2 threads ≈ some speedup if ≥2 files remain to overlap; threads
  = cores ≈ close to the best achievable speedup for this workload; pushing
  well past both core count *and* file count buys nothing further (idle
  threads, plus a little extra `pthread_create`/`join` overhead) — a good
  report shows this leveling-off explicitly rather than assuming more
  threads always means faster.
- Explain why 3 separate mutexes (rather than 1) reduce unnecessary
  contention: two threads can be picking up new files and writing previous
  results into the output file at the same time, they're just never allowed
  to do *either individual operation* concurrently with another thread doing
  that *same* operation.

---

## Rubric self-check

| Rubric criterion | Where it's satisfied |
|---|---|
| **System Call Tracing and Process Communication** (3 pts) | `q1_pipeline.c`: two `fork()`s, `execvp()` ×2, `pipe()`, correct redirection, output captured to file, parent reads/displays part of it. Section "Question 1" above gives the `strace` commands and a syscall-by-syscall explanation to write up. |
| **System Calls vs Standard I/O Analysis** (4 pts) | Both versions implemented, both copy a real 100 MB file, both byte-verified correct via `sha256sum`, real timing table included, `strace -c` commands + the buffering mechanism explained so you can interpret your own counts. |
| **Multithreading and Synchronization** (4 pts) | 16 POSIX threads, workload divided into exactly equal 12500-number ranges, `pthread_mutex_t` used correctly and minimally, output format matches spec exactly, correctness independently cross-verified. |
| **Concurrent File Processing and Synchronization** (6 pts) | Thread-pool design lets one thread fully own one file at a time while `number_of_threads` stays independently tunable; all 3 required thread counts tested; shared output file and shared total both mutex-protected; results verified against an independent `grep` ground truth. |

## What you still need to do before submitting
1. Run the `strace` commands in the Question 1 and Question 2 sections
   **on your own machine** and paste your real log excerpts into those
   sections (replacing/augmenting the explanation tables) — this sandbox
   couldn't install `strace` itself (no network egress), so this is the one
   piece that has to come from you.
2. Re-run the Question 2 and Question 4 timing tables on your own hardware,
   ideally one with more than 1 core, so the thread-count comparison in Q4
   actually has cores to show off.
3. Skim through each `.c` file once before you submit — you'll be able to
   explain any line if asked, and the in-code comments walk through the
   "why," not just the "what," for exactly that reason.
