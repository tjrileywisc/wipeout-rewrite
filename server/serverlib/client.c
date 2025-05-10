
#include "client.h"

#include <network_types.h>

#include <msg.h>
#include <network.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <threads.h>

#include <ServerInfo.pb-c.h>

typedef enum { DISCONNECTED, CONNECTED } connect_state_t;

thrd_t worker_threds[10];
int num_threads = 0;

typedef struct {
    const char *name;
    connect_state_t state;
} client_t;

/**
 * @brief Processes queued messages from clients
 */
static void server_response_thread() {}


static void server_connect_client() {
    // handle client connection
    client_t *client = malloc(sizeof(client_t));
    if (!client) {
        fprintf(stderr, "Failed to allocate memory for client\n");
        return;
    }
    client->name = "Client";
    client->state = CONNECTED;

	// TODO: tell client they are connected
}

static void server_disconnect_client() {
	// handle client disconnection
	client_t *client = malloc(sizeof(client_t));
	if (!client) {
		fprintf(stderr, "Failed to allocate memory for client\n");
		return;
	}
	client->name = "Client";
	client->state = DISCONNECTED;

	// TODO: client will disconnect on their end,
	// no need to tell them we are removing them
}

static void server_status() {
	// handle server status
	Wipeout__ServerInfo msg;
	wipeout__server_info__init(&msg);
	msg.name = "my server";
	msg.port = 8000;

	size_t len = wipeout__server_info__get_packed_size(&msg);
	uint8_t *buffer = malloc(len);
	if (!buffer) {
		fprintf(stderr, "Failed to allocate buffer for server info\n");
		return;
	}
	wipeout__server_info__pack(&msg, buffer);
}

void server_parse_msg(const char *cmd) {
    if (strcmp(cmd, "connect") == 0) {
        // handle connect
    } else if (strcmp(cmd, "disconnect") == 0) {
        // handle disconnect
    } else if (strcmp(cmd, "status") == 0) {
        // handle status
    } else if (strcmp(cmd, "hello") == 0) {
        printf("Hello from client!\n");
        // handle hello (just echo back the client's address)
    } else {
        printf("Unknown command: %s\n", cmd);
    }
}
