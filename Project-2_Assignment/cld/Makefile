CC      = gcc
CFLAGS  = -Wall -Wextra -O2
PTHREAD = -pthread
LM      = -lm

all: q1 q2 q3 q4

q1: q1/q1_pipeline
q1/q1_pipeline: q1/q1_pipeline.c
	$(CC) $(CFLAGS) -o q1/q1_pipeline q1/q1_pipeline.c

q2: q2/copy_syscall q2/copy_stdio
q2/copy_syscall: q2/copy_syscall.c
	$(CC) $(CFLAGS) -o q2/copy_syscall q2/copy_syscall.c
q2/copy_stdio: q2/copy_stdio.c
	$(CC) $(CFLAGS) -o q2/copy_stdio q2/copy_stdio.c

q3: q3/prime_count
q3/prime_count: q3/prime_count.c
	$(CC) $(CFLAGS) $(PTHREAD) -o q3/prime_count q3/prime_count.c $(LM)

q4: q4/search
q4/search: q4/search.c
	$(CC) $(CFLAGS) $(PTHREAD) -o q4/search q4/search.c

# Generates the 100MB test file required by Question 2
testfile100mb:
	dd if=/dev/urandom of=q2/testfile_100mb.bin bs=1M count=100 status=progress

clean:
	rm -f q1/q1_pipeline q2/copy_syscall q2/copy_stdio q3/prime_count q4/search
	rm -f q1/pipeline_output.txt q2/out_syscall.bin q2/out_stdio.bin q2/testfile_100mb.bin
	rm -f q4/results_*.txt

.PHONY: all q1 q2 q3 q4 clean testfile100mb
