#pragma once
/* Lanternbound vertical slice: native, deterministic and fully serializable.
 * No LVGL pointers or wall-clock deadlines are part of the run. */
#include <stdbool.h>
#include <stdint.h>
#include "i18n.h"

enum {
    LN_NONE, LN_ROUTE, LN_CHOOSE, LN_ATTACK, LN_TELEGRAPH, LN_PARRY,
    LN_IMPACT, LN_RELIC, LN_EVENT, LN_CLEARED, LN_BLUEPRINT, LN_WON, LN_LOST, LN_RETURNED,
    LN_STATE_COUNT
};
enum { LN_SENTINEL, LN_MAGE, LN_ROGUE, LN_ARCHETYPES };
enum { LN_BASIC, LN_POWER, LN_SUPPORT, LN_GUARD };
enum { LN_ROOT, LN_WISP, LN_BOSS, LN_WOLF, LN_CRAB, LN_BAT, LN_MOTH, LN_ENEMIES };
enum { LN_HIT, LN_HEAVY, LN_DOUBLE, LN_MEND };
enum { LR_HEART, LR_THORN, LR_GLASS, LR_BELL, LR_COAL, LR_COMPASS, LR_COUNT };
enum { LN_FEEDBACK_HERO = 1, LN_FEEDBACK_FOE, LN_FEEDBACK_HEAL, LN_FEEDBACK_BURN };
enum { LN_EFFECT_RESONANCE = 1u, LN_EFFECT_BURN = 2u };

typedef struct {
    int16_t hp, max_hp;
    uint8_t kind, intent, burn, burn_turns;
} ln_foe;

typedef struct {
    /* Persistent profile/options, preserved when starting/retrying a run. */
    uint16_t wins;
    uint8_t blueprints, assist, reduced_motion;
    /* Run snapshot. Every rule-relevant field is saved, including timing. */
    uint8_t state, archetype, room, branch, nfoes, target, acting;
    uint8_t action, next_state, resonance, relics, offers[2], turn;
    uint8_t paused, guard, shield, focused, quality, feedback;
    int16_t hp, max_hp, sp, max_sp;
    int16_t last_damage;
    uint16_t elapsed_ms, currency, settled;
    uint32_t rng;
    ln_foe foes[2];
    /* Appended in v3; the original 64 bytes are the v2 snapshot ABI. */
    uint8_t equipped; /* blueprint id + 1, zero = no starting relic */
    uint8_t seen;     /* bestiary discovery bits */
    /* v4 uses the former tail padding; v1/v2/v3 prefix sizes stay unchanged. */
    uint8_t effect_actor, effect_flags;
} rpg_lantern;

wf_text ln_archetype_name(int archetype);
wf_text ln_skill_name(int archetype, int skill);
wf_text ln_relic_name(int relic);
wf_text ln_relic_text(int relic);
wf_text ln_enemy_name(int kind);
wf_text ln_intent_name(int intent);
bool ln_active(const rpg_lantern *l);
void ln_start(rpg_lantern *l, int archetype, uint32_t seed);
void ln_home(rpg_lantern *l);
bool ln_enter(rpg_lantern *l, int branch);
bool ln_action(rpg_lantern *l, int action, int target);
int ln_quality(const rpg_lantern *l);
bool ln_tap(rpg_lantern *l);
/* Returns true when a discrete state transition occurs. Paused runs do not tick. */
bool ln_tick(rpg_lantern *l, unsigned delta_ms);
bool ln_choose_relic(rpg_lantern *l, int choice);
int ln_relic_count(const rpg_lantern *l);
/* Full loadouts require one atomic exchange; browsing never removes an item. */
bool ln_replace_relic(rpg_lantern *l, int choice, int outgoing);
bool ln_skip_relic(rpg_lantern *l);
bool ln_keep_blueprint(rpg_lantern *l, int relic);
bool ln_event(rpg_lantern *l, int choice);
bool ln_continue(rpg_lantern *l);
bool ln_retreat(rpg_lantern *l);
/* Idempotent bank transfer: caller adds result to its persistent currency. */
unsigned ln_bank(rpg_lantern *l);
void ln_sanitize(rpg_lantern *l);
