#pragma once
/* Zones, wild creatures, exploration steps and idle expeditions. */
#include <stdbool.h>
#include <stdint.h>
#include "game.h"
#include "battle.h"

typedef enum {
    WF_SP_GELATINO = 0, WF_SP_FUNGOTTO, WF_SP_SCINTILLO, WF_SP_RE_GELATINO,
    WF_SP_GRANCHIETTO, WF_SP_MEDUSINA, WF_SP_GELATINO_BLU, WF_SP_GRANCHIO_RE,
    WF_SP_RADICOTTO, WF_SP_PIPISTRELLO, WF_SP_GUFO, WF_SP_ANTICO_CEPPO,
    WF_SP_SALAMANDRA, WF_SP_GOLEM, WF_SP_BRACEPIPPO, WF_SP_DRAGO,
    WF_SP_TEMPESTINO, WF_SP_CRISTALLO, WF_SP_GRIFONCINO, WF_SP_CUSTODE,
    WF_SP_COUNT
} wf_species;

typedef enum {
    WF_ARCH_SLIME = 0, WF_ARCH_SHROOM, WF_ARCH_WISP, WF_ARCH_CRAB, WF_ARCH_JELLY,
    WF_ARCH_GOLEM, WF_ARCH_BAT, WF_ARCH_OWL, WF_ARCH_LIZARD, WF_ARCH_DRAGON,
    WF_ARCH_CRYSTAL, WF_ARCH_BIRD, WF_ARCH_COUNT
} wf_archetype;

typedef struct {
    const char *name;
    uint8_t element;
    uint8_t archetype;
    /* multipliers x100 applied to the level curve */
    uint16_t m_hp, m_atk, m_def, m_spd;
    uint8_t boss;
} wf_species_info;

const wf_species_info *wf_species_get(int sp);

#define WF_ZONE_COUNT 5
#define WF_BOSS_AFTER 6

typedef struct {
    const char *name;
    const char *blurb;
    uint8_t min_level, max_level;
    uint8_t foes[3];
    uint8_t boss;
    uint8_t boss_level;
    uint8_t scene;          /* background palette used by the UI */
    uint8_t drop;           /* signature food found here */
} wf_zone_info;

const wf_zone_info *wf_zone_get(int zone);

void wf_foe_make(wf_fighter *out, int species, int level);

/* ---- Exploration ------------------------------------------------------ */
typedef enum {
    WF_STEP_BATTLE = 0,
    WF_STEP_BOSS,
    WF_STEP_ITEM,
    WF_STEP_COINS,
    WF_STEP_SPRING,
    WF_STEP_CHEST
} wf_step_kind;

typedef struct {
    uint8_t kind;
    uint8_t species;
    uint8_t level;
    uint8_t item;
    uint8_t count;
    uint16_t coins;
} wf_step;

/* One exploration step in `zone`. Costs energy and hunger. For battle steps
 * the caller starts a wf_battle with wf_foe_make(step.species, step.level). */
int wf_explore(wf_game *g, int zone, uint32_t now, wf_step *out);
/* The zone guardian is optional: it can be challenged once enough wild
 * battles were won in the zone, whenever the player feels ready. */
bool wf_boss_ready(const wf_game *g, int zone);
int wf_boss_step(wf_game *g, int zone, uint32_t now, wf_step *out);
/* Record a finished zone battle (progress, unlocks, rewards, quests).
 * Returns levels gained; *unlocked set when a new zone opened. */
int wf_battle_finish(wf_game *g, int zone, const wf_battle *b, bool *unlocked, uint32_t *xp, uint32_t *coins);

/* ---- Expeditions ------------------------------------------------------ */
#define WF_EXPEDITION_OPTIONS 3
uint16_t wf_expedition_minutes(int option);

typedef struct {
    uint32_t xp;
    uint32_t coins;
    uint8_t items[WF_IT_COUNT];
    uint8_t levels;
} wf_haul;

int wf_expedition_start(wf_game *g, int zone, int option, uint32_t now);
/* Pure preview of the haul for the current expedition (deterministic seed). */
void wf_expedition_haul(const wf_game *g, wf_haul *out);
/* Applies the haul once the companion is back. */
int wf_expedition_collect(wf_game *g, uint32_t now, wf_haul *out);
bool wf_expedition_ready(const wf_game *g, uint32_t now);
