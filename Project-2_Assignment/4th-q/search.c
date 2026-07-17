/*
 * Question 4: Multithreaded keyword search across multiple files
 * --------------------------------------------------------------------
 * Usage: ./search keyword output.txt file1.txt file2.txt ... <num_threads>
 *
 * Design: a thread POOL of <num_threads> workers pulls files, one at a
 * time, from a shared work queue (protected by file_index_mutex). This
 * satisfies "each thread processes one file" as the unit of work, while
 * still letting num_threads be tuned independently of the file count
 * (required later for the 2-threads / avg-cores / max-threads tests):
 * if num_threads >= num_files, it behaves like one thread per file;
 * if num_threads < num_files, workers pick up additional files as they
 * finish, which is the standard, load-balanced way to do this.
 *
 * Two independent critical sections are protected with mutexes:
 *   1. file_index_mutex  -> who gets which file next
 *   2. output_mutex      -> serializes writes to the shared output file
 *   3. total_mutex       -> protects the grand-total occurrence counter
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <time.h>
#include <errno.h>

#define MAX_LINE 4096

typedef struct {
    char **files;
    int num_files;
    int next_file_index;
    pthread_mutex_t file_index_mutex;

    const char *keyword;
    FILE *output_fp;
    pthread_mutex_t output_mutex;

    long total_occurrences;
    pthread_mutex_t total_mutex;
} SharedData;

typedef struct {
    SharedData *shared;
    int thread_id;
} ThreadArg;

/* Count (possibly overlapping-free, left-to-right) occurrences of
   `keyword` within a single line of text. */
static long count_occurrences_in_line(const char *line, const char *keyword) {
    long count = 0;
    size_t klen = strlen(keyword);
    if (klen == 0) return 0;
    const char *p = line;
    while ((p = strstr(p, keyword)) != NULL) {
        count++;
        p += klen;
    }
    return count;
}

static void *worker(void *arg) {
    ThreadArg *targ = (ThreadArg *)arg;
    SharedData *shared = targ->shared;

    while (1) {
        pthread_mutex_lock(&shared->file_index_mutex);
        int idx = shared->next_file_index;
        if (idx >= shared->num_files) {
            pthread_mutex_unlock(&shared->file_index_mutex);
            break;                              /* no more files left */
        }
        shared->next_file_index++;
        pthread_mutex_unlock(&shared->file_index_mutex);

        const char *filename = shared->files[idx];
        FILE *fp = fopen(filename, "r");
        if (!fp) {
            fprintf(stderr, "[thread %d] could not open %s: %s\n",
                    targ->thread_id, filename, strerror(errno));
            continue;
        }

        long file_count = 0;
        char line[MAX_LINE];
        while (fgets(line, sizeof(line), fp)) {
            file_count += count_occurrences_in_line(line, shared->keyword);
        }
        fclose(fp);

        pthread_mutex_lock(&shared->output_mutex);
        fprintf(shared->output_fp, "Thread %d | File: %-20s | Occurrences: %ld\n",
                targ->thread_id, filename, file_count);
        fflush(shared->output_fp);
        pthread_mutex_unlock(&shared->output_mutex);

        pthread_mutex_lock(&shared->total_mutex);
        shared->total_occurrences += file_count;
        pthread_mutex_unlock(&shared->total_mutex);
    }
    return NULL;
}

int main(int argc, char *argv[]) {
    if (argc < 5) {
        fprintf(stderr, "Usage: %s keyword output.txt file1.txt [file2.txt ...] num_threads\n", argv[0]);
        return EXIT_FAILURE;
    }

    const char *keyword         = argv[1];
    const char *output_filename = argv[2];
    int num_threads = atoi(argv[argc - 1]);
    if (num_threads <= 0) {
        fprintf(stderr, "num_threads must be a positive integer\n");
        return EXIT_FAILURE;
    }

    int num_files = argc - 4;         /* exclude prog, keyword, outfile, thread count */
    char **files  = &argv[3];
    if (num_files <= 0) {
        fprintf(stderr, "No input files provided\n");
        return EXIT_FAILURE;
    }

    FILE *output_fp = fopen(output_filename, "w");
    if (!output_fp) { perror("fopen output"); return EXIT_FAILURE; }

    SharedData shared;
    shared.files = files;
    shared.num_files = num_files;
    shared.next_file_index = 0;
    pthread_mutex_init(&shared.file_index_mutex, NULL);
    shared.keyword = keyword;
    shared.output_fp = output_fp;
    pthread_mutex_init(&shared.output_mutex, NULL);
    shared.total_occurrences = 0;
    pthread_mutex_init(&shared.total_mutex, NULL);

    pthread_t  *threads     = malloc(sizeof(pthread_t) * (size_t)num_threads);
    ThreadArg  *thread_args = malloc(sizeof(ThreadArg) * (size_t)num_threads);
    if (!threads || !thread_args) { perror("malloc"); return EXIT_FAILURE; }

    struct timespec t_start, t_end;
    clock_gettime(CLOCK_MONOTONIC, &t_start);

    for (int i = 0; i < num_threads; i++) {
        thread_args[i].shared = &shared;
        thread_args[i].thread_id = i;
        if (pthread_create(&threads[i], NULL, worker, &thread_args[i]) != 0) {
            fprintf(stderr, "Failed to create thread %d\n", i);
            return EXIT_FAILURE;
        }
    }
    for (int i = 0; i < num_threads; i++) pthread_join(threads[i], NULL);

    clock_gettime(CLOCK_MONOTONIC, &t_end);
    double elapsed = (t_end.tv_sec - t_start.tv_sec) + (t_end.tv_nsec - t_start.tv_nsec) / 1e9;

    fprintf(output_fp, "TOTAL occurrences of '%s' across %d file(s): %ld\n",
            keyword, num_files, shared.total_occurrences);
    fclose(output_fp);

    printf("Search complete using %d thread(s) across %d file(s).\n", num_threads, num_files);
    printf("Total occurrences of '%s': %ld\n", keyword, shared.total_occurrences);
    printf("Elapsed time (s): %.6f\n", elapsed);

    pthread_mutex_destroy(&shared.file_index_mutex);
    pthread_mutex_destroy(&shared.output_mutex);
    pthread_mutex_destroy(&shared.total_mutex);
    free(threads);
    free(thread_args);
    return EXIT_SUCCESS;
}
