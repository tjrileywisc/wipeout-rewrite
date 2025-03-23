

#include <msg.h>
#include <network.h>

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#define PACKET_HDR_SIZE 4

typedef struct {
    const char* name;
    int num_clients;
} server_t;

server_t server;

// command callbacks
static void server_status() {

}

static void server_anonymous_packet(netadr_t from, msg_t* msg) {

    if(!strncmp("connect", &msg->data[PACKET_HDR_SIZE], strlen("connect"))) {
        // a more complex message
    }

    char* s = msg_read_string_line(msg);
    char* tokens = strtok(s, ' ');

    if(strcmp(tokens[0], "getstatus")) {
        server_status();
    }
    else if (strcmp(tokens[0], "connect")) {
        // connect
    }
    else if (strcmp(tokens[0], "command")) {
        // perform a command given the message
    }
    else {
        printf("this packet is bad...\n");
    }

}

static void server_packet_event(netadr_t from, msg_t* msg) {

    // is it from an anonymous client (i.e. one not connected to us)
    // starts with 0xffff_ffff
    if ( msg->cursize >= PACKET_HDR_SIZE && *(int *)msg->data == -1) {
        server_anonymous_packet(from, msg);
        return;
	}
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