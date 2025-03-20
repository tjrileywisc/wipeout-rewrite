
#ifndef NETWORK_H
#define NETWORK_H

#include "network_types.h"

void network_open_ip();

void network_out_of_band_print(netsrc_t sock, netadr_t adr, const char* format, ... );

void network_send_packet(netsrc_t sock, int length, const void* data, netadr_t dest_net);

#endif