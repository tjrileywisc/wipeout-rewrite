
#include "client.h"

#include <msg.h>
#include <network.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <ServerInfo.pb-c.h>

// concerning syncing of clients to the game state

typedef enum
{
	DISCONNECTED,
	CONNECTED
} connect_state_t;

typedef struct
{
	const char *name;
	connect_state_t state;
} client_t;

void server_parse_msg(const char* cmd)
{
	if(strcmp(cmd, "connect") == 0)
	{
		// handle connect
	}
	else if(strcmp(cmd, "disconnect") == 0)
	{
		// handle disconnect
	}
	else if(strcmp(cmd, "status") == 0)
	{
		// handle status
		Wipeout__ServerInfo msg;
		wipeout__server_info__init(&msg);
		msg.name = "my server";
		msg.port = 8000;

		size_t len = wipeout__server_info__get_packed_size(&msg);
		uint8_t *buffer = malloc(len);
		if(!buffer)
		{
			fprintf(stderr, "Failed to allocate buffer for server info\n");
			return;
		}
		wipeout__server_info__pack(&msg, buffer);

		// TODO: send it back to the client
		free(buffer);
	}
	else if(strcmp(cmd, "hello") == 0)
	{
		printf("Hello from client!\n");
		// handle hello (just echo back the client's address)
	}
	else
	{
		printf("Unknown command: %s\n", cmd);
	}
}
