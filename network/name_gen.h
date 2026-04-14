
#pragma once

#include <stddef.h>

#define NAME_GEN_MAX_LEN 32

// Fill buf with a randomly generated "adjective_noun" name.
// 16 adjectives x 16 nouns = 256 unique combinations.
void name_gen_random(char *buf, size_t len);
