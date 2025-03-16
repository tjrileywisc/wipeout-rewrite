

#include <errno.h>
#include <stdbool.h>
#include <stdio.h>

#if defined(WIN32)
#include <winsock2.h>
#else
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <unistd.h>
#endif

int ip_socket;

void string_to_socket_addr(const char *str_addr, struct sockaddr *sadr) {
    struct hostent *h;

    memset(sadr, 0, sizeof(*sadr));
    ((struct sockaddr_in *)sadr)->sin_family = AF_INET;

    ((struct sockaddr_in *)sadr)->sin_port = 0;

    if (str_addr[0] >= '0' && str_addr[0] <= '9') {
        // it's an ip4 address
        *(int *)&((struct sockaddr_in *)sadr)->sin_addr = inet_addr(str_addr);
    }
    else {
        // it's a string address
        if (!(h = gethostbyname(str_addr)))
            return;
        *(int *)&((struct sockaddr_in *)sadr)->sin_addr = *(int *)h->h_addr_list[0];
    }

    return;
}

// attempt to open network connection
int network_ip_socket(char* ip_addr, int port) {

    struct sockaddr_in address;

    string_to_socket_addr(ip_addr, (struct sockaddr*)&address);

    //address.sin_addr.s_addr = ip_addr;
    address.sin_port = htons((short)port);
    address.sin_family = AF_INET;

    int new_socket = -1;
    bool _true = true;
    int i = 1;
    
    if((new_socket = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1) {
        printf("unable to open socket: %s\n", strerror(errno));
        return 0;
    }

    if(ioctl(new_socket, FIONBIO, &_true) == -1) {
        printf("can't make socket non-blocking: %s\n", strerror(errno));
        return 0;
    }

    if(setsockopt(new_socket, SOL_SOCKET, SO_BROADCAST, (char*)&i, sizeof(i)) == -1) {
        printf("can't make socket broadcastable: %s\n", strerror(errno));
        return 0;
    }


    if(bind(new_socket, (void*)&address, sizeof(address)) == -1) {
        printf("couldn't bind address and port: %s\n", strerror(errno));
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