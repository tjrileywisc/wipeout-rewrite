

#include <network.h>

#include <stdbool.h>
#include <stdio.h>

typedef struct {
    const char* name;
    int num_clients;
} server_t;

server_t server;

void server_status() {

}

static void server_init() {
    server.name = "master blaster";
    server.num_clients = 0;
}

int main(int argc, char** argv) {

    printf("welcome to the server!\n");

    server_init();

    bool should_quit = false;

    network_open_ip();

    while (!should_quit)
    {
        //orchestrate_frame();
    }

    return 0;
}