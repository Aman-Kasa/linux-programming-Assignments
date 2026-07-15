/*
 * q3_prime_counter.c
 * 16 threads count primes in [1, 200000]. Each thread gets an equal slice.
 * Shared total counter protected by pthread_mutex_t.
 */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdbool.h>
#include <math.h>

#define MAX_NUM 200000
#define NUM_THREADS 16

typedef struct {
    int start;
    int end;
} ThreadData;

int total_primes = 0;           // shared counter
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

// Simple deterministic primality test
bool is_prime(int n) {
    if (n <= 1) return false;
    if (n <= 3) return true;
    if (n % 2 == 0 || n % 3 == 0) return false;
    for (int i = 5; i * i <= n; i += 6)
        if (n % i == 0 || n % (i + 2) == 0)
            return false;
    return true;
}

void* count_primes(void *arg) {
    ThreadData *data = (ThreadData*)arg;
    int local_count = 0;
    for (int i = data->start; i <= data->end; i++) {
        if (is_prime(i))
            local_count++;
    }
    // Update global total under mutex
    pthread_mutex_lock(&mutex);
    total_primes += local_count;
    pthread_mutex_unlock(&mutex);
    return NULL;
}

int main() {
    pthread_t threads[NUM_THREADS];
    ThreadData tdata[NUM_THREADS];
    int range = MAX_NUM / NUM_THREADS;
    int remainder = MAX_NUM % NUM_THREADS;
    int current_start = 1;

    // Create threads
    for (int i = 0; i < NUM_THREADS; i++) {
        tdata[i].start = current_start;
        tdata[i].end = current_start + range - 1;
        if (i < remainder) tdata[i].end++;   // distribute remainder evenly
        if (tdata[i].end > MAX_NUM) tdata[i].end = MAX_NUM;
        if (pthread_create(&threads[i], NULL, count_primes, &tdata[i]) != 0) {
            perror("pthread_create");
            exit(1);
        }
        current_start = tdata[i].end + 1;
    }

    // Wait for all threads
    for (int i = 0; i < NUM_THREADS; i++)
        pthread_join(threads[i], NULL);

    printf("The synchronized total number of prime numbers between 1 and %d is %d\n",
           MAX_NUM, total_primes);
    return 0;
}
