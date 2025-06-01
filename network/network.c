
#include "network.h"
#include "network_wrapper.h"

#include "addr_conversions.h"
#include "msg.h"

#include <errno.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#if defined(WIN32)
#include <iphlpapi.h>
#include <WinSock2.h>
#include <WS2tcpip.h>
#else
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <ifaddrs.h>

#endif

#include <ServerInfo.pb-c.h>

#define SERVER_PORT "8000"

#if defined(WIN32)
static WSADATA winsockdata;
static bool winsock_initialized = false;
#endif

static msg_queue_item_t msg_queue[10];
static int msg_queue_size = 0;

static int client_sockfd = INVALID_SOCKET;

static const char *network_get_last_error(void)
{
#if defined(WIN32)
    int code = WSAGetLastError();

    char *errorMsg = NULL;

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

static void system_send_packet(int sockfd, int length, const void *data, netadr_t dest_net)
{

    if(sockfd == INVALID_SOCKET)
    {
        printf("unable to get socket for sending packet: %s\n", network_get_last_error());
        return;
    }

    struct sockaddr_in dest_addr;

    netadr_to_sockadr(&dest_net, &dest_addr);

    int ret = wrap_sendto(sockfd, data, length, 0, (struct sockaddr *)&dest_addr, sizeof(dest_addr));

    if (ret == -1)
    {
        printf("unable to send packet to %s: %s\n", addr_to_string(dest_net), network_get_last_error());
    }
}

static bool system_get_packet(netadr_t *net_from, msg_t *net_msg)
{
    struct sockaddr_in from;

    if (client_sockfd == INVALID_SOCKET) {
        return false;
    }

    unsigned int fromlen = sizeof(from);
    int ret = wrap_recvfrom(client_sockfd, net_msg->data, net_msg->maxsize, 0, (struct sockaddr *)&from, &fromlen);

    sockadr_to_netadr(&from, net_from);
    net_msg->readcount = 0;

    if (ret == -1)
    {
        int err = errno;

        if (err == EWOULDBLOCK || err == ECONNREFUSED) {
            return false;
        }
        
        printf("%s from %s\n", network_get_last_error(), addr_to_string(*net_from));
    }

    if (ret == net_msg->maxsize)
    {
        printf("oversize packet from %s\n", addr_to_string(*net_from));
        return false;
    }

    net_msg->cursize = ret;
    return true;
}

int network_get_client_socket(void) {

#if defined(WIN32)
    if(!winsock_initialized) {
        if ((WSAStartup(MAKEWORD(2, 2), &winsockdata) != 0)) {
            printf("unable to init windows socket: %s\n", network_get_last_error());
            return INVALID_SOCKET;
        }
        winsock_initialized = true;
    }
#endif


#if defined(WIN32)
    SOCKET new_socket = INVALID_SOCKET;
#else
    int new_socket = INVALID_SOCKET;
#endif

    struct addrinfo hints, *res;
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE;     // fill in my IP for me

    getaddrinfo(NULL, "8000", &hints, &res);

    if ((new_socket = socket(res->ai_family, res->ai_socktype, res->ai_protocol)) ==
        INVALID_SOCKET) {
        printf("unable to open socket: %s\n", network_get_last_error());
        return INVALID_SOCKET;
    }

#if defined(WIN32)
    if (ioctlsocket(new_socket, FIONBIO, &_true) == -1) {
#else
    bool _true = true;
    if (ioctl(new_socket, FIONBIO, &_true) == -1) {
#endif
        printf("can't make socket non-blocking: %s\n",
               network_get_last_error());

        network_close_socket(&new_socket);
        return INVALID_SOCKET;
    }

    int i = 1;
    if (setsockopt(new_socket, SOL_SOCKET, SO_BROADCAST, (char *)&i,
                   sizeof(i)) == -1) {
        printf("can't make socket broadcastable: %s\n",
               network_get_last_error());
        network_close_socket(&new_socket);
        return INVALID_SOCKET;
    }

    // Set receive timeout
    // TODO: can this fail?
    struct timeval timeout;
    timeout.tv_sec = CLIENT_SOCKET_TIMEOUT / 1000; // convert milliseconds to seconds
    timeout.tv_usec = 0;
    setsockopt(new_socket, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

    return new_socket;
}

bool network_bind_socket(int sockfd, char *ip_addr, char* port)
{
    if(sockfd == INVALID_SOCKET)
    {
        printf("could not create socket: %s\n", network_get_last_error());
        return false;
    }

    struct addrinfo hints;
    struct addrinfo* res;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE;

    getaddrinfo(NULL, port, &hints, &res);

    if (bind(sockfd, res->ai_addr, res->ai_addrlen) == -1)
    {
        printf("couldn't bind address and port: %s\n", network_get_last_error());
        return false;
    }

    printf("established connection at %s:%s\n", ip_addr, port);
    //network_set_bound_ip_socket(sockfd);

    return true;
}

bool network_has_bound_ip_socket(void)
{
    return client_sockfd != INVALID_SOCKET;
}

void network_set_bound_ip_socket(int sockfd)
{
    client_sockfd = sockfd;
}

int network_get_bound_ip_socket(void) { 
    return client_sockfd; 
}

void network_close_socket(int* sockfd) {
    if(*sockfd == INVALID_SOCKET) {
        return;
    }
#if defined(WIN32)
    closesocket(*sockfd);
#else
    close(*sockfd);
#endif

    *sockfd = INVALID_SOCKET;
}

static void network_add_msg_queue_item(const char *buf, int numbytes, const struct sockaddr_storage *their_addr) {
    msg_queue_item_t *item = (msg_queue_item_t *)malloc(sizeof(msg_queue_item_t));
    if (!item) {
        perror("Failed to allocate memory for message queue item");
        return;
    }

    item->command = strdup(buf);
    if (!item->command) {
        perror("Failed to allocate memory for command string");
        free(item);
        return;
    }

    memcpy(&item->dest_addr, their_addr, sizeof(struct sockaddr_storage));
    msg_queue[msg_queue_size++] = *item;
}

int network_get_msg_queue_size() {
    return msg_queue_size;
}

void network_clear_msg_queue() {
    for (int i = 0; i < msg_queue_size; i++) {
        free(msg_queue[i].command);
    }
    msg_queue_size = 0;
}

void network_popleft_msg_queue() {
    if (msg_queue_size > 0) {
        free(msg_queue[0].command);
        memmove(&msg_queue[0], &msg_queue[1], (msg_queue_size - 1) * sizeof(msg_queue_item_t));
        msg_queue_size--;
    }
}

bool network_get_msg_queue_item(msg_queue_item_t *item) {
    if (msg_queue_size == 0) {
        return false;
    }

    *item = msg_queue[0];

    return true;
}


bool network_get_packet()
{
    int MAXBUFLEN = 100;
    char buf[MAXBUFLEN];
    struct sockaddr_storage their_addr;
    socklen_t addr_len = sizeof their_addr;
    int numbytes = 0;

    char s[INET_ADDRSTRLEN];

    if ((numbytes = wrap_recvfrom(client_sockfd, buf, MAXBUFLEN-1 , 0,
        (struct sockaddr *)&their_addr, &addr_len)) == -1) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            // No data available right now, not a fatal error
            return false;
        } else {
            perror("recvfrom");
            exit(1);
        }
    }

    struct sockaddr* addr = &((struct sockaddr_in*)&their_addr)->sin_addr;

    printf("listener: got packet from %s\n",
    inet_ntop(their_addr.ss_family,
            addr,
            s, sizeof s));
    printf("listener: packet is %d bytes long\n", numbytes);
    buf[numbytes] = '\0';
    printf("listener: packet contains \"%s\"\n", buf);

    network_add_msg_queue_item(buf, numbytes, &their_addr);

    return true;
}

void network_send_packet(int sockfd, int length, const void *data, netadr_t dest_net)
{
    system_send_packet(sockfd, length, data, dest_net);
}

void network_send_command(const char *command, netadr_t dest)
{
    if(strcmp(command, "server_info") == 0) {
        network_send_packet(network_get_bound_ip_socket(), strlen(command), "server_info", dest);
    }
}

void network_process_command(const char* command) 
{
    if(strcmp(command, "server_info") == 0) {
        Wipeout__ServerInfo* msg = NULL;
        wipeout__server_info__init(msg);
        msg->name = "my server";
        msg->port = 8000;

        unsigned char* out = (unsigned char*)malloc(wipeout__server_info__get_packed_size(msg));
        if(out) {
            wipeout__server_info__pack(msg, out);
        }
    }
}

int network_sleep(int msec)
{
    struct timeval timeout;
    fd_set fdset;

    if (!client_sockfd) {
        return 0;
    }

    FD_ZERO(&fdset);
    FD_SET(client_sockfd, &fdset); // network socket
    timeout.tv_sec = msec / 1000;
    timeout.tv_usec = (msec % 1000) * 1000;
    return select(client_sockfd + 1, &fdset, NULL, NULL, &timeout);
}

void network_get_my_ip(char *subnet, size_t len) {

    struct sockaddr_in dest = {0};
    dest.sin_family = AF_INET;
    dest.sin_port = htons(53); // DNS
    inet_pton(AF_INET, "8.8.8.8", &dest.sin_addr);

    int sockfd = network_get_client_socket();
    if(sockfd == INVALID_SOCKET) {
        perror("could not get ip address, quitting\n");
        return;
    }

    connect(sockfd, (struct sockaddr *)&dest, sizeof(dest));

    struct sockaddr_in local;
    len = sizeof(local);
    getsockname(sockfd, (struct sockaddr *)&local, &len);

    strncpy(subnet, inet_ntoa(local.sin_addr), len);

    network_close_socket(&sockfd);
}
