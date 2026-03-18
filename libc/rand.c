#include <rand.h>

static int has_rdrand = 0;
static uint64_t rng_state = 0;

int init_rand(void) {
  /* see https://wiki.osdev.org/Random_Number_Generator */
  /* CPUID leaf 1: ECX bit 30 = RDRAND support */
  uint32_t ecx;
  asm volatile (
      "mov $1, %%eax\n\t"
      "cpuid"
      : "=c"(ecx)
      :
      : "eax", "ebx", "edx"
  );
  has_rdrand = (ecx >> 30) & 1;

  if (!has_rdrand) {
    /* seed fallback PRNG from TSC */
    uint32_t lo, hi;
    asm volatile ("rdtsc" : "=a"(lo), "=d"(hi));
    rng_state = ((uint64_t)hi << 32) | lo;
    if (rng_state == 0) rng_state = 1;
  }

  return 0;
}

void srand(uint32_t seed) {
  rng_state = seed ? seed : 1;
}

/* xorshift64 PRNG fallback */
static uint64_t xorshift64(void) {
  uint64_t x = rng_state;
  x ^= x << 13;
  x ^= x >> 7;
  x ^= x << 17;
  rng_state = x;
  return x;
}

static uint64_t rdrand64(void) {
  uint64_t val;
  unsigned char ok;
  /* retry up to 10 times on RDRAND failure */
  for (int i = 0; i < 10; i++) {
    asm volatile (
        "rdrand %0\n\t"
        "setc %1"
        : "=r"(val), "=qm"(ok)
    );
    if (ok) return val;
  }
  /* hardware failed, fall back */
  return xorshift64();
}

uint64_t randul(void) {
  return has_rdrand ? rdrand64() : xorshift64();
}

uint32_t randu(void) {
  return (uint32_t)randul();
}

int64_t randl(void) {
  return (int64_t)randul();
}

int32_t rand(void) {
  return (int32_t)randu();
}

uint32_t rand_range(uint32_t min, uint32_t max) {
  if (min >= max) return min;
  return min + randu() % (max - min);
}
