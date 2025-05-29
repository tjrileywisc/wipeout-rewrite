
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

static int ip_socket = INVALID_SOCKET;

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


// perhaps a platform specific interface to send a packet? it's really sent here
static void system_send_packet(int length, const void *data, netadr_t dest_net)
{

    if (!ip_socket)
    {
        printf("ip connection hasn't been established...");
        return;
    }

    int net_socket = ip_socket;
    struct sockaddr_in dest_addr;

    netadr_to_sockadr(&dest_net, &dest_addr);

    int ret = wrap_sendto(net_socket, data, length, 0, (struct sockaddr *)&dest_addr, sizeof(dest_addr));

    if (ret == -1)
    {
        printf("unable to send packet to %s: %s\n", addr_to_string(dest_net), network_get_last_error());
    }
}

static bool system_get_packet(netadr_t *net_from, msg_t *net_msg)
{
    struct sockaddr_in from;

    if (!ip_socket)
        return false;

    int fromlen = sizeof(from);
    int ret = wrap_recvfrom(ip_socket, net_msg->data, net_msg->maxsize, 0, (struct sockaddr *)&from, &fromlen);

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

int network_get_socket(void) {

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

    bool _true = true;
    int i = 1;

    if ((new_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) ==
        INVALID_SOCKET) {
        printf("unable to open socket: %s\n", network_get_last_error());
        return INVALID_SOCKET;
    }

    // TODO: should close the socket if it fails
#if defined(WIN32)
    if (ioctlsocket(new_socket, FIONBIO, &_true) == -1) {
#else
    if (ioctl(new_socket, FIONBIO, &_true) == -1) {
#endif
        printf("can't make socket non-blocking: %s\n",
               network_get_last_error());
        return INVALID_SOCKET;
    }

    if (setsockopt(new_socket, SOL_SOCKET, SO_BROADCAST, (char *)&i,
                   sizeof(i)) == -1) {
        printf("can't make socket broadcastable: %s\n",
               network_get_last_error());
        return INVALID_SOCKET;
    }

    return new_socket;
}

// attempt to open network connection
bool network_bind_ip_socket(int socket, char *ip_addr)
{
    struct addrinfo hints;
    struct addrinfo* res;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_family = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE;

    getaddrinfo(NULL, SERVER_PORT, &hints, &res);

    if (bind(socket, res->ai_addr, res->ai_addrlen) == -1)
    {
        printf("couldn't bind address and port: %s\n", network_get_last_error());
        return false;
    }

    strncpy(ip_addr, inet_ntoa(((struct sockaddr_in*)res->ai_addr)->sin_addr), INET_ADDRSTRLEN);

    return true;
}

bool network_has_bound_ip_socket(void)
{
    return ip_socket;
}

void network_set_bound_ip_socket(int sockfd)
{
    ip_socket = sockfd;
}

int network_get_bound_ip_socket(void) { 
    return ip_socket; 
}

void network_close_socket(int sockfd) {
    if(sockfd == INVALID_SOCKET) {
        return;
    }
#if defined(WIN32)
    closesocket(sockfd);
#else
    close(sockfd);
#endif
}

void network_bind_ip(void) {
    char address[INET_ADDRSTRLEN];

    int sockfd = network_get_socket();
    if (sockfd == INVALID_SOCKET)
    {
        perror("could not create socket... quitting.\n");
        return;
    }

    bool res = network_bind_ip_socket(sockfd, address);

    if (res)
    {
        printf("established connection at %s:%d\n", address, WIPEOUT_PORT);
        return;
    }
    perror("could not establish network connection... quitting.\n");

    network_close_socket(sockfd);
}

void network_connect_ip(const char* addr)
{
    struct addrinfo hints;
    struct addrinfo* servinfo;
    
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;

    int ret;

    if((ret = getaddrinfo(addr, SERVER_PORT, &hints, &servinfo)) != 0) {
        printf("getaddrinfo error: %s\n", network_get_last_error());
        return;
    }

    // loop through all the results and connect to the first we can
    for(struct addrinfo* p = servinfo; p != NULL; p = p->ai_next) {

        int sockfd = INVALID_SOCKET;
        // TODO: should use network_get_socket() here,
        // since it's cross platform
        if ((sockfd = socket(p->ai_family, p->ai_socktype,
                p->ai_protocol)) == -1) {
            perror("client: socket");
            continue;
        }

        // TODO:
        // bind or connect?
        if (connect(ip_socket, p->ai_addr, p->ai_addrlen) == -1) {
            network_close_socket(sockfd);
            perror("client: connect");
            continue;
        }

        ip_socket = sockfd;

        break;
    }

    freeaddrinfo(servinfo);
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

    if ((numbytes = wrap_recvfrom(ip_socket, buf, MAXBUFLEN-1 , 0,
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

void network_send_packet(netsrc_t sock, int length, const void *data, netadr_t dest_net)
{
    system_send_packet(length, data, dest_net);
}

void network_send_command(const char *command, netadr_t dest)
{
    if(strcmp(command, "server_info") == 0) {
        network_send_packet(ip_socket, strlen(command), "server_info", dest);
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

    if (!ip_socket) {
        return 0;
    }

    FD_ZERO(&fdset);
    FD_SET(ip_socket, &fdset); // network socket
    timeout.tv_sec = msec / 1000;
    timeout.tv_usec = (msec % 1000) * 1000;
    return select(ip_socket + 1, &fdset, NULL, NULL, &timeout);
}

void network_get_my_ip(char *subnet, size_t len) {

    struct sockaddr_in dest = {0};
    dest.sin_family = AF_INET;
    dest.sin_port = htons(53); // DNS
    inet_pton(AF_INET, "8.8.8.8", &dest.sin_addr);

    int sockfd = network_get_socket();
    if(sockfd == -1) {
        perror("[-] Error creating socket");
        return;
    }

    connect(sockfd, (struct sockaddr *)&dest, sizeof(dest));

    struct sockaddr_in local;
    len = sizeof(local);
    getsockname(sockfd, (struct sockaddr *)&local, &len);

    strncpy(subnet, inet_ntoa(local.sin_addr), len);

    network_close_socket(sockfd);
}
