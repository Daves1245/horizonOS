#ifndef _KERNEL_RAND_H
#define _KERNEL_RAND_H

#include <stdint.h>

// Seed the random number generator
void seed_rng(uint32_t seed);

// Get next random number (full 32-bit range)
uint32_t random_next(void);

// Get random number in range [0, max)
uint32_t random_range(uint32_t max);

#endif