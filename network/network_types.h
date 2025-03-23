
#ifndef NETWORK_TYPES_H
#define NETWORK_TYPES_H

#include <stdbool.h>

typedef unsigned char byte;

#define MAX_STRING_CHARS 1024

typedef struct {
    byte ip[4];
    unsigned short port;
} netadr_t;

typedef struct {
    int cursize;
    int readcount;
    int bit;
    bool oob;
    byte* data;
} msg_t;

typedef enum {
    CLIENT,
    SERVER
} netsrc_t;

#endif