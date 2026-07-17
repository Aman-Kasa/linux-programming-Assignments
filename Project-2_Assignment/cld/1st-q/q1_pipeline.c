/*
 * Question 1: Process Creation, execvp(), and pipe() IPC
 * ---------------------------------------------------------
 * Emulates the shell pipeline:   ps aux | grep root
 *
 * - Two child processes are created with fork().
 * - Child A execs "ps aux", writing its stdout into the pipe.
 * - Child B execs "grep root", reading stdin from the pipe and
 *   writing its own stdout into an output file (this IS the
 *   "parent captures output into a file" requirement, achieved
 *   via redirection rather than the parent copying bytes itself,
 *   which is exactly how a real shell builds a pipeline).
 * - The parent waits for both children, then opens the result
 *   file itself and displays the first N lines to the terminal.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <string.h>

#define OUTPUT_FILE "pipeline_output.txt"
#define LINES_TO_SHOW 5

static void die(const char *msg) {
    perror(msg);
    exit(EXIT_FAILURE);
}

int main(void) {
    int pipefd[2];

    if (pipe(pipefd) == -1) die("pipe");

    /* ---- Child A: ps aux -> writes to pipe ---- */
    pid_t pid_ps = fork();
    if (pid_ps < 0) die("fork (ps)");

    if (pid_ps == 0) {
        close(pipefd[0]);                 /* not reading here      */
        if (dup2(pipefd[1], STDOUT_FILENO) == -1) die("dup2 ps");
        close(pipefd[1]);

        char *args[] = {"ps", "aux", NULL};
        execvp(args[0], args);
        die("execvp ps");                 /* only reached on error */
    }

    /* ---- Child B: grep root -> writes to output file ---- */
    pid_t pid_grep = fork();
    if (pid_grep < 0) die("fork (grep)");

    if (pid_grep == 0) {
        close(pipefd[1]);                 /* not writing here      */
        if (dup2(pipefd[0], STDIN_FILENO) == -1) die("dup2 grep");
        close(pipefd[0]);

        int fd = open(OUTPUT_FILE, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd < 0) die("open output file");
        if (dup2(fd, STDOUT_FILENO) == -1) die("dup2 outfile");
        close(fd);

        char *args[] = {"grep", "root", NULL};
        execvp(args[0], args);
        die("execvp grep");
    }

    /* ---- Parent: MUST close both ends or grep never sees EOF ---- */
    close(pipefd[0]);
    close(pipefd[1]);

    int status_ps, status_grep;
    waitpid(pid_ps, &status_ps, 0);
    waitpid(pid_grep, &status_grep, 0);

    printf("[parent] ps  exited with status %d\n", WEXITSTATUS(status_ps));
    printf("[parent] grep exited with status %d\n", WEXITSTATUS(status_grep));
    printf("[parent] pipeline complete -> %s\n\n", OUTPUT_FILE);

    /* ---- Parent reads and displays part of the captured output ---- */
    FILE *fp = fopen(OUTPUT_FILE, "r");
    if (!fp) die("fopen output file");

    printf("--- First %d line(s) of %s ---\n", LINES_TO_SHOW, OUTPUT_FILE);
    char line[512];
    int shown = 0;
    while (shown < LINES_TO_SHOW && fgets(line, sizeof(line), fp)) {
        fputs(line, stdout);
        shown++;
    }
    fclose(fp);

    return EXIT_SUCCESS;
}
