#include <kernel/rand.h>

// xorshift32 PRNG state
static uint32_t rng_state = 0;

void seed_rng(uint32_t seed) {
    // Avoid zero state
    rng_state = seed ? seed : 1;
}

uint32_t random_next(void) {
    uint32_t x = rng_state;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    rng_state = x;
    return x;
}

uint32_t random_range(uint32_t max) {
    if (max == 0) return 0;
    return random_next() % max;
}