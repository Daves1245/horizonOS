#ifndef RAND_H
#define RAND_H

#include <stdint.h>

int init_rand();
void srand(uint32_t seed);
uint64_t randul();
uint32_t randu();
int64_t randl();
int32_t rand();
uint32_t rand_range(uint32_t min, uint32_t max);

#endif
