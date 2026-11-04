#include "world.h"
#include <string.h>

wf_rng wf__game_rng(wf_game *g);
void wf__game_rng_store(wf_game *g, const wf_rng *r);

static const wf_species_info species[WF_SP_COUNT] = {
    [WF_SP_GELATINO]     = {"Gelatino", WF_EL_GROVE, WF_ARCH_SLIME, 100, 90, 90, 80, 0},
    [WF_SP_FUNGOTTO]     = {"Fungotto", WF_EL_GROVE, WF_ARCH_SHROOM, 110, 95, 110, 70, 0},
    [WF_SP_SCINTILLO]    = {"Scintillo", WF_EL_EMBER, WF_ARCH_WISP, 85, 110, 80, 120, 0},
    [WF_SP_RE_GELATINO]  = {"Re Gelatino", WF_EL_GROVE, WF_ARCH_SLIME, 220, 115, 105, 85, 1},
    [WF_SP_GRANCHIETTO]  = {"Granchietto", WF_EL_TIDE, WF_ARCH_CRAB, 100, 100, 125, 85, 0},
    [WF_SP_MEDUSINA]     = {"Medusina", WF_EL_TIDE, WF_ARCH_JELLY, 90, 110, 85, 110, 0},
    [WF_SP_GELATINO_BLU] = {"Gelatino Blu", WF_EL_TIDE, WF_ARCH_SLIME, 105, 95, 95, 90, 0},
    [WF_SP_GRANCHIO_RE]  = {"Granchio Corazzato", WF_EL_TIDE, WF_ARCH_CRAB, 230, 115, 135, 80, 1},
    [WF_SP_RADICOTTO]    = {"Radicotto", WF_EL_GROVE, WF_ARCH_GOLEM, 120, 100, 120, 70, 0},
    [WF_SP_PIPISTRELLO]  = {"Pipistrello", WF_EL_NONE, WF_ARCH_BAT, 85, 105, 85, 130, 0},
    [WF_SP_GUFO]         = {"Gufo Lunare", WF_EL_GROVE, WF_ARCH_OWL, 100, 110, 95, 105, 0},
    [WF_SP_ANTICO_CEPPO] = {"Antico Ceppo", WF_EL_GROVE, WF_ARCH_GOLEM, 240, 120, 125, 70, 1},
    [WF_SP_SALAMANDRA]   = {"Salamandra", WF_EL_EMBER, WF_ARCH_LIZARD, 100, 115, 95, 105, 0},
    [WF_SP_GOLEM]        = {"Golem di Lava", WF_EL_EMBER, WF_ARCH_GOLEM, 125, 105, 130, 65, 0},
    [WF_SP_BRACEPIPPO]   = {"Pipistrello Ardente", WF_EL_EMBER, WF_ARCH_BAT, 90, 110, 85, 130, 0},
    [WF_SP_DRAGO]        = {"Drago di Cenere", WF_EL_EMBER, WF_ARCH_DRAGON, 250, 125, 115, 100, 1},
    [WF_SP_TEMPESTINO]   = {"Tempestino", WF_EL_TIDE, WF_ARCH_WISP, 95, 115, 90, 125, 0},
    [WF_SP_CRISTALLO]    = {"Cristallo Errante", WF_EL_NONE, WF_ARCH_CRYSTAL, 110, 105, 135, 80, 0},
    [WF_SP_GRIFONCINO]   = {"Grifoncino", WF_EL_GROVE, WF_ARCH_BIRD, 105, 115, 95, 120, 0},
    [WF_SP_CUSTODE]      = {"Custode Celeste", WF_EL_NONE, WF_ARCH_CRYSTAL, 270, 125, 125, 105, 1},
};

static const wf_zone_info zones[WF_ZONE_COUNT] = {
    {"Prati di Lumen", "Erba alta e lucciole. Si comincia da qui.", 1, 4,
     {WF_SP_GELATINO, WF_SP_FUNGOTTO, WF_SP_SCINTILLO}, WF_SP_RE_GELATINO, 5, 0, WF_IT_SPROUT},
    {"Riva di Vetro", "Sabbia che tintinna sotto le zampe.", 4, 8,
     {WF_SP_GRANCHIETTO, WF_SP_MEDUSINA, WF_SP_GELATINO_BLU}, WF_SP_GRANCHIO_RE, 10, 1, WF_IT_KELP},
    {"Bosco Sussurrante", "Gli alberi parlano piano. Ascolta.", 8, 13,
     {WF_SP_RADICOTTO, WF_SP_PIPISTRELLO, WF_SP_GUFO}, WF_SP_ANTICO_CEPPO, 15, 2, WF_IT_SPROUT},
    {"Caverne di Brace", "Fa caldo. Molto caldo.", 13, 18,
     {WF_SP_SALAMANDRA, WF_SP_GOLEM, WF_SP_BRACEPIPPO}, WF_SP_DRAGO, 20, 3, WF_IT_CHILI},
    {"Guglie del Cielo", "Sopra le nuvole, verso le stelle.", 18, 26,
     {WF_SP_TEMPESTINO, WF_SP_CRISTALLO, WF_SP_GRIFONCINO}, WF_SP_CUSTODE, 28, 4, WF_IT_CANDY},
};

const wf_species_info *wf_species_get(int sp) {
    if (sp < 0 || sp >= WF_SP_COUNT) sp = 0;
    return &species[sp];
}

const wf_zone_info *wf_zone_get(int zone) {
    if (zone < 0 || zone >= WF_ZONE_COUNT) zone = 0;
    return &zones[zone];
}

void wf_foe_make(wf_fighter *out, int sp, int level) {
    const wf_species_info *s = wf_species_get(sp);
    memset(out, 0, sizeof *out);
    wf_copy_name(out->name, s->name);
    out->element = s->element;
    out->level = (uint8_t)level;
    out->sprite = (uint8_t)sp;
    int hp = 18 + level * 7, atk = 7 + level * 26 / 10, def = 4 + level * 22 / 10, spd = 5 + level * 2;
    out->max_hp = out->hp = (int16_t)(hp * s->m_hp / 100);
    out->atk = (int16_t)(atk * s->m_atk / 100);
    out->def = (int16_t)(def * s->m_def / 100);
    out->spd = (int16_t)(spd * s->m_spd / 100);
}

static void need_add(int32_t *v, int32_t d) {
    int32_t x = *v + d;
    *v = x < 0 ? 0 : x > WF_NEED_MAX ? WF_NEED_MAX : x;
}

int wf_explore(wf_game *g, int zone, uint32_t now, wf_step *out) {
    memset(out, 0, sizeof *out);
    if (wf_pet_stage(g) == WF_STAGE_SEED) return WF_ERR_SEED;
    if (wf_pet_away(g, now)) return WF_ERR_AWAY;
    if (g->pet.sleeping) return WF_ERR_ASLEEP;
    if (zone < 0 || zone >= WF_ZONE_COUNT || zone > g->zone_unlocked) return WF_ERR_LOCKED;
    if (g->pet.energy < 8000) return WF_ERR_TIRED;
    const wf_zone_info *z = wf_zone_get(zone);
    wf_rng r = wf__game_rng(g);
    need_add(&g->pet.energy, -5000);
    need_add(&g->pet.hunger, -2500);
    g->explores++;
    wf_quest_bump(g, WF_Q_EXPLORE, 1);
    {
        int roll = wf_rng_range(&r, 0, 99);
        if (roll < 58) {
            out->kind = WF_STEP_BATTLE;
            out->species = z->foes[wf_rng_range(&r, 0, 2)];
            int lo = z->min_level, hi = z->max_level;
            /* Keep foes near the companion's level inside the zone band. */
            int want = g->pet.level + wf_rng_range(&r, -1, 1);
            if (want < lo) want = lo;
            if (want > hi) want = hi;
            out->level = (uint8_t)want;
        } else if (roll < 76) {
            out->kind = WF_STEP_ITEM;
            out->item = wf_rng_chance(&r, 50) ? z->drop : (uint8_t)wf_rng_range(&r, WF_IT_BERRY, WF_IT_CANDY);
            out->count = 1;
            wf_give_item(g, out->item, 1);
        } else if (roll < 88) {
            out->kind = WF_STEP_COINS;
            out->coins = (uint16_t)(4 + zone * 4 + wf_rng_range(&r, 0, 6 + zone * 3));
            g->coins += out->coins;
        } else if (roll < 96) {
            out->kind = WF_STEP_SPRING;
            need_add(&g->pet.energy, 12000);
            need_add(&g->pet.mood, 8000);
        } else {
            out->kind = WF_STEP_CHEST;
            out->item = wf_rng_chance(&r, 40) ? WF_IT_SHARD : WF_IT_POTION;
            out->count = 1;
            out->coins = (uint16_t)(15 + zone * 10);
            g->coins += out->coins;
            wf_give_item(g, out->item, 1);
        }
    }
    wf__game_rng_store(g, &r);
    return WF_OK;
}

bool wf_boss_ready(const wf_game *g, int zone) {
    return zone >= 0 && zone < WF_ZONE_COUNT && zone <= g->zone_unlocked &&
           g->zone_progress[zone] >= WF_BOSS_AFTER;
}

int wf_boss_step(wf_game *g, int zone, uint32_t now, wf_step *out) {
    memset(out, 0, sizeof *out);
    if (wf_pet_stage(g) == WF_STAGE_SEED) return WF_ERR_SEED;
    if (wf_pet_away(g, now)) return WF_ERR_AWAY;
    if (g->pet.sleeping) return WF_ERR_ASLEEP;
    if (!wf_boss_ready(g, zone)) return WF_ERR_LOCKED;
    if (g->pet.energy < 8000) return WF_ERR_TIRED;
    const wf_zone_info *z = wf_zone_get(zone);
    need_add(&g->pet.energy, -6000);
    need_add(&g->pet.hunger, -3000);
    out->kind = WF_STEP_BOSS;
    out->species = z->boss;
    out->level = z->boss_level;
    return WF_OK;
}

int wf_battle_finish(wf_game *g, int zone, const wf_battle *b, bool *unlocked, uint32_t *xp, uint32_t *coins) {
    if (unlocked) *unlocked = false;
    if (xp) *xp = 0;
    if (coins) *coins = 0;
    if (b->state == WF_B_LOST) {
        g->losses++;
        need_add(&g->pet.mood, -10000);
        return 0;
    }
    if (b->state != WF_B_WON) return 0;
    g->wins++;
    wf_quest_bump(g, WF_Q_WIN, 1);
    uint32_t x = wf_battle_xp(b), c = wf_battle_coins(b);
    g->coins += c;
    if (zone >= 0 && zone < WF_ZONE_COUNT) {
        if (b->boss) {
            g->zone_progress[zone] = 0;
            if (zone == g->zone_unlocked && zone + 1 < WF_ZONE_COUNT) {
                g->zone_unlocked++;
                if (unlocked) *unlocked = true;
            }
            wf_give_item(g, WF_IT_SHARD, 1);
        } else if (g->zone_progress[zone] < WF_BOSS_AFTER) {
            g->zone_progress[zone]++;
        }
        /* Fighting in a zone nudges the companion toward its element. */
        int el = wf_species_get(b->f[1].sprite)->element;
        if (el) g->pet.affinity[el] += 2;
    }
    need_add(&g->pet.mood, 3000);
    if (xp) *xp = x;
    if (coins) *coins = c;
    return wf_gain_xp(g, x);
}

/* ---- Expeditions ------------------------------------------------------ */
static const uint16_t expedition_options[WF_EXPEDITION_OPTIONS] = {30, 120, 480};

uint16_t wf_expedition_minutes(int option) {
    if (option < 0 || option >= WF_EXPEDITION_OPTIONS) option = 0;
    return expedition_options[option];
}

int wf_expedition_start(wf_game *g, int zone, int option, uint32_t now) {
    if (wf_pet_stage(g) == WF_STAGE_SEED) return WF_ERR_SEED;
    if (wf_pet_away(g, now)) return WF_ERR_AWAY;
    if (g->pet.sleeping) return WF_ERR_ASLEEP;
    if (zone < 0 || zone >= WF_ZONE_COUNT || zone > g->zone_unlocked) return WF_ERR_LOCKED;
    if (g->pet.energy < 25000) return WF_ERR_TIRED;
    wf_rng r = wf__game_rng(g);
    g->pet.away_since = now;
    g->pet.away_until = now + wf_expedition_minutes(option) * 60u;
    g->pet.away_zone = (uint8_t)zone;
    g->pet.away_seed = wf_rng_next(&r);
    wf__game_rng_store(g, &r);
    return WF_OK;
}

bool wf_expedition_ready(const wf_game *g, uint32_t now) {
    return g->pet.away_until && now >= g->pet.away_until;
}

void wf_expedition_haul(const wf_game *g, wf_haul *out) {
    memset(out, 0, sizeof *out);
    if (!g->pet.away_until) return;
    uint32_t minutes = (g->pet.away_until - g->pet.away_since) / 60u;
    int zone = g->pet.away_zone;
    const wf_zone_info *z = wf_zone_get(zone);
    wf_rng r;
    wf_rng_seed(&r, g->pet.away_seed);
    /* Level gap matters: a strong companion brings back more. */
    int gap = g->pet.level - z->min_level;
    int eff = 100 + (gap > 0 ? (gap > 8 ? 8 : gap) * 6 : gap * 12);
    if (eff < 30) eff = 30;
    out->xp = minutes * (uint32_t)(zone + 2) * (uint32_t)eff / 200u;
    out->coins = minutes * (uint32_t)(zone + 1) * (uint32_t)eff / 400u + (uint32_t)wf_rng_range(&r, 0, 5);
    int finds = (int)(minutes / 30u);
    for (int i = 0; i < finds; i++) {
        int roll = wf_rng_range(&r, 0, 99);
        int item = roll < 40 ? z->drop : roll < 70 ? WF_IT_BERRY : roll < 85 ? WF_IT_BREAD
                 : roll < 95 ? WF_IT_POTION : WF_IT_SHARD;
        if (out->items[item] < WF_ITEM_STACK) out->items[item]++;
    }
}

int wf_expedition_collect(wf_game *g, uint32_t now, wf_haul *out) {
    if (!wf_expedition_ready(g, now)) return WF_ERR_AWAY;
    wf_expedition_haul(g, out);
    uint32_t minutes = (g->pet.away_until - g->pet.away_since) / 60u;
    g->pet.away_until = 0;
    g->pet.away_since = 0;
    g->coins += out->coins;
    for (int i = 0; i < WF_IT_COUNT; i++) wf_give_item(g, i, out->items[i]);
    need_add(&g->pet.energy, -(int32_t)(minutes > 240 ? 20000 : minutes * 60));
    need_add(&g->pet.mood, 10000);
    need_add(&g->pet.bond, 2000);
    out->levels = (uint8_t)wf_gain_xp(g, out->xp);
    return WF_OK;
}
