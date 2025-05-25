
#pragma once

#include "network_types.h"

#if defined(WIN32)
#include <WinSock2.h>
#include <win_defines.h>
#else
#include <sys/socket.h>
#endif

static int WIPEOUT_PORT = 8000;

typedef struct {
    const char* command;
    struct sockaddr_storage dest_addr;
} msg_queue_item_t;

/**
 * @brief Check if the current socket is valid and bound
 * 
 * @return true if the socket is valid and bound
 */
bool network_has_bound_ip_socket(void);

/**
 * @brief Set a bound ip socket; really
 * should only be used for testing
 * 
 * @param sockfd the socket fd
 */
void network_set_bound_ip_socket(int sockfd);

/**
 * @brief Get the currently bound ip socket;
 * callers should check if the socket is valid first
 * 
 * @return the socket fd
 */
int network_get_bound_ip_socket(void);

/**
 * @brief Gets a socket for network communication;
 * this UDP socket is not yet bound to an address, and
 * is non-blocking and broadcastable. If a socket
 * could not be created, -1 is returned.
 * 
 * @return int 
 */
int network_get_socket(void);

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

bool network_get_packet(void);

void network_send_packet(netsrc_t sock, int length, const void* data, netadr_t dest_net);

/**
 * @brief Send a command to the server or to a client
 */
void network_send_command(const char* command, netadr_t dest);

void network_process_command(const char* command);

/**
 * @brief clears the current message queue
 */
void network_clear_msg_queue(void);

/**
 * @brief gets the size of the message queue
 */
int network_get_msg_queue_size(void);

/**
 * @brief pop out the first message queue item
 */
void network_popleft_msg_queue(void);

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

/**
 * @brief get the local subnet for this network participant
 * 
 * @param[in,out] subnet the subnet address
 * @param len the length of the subnet address
 */
void network_get_my_ip(char* subnet, size_t len);
