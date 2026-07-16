#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdbool.h>

#define MAX_NUM 200000
#define NUM_THREADS 16

int total_primes = 0;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

typedef struct {
    int start;
    int end;
} ThreadData;

bool is_prime(int n) {
    if (n <= 1) return false;
    if (n <= 3) return true;
    if (n % 2 == 0 || n % 3 == 0) return false;
    for (int i = 5; i * i <= n; i += 6) {
        if (n % i == 0 || n % (i + 2) == 0) return false;
    }
    return true;
}

void* count_primes(void* arg) {
    ThreadData* data = (ThreadData*)arg;
    int local_count = 0;

    for (int i = data->start; i <= data->end; i++) {
        if (is_prime(i)) {
            local_count++;
        }
    }

    // Critical section
    pthread_mutex_lock(&mutex);
    total_primes += local_count;
    pthread_mutex_unlock(&mutex);

    pthread_exit(NULL);
}

int main() {
    pthread_t threads[NUM_THREADS];
    ThreadData t_data[NUM_THREADS];
    int segment = MAX_NUM / NUM_THREADS;

    for (int i = 0; i < NUM_THREADS; i++) {
        t_data[i].start = i * segment + 1;
        t_data[i].end = (i == NUM_THREADS - 1) ? MAX_NUM : (i + 1) * segment;
        
        pthread_create(&threads[i], NULL, count_primes, (void*)&t_data[i]);
    }

    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }

    printf("The synchronized total number of prime numbers between 1 and %d is %d\n", MAX_NUM, total_primes);
    return 0;
}
