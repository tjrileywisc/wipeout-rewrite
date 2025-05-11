#include <network.h>

#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <setjmp.h>
#include <cmocka.h>

#include <sys/socket.h>
#include <unistd.h>
 
/* A test case that does nothing and succeeds. */
void test_server(void **state) {
    (void) state; /* unused */
}

void enqueues_client_msg(void **state) {
    (void) state;

    //will_return(server_get_queue, true);

    //server_get_queue();

    //will_return(server_get_queue, true);
}