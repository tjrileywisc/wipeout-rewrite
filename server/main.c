
#include <client.h>

#include <msg.h>
#include <network.h>

#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#if defined(WIN32)
#else
#include <unistd.h>
#endif

typedef struct
{
    const char *name;
    int num_clients;
} server_t;

static server_t server;

static void server_init()
{
    server.name = "master blaster";
    server.num_clients = 0;
}

int main(int, char **)
{
    printf("welcome to the server!\n");

    server_init();
    network_bind_ip();

    while (true)
    {
        if(network_sleep(100) <= 0) {
            // no network activity, continue
            continue;
        }

        network_get_packet();
    }

    return 0;
}
