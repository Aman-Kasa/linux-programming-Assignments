#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include "common.h"

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
    scanf("%63s", user_id);

    // Authentication
    msg.type = MSG_AUTH_REQ;
    strcpy(msg.payload, user_id);
    send(sock, &msg, sizeof(Message), 0);
    
    recv(sock, &response, sizeof(Message), 0);
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
            continue;
        }

        memset(&msg, 0, sizeof(Message));
        
        if (choice == 1) {
            msg.type = MSG_LIST_REQ;
            send(sock, &msg, sizeof(Message), 0);
            recv(sock, &response, sizeof(Message), 0);
            printf("\n--- Equipment List ---\n%s----------------------\n", response.payload);
        } 
        else if (choice == 2) {
            int eq_id;
            printf("Enter Equipment ID to reserve: ");
            scanf("%d", &eq_id);
            
            msg.type = MSG_RSV_REQ;
            snprintf(msg.payload, sizeof(msg.payload), "%d", eq_id);
            send(sock, &msg, sizeof(Message), 0);
            
            recv(sock, &response, sizeof(Message), 0);
            if (response.status == 1) {
                printf("\n[SUCCESS] %s\n", response.payload);
            } else {
                printf("\n[DENIED] %s\n", response.payload);
            }
        } 
        else if (choice == 3) {
            msg.type = MSG_EXIT;
            send(sock, &msg, sizeof(Message), 0);
            printf("\nSession closed. Goodbye, %s\n", user_id);
            break;
        }
    }

    close(sock);
    return 0;
}
