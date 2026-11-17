#pragma once
#include "lantern.h"

enum {
    LNC_ATTACK = 1u, LNC_PARRY = 2u, LNC_GUARD = 4u,
    LNC_SKILL = 8u, LNC_RESONANCE = 16u, LNC_ALL = 31u
};
enum {
    LNT_NONE, LNT_ATTACK, LNT_ATTACK_TIMING, LNT_INTENT, LNT_PARRY_TIMING,
    LNT_GUARD, LNT_SKILL, LNT_RESONANCE
};
typedef struct { uint8_t learned, disabled; } ln_coach;

/* Pure presentation policy: never changes run state, RNG, timing or resources. */
int ln_coach_tip(const ln_coach *c, const rpg_lantern *l, bool skill_menu);
void ln_coach_observe(ln_coach *c, const rpg_lantern *before, const rpg_lantern *after);
void ln_coach_ack(ln_coach *c, int tip);
wf_text ln_coach_text(int tip);
wf_text ln_coach_caption(int tip); /* compact choice hint; leaves actors visible */
