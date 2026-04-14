
#include <client_com.h>

#include <msg.h>
#include <name_gen.h>
#include <network.h>

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#if defined(WIN32)
#else
#include <unistd.h>
#endif

typedef struct
{
    char name[NAME_GEN_MAX_LEN];
    int num_clients;
} server_t;

static server_t server;

static void server_init(const char *name)
{
    snprintf(server.name, sizeof(server.name), "%s", name);
    server.num_clients = 0;

    client_com_init(server.name);
}

int main(int argc, char** argv)
{

    srand((unsigned int)time(NULL));

    char name[NAME_GEN_MAX_LEN];
    if (argc > 1) {
        snprintf(name, sizeof(name), "%s", argv[1]);
    } else {
        name_gen_random(name, sizeof(name));
    }

    printf("welcome to the %s server!\n", name);

    server_init(name);

    int sockfd = network_get_client_socket();
    if(sockfd == INVALID_SOCKET) {
        printf("could not create socket, quitting\n");
        return 1;
    }
    if(!network_bind_socket(sockfd, "8000")) {
        printf("could not start server, quitting\n");
        return 1;
    }

    while (true)
    {
        if(network_sleep(100) <= 0) {
            // no network activity, continue
            continue;
        }

        network_get_packet();
        server_process_queue();
    }

    return 0;
}
