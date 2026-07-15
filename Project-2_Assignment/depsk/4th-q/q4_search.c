/*
 * q4_search.c (final)
 * Processes all input files by distributing them among threads.
 * Usage: ./search keyword output.txt file1 file2 ... num_threads
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <time.h>

typedef struct {
    int thread_id;
    int start_idx;
    int end_idx;
    char **file_list;
    char *keyword;
    FILE *out;
    pthread_mutex_t *lock;
    int *total_global;           // optional, to sum total across threads
} ThreadData;

int count_in_file(const char *filename, const char *keyword) {
    FILE *fp = fopen(filename, "r");
    if (!fp) {
        perror(filename);
        return 0;
    }
    int count = 0;
    char *line = NULL;
    size_t len = 0;
    ssize_t nread;
    while ((nread = getline(&line, &len, fp)) != -1) {
        char *ptr = line;
        while ((ptr = strstr(ptr, keyword)) != NULL) {
            count++;
            ptr += strlen(keyword);
        }
    }
    free(line);
    fclose(fp);
    return count;
}

void* process_files(void *arg) {
    ThreadData *td = (ThreadData*)arg;
    int local_total = 0;

    for (int i = td->start_idx; i <= td->end_idx; i++) {
        int cnt = count_in_file(td->file_list[i], td->keyword);
        local_total += cnt;

        // Write result to shared file under mutex
        pthread_mutex_lock(td->lock);
        fprintf(td->out, "%s: %d\n", td->file_list[i], cnt);
        fflush(td->out);
        pthread_mutex_unlock(td->lock);
    }

    // Optionally update a global total (if desired), not required but can be shown.
    return NULL;
}

int main(int argc, char *argv[]) {
    if (argc < 5) {
        fprintf(stderr, "Usage: %s keyword output.txt file1 file2 ... num_threads\n", argv[0]);
        return 1;
    }

    char *keyword = argv[1];
    char *outname = argv[2];
    int num_threads = atoi(argv[argc-1]);
    int num_files = argc - 4;          // indices 3 to argc-2

    if (num_files <= 0 || num_threads <= 0) {
        fprintf(stderr, "Need at least one file and positive thread count.\n");
        return 1;
    }
    if (num_threads > num_files)
        num_threads = num_files;       // each thread must get at least one file

    // File list
    char **file_list = &argv[3];

    // Shared output file
    FILE *out = fopen(outname, "w");
    if (!out) {
        perror(outname);
        return 1;
    }

    pthread_t *threads = malloc(num_threads * sizeof(pthread_t));
    ThreadData *tdata = malloc(num_threads * sizeof(ThreadData));
    pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

    // Distribute files among threads
    int files_per_thread = num_files / num_threads;
    int remainder = num_files % num_threads;
    int current = 0;

    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);

    for (int t = 0; t < num_threads; t++) {
        tdata[t].thread_id = t;
        tdata[t].start_idx = current;
        int count = files_per_thread + (t < remainder ? 1 : 0);
        tdata[t].end_idx = current + count - 1;
        tdata[t].file_list = file_list;
        tdata[t].keyword = keyword;
        tdata[t].out = out;
        tdata[t].lock = &lock;

        pthread_create(&threads[t], NULL, process_files, &tdata[t]);
        current += count;
    }

    for (int t = 0; t < num_threads; t++)
        pthread_join(threads[t], NULL);

    clock_gettime(CLOCK_MONOTONIC, &end);
    double elapsed = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec)/1e9;

    printf("Search completed with %d threads in %.4f seconds.\n", num_threads, elapsed);

    fclose(out);
    free(threads);
    free(tdata);
    return 0;
}
