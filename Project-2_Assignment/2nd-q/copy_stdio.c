/*
 * Question 2 - Version 2: Large file copy using STANDARD I/O
 * -------------------------------------------------------------
 * Uses fopen/fread/fwrite/fclose. The C library adds its own
 * user-space buffer (a FILE*'s internal buffer, sized from the
 * underlying file's block size) underneath these calls, so the
 * number of actual read/write SYSCALLS is governed by libc, not
 * directly by the size we pass to fread()/fwrite().
 *
 * Usage: ./copy_stdio <source_file> <dest_file>
 */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define CHUNK_SIZE 8192

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <source> <destination>\n", argv[0]);
        return EXIT_FAILURE;
    }

    struct timespec t_start, t_end;
    clock_gettime(CLOCK_MONOTONIC, &t_start);

    FILE *src = fopen(argv[1], "rb");
    if (!src) { perror("fopen source"); return EXIT_FAILURE; }

    FILE *dst = fopen(argv[2], "wb");
    if (!dst) { perror("fopen destination"); fclose(src); return EXIT_FAILURE; }

    char buf[CHUNK_SIZE];
    size_t n_read;
    long total_bytes = 0;
    long fread_calls = 0, fwrite_calls = 0;

    while ((n_read = fread(buf, 1, CHUNK_SIZE, src)) > 0) {
        fread_calls++;
        size_t n_written = fwrite(buf, 1, n_read, dst);
        fwrite_calls++;
        if (n_written != n_read) { perror("fwrite"); fclose(src); fclose(dst); return EXIT_FAILURE; }
        total_bytes += n_written;
    }
    if (ferror(src)) perror("fread");

    fclose(src);
    fclose(dst);

    clock_gettime(CLOCK_MONOTONIC, &t_end);
    double elapsed = (t_end.tv_sec - t_start.tv_sec) + (t_end.tv_nsec - t_start.tv_nsec) / 1e9;

    printf("=== Version 2: fread()/fwrite() standard I/O ===\n");
    printf("Bytes copied     : %ld\n", total_bytes);
    printf("fread() calls    : %ld\n", fread_calls);
    printf("fwrite() calls   : %ld\n", fwrite_calls);
    printf("Elapsed time (s) : %.6f\n", elapsed);

    return EXIT_SUCCESS;
}
