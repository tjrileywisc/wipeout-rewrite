
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

#include <errno.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <ifaddrs.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdatomic.h>
#include <threads.h>

atomic_bool network_discovery_on = false;

thrd_t network_discovery_thread;

void server_com_client_init() {

    char my_ip[INET_ADDRSTRLEN];
    network_get_my_ip(my_ip, INET_ADDRSTRLEN);
    network_connect_ip(my_ip);
}

/**
 * @brief Run network discovery to find servers
 */
static int server_com_network_discovery() {

    int BAD_RESULT = 0;
    int GOOD_RESULT = 1;

    if(!network_discovery_on) {
		return BAD_RESULT;
	}

    int sockfd = network_get_socket();
    if (sockfd == INVALID_SOCKET) {
        perror("[-] Error creating socket");
        return BAD_RESULT;
    }

    // Set receive timeout
    struct timeval timeout;
    timeout.tv_sec = 3;
    timeout.tv_usec = 0;
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

    // Send broadcast to all interfaces
    struct ifaddrs *ifaddr = NULL;
    if (getifaddrs(&ifaddr) == -1) {
        perror("getifaddrs");
        network_close_socket(sockfd);
        return BAD_RESULT;
    }

    // check multiple interfaces, since we don't know if the user
    // has ethernet or wifi but they probably call it their LAN
    struct ifaddrs *ifa;
    struct ifreq ifr;
    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
        if (!ifa->ifa_addr || ifa->ifa_addr->sa_family != AF_INET)
            continue;

        // Skip loopback and non-broadcast interfaces
        if ((ifa->ifa_flags & IFF_LOOPBACK) || !(ifa->ifa_flags & IFF_BROADCAST))
            continue;

        // Prepare ioctl request
        memset(&ifr, 0, sizeof(ifr));
        strncpy(ifr.ifr_name, ifa->ifa_name, IFNAMSIZ - 1);

        if (ioctl(sockfd, SIOCGIFBRDADDR, &ifr) < 0) {
            perror("ioctl SIOCGIFBRDADDR");
            continue;
        }

        struct sockaddr_in baddr;
        memcpy(&baddr, &ifr.ifr_broadaddr, sizeof(baddr));
        baddr.sin_port = htons(WIPEOUT_PORT);

        const char *message = "hello";
        wrap_sendto(sockfd, message, strlen(message), 0,
            (struct sockaddr *)&baddr, sizeof(baddr));

        printf("[*] Broadcast packet sent. Waiting for replies...\n");
    }

    freeifaddrs(ifaddr);

    static int BUF_SIZE = 10;
    char buffer[BUF_SIZE];
    while (1) {
        struct sockaddr_in sender;
        socklen_t sender_len = sizeof(sender);
        ssize_t n = wrap_recvfrom(sockfd, buffer, BUF_SIZE - 1, 0,
                            (struct sockaddr *)&sender, &sender_len);

        if (n < 0) {
            if (errno == EWOULDBLOCK || errno == EAGAIN) {
                printf("Timeout reached.\n");
                break;
            } else {
                perror("recvfrom");
                break;
            }
        }

        buffer[n] = '\0';
        printf("[+] Response from %s:%d: %s\n",
            inet_ntoa(sender.sin_addr), ntohs(sender.sin_port), buffer);
    }

    network_close_socket(sockfd);

    return GOOD_RESULT;
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