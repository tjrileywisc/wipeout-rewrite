
#include "server_com.h"

#include "menu.h"
#include "network_wrapper.h"

#include <addr_conversions.h>
#include <network.h>
#include <ServerInfo.pb-c.h>

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
thrd_t network_discovery_response_thread;

static int DISCOVERY_TIMEOUT = 30; // seconds

static int sockfd = INVALID_SOCKET;

static server_info_t* servers = NULL; // dynamically allocated array of server_info_t
static unsigned int n_servers = 0;

static menu_page_t* server_menu_page = NULL; // menu for server discovery

void server_com_client_init() {
    // TODO
}

static void server_com_update_servers() {

    /*
    TODO: how do we update the server menu page?
    */
    if(!server_menu_page) {
        printf("No server menu page set, cannot update servers.\n");
        return;
    }

    server_info_t server = servers[n_servers];
    char name[32];
    snprintf(name, sizeof(name), "%s", server.name);
    menu_page_add_button(server_menu_page, n_servers, name, NULL); // TODO: add a function to connect to this server
    printf("Server %d: %s\n", n_servers, name);
}

/**
 * @brief since we're connecting over UDP, need to have a thread running
 * separately to listen for discovery responses so we don't miss any
 */
static int server_com_discovery_response(void* arg) {
    (void)arg; // unused

    struct sockaddr_in from;
    socklen_t fromlen = sizeof(from);
    char buffer[1024];

    servers = realloc(servers, 0); // start with an empty list
    n_servers = 0;

    time_t start_time = time(NULL);

    while (1) {

        while(!network_discovery_on) {
            thrd_yield(); // wait until discovery is enabled
        }

        if(time(NULL) - start_time > DISCOVERY_TIMEOUT) {
            printf("No responses received in %d seconds, stopping discovery.\n", DISCOVERY_TIMEOUT);
            network_discovery_on = false; // stop listening
            continue;
        }

        ssize_t len = recvfrom(sockfd, buffer, sizeof(buffer)-1, 0,
                               (struct sockaddr*)&from, &fromlen);

        // TODO: how do we need to handle recvfrom timeouts and errors?
        // if (len < 0) {
        //     if (errno == EWOULDBLOCK || errno == EAGAIN) {
        //         printf("Timeout reached.\n");
        //         break;
        //     } else {
        //         perror("recvfrom");
        //         break;
        //     }
        // }

        if (len > 0) {
            Wipeout__ServerInfo* msg = wipeout__server_info__unpack(NULL, len, (const uint8_t*)buffer);
            if(msg == NULL) {
                fprintf(stderr, "Failed to unpack server info message\n");
                continue;
            }
            // TODO:
            // translate protobuf message to something we can use
            buffer[len] = '\0';
            printf("Found server %s @ %s:%d\n", msg->name, inet_ntoa(from.sin_addr), msg->port);

            servers = realloc(servers, sizeof(server_info_t) * (n_servers + 1));
            servers[n_servers] = (server_info_t) {
                .name = msg->name
            };
            server_com_update_servers(); // update the menu with the new server
            n_servers++;
        }
    }

    return 1;
}

/**
 * @brief Run network discovery to find servers
 */
static int server_com_network_discovery(void* arg) {
    (void)arg; // unused

    // Send broadcast to all interfaces
    struct ifaddrs *ifaddr = NULL;
    if (getifaddrs(&ifaddr) == -1) {
        perror("getifaddrs");
        return 0;
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

        const char *message = "status";
        wrap_sendto(sockfd, message, strlen(message), 0,
            (struct sockaddr *)&baddr, sizeof(baddr));

        printf("[*] Broadcast packet sent. Waiting for replies...\n");
        // tell receiver thread to start listening
        network_discovery_on = true;
    }

    freeifaddrs(ifaddr);

    return 1;
}

void server_com_init_network_discovery() {

    sockfd = network_get_client_socket();
    if(sockfd == INVALID_SOCKET) {
        printf("unable to create socket to run network discovery\n");
        return;
    }

    char my_ip[INET_ADDRSTRLEN];
    network_get_my_ip(my_ip, INET_ADDRSTRLEN);

    if(!network_bind_socket(sockfd, my_ip, "8001")) {
        printf("unable to bind socket for network discovery\n");
        network_close_socket(&sockfd);
        return;
    }

    // immediately make us ready to get responses before we start broadcasting
    thrd_create(&network_discovery_response_thread, (thrd_start_t)server_com_discovery_response, NULL);
    thrd_detach(network_discovery_response_thread);

    thrd_create(&network_discovery_thread, (thrd_start_t)server_com_network_discovery, NULL);
    thrd_detach(network_discovery_thread);
}

void server_com_halt_network_discovery() {
    network_discovery_on = false;
    thrd_join(network_discovery_thread, NULL);
    thrd_join(network_discovery_response_thread, NULL);
}

// server_info_t *server_com_get_servers() { 
//     return servers; 
// }

// unsigned int server_com_get_n_servers() { 
//     return n_servers; 
// }

void server_com_set_menu_page(menu_page_t *page) {
    server_menu_page = page;
}
