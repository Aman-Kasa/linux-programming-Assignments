#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define BUF_SIZE 4096

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <source> <dest>\n", argv[0]);
        return 1;
    }
    FILE *src = fopen(argv[1], "rb");
    if (!src) { perror("fopen src"); return 1; }
    FILE *dst = fopen(argv[2], "wb");
    if (!dst) { perror("fopen dst"); fclose(src); return 1; }

    char buf[BUF_SIZE];
    size_t nread;
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);

    while ((nread = fread(buf, 1, BUF_SIZE, src)) > 0) {
        if (fwrite(buf, 1, nread, dst) != nread) {
            perror("fwrite");
            fclose(src); fclose(dst);
            return 1;
        }
    }
    clock_gettime(CLOCK_MONOTONIC, &end);
    fclose(src);
    fclose(dst);

    double elapsed = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec)/1e9;
    printf("Standard I/O copy: %.4f seconds\n", elapsed);
    return 0;
}
