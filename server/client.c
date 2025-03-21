
#include <network.h>
#include <stdio.h>

// concerning syncing of clients to the game state

typedef enum {
	DISCONNECTED,
	CONNECTED
} connect_state_t;

typedef struct {
	const char* name;
	connect_state_t state;
} client_t;

typedef struct {

} server_t;

/* _this server_ */
static server_t server;

void server_connect_client(netadr_t from) {

    client_t* client;

    // see https://github.com/id-Software/Quake-III-Arena/blob/dbe4ddb10315479fc00086f08e25d968b4b43c49/code/server/sv_client.c#L230

    // (skipped a whole bunch of checks to avoid connecting, like invalid client versions and the nature of the connection)

    // build a new connection
	// accept the new client
	// this is the only place a client_t is ever initialized

	// send the connect packet to the client
	network_out_of_band_print(SERVER, from, "connnectRepsonse");

	printf(" %s has connected...\n", client->name );

	client->state = CONNECTED;
}