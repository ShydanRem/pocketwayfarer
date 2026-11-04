#pragma once
/* Touch-timing minigames used by battles and training.
 *
 * Ring: a spark orbits the rim of the round display; tap while it crosses the
 * golden arc. Parry: a shockwave closes in from the rim; tap when it reaches
 * the guard circle. Both turn a tap time into a quality WF_Q_MISS..PERFECT. */
#include <stdint.h>
#include "rng.h"

typedef struct {
    uint16_t period_ms;     /* one full revolution */
    int16_t start;          /* spark angle at t=0, degrees clockwise from 12 o'clock */
    int16_t center;         /* centre of the target arc */
    uint8_t good, great, perfect;   /* half-widths in degrees */
} wf_ring;

/* spd: companion speed, foe_level: opponent level (0 for training). */
void wf_ring_make(wf_ring *r, wf_rng *rng, int spd, int foe_level, int mood_percent);
int wf_ring_angle(const wf_ring *r, uint32_t elapsed_ms);
int wf_ring_judge(const wf_ring *r, uint32_t elapsed_ms);
/* After two laps without a tap the swing counts as a miss. */
int wf_ring_expired(const wf_ring *r, uint32_t elapsed_ms);
/* Signed angular distance a-b in -180..179. */
int wf_angle_diff(int a, int b);

typedef struct {
    uint16_t travel_ms;     /* time for the wave to reach the guard circle */
    uint8_t perfect_ms, great_ms, good_ms;
} wf_parry;

void wf_parry_make(wf_parry *p, wf_rng *rng, int spd, int foe_spd, int boss);
int wf_parry_judge(const wf_parry *p, uint32_t tap_ms);
/* Wave radius progress 0..1000 (1000 = on the guard circle), keeps growing after. */
int wf_parry_progress(const wf_parry *p, uint32_t elapsed_ms);
