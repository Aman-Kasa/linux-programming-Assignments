#ifndef COMMON_H
#define COMMON_H

#define PORT 8888
#define MAX_PAYLOAD 1024

// Structured message types for reliable communication
typedef enum {
    MSG_AUTH_REQ,
    MSG_AUTH_RSP,
    MSG_LIST_REQ,
    MSG_LIST_RSP,
    MSG_RSV_REQ,
    MSG_RSV_RSP,
    MSG_EXIT
} MsgType;

// Standardized message envelope
typedef struct {
    MsgType type;
    int status; // 1 for success, 0 for failure
    char payload[MAX_PAYLOAD];
} Message;

#endif
