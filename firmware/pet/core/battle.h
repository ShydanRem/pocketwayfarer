#pragma once
/* Turn-based battle engine.
 *
 * The UI turns touch timing into a quality (0..3) with timing.h; the engine
 * only sees actions and qualities, so it is fully deterministic and shared by
 * solo battles (pet vs wild foe) and linked duels (pet vs pet). */
#include <stdbool.h>
#include <stdint.h>
#include "game.h"
#include "timing.h"

typedef enum {
    WF_ACT_STRIKE = 0,
    WF_ACT_SKILL,
    WF_ACT_GUARD,
    WF_ACT_POTION,
    WF_ACT_TONIC,
    WF_ACT_FLEE,
    WF_ACT_COUNT
} wf_action;

#define WF_FOCUS_MAX 3
#define WF_SKILL_COST 2

typedef struct {
    char name[WF_NAME_LEN + 4];
    uint8_t element;
    uint8_t level;
    uint8_t sprite;          /* form id for pets, species id for foes */
    uint8_t is_pet;
    int16_t hp, max_hp;
    int16_t atk, def, spd;
    uint8_t focus;
    uint8_t burn;            /* turns of burn left */
    uint8_t rooted;          /* next attack weakened */
    uint8_t guarding;
} wf_fighter;

typedef enum {
    WF_EV_HIT = 0,      /* value = damage */
    WF_EV_GUARD,        /* actor raises guard */
    WF_EV_HEAL,         /* value = hp restored */
    WF_EV_BURN,         /* value = burn damage taken by actor */
    WF_EV_IGNITE,       /* actor set target ablaze */
    WF_EV_ROOT,         /* actor rooted target */
    WF_EV_COUNTER,      /* value = counter damage dealt by actor */
    WF_EV_FOCUS,        /* value = new focus */
    WF_EV_KO,           /* actor is knocked out */
    WF_EV_FLEE_OK,
    WF_EV_FLEE_FAIL,
    WF_EV_NO_FOCUS
} wf_event_kind;

typedef struct {
    uint8_t kind;
    uint8_t actor;      /* 0 = side A (pet / inviter), 1 = side B */
    uint8_t quality;
    uint8_t effect;     /* 0 normal, 1 super effective, 2 resisted */
    uint8_t skill;      /* hit came from a skill */
    int16_t value;
} wf_event;

#define WF_EVENTS 12

typedef enum {
    WF_B_PLAYER = 0,    /* waiting for side A's action        */
    WF_B_FOE,           /* solo only: waiting for parry input */
    WF_B_WON,
    WF_B_LOST,
    WF_B_FLED
} wf_battle_state;

typedef struct {
    wf_fighter f[2];
    wf_rng rng;
    uint8_t state;
    uint8_t boss;
    uint8_t duel;
    uint16_t round;
    uint8_t foe_intent;      /* solo: action the foe is about to use */
    wf_event ev[WF_EVENTS];
    uint8_t ev_count;
} wf_battle;

void wf_fighter_from_pet(wf_fighter *out, const wf_game *g);
void wf_battle_start(wf_battle *b, const wf_fighter *a, const wf_fighter *b_side,
                     uint32_t seed, bool boss, bool duel);
/* Element multiplier x100 for attacker element vs defender element. */
int wf_element_mult(int attacker, int defender);
const char *wf_element_name(int element);
/* Damage preview helpers (also used by tests). */
int wf_damage(const wf_fighter *att, const wf_fighter *def, int power, int quality, int roll);

/* Solo battle --------------------------------------------------------- */
/* Side A acts. Potion/tonic use is applied here; the caller removes the item. */
void wf_battle_player(wf_battle *b, int action, int quality);
/* Side B (the foe) attacks; `parry` is the player's defensive timing quality. */
void wf_battle_foe(wf_battle *b, int parry);
bool wf_battle_over(const wf_battle *b);

/* Linked duel: both sides choose simultaneously, then resolve together. */
void wf_duel_round(wf_battle *b, int act_a, int q_a, int act_b, int q_b);

/* Rewards for winning a solo battle. */
uint32_t wf_battle_xp(const wf_battle *b);
uint32_t wf_battle_coins(const wf_battle *b);
