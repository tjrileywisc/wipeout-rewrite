
#include "server_com.h"

#include <network.h>

void client_init() {
    network_connect_ip("localhost");
}

void server_com_network_discovery() {

    /*
    int sockfd = network_get_ip_socket();

    // Enable broadcast option
    int broadcastEnable = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, &broadcastEnable, sizeof(broadcastEnable));

    // Set receive timeout
    struct timeval timeout;
    timeout.tv_sec = 30;
    timeout.tv_usec = 0;
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

    memset(&broadcast_addr, 0, sizeof(broadcast_addr));
    broadcast_addr.sin_family = AF_INET;
    broadcast_addr.sin_port = htons(8000);
    inet_pton(AF_INET, BROADCAST_IP, &broadcast_addr.sin_addr);

    const char *message = "discovery_ping";
    sendto(sockfd, message, strlen(message), 0,
           (struct sockaddr *)&broadcast_addr, sizeof(broadcast_addr));

    printf("[*] Broadcast packet sent. Waiting for replies...\n");

    while (1) {
        struct sockaddr_in sender;
        socklen_t sender_len = sizeof(sender);
        ssize_t n = recvfrom(sockfd, buffer, BUF_SIZE - 1, 0,
                             (struct sockaddr *)&sender, &sender_len);

        if (n < 0) {
            printf("[-] Timeout reached. No more responses.\n");
            break;
        }

        buffer[n] = '\0';
        printf("[+] Response from %s:%d: %s\n",
               inet_ntoa(sender.sin_addr), ntohs(sender.sin_port), buffer);
    }
    */
}