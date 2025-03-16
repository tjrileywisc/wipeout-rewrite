

#include <stdio.h>
#include <winsock2.h>

int ip_socket;

// attempt to open network connection
int network_ip_socket(char* ip_addr, int port) {

    struct sockaddr_in address;
    address.sin_addr.s_addr = ip_addr;
    address.sin_port = port;
    address.sin_family = AF_INET;

    int new_socket = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);

    if(new_socket == INVALID_SOCKET) {
        printf("unable to open socket\n");
        return 0;
    }

    if(ioctl(new_socket, FIONBIO) == -1) {
        printf("can't make socket non-blocking\n");
        return 0;
    }

    if(setsockopt(new_socket, SOL_SOCKET, SO_BROADCAST, SO_DEBUG, 1) == -1) {
        printf("can't make socket broadcastable\n");
        return 0;
    }


    if(bind(new_socket, (void*)&address, sizeof(address)) == -1) {
        printf("couldn't bind address and port\n");
        close(new_socket);
        return 0;
    }

    return new_socket;
}

void network_open_ip() {

    char* address = "localhost";
    int port = 8000;

    // make several attempts to grab an open port
    for(int i = 0; i < 10; i++) {
        ip_socket = network_ip_socket(address, port + i);

        if(ip_socket) {
            printf("established connection at %s:%d\n", address, port);
            return;
        }
    }
    perror("could not establish network connection... quitting.\n");
}