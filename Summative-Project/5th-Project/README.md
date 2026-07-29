# Concurrent TCP Client–Server Lab Equipment Booking System

## Communication Protocol
All messages use a fixed-size `Message` structure (common.h):
- `type`: enum (AUTH_REQ, AUTH_RSP, LIST_REQ, LIST_RSP, RSV_REQ, RSV_RSP, EXIT)
- `status`: 1 = success, 0 = failure
- `payload`: text up to 1023 characters + null terminator

Protocol flow:
1. Client sends AUTH_REQ with user ID.
2. Server replies AUTH_RSP (status 1/0 and message).
3. Client may request LIST_REQ → server returns LIST_RSP with equipment list.
4. Client sends RSV_REQ with equipment ID → server replies RSV_RSP (status and text).
5. Client sends EXIT to terminate.

## Authentication Process
- Server checks against a hardcoded list of valid user IDs.
- Duplicate logins are rejected.
- Server full (max 10 users) is also rejected.
- Once authenticated, further AUTH_REQ are ignored.

## Concurrency Approach
- One thread per client (`pthread_create` + `pthread_detach`).
- All shared data (equipment list, active users) protected by separate mutexes.

## Shared Resource Protection
- `eq_mutex` guards the equipment array.
- `users_mutex` guards the active user array.
- Mutexes are held for the shortest possible duration; no deadlock risks because the two mutexes are never held simultaneously.

## Client Session Management
- Client connects → authentication → interactive menu → graceful exit or timeout/disconnect.
- On disconnect, the thread removes the user from the active list and closes the socket.

## Error and Disconnection Handling
- Robust `send_all`/`recv_all` handle partial network reads/writes.
- Idle clients are dropped after 30 seconds (`SO_RCVTIMEO`).
- Server uses a signal flag for graceful shutdown on SIGINT.
- Server rejects reservation of already‑reserved equipment and invalid IDs.

## Build Instructions
Compile with GCC:
