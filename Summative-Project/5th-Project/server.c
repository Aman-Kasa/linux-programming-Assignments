#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <signal.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <poll.h>
#include "common.h"

#define MAX_CLIENTS 10
#define MAX_EQUIPMENT 5
#define CLIENT_TIMEOUT_SEC 30

// --- Shared Data Structures ---
typedef struct {
    int id;
    char name[64];
    int is_reserved;
    char reserved_by[64];
} Equipment;

Equipment eq_list[MAX_EQUIPMENT];
pthread_mutex_t eq_mutex = PTHREAD_MUTEX_INITIALIZER;

char active_users[MAX_CLIENTS][64];
int active_user_count = 0;
pthread_mutex_t users_mutex = PTHREAD_MUTEX_INITIALIZER;

const char *valid_users[] = {"x", "y", "z", "aman", "thadee"};
const int num_valid_users = 5;

// Graceful shutdown flag
volatile sig_atomic_t shutdown_flag = 0;

int server_socket;

// --- Robust send/recv wrappers (handle partial transfers) ---
static int send_all(int sock, const void *buf, size_t len) {
    const char *ptr = (const char *)buf;
    size_t remaining = len;
    while (remaining > 0) {
        ssize_t n = send(sock, ptr, remaining, 0);
        if (n <= 0) return -1;   // error or closed
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
        if (n == 0) return 0;          // connection closed
        if (n < 0) {
            if (errno == EINTR) continue;
            if (errno == EAGAIN || errno == EWOULDBLOCK) return -1; // timeout
            return -1;                 // real error
        }
        remaining -= n;
        ptr += n;
    }
    return 1; // success
}

// --- Helper Functions ---
void init_equipment() {
    char *names[] = {"Oscilloscope", "HackRF One", "Microscope", "Logic Analyzer", "Spectrometer"};
    for (int i = 0; i < MAX_EQUIPMENT; i++) {
        eq_list[i].id = i + 1;
        strcpy(eq_list[i].name, names[i]);
        eq_list[i].is_reserved = 0;
        memset(eq_list[i].reserved_by, 0, 64);
    }
}

void print_server_state() {
    printf("\n--- CURRENT SERVER STATE ---\n");
    printf("Active Users: ");
    pthread_mutex_lock(&users_mutex);
    if (active_user_count == 0) printf("None");
    for (int i = 0; i < active_user_count; i++) printf("[%s] ", active_users[i]);
    pthread_mutex_unlock(&users_mutex);

    printf("\nEquipment Status:\n");
    pthread_mutex_lock(&eq_mutex);
    for (int i = 0; i < MAX_EQUIPMENT; i++) {
        printf("  %d: %s - %s\n", eq_list[i].id, eq_list[i].name,
               eq_list[i].is_reserved ? "RESERVED" : "AVAILABLE");
    }
    pthread_mutex_unlock(&eq_mutex);
    printf("----------------------------\n\n");
}

void handle_signal(int sig) {
    shutdown_flag = 1;   // async-signal-safe: only set flag
}

// --- Client Thread Handler ---
void *handle_client(void *client_socket_ptr) {
    int sock = *(int *)client_socket_ptr;
    free(client_socket_ptr);

    // Set receive timeout to drop idle clients
    struct timeval tv;
    tv.tv_sec = CLIENT_TIMEOUT_SEC;
    tv.tv_usec = 0;
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    Message msg;
    char current_user[64] = {0};
    int authenticated = 0;

    while (1) {
        int ret = recv_all(sock, &msg, sizeof(Message));
        if (ret <= 0) {
            if (ret == 0)
                printf("[SERVER] Client disconnected.\n");
            else if (ret == -1 && (errno == EAGAIN || errno == EWOULDBLOCK))
                printf("[SERVER] Client %s timed out.\n", current_user[0] ? current_user : "unknown");
            else
                perror("recv error");
            break;
        }

        Message response;
        memset(&response, 0, sizeof(Message));

        switch (msg.type) {
            case MSG_AUTH_REQ:
                // Ignore re-authentication attempts
                if (authenticated) {
                    response.type = MSG_AUTH_RSP;
                    response.status = 0;
                    strcpy(response.payload, "Already authenticated.");
                    send_all(sock, &response, sizeof(Message));
                    break;
                }

                // Check credentials
                for (int i = 0; i < num_valid_users; i++) {
                    if (strcmp(msg.payload, valid_users[i]) == 0) {
                        // Prevent duplicate logins
                        pthread_mutex_lock(&users_mutex);
                        int dup = 0;
                        for (int j = 0; j < active_user_count; j++) {
                            if (strcmp(active_users[j], msg.payload) == 0) {
                                dup = 1;
                                break;
                            }
                        }

                        if (dup) {
                            pthread_mutex_unlock(&users_mutex);
                            response.status = 0;
                            strcpy(response.payload, "User already logged in elsewhere.");
                            send_all(sock, &response, sizeof(Message));
                            break;
                        }

                        // Server full?
                        if (active_user_count >= MAX_CLIENTS) {
                            pthread_mutex_unlock(&users_mutex);
                            response.status = 0;
                            strcpy(response.payload, "Server full. Try again later.");
                            send_all(sock, &response, sizeof(Message));
                            break;
                        }

                        // All checks passed – add user
                        strcpy(active_users[active_user_count++], msg.payload);
                        pthread_mutex_unlock(&users_mutex);

                        authenticated = 1;
                        strcpy(current_user, msg.payload);

                        response.status = 1;
                        strcpy(response.payload, "Authentication successful.");
                        response.type = MSG_AUTH_RSP;
                        send_all(sock, &response, sizeof(Message));
                        printf("[SERVER] User authenticated: %s\n", current_user);
                        print_server_state();
                        break;
                    }
                }
                if (!authenticated) {
                    response.type = MSG_AUTH_RSP;
                    response.status = 0;
                    strcpy(response.payload, "Invalid User ID.");
                    send_all(sock, &response, sizeof(Message));
                    printf("[SERVER] Failed auth attempt for ID: %s\n", msg.payload);
                }
                break;

            case MSG_LIST_REQ:
                if (!authenticated) {
                    // Ignore unauthenticated requests; client should not send these
                    break;
                }
                response.type = MSG_LIST_RSP;
                response.status = 1;

                pthread_mutex_lock(&eq_mutex);
                for (int i = 0; i < MAX_EQUIPMENT; i++) {
                    char line[256];   // larger buffer to safely hold any possible combination
                    snprintf(line, sizeof(line), "ID: %d | %-15s | Status: %s%.63s\n",
                             eq_list[i].id, eq_list[i].name,
                             eq_list[i].is_reserved ? "RESERVED by " : "AVAILABLE",
                             eq_list[i].is_reserved ? eq_list[i].reserved_by : "");
                    strcat(response.payload, line);
                }
                pthread_mutex_unlock(&eq_mutex);
                send_all(sock, &response, sizeof(Message));
                break;

            case MSG_RSV_REQ:
                if (!authenticated) break;
                response.type = MSG_RSV_RSP;
                int req_id = atoi(msg.payload);
                int found = 0;

                pthread_mutex_lock(&eq_mutex);
                for (int i = 0; i < MAX_EQUIPMENT; i++) {
                    if (eq_list[i].id == req_id) {
                        found = 1;
                        if (!eq_list[i].is_reserved) {
                            eq_list[i].is_reserved = 1;
                            strcpy(eq_list[i].reserved_by, current_user);
                            response.status = 1;
                            snprintf(response.payload, sizeof(response.payload),
                                     "Successfully reserved %s.", eq_list[i].name);
                            printf("[SERVER] %s reserved %s.\n", current_user, eq_list[i].name);
                        } else {
                            response.status = 0;
                            snprintf(response.payload, sizeof(response.payload),
                                     "Item %s is already reserved by %s.",
                                     eq_list[i].name, eq_list[i].reserved_by);
                        }
                        break;
                    }
                }
                pthread_mutex_unlock(&eq_mutex);

                if (!found) {
                    response.status = 0;
                    strcpy(response.payload, "Invalid Equipment ID.");
                }
                send_all(sock, &response, sizeof(Message));
                print_server_state();
                break;

            case MSG_EXIT:
                printf("[SERVER] User %s requested disconnect.\n", current_user[0] ? current_user : "unknown");
                goto cleanup;

            default:
                // Unknown message type – ignore
                break;
        }
    }

cleanup:
    if (authenticated) {
        pthread_mutex_lock(&users_mutex);
        for (int i = 0; i < active_user_count; i++) {
            if (strcmp(active_users[i], current_user) == 0) {
                for (int j = i; j < active_user_count - 1; j++) {
                    strcpy(active_users[j], active_users[j + 1]);
                }
                active_user_count--;
                break;
            }
        }
        pthread_mutex_unlock(&users_mutex);
        printf("[SERVER] Client disconnected: %s\n", current_user);
        print_server_state();
    }
    close(sock);
    pthread_exit(NULL);
}

int main() {
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);

    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);
    init_equipment();

    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1) {
        perror("Socket creation failed");
        exit(1);
    }

    int opt = 1;
    setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        exit(1);
    }

    if (listen(server_socket, 5) < 0) {
        perror("Listen failed");
        exit(1);
    }

    printf("[SERVER] Lab Booking Server running on port %d...\n", PORT);

    // Use poll() to periodically check shutdown_flag
    struct pollfd pfd;
    pfd.fd = server_socket;
    pfd.events = POLLIN;

    while (!shutdown_flag) {
        int ready = poll(&pfd, 1, 1000);  // 1-second timeout

        if (ready < 0) {
            if (errno == EINTR) continue; // interrupted by a signal
            perror("poll error");
            break;
        }

        if (ready == 0)   // timeout, just re-check shutdown_flag
            continue;

        // There is an incoming connection
        int *new_sock = malloc(sizeof(int));
        *new_sock = accept(server_socket, (struct sockaddr *)&client_addr, &client_len);

        if (*new_sock < 0) {
            if (shutdown_flag) {
                free(new_sock);
                break;
            }
            perror("Accept failed");
            free(new_sock);
            continue;
        }

        pthread_t client_thread;
        if (pthread_create(&client_thread, NULL, handle_client, (void *)new_sock) < 0) {
            perror("Could not create thread");
            close(*new_sock);
            free(new_sock);
            continue;
        }
        pthread_detach(client_thread);
    }

    printf("\n[SERVER] Shutting down...\n");
    close(server_socket);
    return 0;
}
