
#include "mocks.h"

#include <network.h>

#include <netinet/in.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <setjmp.h>
#include <cmocka.h>

void test_network_get_packet(void **state) {
    (void) state;

    const char *mock_data = "Hello, World!";
    struct sockaddr_in mock_addr = {0};
    socklen_t mock_addr_len = sizeof(mock_addr);

    expect_value(__wrap_recvfrom, sockfd, 3);
    expect_value(__wrap_recvfrom, len, 99);
    expect_value(__wrap_recvfrom, flags, 0);
    will_return(__wrap_recvfrom, &mock_addr);        // for src_addr
    will_return(__wrap_recvfrom, mock_addr_len);     // for addrlen
    will_return(__wrap_recvfrom, mock_data);         // for buf content
    will_return(__wrap_recvfrom, strlen(mock_data)); // return value

    network_set_ip_socket(3); // Set the socket descriptor

    bool result = network_get_packet();

    // Check the result
    assert_true(result);

    // check message queue, should have one item
    int queue_size = network_get_msg_queue_size();
    assert_int_equal(queue_size, 1);
}

void test_network_get_packet_no_data(void **state) {
    (void) state;

    const char *mock_data = "Hello, World!";
    struct sockaddr_in mock_addr = {0};
    socklen_t mock_addr_len = sizeof(mock_addr);

    expect_value(__wrap_recvfrom, sockfd, 3);
    expect_value(__wrap_recvfrom, len, 99);
    expect_value(__wrap_recvfrom, flags, 0);
    will_return(__wrap_recvfrom, &mock_addr);        // for src_addr
    will_return(__wrap_recvfrom, mock_addr_len);     // for addrlen
    will_return(__wrap_recvfrom, mock_data);         // for buf content
    will_return(__wrap_recvfrom, -1); // return value

    network_set_ip_socket(3); // Set the socket descriptor

    bool result = network_get_packet();

    // TODO: this is unreachable unless we set the errno to EAGAIN
    // Check the result
    assert_false(result);

    // check message queue, should still be empty
    int queue_size = network_get_msg_queue_size();
    assert_int_equal(queue_size, 0);
}
