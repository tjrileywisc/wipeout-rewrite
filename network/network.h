
#pragma once

#include "network_types.h"
#include <sys/socket.h>


typedef struct {
    const char* command;
    struct sockaddr_storage dest_addr;
} msg_queue_item_t;

bool network_has_ip_socket(void);

void network_set_ip_socket(int sockfd);

/**
 * @brief Bind to a port for running in a
 * server context
 */
void network_bind_ip(void);

/**
 * @brief Connect to any port for running
 * in a client context
 */
void network_connect_ip(const char* addr);

void network_close_connection(void);

bool network_get_packet();

void network_send_packet(netsrc_t sock, int length, const void* data, netadr_t dest_net);

/**
 * @brief Send a command to the server or to a client
 */
void network_send_command(const char* command, netadr_t dest);

void network_process_command(const char* command);

int network_get_msg_queue_size(void);

/**
 * @brief get the first message queue item
 * 
 * @param item pointer to the item
 * 
 * @return true if there is an item, false otherwise
 */
bool network_get_msg_queue_item(msg_queue_item_t *item);

/**
 * @brief sleep for a duration, or until we have socket activity
 * 
 * @param msec duration in milliseconds
 * @return 0 if network is not available, or 1 if so
 */
int network_sleep(int msec);
