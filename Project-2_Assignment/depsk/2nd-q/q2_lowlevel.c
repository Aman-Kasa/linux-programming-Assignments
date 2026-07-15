#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>

#define BUF_SIZE 4096

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <source> <dest>\n", argv[0]);
        return 1;
    }
    int src = open(argv[1], O_RDONLY);
    if (src < 0) { perror("open src"); return 1; }
    int dst = open(argv[2], O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (dst < 0) { perror("open dst"); close(src); return 1; }

    char buf[BUF_SIZE];
    ssize_t nread, nwritten;
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);

    while ((nread = read(src, buf, BUF_SIZE)) > 0) {
        char *ptr = buf;
        ssize_t left = nread;
        while (left > 0) {
            nwritten = write(dst, ptr, left);
            if (nwritten < 0) { perror("write"); close(src); close(dst); return 1; }
            left -= nwritten;
            ptr += nwritten;
        }
    }
    clock_gettime(CLOCK_MONOTONIC, &end);
    close(src);
    close(dst);

    double elapsed = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec)/1e9;
    printf("Low-level copy: %.4f seconds\n", elapsed);
    return 0;
}
