
#ifndef NETWORK_TYPES_H
#define NETWORK_TYPES_H

typedef unsigned char byte;

typedef struct {
    byte ip[4];
    unsigned short port;
} netadr_t;

typedef enum {
    CLIENT,
    SERVER
} netsrc_t;

#endif