
#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>

extern void test_network(void **state);
extern void test_network_get_packet(void **state);
extern void test_network_get_packet_no_data(void **state);

extern void test_server(void **state);

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_network),
        cmocka_unit_test(test_network_get_packet),
        cmocka_unit_test(test_network_get_packet_no_data),
        cmocka_unit_test(test_server),
    };
 
    return cmocka_run_group_tests(tests, NULL, NULL);
}