
#include "utils.h"

#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>

extern void name_gen_valid_uppercase(void **state);
extern void name_gen_valid_digits(void **state);
extern void name_gen_valid_empty(void **state);
extern void name_gen_invalid_lowercase(void **state);
extern void name_gen_invalid_mixed_case(void **state);
extern void name_gen_invalid_underscore(void **state);
extern void name_gen_invalid_punctuation(void **state);

extern void test_network_get_packet(void **state);
extern void test_network_get_packet_no_data(void **state);
extern void test_network_get_local_subnet(void **state);
extern void network_close_socket_sets_socket_invalid(void **state);


extern void empties_queue_after_process(void **state);
extern void unknown_message_echo(void **state);
extern void server_status_query(void **state);
extern void server_connect_client_ok(void **state);
extern void server_connect_fails_too_many_clients(void **state);

int main(void) {
    const struct CMUnitTest name_gen_tests[] = {
        cmocka_unit_test(name_gen_valid_uppercase),
        cmocka_unit_test(name_gen_valid_digits),
        cmocka_unit_test(name_gen_valid_empty),
        cmocka_unit_test(name_gen_invalid_lowercase),
        cmocka_unit_test(name_gen_invalid_mixed_case),
        cmocka_unit_test(name_gen_invalid_underscore),
        cmocka_unit_test(name_gen_invalid_punctuation),
    };

    const struct CMUnitTest network_tests[] = {
        // network library tests
        cmocka_unit_test(test_network_get_packet),
        cmocka_unit_test(test_network_get_packet_no_data),
        cmocka_unit_test(test_network_get_local_subnet),
        cmocka_unit_test(network_close_socket_sets_socket_invalid),


    };

    const struct CMUnitTest server_tests[] = {
        cmocka_unit_test_prestate_setup_teardown(empties_queue_after_process, server_test_setup, network_test_cleanup, NULL),
        cmocka_unit_test_prestate_setup_teardown(unknown_message_echo, server_test_setup, network_test_cleanup, NULL),
        cmocka_unit_test_prestate_setup_teardown(server_status_query, server_test_setup, network_test_cleanup, NULL),
        cmocka_unit_test_prestate_setup_teardown(server_connect_client_ok, server_test_setup, network_test_cleanup, NULL),
        cmocka_unit_test_prestate_setup_teardown(server_connect_fails_too_many_clients, server_test_setup, network_test_cleanup, NULL),
    };
 
    return cmocka_run_group_tests_name("name_gen_tests", name_gen_tests, NULL, NULL) ||
           cmocka_run_group_tests_name("network_tests", network_tests, NULL, NULL) ||
           cmocka_run_group_tests_name("server_tests", server_tests, NULL, NULL);
}