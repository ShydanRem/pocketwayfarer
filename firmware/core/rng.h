#pragma once
/* Small deterministic PRNG shared by every device. Integer-only so that two
 * Pockets resolving the same duel with the same seed always agree. */
#include <stdint.h>

typedef struct { uint32_t s; } wf_rng;

void wf_rng_seed(wf_rng *r, uint32_t seed);
uint32_t wf_rng_next(wf_rng *r);
/* Uniform integer in [lo, hi] (inclusive). */
int wf_rng_range(wf_rng *r, int lo, int hi);
/* True with probability percent/100. */
int wf_rng_chance(wf_rng *r, int percent);
