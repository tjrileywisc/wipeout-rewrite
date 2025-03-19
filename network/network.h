
#ifndef NETWORK_H
#define NETWORK_H

#include "network_types.h"

void network_open_ip();

void network_send_packet(int length, void* data, netadr_t dest_net);

#endif