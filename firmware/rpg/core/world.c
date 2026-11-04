/* Dungeon floors made of rooms, field encounters and idle hunting. */
#include <string.h>
#include "rpg.h"

static wf_rng game_rng(rpg_game *g) {
    wf_rng r = {g->rng_state ? g->rng_state : 0x5eedu};
    wf_rng_next(&r);
    return r;
}

static int clampi(int v, int lo, int hi) { return v < lo ? lo : v > hi ? hi : v; }

/* ---- Floor generation ---------------------------------------------------------------- */
static int pick_room(wf_rng *r, int row) {
    int roll = wf_rng_range(r, 0, 99);
    if (row == 0) return roll < 70 ? ROOM_FIGHT : roll < 85 ? ROOM_CHEST : ROOM_EVENT;
    if (row == RPG_ROWS - 1 && roll < 25) return ROOM_FOUNTAIN;
    if (roll < 42) return ROOM_FIGHT;
    if (roll < 54) return ROOM_ELITE;
    if (roll < 66) return ROOM_CHEST;
    if (roll < 76) return ROOM_TRAP;
    if (roll < 84) return ROOM_FOUNTAIN;
    if (roll < 90) return ROOM_MERCHANT;
    return ROOM_EVENT;
}

void rpg_dungeon_floor(rpg_run *run) {
    wf_rng r;
    wf_rng_seed(&r, run->seed ^ (run->floor * 0x9E3779B9u));
    memset(run->rooms, 0, sizeof run->rooms);
    memset(run->links, 0, sizeof run->links);
    memset(run->cleared, 0, sizeof run->cleared);
    for (int row = 0; row < RPG_ROWS; row++) {
        int n = wf_rng_range(&r, 2, 3);
        int skip = n == 3 ? -1 : wf_rng_range(&r, 0, 2);
        for (int c = 0; c < RPG_COLS; c++)
            if (c != skip) run->rooms[row][c] = (uint8_t)pick_room(&r, row);
    }
    run->rooms[RPG_ROWS][1] = run->floor % 5 == 0 ? ROOM_BOSS : ROOM_STAIRS;
    for (int row = 0; row < RPG_ROWS; row++) {
        for (int c = 0; c < RPG_COLS; c++) {
            if (!run->rooms[row][c]) continue;
            for (int d = 0; d < RPG_COLS; d++)
                if (run->rooms[row + 1][d] && (d - c <= 1 && c - d <= 1)) run->links[row][c] |= (uint8_t)(1u << d);
            if (!run->links[row][c]) {           /* far corner: link to the nearest */
                for (int k = 1; k < RPG_COLS && !run->links[row][c]; k++)
                    for (int d = 0; d < RPG_COLS; d++)
                        if (run->rooms[row + 1][d] && (d - c == k || c - d == k)) { run->links[row][c] |= (uint8_t)(1u << d); break; }
            }
        }
        for (int d = 0; d < RPG_COLS; d++) {     /* every room needs a way in */
            if (!run->rooms[row + 1][d]) continue;
            bool in = false;
            for (int c = 0; c < RPG_COLS; c++) in |= (run->links[row][c] >> d) & 1;
            if (in) continue;
            for (int k = 1; k < RPG_COLS && !in; k++)
                for (int c = 0; c < RPG_COLS; c++)
                    if (run->rooms[row][c] && (d - c == k || c - d == k)) { run->links[row][c] |= (uint8_t)(1u << d); in = true; break; }
        }
    }
    run->row = -1;
    run->col = 1;
}

bool rpg_room_reachable(const rpg_run *run, int row, int col) {
    if (!run->active || row < 0 || row > RPG_ROWS || col < 0 || col >= RPG_COLS || !run->rooms[row][col]) return false;
    if (row != run->row + 1) return false;
    if (run->row < 0) return true;
    return (run->links[run->row][run->col] >> col) & 1;
}

bool rpg_dungeon_checkpoint(const rpg_game *g, int zone) {
    return zone >= 0 && zone < RPG_ZONES && g->best_floor[zone] >= 5;
}

int rpg_dungeon_enter(rpg_game *g, int zone, int checkpoint, uint32_t now) {
    if (zone < 0 || zone >= RPG_ZONES || zone > g->zone_unlocked) return -RPG_ERR_LOCKED;
    if (g->hunt.active || g->run.active) return -RPG_ERR_BUSY;
    if (g->hero.stamina < 10) return -RPG_ERR_STAMINA;
    if (g->hero.hp <= 0) return -RPG_ERR_DEAD;
    wf_rng r = game_rng(g);
    memset(&g->run, 0, sizeof g->run);
    g->run.active = 1;
    g->run.zone = (uint8_t)zone;
    g->run.floor = checkpoint && rpg_dungeon_checkpoint(g, zone) ? 6 : 1;
    g->run.seed = wf_rng_next(&r) ^ now;
    g->rng_state = r.s;
    rpg_dungeon_floor(&g->run);
    return RPG_OK;
}

static int floor_level(const rpg_run *run, int extra) {
    const rpg_zone_info *z = rpg_zone(run->zone);
    return z->min_level + (z->max_level - z->min_level) * (run->floor - 1) / (RPG_FLOORS - 1) + extra;
}

static int dungeon_pack(rpg_game *g, wf_rng *r, int kind, rpg_foe *foes) {
    const rpg_run *run = &g->run;
    const rpg_zone_info *z = rpg_zone(run->zone);
    if (kind == ROOM_BOSS) {
        int sp = run->floor >= RPG_FLOORS ? z->final_boss : z->mid_boss;
        rpg_foe_make(&foes[0], sp, floor_level(run, 2), false);
        return 1;
    }
    int n = kind == ROOM_ELITE ? 1 + wf_rng_chance(r, 50) : wf_rng_range(r, 1, run->floor <= 2 ? 2 : 3);
    for (int i = 0; i < n; i++) {
        int lv = clampi(floor_level(run, wf_rng_range(r, -1, 1)), 1, RPG_MAX_LEVEL);
        rpg_foe_make(&foes[i], z->dungeon_foes[wf_rng_range(r, 0, 3)], lv, kind == ROOM_ELITE && i == 0);
    }
    return n;
}

int rpg_room_enter(rpg_game *g, int row, int col, rpg_foe *foes, int *n) {
    rpg_run *run = &g->run;
    *n = 0;
    if (!rpg_room_reachable(run, row, col)) return -RPG_ERR_LOCKED;
    int kind = run->rooms[row][col];
    if (kind != ROOM_STAIRS && kind != ROOM_BOSS && g->hero.stamina < 2) return -RPG_ERR_STAMINA;
    if (kind != ROOM_STAIRS && kind != ROOM_BOSS) g->hero.stamina = (int16_t)(g->hero.stamina - 2);
    run->row = (int8_t)row;
    run->col = (int8_t)col;
    rpg_quest_bump(g, Q_ROOMS, 1);
    if (kind == ROOM_FIGHT || kind == ROOM_ELITE || kind == ROOM_BOSS) {
        wf_rng r = game_rng(g);
        *n = dungeon_pack(g, &r, kind, foes);
        g->rng_state = r.s;
    }
    return kind;
}

void rpg_room_resolve(rpg_game *g, rpg_room_result *res, rpg_foe *foes, int *n) {
    rpg_run *run = &g->run;
    memset(res, 0, sizeof *res);
    *n = 0;
    if (run->row < 0) return;
    int kind = run->rooms[run->row][run->col];
    res->kind = kind;
    run->cleared[run->row] |= (uint8_t)(1u << run->col);
    wf_rng r = game_rng(g);
    rpg_stats st;
    rpg_stats_get(g, &st);
    rpg_hero *h = &g->hero;
    switch (kind) {
    case ROOM_CHEST:
        if (wf_rng_chance(&r, 15)) {
            res->mimic = 1;
            rpg_foe_make(&foes[0], SP_MIMIC, floor_level(run, 1), true);
            *n = 1;
            break;
        }
        rpg_item_roll(&res->item, &r, floor_level(run, 0), 15 + st.stat[ST_LUK] / 5, 1 << rpg_class_family(h->cls));
        res->got_item = rpg_bag_add(g, &res->item);
        res->value = 10 + floor_level(run, 0) * 3;
        run->gold += (uint32_t)res->value;
        g->gold += (uint32_t)res->value;
        break;
    case ROOM_TRAP: {
        int avoid = clampi(35 + (st.stat[ST_AGI] + st.stat[ST_DEX]) / 2, 35, 90);
        if (wf_rng_chance(&r, avoid)) {
            res->value = 0;
        } else {
            res->value = st.max_hp * 15 / 100;
            h->hp -= res->value;
            if (h->hp < 1) h->hp = 1;
        }
        break;
    }
    case ROOM_FOUNTAIN:
        res->value = st.max_hp * 40 / 100;
        h->hp += res->value;
        h->sp += st.max_sp * 40 / 100;
        if (h->hp > st.max_hp) h->hp = st.max_hp;
        if (h->sp > st.max_sp) h->sp = st.max_sp;
        break;
    case ROOM_EVENT: {
        int e = wf_rng_range(&r, 0, 3);
        res->value = e;
        if (e == 0) {                 /* shrine: blessing */
            h->morale = (int16_t)clampi(h->morale + 15, 0, 100);
            h->hp = st.max_hp;
        } else if (e == 1) {          /* lost adventurer */
            res->cons = CO_POTION + 1;
            if (g->cons[CO_POTION] < 99) g->cons[CO_POTION]++;
        } else if (e == 2) {          /* ore vein */
            res->cons = CO_ORE + 1;
            g->cons[CO_ORE] = (uint8_t)clampi(g->cons[CO_ORE] + 2, 0, 99);
        } else {                      /* treasure pile */
            uint32_t gold = 20u + (uint32_t)floor_level(run, 0) * 5u;
            g->gold += gold;
            run->gold += gold;
        }
        break;
    }
    default:
        break;
    }
    g->rng_state = r.s;
}

int rpg_dungeon_descend(rpg_game *g) {
    rpg_run *run = &g->run;
    if (!run->active) return -1;
    if (run->floor > g->best_floor[run->zone]) g->best_floor[run->zone] = run->floor;
    if (run->floor >= RPG_FLOORS) {
        if (run->zone == g->zone_unlocked && g->zone_unlocked + 1 < RPG_ZONES) g->zone_unlocked++;
        run->active = 0;
        return 1;
    }
    run->floor++;
    rpg_dungeon_floor(run);
    return 0;
}

void rpg_dungeon_leave(rpg_game *g, bool defeated) {
    rpg_run *run = &g->run;
    if (!run->active) return;
    if (run->floor - 1 > g->best_floor[run->zone]) g->best_floor[run->zone] = (uint8_t)(run->floor - 1);
    if (defeated) {
        uint32_t lost = run->gold / 2u;
        g->gold = g->gold > lost ? g->gold - lost : 0;
        g->hero.morale = (int16_t)clampi(g->hero.morale - 10, 0, 100);
        rpg_stats st;
        rpg_stats_get(g, &st);
        g->hero.hp = st.max_hp / 10 > 0 ? st.max_hp / 10 : 1;
    }
    run->active = 0;
}

/* ---- Fields --------------------------------------------------------------------------- */
static int field_level(const rpg_game *g, int zone, wf_rng *r) {
    const rpg_zone_info *z = rpg_zone(zone);
    return clampi(g->hero.level + wf_rng_range(r, -2, 1), z->min_level, z->max_level);
}

int rpg_field_encounter(rpg_game *g, int zone, rpg_foe *foes, int *n) {
    *n = 0;
    if (zone < 0 || zone >= RPG_ZONES || zone > g->zone_unlocked) return RPG_ERR_LOCKED;
    if (g->run.active || g->hunt.active) return RPG_ERR_BUSY;
    if (g->hero.stamina < 1) return RPG_ERR_STAMINA;
    if (g->hero.hp <= 0) return RPG_ERR_DEAD;
    wf_rng r = game_rng(g);
    const rpg_zone_info *z = rpg_zone(zone);
    int roll = wf_rng_range(&r, 0, 99);
    *n = roll < 50 ? 1 : roll < 85 ? 2 : 3;
    for (int i = 0; i < *n; i++)
        rpg_foe_make(&foes[i], z->field_foes[wf_rng_range(&r, 0, 3)], field_level(g, zone, &r), wf_rng_chance(&r, 6));
    g->hero.stamina--;
    g->rng_state = r.s;
    return RPG_OK;
}

/* ---- Idle hunting ---------------------------------------------------------------------- */
int rpg_hunt_start(rpg_game *g, int zone, uint32_t now) {
    if (zone < 0 || zone >= RPG_ZONES || zone > g->zone_unlocked) return RPG_ERR_LOCKED;
    if (g->run.active || g->hunt.active) return RPG_ERR_BUSY;
    if (g->hero.stamina < 10) return RPG_ERR_STAMINA;
    if (g->hero.hunger < 10000) return RPG_ERR_HUNGRY;
    wf_rng r = game_rng(g);
    g->hunt.active = 1;
    g->hunt.zone = (uint8_t)zone;
    g->hunt.since = now;
    g->hunt.seed = wf_rng_next(&r);
    g->hunt.hunger0 = g->hero.hunger;
    g->hunt.stamina0 = g->hero.stamina;
    g->rng_state = r.s;
    return RPG_OK;
}

static int foe_power(int species, int level) {
    rpg_foe f;
    rpg_foe_make(&f, species, level, false);
    return f.atk * 3 + f.def * 2 + f.max_hp / 4;
}

void rpg_hunt_preview(const rpg_game *g, uint32_t now, rpg_haul *out) {
    memset(out, 0, sizeof *out);
    if (!g->hunt.active) return;
    uint32_t minutes = now > g->hunt.since ? (now - g->hunt.since) / 60u : 0;
    if (minutes > 720u) minutes = 720u;
    uint32_t by_stamina = (uint32_t)g->hunt.stamina0 * 10u;
    uint32_t by_hunger = (uint32_t)g->hunt.hunger0 / 100u;
    if (minutes > by_stamina) { minutes = by_stamina; out->stopped = 1; }
    if (minutes > by_hunger) { minutes = by_hunger; out->stopped = 2; }
    const rpg_zone_info *z = rpg_zone(g->hunt.zone);
    int level = clampi(g->hero.level, z->min_level, z->max_level);
    int ratio = rpg_power(g) * 100 / (foe_power(z->field_foes[0], level) * 2);
    if (ratio < 60) {                       /* too dangerous: gives up early */
        if (minutes > 30u) { minutes = 30u; out->stopped = 3; }
    }
    ratio = clampi(ratio, 30, 200);
    out->minutes = minutes;
    out->kills = minutes * (uint32_t)(20 * ratio / 100) / 60u;
    uint32_t L = (uint32_t)level;
    uint32_t xp = L * L / 2u + L * 6u + 8u;
    out->xp = out->kills * xp * 6u / 10u;
    out->job_xp = out->xp * 7u / 10u;
    rpg_stats st;
    rpg_stats_get(g, &st);
    out->gold = out->kills * (L * 2u + 5u) * 6u / 10u * (uint32_t)(100 + st.gold_bonus) / 100u;
    wf_rng r;
    wf_rng_seed(&r, g->hunt.seed);
    int fam = 1 << rpg_class_family(g->hero.cls);
    for (uint32_t k = 0; k < out->kills; k++) {
        if (wf_rng_range(&r, 0, 999) < 30 + st.stat[ST_LUK] / 2 && out->nitems < 4)
            rpg_item_roll(&out->items[out->nitems++], &r, level, st.stat[ST_LUK] / 5, fam);
        if (wf_rng_chance(&r, 5) && out->cons[CO_POTION] < 20) out->cons[CO_POTION]++;
        if (wf_rng_chance(&r, 8) && out->cons[CO_ORE] < 20) out->cons[CO_ORE]++;
        if (wf_rng_range(&r, 0, 999) < 3) out->gems++;
    }
}

int rpg_hunt_collect(rpg_game *g, uint32_t now, rpg_haul *out) {
    if (!g->hunt.active) return RPG_ERR_NONE;
    rpg_hunt_preview(g, now, out);
    const rpg_zone_info *z = rpg_zone(g->hunt.zone);
    g->hunt.active = 0;
    g->gold += out->gold;
    for (int i = 0; i < out->nitems; i++) rpg_bag_add(g, &out->items[i]);
    for (int c = 0; c < CO_COUNT; c++) g->cons[c] = (uint8_t)clampi(g->cons[c] + out->cons[c], 0, 99);
    if (out->gems) {
        wf_rng r;
        wf_rng_seed(&r, g->hunt.seed ^ 0xabcdu);
        for (int i = 0; i < out->gems; i++) {
            int sp = z->field_foes[wf_rng_range(&r, 0, 3)];
            if (g->gems[sp] < 99) g->gems[sp]++;
        }
    }
    g->kills += out->kills;
    rpg_quest_bump(g, Q_KILL, (int)(out->kills > 50 ? 50 : out->kills));
    int spent = (int)(out->minutes / 10u);
    g->hero.stamina = (int16_t)clampi(g->hero.stamina - spent, 0, 100);
    out->levels = (uint8_t)rpg_gain_xp(g, out->xp, out->job_xp, NULL);
    return RPG_OK;
}
