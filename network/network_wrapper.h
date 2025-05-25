#pragma once

#if defined(_WIN32)
#include <WinSock2.h>
#include "win_defines.h"
#else
#endif

ssize_t wrap_recvfrom(int sockfd, void *buf, size_t len, int flags,
                    struct sockaddr *src_addr, socklen_t *addrlen);
