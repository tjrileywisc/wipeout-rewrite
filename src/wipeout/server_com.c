
#include "server_com.h"

#include "menu.h"
#include "network_wrapper.h"

#include <addr_conversions.h>
#include <network.h>
#include <ServerInfo.pb-c.h>
#include <stdbool.h>

#if defined(WIN32)
#include <ws2ipdef.h>
#else
#include <arpa/inet.h>
#include <sys/prctl.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdatomic.h>
#include <threads.h>

atomic_bool network_discovery_on = ATOMIC_VAR_INIT(false);

thrd_t network_discovery_thread;
thrd_t network_discovery_response_thread;
static bool network_discovery_threads_started = false;

static int DISCOVERY_TIMEOUT = 3; // seconds

static int sockfd = INVALID_SOCKET;

struct server_info_t{
    const char* name;
    struct sockaddr_in addr; // server address
};

static server_info_t* servers = NULL; // dynamically allocated array of server_info_t
static unsigned int n_servers = 0;

static menu_page_t* server_menu_page = NULL; // menu for server discovery

void server_com_client_init(void) {
    // TODO
}

static void server_com_client_connect(menu_t*, int index) {

    // TODO: causes a problem if we try to connect
    // before server discovery has terminated;
    // should we just halt discovery when we connect?

    if(!servers || n_servers == 0) {
        printf("No servers available to connect to.\n");
        return;
    }

    struct server_info_t server = servers[index];
    printf("Connecting to server: %s\n", server.name);

    socklen_t fromlen = sizeof(server.addr);

    const char* message = "connect";
    network_send_packet(sockfd, strlen(message), message, server.addr);

    char buffer[1024];
    ssize_t len = wrap_recvfrom(sockfd, buffer, sizeof(buffer)-1, 0,
                               (struct sockaddr*)(&server.addr), &fromlen);

    if (len < 0) {
        perror("recvfrom");
        return;
    }

    buffer[len] = '\0'; // null-terminate the received data
    printf("Received response from server: %s\n", buffer);
    if (strcmp(buffer, "connected") == 0) {
        printf("Successfully connected to server %s\n", server.name);
    }
    else {
        printf("Failed to connect to server %s: %s\n", server.name, buffer);
    }
}

static void server_com_update_servers() {
    // TODO:
    // is this function necessary?
    // we can't update or add a button after
    // startup, only change the text


    if(!server_menu_page) {
        printf("No server menu page set, cannot update servers.\n");
        return;
    }

    server_info_t server = servers[n_servers];
    char name[32];
    snprintf(name, sizeof(name), "%s", server.name);

    menu_page_add_button(server_menu_page, n_servers, name, server_com_client_connect);
}

/**
 * @brief since we're connecting over UDP, need to have a thread running
 * separately to listen for discovery responses so we don't miss any
 */
static int server_com_discovery_response(void* arg) {
    (void)arg; // unused

#ifndef _WIN32
    prctl(PR_SET_NAME, "brdcst_resp", 0, 0, 0);
#endif

    struct sockaddr_in from;
    socklen_t fromlen = sizeof(from);
    char buffer[1024];
    time_t start_time = time(NULL);

    while (true) {

        while(!atomic_load(&network_discovery_on)) {
            thrd_yield(); // wait until discovery is enabled
            
            // Reset servers list once when discovery starts
            if(servers != NULL) {
                free(servers);
                servers = NULL;
                n_servers = 0;
            }
            start_time = time(NULL); // reset start time when we start listening
        }


        if(time(NULL) - start_time > DISCOVERY_TIMEOUT) {
            printf("Discovery has timed out after %d seconds. %d servers found.\n", DISCOVERY_TIMEOUT, n_servers);
            atomic_store(&network_discovery_on, false); // stop listening
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

            buffer[len] = '\0';
            printf("Found server %s @ %s:%d\n", msg->name, inet_ntoa(from.sin_addr), msg->port);

            servers = realloc(servers, sizeof(server_info_t) * (n_servers + 1));
            servers[n_servers] = (server_info_t) {
                .name = msg->name,
                .addr = {
                    .sin_family = AF_INET,
                    .sin_port = htons(msg->port),
                    .sin_addr = from.sin_addr
                }
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
    (void)arg;

#ifdef _WIN32
    if(!system_init_winsock()) {
        printf("Failed to initialize Winsock for network discovery.\n");
        return 0;
    }
#else
    prctl(PR_SET_NAME, "brdcst_disc", 0, 0, 0);
#endif

    // the outer loop runs forever, but we don't want to broadcast more than once
    // while discovery is enabled
    static bool has_run = false;

    while(true) {
        while(!atomic_load(&network_discovery_on)) {
            thrd_yield(); // wait until discovery is enabled
            has_run = false; // send one ping only
        }
        
        if(has_run) {
            thrd_sleep(&(struct timespec){.tv_sec = 0, .tv_nsec = 10000000}, NULL); // sleep for 10ms
            continue;
        }
    
        const char* message = "status";
        broadcast_list_t broadcasts = network_get_broadcast_addresses();

        for (size_t i = 0; i < broadcasts.count; ++i) {
            struct sockaddr_in addr = {
                .sin_family = AF_INET,
                .sin_port = htons(WIPEOUT_PORT),
                .sin_addr = broadcasts.list[i].broadcast,
            };

            wrap_sendto(sockfd, message, strlen(message), 0,
                        (struct sockaddr*)&addr, sizeof(addr));

            printf("[*] Broadcast packet sent. Waiting for replies...\n");
        }

        free(broadcasts.list);
        has_run = true;
    }

    return 1;
}
void server_com_init_network_discovery(void) {

    if(network_discovery_threads_started) {
        printf("network discovery already running\n");
    } else {
        sockfd = network_get_client_socket();
        if(sockfd == INVALID_SOCKET) {
            printf("unable to create socket to run network discovery\n");
            return;
        }

        if(!network_bind_socket(sockfd, "8001")) {
            printf("unable to bind socket for network discovery\n");
            network_close_socket(&sockfd);
            return;
        }

        thrd_create(&network_discovery_response_thread, (thrd_start_t)server_com_discovery_response, NULL);
        thrd_detach(network_discovery_response_thread);

        thrd_create(&network_discovery_thread, (thrd_start_t)server_com_network_discovery, NULL);
        thrd_detach(network_discovery_thread);

        network_discovery_threads_started = true;
    }
    atomic_store(&network_discovery_on, true);
}


void server_com_halt_network_discovery(void) {
    atomic_store(&network_discovery_on, false);
}

server_info_t *server_com_get_servers(void) { 
    return servers; 
}

unsigned int server_com_get_n_servers(void) { 
    return n_servers; 
}

void server_com_set_menu_page(menu_page_t *page) {
    server_menu_page = page;
}
