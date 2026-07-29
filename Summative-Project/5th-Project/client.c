#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <errno.h>
#include "common.h"

// Robust send/recv wrappers (same logic as server)
static int send_all(int sock, const void *buf, size_t len) {
    const char *ptr = (const char *)buf;
    size_t remaining = len;
    while (remaining > 0) {
        ssize_t n = send(sock, ptr, remaining, 0);
        if (n <= 0) return -1;
        remaining -= n;
        ptr += n;
    }
    return 0;
}

static int recv_all(int sock, void *buf, size_t len) {
    char *ptr = (char *)buf;
    size_t remaining = len;
    while (remaining > 0) {
        ssize_t n = recv(sock, ptr, remaining, 0);
        if (n == 0) return 0;
        if (n < 0) {
            if (errno == EINTR) continue;
            return -1;
        }
        remaining -= n;
        ptr += n;
    }
    return 1;
}

void clear_stdin() {
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
}

int main() {
    int sock;
    struct sockaddr_in server_addr;
    Message msg, response;
    char user_id[64];

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1) {
        perror("Socket creation failed");
        exit(1);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    server_addr.sin_port = htons(PORT);

    if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connection failed");
        exit(1);
    }

    printf("--- University Lab Booking System ---\n");
    printf("Enter User ID: ");
    if (scanf("%63s", user_id) != 1) {
        fprintf(stderr, "Input error\n");
        close(sock);
        return 1;
    }
    clear_stdin();  // remove leftover newline

    // Authentication
    memset(&msg, 0, sizeof(msg));
    msg.type = MSG_AUTH_REQ;
    strncpy(msg.payload, user_id, MAX_PAYLOAD - 1);
    if (send_all(sock, &msg, sizeof(Message)) != 0) {
        perror("send failed");
        close(sock);
        return 1;
    }

    if (recv_all(sock, &response, sizeof(Message)) <= 0) {
        printf("Server closed connection.\n");
        close(sock);
        return 1;
    }

    if (response.status == 0) {
        printf("Authentication failed: %s\n", response.payload);
        close(sock);
        return 1;
    }

    printf("\n[%s] %s\n", user_id, response.payload);

    int choice;
    while (1) {
        printf("\n1. View Available Equipment\n");
        printf("2. Request Reservation\n");
        printf("3. Exit Session\n");
        printf("Select option: ");

        if (scanf("%d", &choice) != 1) {
            clear_stdin();
            printf("Invalid input. Try again.\n");
            continue;
        }
        clear_stdin();

        memset(&msg, 0, sizeof(msg));

        if (choice == 1) {
            msg.type = MSG_LIST_REQ;
            send_all(sock, &msg, sizeof(Message));
            if (recv_all(sock, &response, sizeof(Message)) <= 0) break;
            printf("\n--- Equipment List ---\n%s----------------------\n", response.payload);
        }
        else if (choice == 2) {
            int eq_id;
            printf("Enter Equipment ID to reserve: ");
            if (scanf("%d", &eq_id) != 1) {
                clear_stdin();
                printf("Invalid ID.\n");
                continue;
            }
            clear_stdin();

            msg.type = MSG_RSV_REQ;
            snprintf(msg.payload, sizeof(msg.payload), "%d", eq_id);
            send_all(sock, &msg, sizeof(Message));

            if (recv_all(sock, &response, sizeof(Message)) <= 0) break;
            if (response.status == 1) {
                printf("\n[SUCCESS] %s\n", response.payload);
            } else {
                printf("\n[DENIED] %s\n", response.payload);
            }
        }
        else if (choice == 3) {
            msg.type = MSG_EXIT;
            send_all(sock, &msg, sizeof(Message));
            printf("\nSession closed. Goodbye, %s\n", user_id);
            break;
        }
        else {
            printf("Invalid choice.\n");
        }
    }

    close(sock);
    return 0;
}
