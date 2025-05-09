
#pragma once

#include "network_types.h"

bool network_has_ip_socket(void);

/**
 * Bind to a port for running in a
 * server context
 */
void network_bind_ip(void);

/**
 * Connect to any port for running
 * in a client context
 */
void network_connect_ip(const char* addr);

void network_close_connection(void);

bool network_get_packet();

void network_send_packet(netsrc_t sock, int length, const void* data, netadr_t dest_net);

/**
 * Send a command to the server or to a client
 */
void network_send_command(const char* command, netadr_t dest);

void network_process_command(const char* command);

/**
 * sleep for a duration, or until we have socket activity
 * 
 * @param msec duration in milliseconds
 * @return 0 if network is not available, or 1 if so
 */
int network_sleep(int msec);
