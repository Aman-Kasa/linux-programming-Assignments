/*
 * q1_pipeline.c - Implements: ps aux | grep root
 * Parent creates two children connected by a pipe.
 * Child1: execvp("ps", ...)  stdout -> pipe write end.
 * Child2: execvp("grep", ...) stdin <- pipe read end, stdout -> file.
 * Parent captures the output file, reads and displays first 5 lines.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>

#define OUTPUT_FILE "pipeline_output.txt"
#define DISPLAY_LINES 5

void error_exit(const char *msg) {
    perror(msg);
    exit(EXIT_FAILURE);
}

int main(void) {
    int pipefd[2];
    pid_t pid1, pid2;

    // Create pipe before fork (shared between children)
    if (pipe(pipefd) == -1)
        error_exit("pipe");

    pid1 = fork();
    if (pid1 < 0) error_exit("fork1");

    if (pid1 == 0) { // Child 1: ps aux
        // Redirect stdout to pipe write end
        if (dup2(pipefd[1], STDOUT_FILENO) == -1)
            error_exit("dup2 child1");
        // Close all pipe fds (we already duplicated the needed one)
        close(pipefd[0]);
        close(pipefd[1]);

        // Execute ps
        char *args[] = { "ps", "aux", NULL };
        execvp("ps", args);
        error_exit("execvp ps");   // only reached on error
    }

    pid2 = fork();
    if (pid2 < 0) error_exit("fork2");

    if (pid2 == 0) { // Child 2: grep root
        // Redirect stdin from pipe read end
        if (dup2(pipefd[0], STDIN_FILENO) == -1)
            error_exit("dup2 child2");
        close(pipefd[0]);
        close(pipefd[1]);

        // Open output file, redirect stdout to it
        int fd_out = open(OUTPUT_FILE, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd_out == -1)
            error_exit("open output file");
        if (dup2(fd_out, STDOUT_FILENO) == -1)
            error_exit("dup2 stdout to file");
        close(fd_out);

        // Execute grep
        char *args[] = { "grep", "root", NULL };
        execvp("grep", args);
        error_exit("execvp grep");
    }

    // Parent: close both pipe ends immediately
    close(pipefd[0]);
    close(pipefd[1]);

    // Wait for both children to finish
    waitpid(pid1, NULL, 0);
    waitpid(pid2, NULL, 0);

    printf("Pipeline completed. Output saved to %s\n", OUTPUT_FILE);

    // Read and display first DISPLAY_LINES lines
    FILE *fp = fopen(OUTPUT_FILE, "r");
    if (!fp) {
        perror("fopen output");
        return 1;
    }
    printf("\n--- First %d lines of output ---\n", DISPLAY_LINES);
    char line[512];
    for (int i = 0; i < DISPLAY_LINES && fgets(line, sizeof(line), fp); i++) {
        fputs(line, stdout);
    }
    fclose(fp);
    return 0;
}
