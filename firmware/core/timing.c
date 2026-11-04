#include "timing.h"
#include "battle.h"

int wf_angle_diff(int a, int b) {
    int d = (a - b) % 360;
    if (d < -180) d += 360;
    if (d >= 180) d -= 360;
    return d;
}

static int clampi(int v, int lo, int hi) { return v < lo ? lo : v > hi ? hi : v; }

void wf_ring_make(wf_ring *r, wf_rng *rng, int spd, int foe_level, int mood_percent) {
    /* Faster foes spin the spark faster; a quick, happy companion widens the arc. */
    r->period_ms = (uint16_t)clampi(1700 - foe_level * 22, 900, 1700);
    r->start = (int16_t)wf_rng_range(rng, 0, 359);
    /* Keep the target at least a third of a lap away from the start. */
    r->center = (int16_t)((r->start + wf_rng_range(rng, 130, 250)) % 360);
    int width = 26 + spd / 4 + (mood_percent - 50) / 10;
    r->good = (uint8_t)clampi(width, 18, 48);
    r->great = (uint8_t)clampi(r->good * 55 / 100, 9, 26);
    r->perfect = (uint8_t)clampi(r->good * 22 / 100, 4, 10);
}

int wf_ring_angle(const wf_ring *r, uint32_t elapsed_ms) {
    uint32_t a = (uint32_t)r->start + (elapsed_ms % r->period_ms) * 360u / r->period_ms;
    return (int)(a % 360u);
}

int wf_ring_judge(const wf_ring *r, uint32_t elapsed_ms) {
    if (wf_ring_expired(r, elapsed_ms)) return WF_Q_MISS;
    int d = wf_angle_diff(wf_ring_angle(r, elapsed_ms), r->center);
    if (d < 0) d = -d;
    if (d <= r->perfect) return WF_Q_PERFECT;
    if (d <= r->great) return WF_Q_GREAT;
    if (d <= r->good) return WF_Q_GOOD;
    return WF_Q_MISS;
}

int wf_ring_expired(const wf_ring *r, uint32_t elapsed_ms) {
    return elapsed_ms >= (uint32_t)r->period_ms * 2u;
}

void wf_parry_make(wf_parry *p, wf_rng *rng, int spd, int foe_spd, int boss) {
    p->travel_ms = (uint16_t)clampi(1100 - foe_spd * 6 + wf_rng_range(rng, -120, 160) - (boss ? 80 : 0), 600, 1400);
    int bonus = clampi(spd / 3, 0, 30);
    p->perfect_ms = (uint8_t)(45 + bonus / 3);
    p->great_ms = (uint8_t)(95 + bonus / 2);
    p->good_ms = (uint8_t)(170 + bonus);
}

int wf_parry_judge(const wf_parry *p, uint32_t tap_ms) {
    int d = (int)tap_ms - (int)p->travel_ms;
    if (d < 0) d = -d;
    if (d <= p->perfect_ms) return WF_Q_PERFECT;
    if (d <= p->great_ms) return WF_Q_GREAT;
    if (d <= p->good_ms) return WF_Q_GOOD;
    return WF_Q_MISS;
}

int wf_parry_progress(const wf_parry *p, uint32_t elapsed_ms) {
    return (int)(elapsed_ms * 1000u / p->travel_ms);
}
