
#pragma once

#include <stdbool.h>
#include <stddef.h>

#define NAME_GEN_MAX_LEN 32

// Fill buf with a randomly generated "ADJECTIVE NOUN" name.
// 16 adjectives x 16 nouns = 256 unique combinations.
void name_gen_random(char *buf, size_t len);

// Returns true if every character in name is valid for the UI charset (A-Z, 0-9, space).
bool name_gen_is_valid(const char *name);
