
#include <name_gen.h>

#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>

void name_gen_valid_uppercase(void **state) {
    (void)state;
    assert_true(name_gen_is_valid("BLAZING APEX"));
}

void name_gen_valid_digits(void **state) {
    (void)state;
    assert_true(name_gen_is_valid("SHIP 42"));
}

void name_gen_valid_empty(void **state) {
    (void)state;
    assert_true(name_gen_is_valid(""));
}

void name_gen_invalid_lowercase(void **state) {
    (void)state;
    assert_false(name_gen_is_valid("blazing apex"));
}

void name_gen_invalid_mixed_case(void **state) {
    (void)state;
    assert_false(name_gen_is_valid("Blazing Apex"));
}

void name_gen_invalid_underscore(void **state) {
    (void)state;
    assert_false(name_gen_is_valid("BLAZING_APEX"));
}

void name_gen_invalid_punctuation(void **state) {
    (void)state;
    assert_false(name_gen_is_valid("SERVER!"));
}
