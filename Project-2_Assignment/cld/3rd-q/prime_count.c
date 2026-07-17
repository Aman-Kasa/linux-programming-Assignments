/*
 * Question 3: Multithreaded prime counting with pthread_mutex_t
 * -----------------------------------------------------------------
 * Counts primes in [1, MAX_NUM] using NUM_THREADS POSIX threads.
 * The range is split into NUM_THREADS equal contiguous segments
 * (200000 / 16 = 12500 numbers per thread, dividing evenly).
 * Each thread counts primes in its own segment into a LOCAL
 * variable (no lock contention during the actual work), then
 * takes the mutex exactly once to add its local result into the
 * shared total. This is both correct and efficient.
 */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <math.h>
#include <time.h>

#define MAX_NUM     200000
#define NUM_THREADS 16

static long shared_counter = 0;
static pthread_mutex_t counter_mutex = PTHREAD_MUTEX_INITIALIZER;

typedef struct {
    int start;
    int end;
    int thread_id;
    long local_count;
} ThreadData;

static int is_prime(int n) {
    if (n < 2) return 0;
    if (n == 2) return 1;
    if (n % 2 == 0) return 0;
    int limit = (int)sqrt((double)n);
    for (int i = 3; i <= limit; i += 2) {
        if (n % i == 0) return 0;
    }
    return 1;
}

static void *count_primes(void *arg) {
    ThreadData *data = (ThreadData *)arg;
    long local = 0;

    for (int n = data->start; n <= data->end; n++) {
        if (is_prime(n)) local++;
    }
    data->local_count = local;

    pthread_mutex_lock(&counter_mutex);
    shared_counter += local;
    pthread_mutex_unlock(&counter_mutex);

    return NULL;
}

int main(void) {
    pthread_t threads[NUM_THREADS];
    ThreadData tdata[NUM_THREADS];

    int segment = MAX_NUM / NUM_THREADS;   /* 12500, divides evenly */

    struct timespec t_start, t_end;
    clock_gettime(CLOCK_MONOTONIC, &t_start);

    for (int i = 0; i < NUM_THREADS; i++) {
        tdata[i].thread_id = i;
        tdata[i].start = i * segment + 1;
        tdata[i].end   = (i == NUM_THREADS - 1) ? MAX_NUM : (i + 1) * segment;
        tdata[i].local_count = 0;

        if (pthread_create(&threads[i], NULL, count_primes, &tdata[i]) != 0) {
            fprintf(stderr, "Failed to create thread %d\n", i);
            return EXIT_FAILURE;
        }
    }

    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }

    clock_gettime(CLOCK_MONOTONIC, &t_end);
    double elapsed = (t_end.tv_sec - t_start.tv_sec) + (t_end.tv_nsec - t_start.tv_nsec) / 1e9;

    printf("%-8s %-12s %-12s %s\n", "Thread", "Range Start", "Range End", "Primes Found");
    for (int i = 0; i < NUM_THREADS; i++) {
        printf("%-8d %-12d %-12d %ld\n", tdata[i].thread_id, tdata[i].start, tdata[i].end, tdata[i].local_count);
    }

    printf("\nThe synchronized total number of prime numbers between 1 and %d is %ld\n",
           MAX_NUM, shared_counter);
    printf("Elapsed time (s): %.6f\n", elapsed);

    pthread_mutex_destroy(&counter_mutex);
    return EXIT_SUCCESS;
}
