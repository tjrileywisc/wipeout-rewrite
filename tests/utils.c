
#include "utils.h"

#include <network.h>
#include <client_com.h>

int network_test_cleanup(void** state) {
    (void)state; // unused
    // Reset the network state after each test
    network_clear_msg_queue();
    network_set_bound_ip_socket(INVALID_SOCKET);

    return 0;
}

int server_test_cleanup(void** state) {
    (void)state; // unused
    // Reset the server state after each test
    server_set_connected_clients_count(0);
    return 0;
}