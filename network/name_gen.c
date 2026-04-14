
#include "name_gen.h"

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

static const char *adjectives[] = {
    "QUANTUM", "TURBO",    "NEON",     "HYPER",
    "SONIC",   "CHROME",   "PULSE",    "VOID",
    "DARK",    "SWIFT",    "BLAZING",  "STELLAR",
    "COSMIC",  "PHANTOM",  "ELECTRIC", "FLUX"
};

static const char *nouns[] = {
    "RACER",  "PILOT",  "VIPER",  "FALCON",
    "STORM",  "COMET",  "NOVA",   "BLADE",
    "HAWK",   "FURY",   "BOLT",   "VECTOR",
    "FLASH",  "ARROW",  "ZERO",   "APEX"
};

void name_gen_random(char *buf, size_t len) {
    int adj  = rand() % 16;
    int noun = rand() % 16;
    snprintf(buf, len, "%s %s", adjectives[adj], nouns[noun]);
}
