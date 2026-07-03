#include <stdio.h>
#include <stdlib.h>

void sync_files(const char *src, const char *dest) {
    FILE *s = fopen(src, "rb");
    FILE *d = fopen(dest, "wb");
    if (!s || !d) {
        printf("Error opening files for synchronization.\n");
        return;
    }
    char buffer[1024];
    size_t bytes;
    while ((bytes = fread(buffer, 1, sizeof(buffer), s)) > 0) {
        fwrite(buffer, 1, bytes, d);
    }
    fclose(s);
    fclose(d);
    printf("Synchronization complete.\n");
}

int main() {
    printf("Starting data synchronization tool...\n");
    sync_files("source.dat", "backup.dat");
    return 0;
}
