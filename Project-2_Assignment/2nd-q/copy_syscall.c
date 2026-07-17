/*
 * Question 2 - Version 1: Large file copy using LOW-LEVEL SYSTEM CALLS
 * ----------------------------------------------------------------------
 * Every read/write of a chunk is a direct syscall (open, read, write,
 * close) with NO library-level buffering layer in between.
 *
 * Usage: ./copy_syscall <source_file> <dest_file>
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>

#define CHUNK_SIZE 8192

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <source> <destination>\n", argv[0]);
        return EXIT_FAILURE;
    }

    struct timespec t_start, t_end;
    clock_gettime(CLOCK_MONOTONIC, &t_start);

    int src_fd = open(argv[1], O_RDONLY);
    if (src_fd < 0) { perror("open source"); return EXIT_FAILURE; }

    int dst_fd = open(argv[2], O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (dst_fd < 0) { perror("open destination"); close(src_fd); return EXIT_FAILURE; }

    char buf[CHUNK_SIZE];
    ssize_t n_read;
    long total_bytes = 0;
    long read_calls = 0, write_calls = 0;

    while ((n_read = read(src_fd, buf, CHUNK_SIZE)) > 0) {
        read_calls++;
        ssize_t written_so_far = 0;
        while (written_so_far < n_read) {
            ssize_t n_written = write(dst_fd, buf + written_so_far, n_read - written_so_far);
            if (n_written < 0) { perror("write"); close(src_fd); close(dst_fd); return EXIT_FAILURE; }
            write_calls++;
            written_so_far += n_written;
        }
        total_bytes += n_read;
    }
    if (n_read < 0) perror("read");

    close(src_fd);
    close(dst_fd);

    clock_gettime(CLOCK_MONOTONIC, &t_end);
    double elapsed = (t_end.tv_sec - t_start.tv_sec) + (t_end.tv_nsec - t_start.tv_nsec) / 1e9;

    printf("=== Version 1: read()/write() syscalls ===\n");
    printf("Bytes copied     : %ld\n", total_bytes);
    printf("read() calls     : %ld\n", read_calls);
    printf("write() calls    : %ld\n", write_calls);
    printf("Elapsed time (s) : %.6f\n", elapsed);

    return EXIT_SUCCESS;
}
