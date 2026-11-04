#include "rng.h"

void wf_rng_seed(wf_rng *r, uint32_t seed) {
    /* splitmix-style scramble so that small seeds still look random */
    seed += 0x9E3779B9u;
    seed = (seed ^ (seed >> 16)) * 0x85EBCA6Bu;
    seed = (seed ^ (seed >> 13)) * 0xC2B2AE35u;
    seed ^= seed >> 16;
    r->s = seed ? seed : 0xA5A5A5A5u;
}

uint32_t wf_rng_next(wf_rng *r) {
    uint32_t x = r->s;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    r->s = x;
    return x;
}

int wf_rng_range(wf_rng *r, int lo, int hi) {
    if (hi <= lo) return lo;
    uint32_t span = (uint32_t)(hi - lo) + 1u;
    return lo + (int)(wf_rng_next(r) % span);
}

int wf_rng_chance(wf_rng *r, int percent) {
    return wf_rng_range(r, 0, 99) < percent;
}
