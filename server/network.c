

#include <errno.h>
#include <stdbool.h>
#include <stdio.h>

#if defined(WIN32)
#include <winsock2.h>
#include <Windows.h>
#else
#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <unistd.h>
#endif

#if defined(WIN32)
static WSADATA	winsockdata;
#endif

int ip_socket;

const char *network_get_last_error()
{
#if defined(WIN32)
    DWORD code = WSAGetLastError();

    char* errorMsg = NULL;

    FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        code,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPSTR)&errorMsg,
        0,
        NULL);

    return errorMsg;

#else
    return strerror(errno);
#endif
}

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

    SOCKET new_socket;
    struct sockaddr_in address;

    string_to_socket_addr(ip_addr, (struct sockaddr*)&address);

    address.sin_port = htons((short)port);
    address.sin_family = AF_INET;

    bool _true = true;
    int i = 1;
    
    if((new_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == INVALID_SOCKET) {
        printf("unable to open socket: %s\n", network_get_last_error());
        return 0;
    }

#if defined(WIN32)
    if(ioctlsocket(new_socket, FIONBIO, &_true) == -1) {
#else
    if(ioctl(new_socket, FIONBIO, &_true) == -1) {
#endif
        printf("can't make socket non-blocking: %s\n", network_get_last_error());
        return 0;
    }

    if(setsockopt(new_socket, SOL_SOCKET, SO_BROADCAST, (char*)&i, sizeof(i)) == -1) {
        printf("can't make socket broadcastable: %s\n", network_get_last_error());
        return 0;
    }


    if(bind(new_socket, (void*)&address, sizeof(address)) == -1) {
        printf("couldn't bind address and port: %s\n", network_get_last_error());
#if defined(WIN32)
        closesocket(new_socket);
#else
        close(new_socket);
#endif
        return 0;
    }

    return new_socket;
}

void network_open_ip()
{

    if ((WSAStartup(MAKEWORD(1, 1), &winsockdata)))
    {
        printf("unable to init windows socket: %s\n", network_get_last_error());
        return;
    }

    char *address = "localhost";
    int port = 8000;

    // make several attempts to grab an open port
    for (int i = 0; i < 10; i++)
    {
        ip_socket = network_ip_socket(address, port + i);

        if (ip_socket)
        {
            printf("established connection at %s:%d\n", address, port);
            return;
        }
    }
    perror("could not establish network connection... quitting.\n");
}