
#include "mocks.h"

#include <network.h>

#include <errno.h>
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

void test_network_get_packet_no_data(void**) {
    network_clear_msg_queue();

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
    errno = EAGAIN; // Simulate no data available
    bool result = network_get_packet();
    assert_false(result);

    // check message queue, should still be empty
    int queue_size = network_get_msg_queue_size();
    assert_int_equal(queue_size, 0);
}

void test_network_get_local_subnet(void **) {
    char my_ip[INET_ADDRSTRLEN];
    network_get_my_ip(my_ip, INET_ADDRSTRLEN);

    assert_non_null(my_ip);
    assert_string_not_equal(my_ip, "127.0.0.1");
}
