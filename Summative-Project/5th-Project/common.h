#ifndef COMMON_H
#define COMMON_H

#define PORT 8888
#define MAX_PAYLOAD 1024

typedef enum {
    MSG_AUTH_REQ,
    MSG_AUTH_RSP,
    MSG_LIST_REQ,
    MSG_LIST_RSP,
    MSG_RSV_REQ,
    MSG_RSV_RSP,
    MSG_EXIT
} MsgType;

typedef struct {
    MsgType type;
    int status;               // 1 = success, 0 = failure
    char payload[MAX_PAYLOAD];
} Message;

#endif
