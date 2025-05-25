
#include "server_com.h"

#include "network_wrapper.h"

#include <addr_conversions.h>
#include <network.h>

#if defined(WIN32)
#include <ws2ipdef.h>
#include <WS2tcpip.h>
#else
#include <arpa/inet.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdatomic.h>
#include <threads.h>

atomic_bool network_discovery_on = false;

thrd_t network_discovery_thread;

void client_init() {

    char my_ip[INET_ADDRSTRLEN];
    network_get_my_ip(my_ip, INET_ADDRSTRLEN);
    network_connect_ip(my_ip);
}

/**
 * @brief Run network discovery to find servers
 */
static int page_network_query(void*) {

	if(!network_discovery_on) {
		return 0;
	}

	if(!network_has_bound_ip_socket()) {
		return 0;
	}

	// TODO: should spend 30 seconds trying to find servers
	// on the current interface (LAN + localhost, or internet)

	netadr_t dest;
	string_to_addr("localhost", &dest);
	dest.port = htons(WIPEOUT_PORT);

	const char* data = "hello";
	network_send_packet(CLIENT, strlen(data), data, dest);

	if(network_sleep(1000) > 0) {
		network_get_packet();
	}

	return 0;
}

/**
 * @brief Run network discovery to find servers
 */
static int server_com_network_discovery() {

    if(!network_discovery_on) {
		return 0;
	}

	if(!network_has_bound_ip_socket()) {
		return 0;
	}

    int sockfd = network_get_bound_ip_socket();

    // Enable broadcast option
    int broadcastEnable = 1;
    if(setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, &broadcastEnable, sizeof(broadcastEnable)) != 0) {
        perror("[-] Error setting socket to broadcast");
        return 0;
    }

    // Set receive timeout
    struct timeval timeout;
    timeout.tv_sec = 30;
    timeout.tv_usec = 0;
    if(setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) != 0) {
        perror("[-] Error setting socket timeout");
        return 0;
    }

    struct sockaddr_in broadcast_addr;
    memset(&broadcast_addr, 0, sizeof(broadcast_addr));
    broadcast_addr.sin_family = AF_INET;
    broadcast_addr.sin_port = htons(WIPEOUT_PORT);
    char my_ip[INET_ADDRSTRLEN];
    network_get_my_ip(my_ip, INET_ADDRSTRLEN);

    char broadcast_ip[INET_ADDRSTRLEN];
    char* last_dot = strrchr(my_ip, '.');
    if (last_dot != NULL) {
        strncpy(broadcast_ip, my_ip, last_dot - my_ip);
        broadcast_ip[last_dot - my_ip] = '\0';
        strcat(broadcast_ip, ".255");
    } else {
        fprintf(stderr, "[-] Invalid ip format.\n");
        return 0;
    }

    inet_pton(AF_INET, broadcast_ip, &broadcast_addr.sin_addr);

    const char *message = "hello";
    sendto(sockfd, message, strlen(message), 0,
           (struct sockaddr *)&broadcast_addr, sizeof(broadcast_addr));

    printf("[*] Broadcast packet sent. Waiting for replies...\n");

    static int BUF_SIZE = 10;
    char buffer[BUF_SIZE];
    while (1) {
        struct sockaddr_in sender;
        socklen_t sender_len = sizeof(sender);
        ssize_t n = wrap_recvfrom(sockfd, buffer, BUF_SIZE - 1, 0,
                             (struct sockaddr *)&sender, &sender_len);

        if (n < 0) {
            printf("[-] Timeout reached. No more responses.\n");
            break;
        }

        buffer[n] = '\0';
        printf("[+] Response from %s:%d: %s\n",
               inet_ntoa(sender.sin_addr), ntohs(sender.sin_port), buffer);
    }

    return 1;
}

void server_com_init_network_discovery() {
    network_discovery_on = true;
    thrd_create(&network_discovery_thread, (thrd_start_t)server_com_network_discovery, NULL);
    thrd_detach(network_discovery_thread);
}

void server_com_halt_network_discovery() {
    network_discovery_on = false;
    thrd_join(network_discovery_thread, NULL);
}