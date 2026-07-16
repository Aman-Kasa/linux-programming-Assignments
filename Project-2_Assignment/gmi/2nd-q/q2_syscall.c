#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

#define BUF_SIZE 4096 // Standard page size

int main() {
    int fd_in = open("largefile.bin", O_RDONLY);
    int fd_out = open("copy_syscall.bin", O_WRONLY | O_CREAT | O_TRUNC, 0644);
    
    if (fd_in < 0 || fd_out < 0) {
        perror("File open error");
        exit(1);
    }

    char buffer[BUF_SIZE];
    ssize_t bytes_read;
    
    while ((bytes_read = read(fd_in, buffer, BUF_SIZE)) > 0) {
        write(fd_out, buffer, bytes_read);
    }

    close(fd_in);
    close(fd_out);
    return 0;
}
