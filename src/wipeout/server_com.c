
#include "server_com.h"

#include "menu.h"
#include "network_wrapper.h"

#include <addr_conversions.h>
#include <network.h>
#include <name_gen.h>
#include <protocol.h>
#include <ClientList.pb-c.h>
#include <ServerInfo.pb-c.h>
#include <stdbool.h>

#if defined(WIN32)
#include <ws2ipdef.h>
#else
#include <arpa/inet.h>
#include <sys/prctl.h>
#endif

#include <stdio.h>
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

#define MAX_SERVERS 16
static server_info_t servers[MAX_SERVERS];
static unsigned int n_servers = 0;

#define MAX_CONNECTED_CLIENTS 8
typedef struct {
    char name[NAME_GEN_MAX_LEN];
    char ip[INET_ADDRSTRLEN];
} connected_client_t;
static connected_client_t connected_clients[MAX_CONNECTED_CLIENTS];
static unsigned int n_connected_clients = 0;

static menu_page_t* server_menu_page = NULL; // menu for server discovery
static void (*on_connect_callback)(menu_t *menu) = NULL;
static menu_t *pending_connect_menu = NULL;
static struct sockaddr_in connected_server_addr = {0};

void server_com_client_init(void) {
    // TODO
}

static void server_com_client_connect(menu_t *menu, int index) {

    // TODO: causes a problem if we try to connect
    // before server discovery has terminated;
    // should we just halt discovery when we connect?

    if(n_servers == 0) {
        printf("No servers available to connect to.\n");
        return;
    }

    struct server_info_t server = servers[index];
    printf("Connecting to server: %s\n", server.name);

    const char* message = "connect";
    network_send_packet(sockfd, strlen(message), message, server.addr);

    // The response arrives on the same non-blocking socket that the discovery
    // thread is already reading. Store the menu and let the discovery thread
    // handle the "connected"/"connect_failed" reply.
    pending_connect_menu = menu;
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
        }
        // Reset state once when discovery (re)starts
        n_servers = 0;
        start_time = time(NULL);

        while (atomic_load(&network_discovery_on)) {
            if(time(NULL) - start_time > DISCOVERY_TIMEOUT) {
                printf("Discovery has timed out after %d seconds. %d servers found.\n", DISCOVERY_TIMEOUT, n_servers);
                atomic_store(&network_discovery_on, false); // stop listening
                break;
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
            uint8_t type_byte = (uint8_t)buffer[0];

            if (type_byte == MSG_TYPE_SERVER_INFO) {
                Wipeout__ServerInfo* msg = wipeout__server_info__unpack(NULL, len - 1, (const uint8_t*)buffer + 1);
                if (msg == NULL) {
                    fprintf(stderr, "Failed to unpack ServerInfo message\n");
                    continue;
                }
                printf("Found server %s @ %s:%d\n", msg->name, inet_ntoa(from.sin_addr), msg->port);
                bool duplicate = false;
                for (unsigned int i = 0; i < n_servers; i++) {
                    if (servers[i].addr.sin_addr.s_addr == from.sin_addr.s_addr &&
                        servers[i].addr.sin_port == htons(msg->port)) {
                        duplicate = true;
                        break;
                    }
                }
                if (!duplicate && n_servers < MAX_SERVERS) {
                    servers[n_servers] = (server_info_t) {
                        .name = msg->name,
                        .addr = {
                            .sin_family = AF_INET,
                            .sin_port = htons(msg->port),
                            .sin_addr = from.sin_addr
                        }
                    };
                    server_com_update_servers();
                    n_servers++;
                }

            } else if (type_byte == MSG_TYPE_CLIENT_LIST) {
                Wipeout__ClientList* msg = wipeout__client_list__unpack(NULL, len - 1, (const uint8_t*)buffer + 1);
                if (msg == NULL) {
                    fprintf(stderr, "Failed to unpack ClientList message\n");
                    continue;
                }
                n_connected_clients = 0;
                for (size_t i = 0; i < msg->n_clients && i < MAX_CONNECTED_CLIENTS; i++) {
                    snprintf(connected_clients[i].name, NAME_GEN_MAX_LEN, "%s", msg->clients[i]->name);
                    snprintf(connected_clients[i].ip,   INET_ADDRSTRLEN,  "%s", msg->clients[i]->ip);
                    n_connected_clients++;
                }
                wipeout__client_list__free_unpacked(msg, NULL);

            } else {
                // Plain-text message
                buffer[len] = '\0';
                if (strcmp(buffer, "connected") == 0) {
                    printf("Successfully connected to server\n");
                    connected_server_addr = from;
                    if (on_connect_callback && pending_connect_menu) {
                        on_connect_callback(pending_connect_menu);
                        pending_connect_menu = NULL;
                    }
                } else if (strcmp(buffer, "connect_failed") == 0) {
                    printf("Failed to connect to server\n");
                    pending_connect_menu = NULL;
                } else {
                    fprintf(stderr, "Unrecognised message from server\n");
                }
            }
        }
        } // end inner discovery loop
    } // end outer while(true)

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

void server_com_set_connect_callback(void (*callback)(menu_t *menu)) {
    on_connect_callback = callback;
}

void server_com_disconnect(void) {
    const char* message = "disconnect";
    network_send_packet(sockfd, strlen(message), message, connected_server_addr);
    connected_server_addr = (struct sockaddr_in){0};
    n_connected_clients = 0;
}

unsigned int server_com_get_n_connected_clients(void) {
    return n_connected_clients;
}

const char *server_com_get_connected_client_name(unsigned int index) {
    if (index >= n_connected_clients) return "";
    return connected_clients[index].name;
}

const char *server_com_get_connected_client_ip(unsigned int index) {
    if (index >= n_connected_clients) return "";
    return connected_clients[index].ip;
}
