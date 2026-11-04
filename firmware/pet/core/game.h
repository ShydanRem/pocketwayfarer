#pragma once
/* Pocket Wayfarer — game state shared by firmware and host tools.
 *
 * Everything in core/ is plain C11 with integer arithmetic only: no LVGL,
 * no ESP-IDF and no floating point, so that the host tests, the preview
 * renderer and every linked device compute exactly the same results. */
#include <stdbool.h>
#include <stdint.h>
#include "rng.h"

#define WF_NAME_LEN 12
#define WF_NEED_MAX 100000      /* needs are stored in milli-percent */
#define WF_MAX_LEVEL 40
#define WF_FRIENDS 12
#define WF_QUESTS 3
#define WF_MESS_MAX 3
#define WF_ITEM_STACK 99

/* ---- Elements ------------------------------------------------------- */
typedef enum {
    WF_EL_NONE = 0,
    WF_EL_EMBER,   /* Brace  beats Fronda */
    WF_EL_TIDE,    /* Marea  beats Brace  */
    WF_EL_GROVE,   /* Fronda beats Marea  */
    WF_EL_COUNT
} wf_element;

/* ---- Companion forms ------------------------------------------------ */
typedef enum {
    WF_STAGE_SEED = 0,
    WF_STAGE_BABY,
    WF_STAGE_TEEN,
    WF_STAGE_ADULT
} wf_stage;

typedef enum {
    WF_FORM_SEED = 0,
    WF_FORM_LUMINO,
    WF_FORM_FAVILLO,      /* ember teen */
    WF_FORM_SPRUZZO,      /* tide teen  */
    WF_FORM_FOGLIOLO,     /* grove teen */
    WF_FORM_PIROLUME,     /* ember adult, good care */
    WF_FORM_CENEROTTO,    /* ember adult, wild      */
    WF_FORM_CORALBA,      /* tide adult, good care  */
    WF_FORM_ABISSINO,     /* tide adult, wild       */
    WF_FORM_SILVANTE,     /* grove adult, good care */
    WF_FORM_ROVACCIO,     /* grove adult, wild      */
    WF_FORM_COUNT
} wf_form;

typedef struct {
    const char *name;
    const char *blurb;
    uint8_t stage;
    uint8_t element;
    /* level-1 stats and growth per level (x10 for fractional growth) */
    int16_t hp, atk, def, spd;
    int16_t g_hp, g_atk, g_def, g_spd;
} wf_form_info;

const wf_form_info *wf_form_get(int form);

/* ---- Items ---------------------------------------------------------- */
typedef enum {
    WF_IT_BERRY = 0,     /* Bacca lumina          */
    WF_IT_BREAD,         /* Pane al miele         */
    WF_IT_STEW,          /* Stufato del viandante */
    WF_IT_CANDY,         /* Caramella astrale     */
    WF_IT_CHILI,         /* Peperoncino di brace  */
    WF_IT_KELP,          /* Alga dolce            */
    WF_IT_SPROUT,        /* Germoglio tenero      */
    WF_IT_POTION,        /* Pozione               */
    WF_IT_TONIC,         /* Tonico di focus       */
    WF_IT_SHARD,         /* Frammento d'astro     */
    WF_IT_COUNT
} wf_item;

typedef enum { WF_KIND_FOOD, WF_KIND_BATTLE, WF_KIND_TREASURE } wf_item_kind;

typedef struct {
    const char *name;
    const char *blurb;
    uint8_t kind;
    uint16_t price;          /* 0 = not sold */
    int32_t hunger, energy, mood;
    uint8_t element;         /* affinity granted when eaten */
    uint8_t affinity;
} wf_item_info;

const wf_item_info *wf_item_get(int item);

/* ---- Companion ------------------------------------------------------ */
typedef struct {
    char name[WF_NAME_LEN + 1];
    uint8_t form;
    uint8_t level;
    uint32_t xp;
    int32_t hunger, energy, mood;   /* 0..WF_NEED_MAX */
    int32_t bond;                   /* 0..WF_NEED_MAX */
    uint8_t sleeping;
    uint8_t mess;
    uint16_t care_mistakes;
    uint16_t affinity[WF_EL_COUNT];
    int16_t train_hp, train_atk, train_def, train_spd;
    uint16_t hatch_heat;            /* seed: warmth from taps */
    uint8_t pending_form;           /* evolution waiting for the cutscene */
    uint32_t born_at;
    uint32_t last_update;
    uint32_t last_pat;
    uint16_t mess_timer;            /* minutes since last mess spawned */
    uint16_t neglect_minutes;
    /* Expedition */
    uint32_t away_until;
    uint32_t away_since;
    uint32_t away_seed;
    uint8_t away_zone;
} wf_pet;

typedef enum {
    WF_Q_FEED = 0, WF_Q_PLAY, WF_Q_WIN, WF_Q_EXPLORE, WF_Q_GREET, WF_Q_CLEAN, WF_Q_TRAIN,
    WF_Q_COUNT
} wf_quest_kind;

typedef struct {
    uint8_t kind;
    uint8_t goal;
    uint8_t progress;
    uint8_t claimed;
    uint16_t coins;
    uint8_t item;
} wf_quest;

typedef struct {
    uint8_t used;
    uint8_t id[6];
    char name[WF_NAME_LEN + 1];
    uint8_t form;
    uint8_t level;
    uint32_t last_greet_day;
    uint16_t greets;
    uint16_t duels;
} wf_friend;

typedef struct {
    wf_pet pet;
    uint32_t coins;
    uint8_t inv[WF_IT_COUNT];
    uint8_t zone_unlocked;          /* highest zone index the player may enter */
    uint8_t zone_progress[8];       /* wins toward the zone boss */
    uint16_t wins, losses;
    uint16_t duel_wins, duel_losses;
    uint32_t day;                   /* last day index seen */
    uint8_t streak;
    uint8_t daily_claimed;
    wf_quest quests[WF_QUESTS];
    wf_friend friends[WF_FRIENDS];
    uint32_t rng_state;
    uint32_t feeds, plays, explores;
    uint8_t brightness;
    uint8_t sound;
    uint8_t hour_offset;            /* added to RTC hours for day/night (0..23) */
} wf_game;

/* ---- Lifecycle ------------------------------------------------------ */
void wf_game_new(wf_game *g, const char *name, uint32_t now, uint32_t seed);
/* Advances the simulation to `now`. Safe to call often; offline catch-up is
 * capped at 14 days. Returns true when anything visible changed. */
bool wf_game_tick(wf_game *g, uint32_t now);
/* New-day bookkeeping (streak, fresh quests). Called by wf_game_tick. */
void wf_game_roll_day(wf_game *g, uint32_t day);

/* ---- Derived values -------------------------------------------------- */
typedef struct { int hp, atk, def, spd; } wf_stats;
wf_stats wf_pet_stats(const wf_game *g);
uint32_t wf_xp_to_next(int level);
int wf_pet_element(const wf_game *g);
int wf_pet_stage(const wf_game *g);
bool wf_pet_away(const wf_game *g, uint32_t now);
bool wf_pet_can_act(const wf_game *g, uint32_t now);
int wf_need_percent(int32_t v);
typedef enum { WF_MOOD_GREAT, WF_MOOD_OK, WF_MOOD_HUNGRY, WF_MOOD_TIRED, WF_MOOD_SAD, WF_MOOD_SLEEP } wf_mood_face;
int wf_pet_face(const wf_game *g);
const char *wf_pet_status_text(const wf_game *g, uint32_t now);

/* ---- Care ------------------------------------------------------------ */
typedef enum {
    WF_OK = 0,
    WF_ERR_ASLEEP,
    WF_ERR_AWAY,
    WF_ERR_FULL,
    WF_ERR_NONE_LEFT,
    WF_ERR_NOT_TIRED,
    WF_ERR_TIRED,
    WF_ERR_NO_MESS,
    WF_ERR_COINS,
    WF_ERR_SEED,
    WF_ERR_LOCKED,
    WF_ERR_COOLDOWN,
    WF_ERR_STACK
} wf_result;

const char *wf_result_text(int r);

int wf_feed(wf_game *g, int item, uint32_t now);
int wf_sleep_toggle(wf_game *g, uint32_t now);
int wf_clean(wf_game *g, uint32_t now);
int wf_pat(wf_game *g, uint32_t now);
/* Seed: each tap warms it; returns true when it hatches. */
bool wf_warm_seed(wf_game *g, uint32_t now);
/* Minigame "Pioggia di stelle": score 0..30 caught stars. */
int wf_play_finish(wf_game *g, int score, uint32_t now, int *coins_out);
/* Training: stat 0=atk 1=def 2=spd 3=hp; quality_sum from five ring swings (0..15). */
int wf_train_finish(wf_game *g, int stat, int quality_sum, uint32_t now, int *gain_out);
int wf_buy(wf_game *g, int item);
bool wf_give_item(wf_game *g, int item, int count);
/* XP: returns number of levels gained. Sets pending evolution if due. */
int wf_gain_xp(wf_game *g, uint32_t xp);
/* Applies a pending evolution; returns the new form or -1. */
int wf_evolve(wf_game *g);
int wf_evolution_target(const wf_game *g);

/* ---- Quests & daily -------------------------------------------------- */
void wf_quest_bump(wf_game *g, int kind, int amount);
int wf_quest_claim(wf_game *g, int index);
const char *wf_quest_text(const wf_quest *q, char *buf, int cap);
/* Returns coins granted (0 when already claimed today). */
int wf_daily_claim(wf_game *g, int *item_out);

/* ---- Friends --------------------------------------------------------- */
wf_friend *wf_friend_find(wf_game *g, const uint8_t id[6]);
wf_friend *wf_friend_upsert(wf_game *g, const uint8_t id[6], const char *name, int form, int level);
/* Greeting a friend: +mood/+bond once per friend per day. */
bool wf_greet(wf_game *g, const uint8_t id[6], const char *name, int form, int level, uint32_t now);

uint32_t wf_day_of(uint32_t now);
void wf_copy_name(char *dst, const char *src);
