#include <stdio.h>
#include <stdlib.h>

int main() {
    // 1. File Creation & Log Writing
    FILE *fp = fopen("runtime_log.txt", "w");
    if (fp == NULL) {
        perror("Failed to create file");
        return 1;
    }
    fprintf(fp, "Log Entry: System execution monitored successfully.\n");
    fclose(fp);

    // 2. Reading Files
    fp = fopen("runtime_log.txt", "r");
    if (fp == NULL) {
        perror("Failed to open file for reading");
        return 1;
    }
    int ch;
    while ((ch = fgetc(fp)) != EOF) {
        // Continuous reading until end of file
    }
    fclose(fp);

    return 0;
}
