#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

// Global shared variables
char *keyword;
char *output_file;
char **files;
int num_files;
int current_file_index = 0;

// Mutexes
pthread_mutex_t queue_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t file_mutex = PTHREAD_MUTEX_INITIALIZER;

void* search_files(void* arg) {
    while (1) {
        int my_index;
        
        // Lock queue to get the next file index
        pthread_mutex_lock(&queue_mutex);
        if (current_file_index >= num_files) {
            pthread_mutex_unlock(&queue_mutex);
            break; // No more files to process
        }
        my_index = current_file_index;
        current_file_index++;
        pthread_mutex_unlock(&queue_mutex);

        // Process the file
        FILE *fp = fopen(files[my_index], "r");
        if (!fp) continue;

        int count = 0;
        char buffer[1024];
        while (fscanf(fp, "%1023s", buffer) == 1) {
            if (strcmp(buffer, keyword) == 0) {
                count++;
            }
        }
        fclose(fp);

        // Write results safely to shared output
        pthread_mutex_lock(&file_mutex);
        FILE *out_fp = fopen(output_file, "a");
        if (out_fp) {
            fprintf(out_fp, "Thread %ld found %d occurrences in %s\n", 
                    pthread_self(), count, files[my_index]);
            fclose(out_fp);
        }
        pthread_mutex_unlock(&file_mutex);
    }
    pthread_exit(NULL);
}

int main(int argc, char *argv[]) {
    if (argc < 5) {
        printf("Usage: %s <keyword> <output.txt> <file1.txt> ... <num_threads>\n", argv[0]);
        return 1;
    }

    keyword = argv[1];
    output_file = argv[2];
    int num_threads = atoi(argv[argc - 1]);
    
    files = &argv[3];
    num_files = argc - 4; // Total args - keyword, output, executable, num_threads

    // Clear output file
    FILE *clear_fp = fopen(output_file, "w");
    if (clear_fp) fclose(clear_fp);

    pthread_t *threads = malloc(num_threads * sizeof(pthread_t));

    for (int i = 0; i < num_threads; i++) {
        pthread_create(&threads[i], NULL, search_files, NULL);
    }

    for (int i = 0; i < num_threads; i++) {
        pthread_join(threads[i], NULL);
    }

    free(threads);
    return 0;
}
