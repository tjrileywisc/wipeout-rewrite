#include <network.h>
#include <client.h>

#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <setjmp.h>
#include <cmocka.h>

#include <string.h>

#if defined(WIN32)
#include <WinSock2.h>
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#endif

void empties_queue_after_process(void**) {

    network_clear_msg_queue();
    const char *mock_data = "hello";
    struct sockaddr_in mock_addr = {0};
    socklen_t mock_addr_len = sizeof(mock_addr);

    expect_value(wrap_recvfrom, sockfd, 3);
    expect_value(wrap_recvfrom, len, 99);
    expect_value(wrap_recvfrom, flags, 0);
    will_return(wrap_recvfrom, &mock_addr);        // for src_addr
    will_return(wrap_recvfrom, mock_addr_len);     // for addrlen
    will_return(wrap_recvfrom, mock_data);         // for buf content
    will_return(wrap_recvfrom, strlen(mock_data)); // return value

    network_set_bound_ip_socket(3); // Set the socket descriptor

    network_get_packet();

    int queue_size = network_get_msg_queue_size();
    assert_int_equal(queue_size, 1);
    
    // should run a step to check if we have work to do
    server_process_queue();

    queue_size = network_get_msg_queue_size();
    assert_int_equal(queue_size, 0);
}