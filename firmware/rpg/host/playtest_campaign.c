/* Host playtest. One profile per family and seed with normal loot, leveling,
 * prices and room costs. This is a pacing probe with bounded retries. */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "rpg.h"
#include "timing.h"

static const uint32_t T0 = 9800u * 86400u + 10u * 3600u;

static void new_hero(rpg_game *g, int family, uint32_t seed) {
    rpg_new_hero profile = {.cls = (uint8_t)family, .race = (uint8_t)(seed % RACE_COUNT)};
    snprintf(profile.name, sizeof profile.name, "Test%d", family);
    rpg_game_new(g, &profile, T0, seed);
}

/* Upgrades use only earned stats, class points, quest payouts and dropped gear. */
static void invest(rpg_game *g, int branch) {
    for (int i = 0; i < RPG_QUESTS; i++) rpg_quest_claim(g, i);
    int next[2];
    int branches = rpg_class_next(g->hero.cls, next);
    if (branches && rpg_can_change_class(g, next[branches > 1 ? branch : 0]))
        rpg_change_class(g, next[branches > 1 ? branch : 0]);
    const rpg_class_info *c = rpg_class(g->hero.cls);
    for (int i = 0; g->hero.stat_points && i < 120; i++) {
        int s = i % 3 == 2 ? ST_VIT : i % 3 == 1 ? c->second_stat : c->main_stat;
        if (!rpg_raise_stat(g, s)) rpg_raise_stat(g, ST_VIT);
    }
    int skills[9];
    int count = rpg_class_skills(g->hero.cls, skills);
    for (int i = 0; g->hero.skill_points && i < 6; i++) {
        for (int j = count - 1; j >= 0; j--) rpg_learn_skill(g, skills[j]);
    }
    for (int i = 0; i < RPG_BAG; i++) {
        if (g->bag[i].type == 0xff) continue;
        rpg_game candidate = *g;
        if (rpg_equip(&candidate, i) == RPG_OK && rpg_power(&candidate) > rpg_power(g)) *g = candidate;
        rpg_sell(g, i);
    }
}

static int sustain_fight(rpg_game *g, rpg_foe *foes, int n, int q, uint32_t seed, int flags) {
    rpg_battle b;
    rpg_battle_start(&b, g, foes, n, seed, flags);
    for (int guard = 0; guard < 400 && !rpg_battle_over(&b); guard++) {
        if (b.state == B_HERO) {
            rpg_action a;
            rpg_battle_auto(&b, g, &a);
            if (a.kind == ACT_ATTACK && b.h.sp < b.h.st.max_sp / 4 && g->cons[CO_ETHER] &&
                (b.h.st.matk > b.h.st.atk || (flags & B_FLAG_BOSS))) {
                a.kind = ACT_ITEM;
                a.item = CO_ETHER;
            }
            rpg_battle_hero(&b, g, &a, q);
        } else if (b.state == B_FOES) {
            while (rpg_battle_next_foe(&b)) {
                rpg_battle_foe(&b, q);
                if (rpg_battle_over(&b)) break;
            }
        }
    }
    rpg_loot loot;
    rpg_battle_finish(g, &b, &loot);
    return b.state;
}

static void prepare(rpg_game *g) {
    rpg_stats s;
    rpg_stats_get(g, &s);
    if (g->hero.hp < s.max_hp * 3 / 4 || g->hero.sp < s.max_sp / 3 || g->hero.stamina < 50) {
        rpg_inn(g);
        if (g->hero.hp < s.max_hp * 3 / 4) rpg_use_consumable(g, CO_POTION);
    }
    if (g->cons[CO_POTION] < 4) rpg_buy(g, CO_POTION, 4 - g->cons[CO_POTION]);
    if (g->cons[CO_ETHER] < 2) rpg_buy(g, CO_ETHER, 2 - g->cons[CO_ETHER]);
}

static void world_battle(rpg_game *g, int z, int q, unsigned *fields) {
    rpg_stats s;
    rpg_stats_get(g, &s);
    if (g->hero.hp < s.max_hp * 2 / 3 || g->hero.stamina < 2) rpg_inn(g);
    if (g->hero.hp < s.max_hp / 2) rpg_use_consumable(g, CO_POTION);
    rpg_foe foes[RPG_MAX_FOES];
    int n;
    if (rpg_field_encounter(g, z, foes, &n) == RPG_OK)
        sustain_fight(g, foes, n, q, 1000 + *fields, 0);
    (*fields)++;
}

static int choose_room(const rpg_game *g, int row) {
    /* Prefer supplies and safe rooms to save HP and stamina for bosses. */
    static const int scores[ROOM_KIND_COUNT] = {
        [ROOM_FIGHT] = 3, [ROOM_ELITE] = 1, [ROOM_CHEST] = 5, [ROOM_TRAP] = 2,
        [ROOM_FOUNTAIN] = 6, [ROOM_MERCHANT] = 1, [ROOM_EVENT] = 4,
    };
    int col = -1;
    for (int i = 0; i < RPG_COLS; i++) {
        if (!rpg_room_reachable(&g->run, row, i)) continue;
        if (col < 0 || scores[g->run.rooms[row][i]] > scores[g->run.rooms[row][col]]) col = i;
    }
    return col;
}

/* Result 1 stops at the mid-boss for a town rest. Result 2 clears the region. */
static int descent(rpg_game *g, int z, int q, unsigned *rooms) {
    if (rpg_dungeon_enter(g, z, rpg_dungeon_checkpoint(g, z), T0) != RPG_OK) return 0;
    while (g->run.active) {
        int row = g->run.row + 1;
        int col = choose_room(g, row);
        assert(col >= 0);
        rpg_foe foes[RPG_MAX_FOES];
        int n;
        const int kind = rpg_room_enter(g, row, col, foes, &n);
        if (kind < 0) {
            rpg_dungeon_retreat(g);
            return 0;
        }
        if (!n && kind != ROOM_STAIRS) {
            rpg_room_result res;
            rpg_room_resolve(g, &res, foes, &n);
        }
        if (n) {
            const int flags = B_FLAG_DUNGEON | (kind == ROOM_BOSS ? B_FLAG_BOSS : 0);
            if (sustain_fight(g, foes, n, q, ++*rooms, flags) != B_WON) {
                rpg_dungeon_leave(g, true);
                return 0;
            }
            g->run.cleared[row] |= (uint8_t)(1u << col);
        }
        rpg_stats s;
        rpg_stats_get(g, &s);
        if (g->hero.hp < s.max_hp * 2 / 3) rpg_use_consumable(g, CO_POTION);
        if (g->hero.sp < s.max_sp / 4) rpg_use_consumable(g, CO_ETHER);
        if (row != RPG_ROWS) continue;
        if (g->run.floor == 5 && !rpg_dungeon_checkpoint(g, z)) {
            rpg_dungeon_leave(g, false);
            return 1;
        }
        rpg_dungeon_descend(g);
    }
    return 2;
}

int main(int argc, char **argv) {
    int start = argc > 1 ? atoi(argv[1]) : 90;
    int seeds = argc > 2 ? atoi(argv[2]) : 16;
    if (start < 1 || start > 1000000 || seeds < 1 || seeds > 100) return 2;
    uint32_t last = (uint32_t)(start + seeds);
    const int q = WF_Q_GREAT;
    for (int family = 0; family < FAM_COUNT; family++) {
        for (uint32_t seed = start; seed < last; seed++) {
            rpg_game g;
            new_hero(&g, family, seed);
            rpg_guide_reset(&g);
            rpg_daily_claim(&g, NULL);
            int branch = seed % 2;
            unsigned fields = 0, rooms = 0, losses = 0;
            world_battle(&g, 0, q, &fields);
            invest(&g, branch);
            printf("fam=%d seed=%u ", family, seed);
            int zone = 0;
            for (; zone < RPG_ZONES; zone++) {
                const unsigned first = fields;
                const int min = rpg_zone(zone)->min_level + 1;
                while (g.hero.level < min && fields < first + 110) {
                    world_battle(&g, zone, q, &fields);
                    invest(&g, branch);
                }
                int result = 0;
                for (int i = 0; i < 12; i++) {
                    prepare(&g);
                    result = descent(&g, zone, q, &rooms);
                    invest(&g, branch);
                    if (result == 2) break;
                    if (result == 1) continue;
                    losses++;
                    for (int j = 0; j < 3 && fields < first + 110; j++) {
                        world_battle(&g, zone, q, &fields);
                        invest(&g, branch);
                    }
                }
                printf("z%d fields=%u level=%u floor=%u ", zone + 1, fields - first, g.hero.level, g.best_floor[zone]);
                if (result != 2) break;
            }
            printf("| complete=%d field=%u rooms=%u retries=%u\n", zone, fields, rooms, losses);
        }
    }
}
