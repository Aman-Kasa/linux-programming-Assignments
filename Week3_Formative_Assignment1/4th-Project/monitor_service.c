#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>

// Unified signal routing handler function
void handle_signal(int sig) {
    if (sig == SIGINT) {
        printf("\n[Monitor Service] Monitor Service shutting down safely.\n");
        exit(0);
    } else if (sig == SIGUSR1) {
        printf("[Monitor Service] System status requested by administrator.\n");
    } else if (sig == SIGTERM) {
        printf("[Monitor Service] Emergency shutdown signal received.\n");
        exit(1);
    }
}

int main() {
    struct sigaction sa;
    sa.sa_handler = handle_signal;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    // Registering the core signal actions 
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGUSR1, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);

    printf("Monitor Service initiated. Process ID (PID): %d\n", getpid());
    printf("Awaiting administrative signals...\n");

    // Continuous server processing loop
    while (1) {
        printf("[Monitor Service] System running normally...\n");
        sleep(5);
    }

    return 0;
}
