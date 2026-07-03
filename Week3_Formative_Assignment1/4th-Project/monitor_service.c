#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>

volatile sig_atomic_t running = 1;
volatile sig_atomic_t status_request = 0;

/*
 * Signal handler:
 * ONLY sets flags (safe async-signal behavior)
 */
void handle_signal(int sig) {
    if (sig == SIGINT) {
        running = 0;  // graceful shutdown
    } 
    else if (sig == SIGUSR1) {
        status_request = 1;  // request status print in main loop
    } 
    else if (sig == SIGTERM) {
        running = 0;  // shutdown triggered
    }
}

int main() {
    struct sigaction sa;

    sa.sa_handler = handle_signal;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    if (sigaction(SIGINT, &sa, NULL) == -1) {
        perror("sigaction SIGINT failed");
        exit(1);
    }
    if (sigaction(SIGUSR1, &sa, NULL) == -1) {
        perror("sigaction SIGUSR1 failed");
        exit(1);
    }
    if (sigaction(SIGTERM, &sa, NULL) == -1) {
        perror("sigaction SIGTERM failed");
        exit(1);
    }

    printf("Monitor Service initiated. Process ID (PID): %d\n", getpid());
    printf("Awaiting administrative signals...\n");

    while (running) {

        printf("[Monitor Service] System running normally...\n");
        sleep(5);

        if (status_request) {
            printf("System status requested by administrator.\n");
            status_request = 0;
        }
    }

    if (running == 0) {
        printf("Monitor Service shutting down safely.\n");
    }

    return 0;
}
