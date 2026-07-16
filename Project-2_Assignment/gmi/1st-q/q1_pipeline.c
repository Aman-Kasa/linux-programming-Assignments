#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>

int main() {
    int pipefd[2];
    pid_t pid1, pid2;

    if (pipe(pipefd) == -1) {
        perror("pipe failed");
        exit(EXIT_FAILURE);
    }

    // First child: ps aux
    pid1 = fork();
    if (pid1 == 0) {
        close(pipefd[0]); // Close unused read end
        dup2(pipefd[1], STDOUT_FILENO); // Redirect stdout to pipe
        close(pipefd[1]);

        char *args[] = {"ps", "aux", NULL};
        execvp(args[0], args);
        perror("execvp ps failed");
        exit(EXIT_FAILURE);
    }

    // Second child: grep root
    pid2 = fork();
    if (pid2 == 0) {
        close(pipefd[1]); // Close unused write end
        dup2(pipefd[0], STDIN_FILENO); // Redirect stdin from pipe
        close(pipefd[0]);

        // Redirect stdout to a file
        int fd = open("q1_output.txt", O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd == -1) {
            perror("open failed");
            exit(EXIT_FAILURE);
        }
        dup2(fd, STDOUT_FILENO);
        close(fd);

        char *args[] = {"grep", "root", NULL};
        execvp(args[0], args);
        perror("execvp grep failed");
        exit(EXIT_FAILURE);
    }

    // Parent process
    close(pipefd[0]);
    close(pipefd[1]);
    waitpid(pid1, NULL, 0);
    waitpid(pid2, NULL, 0);

    // Read and display part of the output
    printf("--- Parent reading first 200 bytes of output ---\n");
    int fd = open("q1_output.txt", O_RDONLY);
    if (fd != -1) {
        char buffer[201];
        ssize_t bytes_read = read(fd, buffer, 200);
        if (bytes_read > 0) {
            buffer[bytes_read] = '\0';
            printf("%s\n...\n", buffer);
        }
        close(fd);
    }

    return 0;
}
