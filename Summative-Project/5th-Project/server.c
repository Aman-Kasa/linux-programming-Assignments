#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <signal.h>
#include "common.h"

#define MAX_CLIENTS 10
#define MAX_EQUIPMENT 5

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

// Valid registered users for authentication
const char *valid_users[] = {"aman_a", "student01", "research_lab", "admin_user", "guest"};
const int num_valid_users = 5;

int server_socket;

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
    if (active_user_count == 0) printf("None");
    for (int i = 0; i < active_user_count; i++) printf("[%s] ", active_users[i]);
    printf("\nEquipment Status:\n");
    for (int i = 0; i < MAX_EQUIPMENT; i++) {
        printf("  %d: %s - %s\n", eq_list[i].id, eq_list[i].name, 
               eq_list[i].is_reserved ? "RESERVED" : "AVAILABLE");
    }
    printf("----------------------------\n\n");
}

void handle_shutdown(int sig) {
    printf("\n[SERVER] Shutting down gracefully...\n");
    close(server_socket);
    exit(0);
}

// --- Client Thread Handler ---
void *handle_client(void *client_socket_ptr) {
    int sock = *(int *)client_socket_ptr;
    free(client_socket_ptr);
    
    Message msg;
    char current_user[64] = {0};
    int authenticated = 0;

    while (recv(sock, &msg, sizeof(Message), 0) > 0) {
        Message response;
        memset(&response, 0, sizeof(Message));

        switch (msg.type) {
            case MSG_AUTH_REQ:
                for (int i = 0; i < num_valid_users; i++) {
                    if (strcmp(msg.payload, valid_users[i]) == 0) {
                        authenticated = 1;
                        strcpy(current_user, msg.payload);
                        
                        pthread_mutex_lock(&users_mutex);
                        strcpy(active_users[active_user_count++], current_user);
                        pthread_mutex_unlock(&users_mutex);
                        
                        response.status = 1;
                        strcpy(response.payload, "Authentication successful.");
                        printf("[SERVER] User authenticated: %s\n", current_user);
                        print_server_state();
                        break;
                    }
                }
                if (!authenticated) {
                    response.status = 0;
                    strcpy(response.payload, "Invalid User ID.");
                    printf("[SERVER] Failed auth attempt for ID: %s\n", msg.payload);
                }
                response.type = MSG_AUTH_RSP;
                send(sock, &response, sizeof(Message), 0);
                break;

            case MSG_LIST_REQ:
                if (!authenticated) continue;
                response.type = MSG_LIST_RSP;
                response.status = 1;
                
                pthread_mutex_lock(&eq_mutex);
                for (int i = 0; i < MAX_EQUIPMENT; i++) {
                    char line[128];
                    snprintf(line, sizeof(line), "ID: %d | %-15s | Status: %s%s\n", 
                             eq_list[i].id, eq_list[i].name, 
                             eq_list[i].is_reserved ? "RESERVED by " : "AVAILABLE",
                             eq_list[i].is_reserved ? eq_list[i].reserved_by : "");
                    strcat(response.payload, line);
                }
                pthread_mutex_unlock(&eq_mutex);
                send(sock, &response, sizeof(Message), 0);
                break;

            case MSG_RSV_REQ:
                if (!authenticated) continue;
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
                            snprintf(response.payload, sizeof(response.payload), "Successfully reserved %s.", eq_list[i].name);
                            printf("[SERVER] %s reserved %s.\n", current_user, eq_list[i].name);
                        } else {
                            response.status = 0;
                            snprintf(response.payload, sizeof(response.payload), "Item %s is already reserved.", eq_list[i].name);
                        }
                        break;
                    }
                }
                pthread_mutex_unlock(&eq_mutex);
                
                if (!found) {
                    response.status = 0;
                    strcpy(response.payload, "Invalid Equipment ID.");
                }
                send(sock, &response, sizeof(Message), 0);
                print_server_state();
                break;

            case MSG_EXIT:
                printf("[SERVER] User %s requested disconnect.\n", current_user);
                goto cleanup;
                
            default:
                break;
        }
    }

cleanup:
    if (authenticated) {
        pthread_mutex_lock(&users_mutex);
        for (int i = 0; i < active_user_count; i++) {
            if (strcmp(active_users[i], current_user) == 0) {
                // Shift array to remove user
                for (int j = i; j < active_user_count - 1; j++) {
                    strcpy(active_users[j], active_users[j + 1]);
                }
                active_user_count--;
                break;
            }
        }
        pthread_mutex_unlock(&users_mutex);
        printf("[SERVER] Client disconnected gracefully: %s\n", current_user);
        print_server_state();
    }
    close(sock);
    pthread_exit(NULL);
}

int main() {
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);
    
    signal(SIGINT, handle_shutdown); // Graceful shutdown on Ctrl+C
    init_equipment();

    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1) {
        perror("Socket creation failed");
        exit(1);
    }

    // Prevent "Address already in use" errors during rapid restart testing
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

    while (1) {
        int *new_sock = malloc(sizeof(int));
        *new_sock = accept(server_socket, (struct sockaddr *)&client_addr, &client_len);
        
        if (*new_sock < 0) {
            perror("Accept failed");
            free(new_sock);
            continue;
        }

        pthread_t client_thread;
        if (pthread_create(&client_thread, NULL, handle_client, (void *)new_sock) < 0) {
            perror("Could not create thread");
            free(new_sock);
            continue;
        }
        
        // Detach thread to automatically reclaim resources when it finishes
        pthread_detach(client_thread);
    }

    close(server_socket);
    return 0;
}
