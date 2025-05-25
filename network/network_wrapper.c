
#include "network_wrapper.h"

ssize_t wrap_recvfrom(int sockfd, void *buf, size_t len, int flags,
                    struct sockaddr *src_addr, socklen_t *addrlen) {
    return recvfrom(sockfd, buf, len, flags, src_addr, addrlen);
}