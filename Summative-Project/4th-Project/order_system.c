/**
 * Project 4: Multithreaded Order Processing System
 *
 * Synchronizes a Kitchen (Producer), Delivery (Consumer), and Monitor thread
 * using POSIX Threads, Mutex locks, and Condition Variables.
 *
 * Improvements applied:
 *  - Thread-safe console output via flockfile/funlockfile.
 *  - Signal-safe sleep helper (continues after interrupted sleep).
 *  - Comment clarifying when "orders delivered" counter is updated.
 */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <stdbool.h>
#include <time.h>

// Configuration Constants
#define QUEUE_CAPACITY 5
#define TOTAL_ORDERS 20
#define PREP_TIME_SEC 2
#define DELIVERY_TIME_SEC 4
#define MONITOR_INTERVAL_SEC 5

// Order Structure
typedef struct {
    int order_id;
} Order;

// Circular Buffer Order Queue Structure
typedef struct {
    Order buffer[QUEUE_CAPACITY];
    int head;                   // Index where next order is removed
    int tail;                   // Index where next order is inserted
    int count;                  // Current number of items in queue

    // Shared Statistics
    int orders_prepared;        // Total orders produced
    int orders_delivered;       // Total orders consumed (updated AFTER delivery completes)
    bool producer_done;         // Flag indicating kitchen has finished all orders

    // Synchronization Primitives
    pthread_mutex_t mutex;           // Protects queue and statistics state
    pthread_cond_t cond_not_full;    // Signaled when space becomes available
    pthread_cond_t cond_not_empty;   // Signaled when a new order is added
} OrderQueue;

// Global Queue Instance
static OrderQueue queue;

// ------------------------------------------------------------
// Helper: Get timestamp string for clean logging
// ------------------------------------------------------------
static void get_timestamp(char *buffer, size_t len) {
    time_t rawtime;
    struct tm *info;
    time(&rawtime);
    info = localtime(&rawtime);
    strftime(buffer, len, "%H:%M:%S", info);
}

// ------------------------------------------------------------
// Helper: Signal-safe sleep (continues until full time elapsed)
// ------------------------------------------------------------
static void safe_sleep(int seconds) {
    while (seconds > 0) {
        seconds = sleep(seconds);
    }
}

// ------------------------------------------------------------
// Thread 1: Kitchen (Producer)
// ------------------------------------------------------------
void* kitchen_thread(void *arg) {
    (void)arg; // Unused parameter

    for (int id = 1; id <= TOTAL_ORDERS; id++) {
        // Simulate order preparation time (2 seconds)
        safe_sleep(PREP_TIME_SEC);

        Order new_order = { .order_id = id };
        char timestamp[10];

        // --- Critical Section Start ---
        pthread_mutex_lock(&queue.mutex);

        // Wait while the queue is at maximum capacity
        while (queue.count == QUEUE_CAPACITY) {
            // Print waiting message atomically (no lock held during wait)
            flockfile(stdout);
            get_timestamp(timestamp, sizeof(timestamp));
            printf("[%s] [KITCHEN]  Queue is FULL (%d/%d). Waiting for delivery...\n",
                   timestamp, queue.count, QUEUE_CAPACITY);
            funlockfile(stdout);

            // Releases mutex and blocks until cond_not_full is signaled
            pthread_cond_wait(&queue.cond_not_full, &queue.mutex);
        }

        // Insert new order into circular buffer
        queue.buffer[queue.tail] = new_order;
        queue.tail = (queue.tail + 1) % QUEUE_CAPACITY;
        queue.count++;
        queue.orders_prepared++;

        // Print preparation message atomically (still inside mutex, but stdout is locked separately)
        flockfile(stdout);
        get_timestamp(timestamp, sizeof(timestamp));
        printf("[%s] [KITCHEN]  Prepared Order #%d | Queue Size: %d/%d\n",
               timestamp, new_order.order_id, queue.count, QUEUE_CAPACITY);
        funlockfile(stdout);

        // Signal consumer thread that a new item is available
        pthread_cond_signal(&queue.cond_not_empty);

        pthread_mutex_unlock(&queue.mutex);
        // --- Critical Section End ---
    }

    // Mark production as complete
    pthread_mutex_lock(&queue.mutex);
    queue.producer_done = true;
    // Broadcast to ensure waiting consumer thread exits cleanly
    pthread_cond_broadcast(&queue.cond_not_empty);
    pthread_mutex_unlock(&queue.mutex);

    return NULL;
}

// ------------------------------------------------------------
// Thread 2: Delivery (Consumer)
// ------------------------------------------------------------
void* delivery_thread(void *arg) {
    (void)arg;

    while (1) {
        Order current_order;
        char timestamp[10];

        // --- Critical Section Start ---
        pthread_mutex_lock(&queue.mutex);

        // Wait while the queue is empty AND kitchen is still active
        while (queue.count == 0 && !queue.producer_done) {
            flockfile(stdout);
            get_timestamp(timestamp, sizeof(timestamp));
            printf("[%s] [DELIVERY] Queue is EMPTY. Waiting for kitchen...\n", timestamp);
            funlockfile(stdout);

            // Releases mutex and blocks until cond_not_empty is signaled
            pthread_cond_wait(&queue.cond_not_empty, &queue.mutex);
        }

        // Check for exit condition: empty queue and kitchen is finished
        if (queue.count == 0 && queue.producer_done) {
            pthread_mutex_unlock(&queue.mutex);
            break; // Terminate consumer thread cleanly
        }

        // Remove order from circular buffer
        current_order = queue.buffer[queue.head];
        queue.head = (queue.head + 1) % QUEUE_CAPACITY;
        queue.count--;

        flockfile(stdout);
        get_timestamp(timestamp, sizeof(timestamp));
        printf("[%s] [DELIVERY] Picked up Order #%d | Queue Size: %d/%d\n",
               timestamp, current_order.order_id, queue.count, QUEUE_CAPACITY);
        funlockfile(stdout);

        // Signal producer thread that space is now available
        pthread_cond_signal(&queue.cond_not_full);

        pthread_mutex_unlock(&queue.mutex);
        // --- Critical Section End ---

        // Simulate delivery processing time (4 seconds) OUTSIDE critical section
        safe_sleep(DELIVERY_TIME_SEC);

        // ------------------------------------------------------------
        // NOTE: orders_delivered is incremented AFTER delivery completes,
        // which accurately reflects the real-world meaning of "delivered".
        // While the order is in transit, the monitor may report it as
        // already removed from the queue but not yet counted as delivered.
        // This is intentional and mirrors actual system behaviour.
        // ------------------------------------------------------------
        pthread_mutex_lock(&queue.mutex);
        queue.orders_delivered++;

        flockfile(stdout);
        get_timestamp(timestamp, sizeof(timestamp));
        printf("[%s] [DELIVERY] Completed Delivery for Order #%d\n",
               timestamp, current_order.order_id);
        funlockfile(stdout);

        pthread_mutex_unlock(&queue.mutex);
    }

    return NULL;
}

// ------------------------------------------------------------
// Thread 3: Monitoring Thread
// ------------------------------------------------------------
void* monitor_thread(void *arg) {
    (void)arg;

    while (1) {
        safe_sleep(MONITOR_INTERVAL_SEC);

        char timestamp[10];
        get_timestamp(timestamp, sizeof(timestamp));

        // Acquire lock to safely snapshot shared state
        pthread_mutex_lock(&queue.mutex);

        int prepared = queue.orders_prepared;
        int delivered = queue.orders_delivered;
        int current_size = queue.count;
        bool done = queue.producer_done && (current_size == 0) && (delivered == TOTAL_ORDERS);

        pthread_mutex_unlock(&queue.mutex);

        // Print monitoring report atomically (no risk of interleaving)
        flockfile(stdout);
        printf("\n==========================================");
        printf("\n [%s] SYSTEM MONITOR REPORT", timestamp);
        printf("\n   - Orders Prepared  : %d", prepared);
        printf("\n   - Orders Delivered : %d", delivered);
        printf("\n   - Current Queue Size: %d/%d", current_size, QUEUE_CAPACITY);
        printf("\n==========================================\n\n");
        funlockfile(stdout);

        if (done) {
            break; // Stop monitoring when work is done
        }
    }

    return NULL;
}

// ------------------------------------------------------------
// Main Function: Initializes resources, spawns threads, and cleans up
// ------------------------------------------------------------
int main(void) {
    pthread_t kitchen_tid, delivery_tid, monitor_tid;

    // Initialize Shared Queue State
    queue.head = 0;
    queue.tail = 0;
    queue.count = 0;
    queue.orders_prepared = 0;
    queue.orders_delivered = 0;
    queue.producer_done = false;

    // Initialize Mutex and Condition Variables
    if (pthread_mutex_init(&queue.mutex, NULL) != 0) {
        perror("Failed to initialize mutex");
        return 1;
    }
    if (pthread_cond_init(&queue.cond_not_full, NULL) != 0 ||
        pthread_cond_init(&queue.cond_not_empty, NULL) != 0) {
        perror("Failed to initialize condition variables");
        return 1;
    }

    // Print system startup banner (no other threads yet, no locking needed)
    printf("==========================================");
    printf("\n   STARTING ORDER PROCESSING SYSTEM");
    printf("\n   Target Total Orders: %d", TOTAL_ORDERS);
    printf("\n   Queue Capacity     : %d", QUEUE_CAPACITY);
    printf("\n==========================================\n\n");

    // Spawn Threads
    pthread_create(&kitchen_tid, NULL, kitchen_thread, NULL);
    pthread_create(&delivery_tid, NULL, delivery_thread, NULL);
    pthread_create(&monitor_tid, NULL, monitor_thread, NULL);

    // Wait for all threads to finish execution
    pthread_join(kitchen_tid, NULL);
    pthread_join(delivery_tid, NULL);
    pthread_join(monitor_tid, NULL);

    // Clean up Synchronization Primitives
    pthread_mutex_destroy(&queue.mutex);
    pthread_cond_destroy(&queue.cond_not_full);
    pthread_cond_destroy(&queue.cond_not_empty);

    // Final Summary Report
    printf("\n==========================================");
    printf("\n   SYSTEM EXECUTION COMPLETED CLEANLY");
    printf("\n   Total Prepared : %d", queue.orders_prepared);
    printf("\n   Total Delivered: %d", queue.orders_delivered);
    printf("\n==========================================\n");

    return 0;
}
