
#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>

extern void test_network_get_packet(void **state);
extern void test_network_get_packet_no_data(void **state);
extern void test_network_get_local_subnet(void **state);

extern void empties_queue_after_process(void **state);

int main(void) {
    const struct CMUnitTest tests[] = {
        // network library tests
        cmocka_unit_test(test_network_get_packet),
        cmocka_unit_test(test_network_get_packet_no_data),
        cmocka_unit_test(test_network_get_local_subnet),

        // dedicated server tests
        //cmocka_unit_test(empties_queue_after_process)
    };
 
    return cmocka_run_group_tests(tests, NULL, NULL);
}