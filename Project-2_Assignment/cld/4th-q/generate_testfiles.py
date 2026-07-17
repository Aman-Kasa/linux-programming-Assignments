#!/usr/bin/env python3
"""
Regenerates the sample log files used to test q4/search.
Not required to run this - testfiles/ already ships with generated
output - but it's included so you can reproduce or resize the test
set (e.g. more files, different keyword mix) if you want to.
"""
import random

random.seed(42)
WORDS = ["system", "error", "warning", "info", "boot",
         "network", "disk", "memory", "kernel", "process"]
NUM_FILES = 6
LINES_PER_FILE = 20000

for i in range(1, NUM_FILES + 1):
    with open(f"testfiles/log{i}.txt", "w") as f:
        for _ in range(LINES_PER_FILE):
            line_words = random.choices(WORDS, k=random.randint(5, 12))
            f.write(" ".join(line_words) + "\n")

print(f"Generated {NUM_FILES} files of ~{LINES_PER_FILE} lines each in testfiles/")
