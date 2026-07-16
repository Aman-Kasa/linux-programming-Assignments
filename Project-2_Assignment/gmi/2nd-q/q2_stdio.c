#include <stdio.h>
#include <stdlib.h>

#define BUF_SIZE 4096

int main() {
    FILE *fin = fopen("largefile.bin", "rb");
    FILE *fout = fopen("copy_stdio.bin", "wb");
    
    if (!fin || !fout) {
        perror("File open error");
        exit(1);
    }

    char buffer[BUF_SIZE];
    size_t bytes_read;
    
    while ((bytes_read = fread(buffer, 1, BUF_SIZE, fin)) > 0) {
        fwrite(buffer, 1, bytes_read, fout);
    }

    fclose(fin);
    fclose(fout);
    return 0;
}
