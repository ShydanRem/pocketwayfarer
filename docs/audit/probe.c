/* HISTORICAL BASELINE ONLY: assertions intentionally expect the original bugs.
 * Current regression suite: firmware/rpg/host/test_reliability.c.
 * Read-only core probes for the 2026-11-16 design/quality audit.
 * No production code is changed. See REVIEW-2026-11-16.md for interpretation.
 * Compile from repository root with the command documented there. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "rpg.h"

static const uint32_t t0 = 9800u * 86400u + 10u * 3600u;

static void fresh(rpg_game *g, uint32_t seed) {
    rpg_new_hero nh = {0};
    strcpy(nh.name, "Audit");
    nh.cls = FAM_ARCHER;
    rpg_game_new(g, &nh, t0, seed);
}

static rpg_game restored(const rpg_game *g) {
    uint8_t buf[4096];
    rpg_game back;
    size_t n = rpg_save_encode(g, buf, sizeof buf);
    assert(n && rpg_save_decode(buf, n, &back));
    return back;
}

static int reachable(const rpg_game *g) {
    int n = 0;
    for (int r = 0; r <= RPG_ROWS; r++)
        for (int c = 0; c < RPG_COLS; c++)
            n += rpg_room_reachable(&g->run, r, c);
    return n;
}

int main(void) {
    rpg_game g, minute, hour;
    fresh(&g, 7);
    g.hero.hunger = 0;
    g.hero.morale = 100;
    minute = hour = g;
    for (int i = 1; i <= 60; i++) rpg_tick(&minute, t0 + (uint32_t)i * 60u);
    rpg_tick(&hour, t0 + 3600u);
    printf("same 60 minutes, hunger zero: morale minute-ticks=%d; hour-tick=%d\n",
           minute.hero.morale, hour.hero.morale);
    assert(minute.hero.morale == 40 && hour.hero.morale == 98);

    fresh(&g, 7);
    g.quests[0] = (rpg_quest){.kind = Q_GOLD, .goal = 4};
    uint32_t gold0 = g.gold;
    for (int i = 0; i < 10; i++) {
        rpg_foe f;
        rpg_battle b;
        rpg_loot loot;
        rpg_foe_make(&f, SP_SLIME, 1, false);
        rpg_battle_start(&b, &g, &f, 1, (uint32_t)i + 1, 0);
        b.foe[0].hp = 0;
        b.state = B_WON; /* reward-path probe, not a simulated combat */
        rpg_battle_finish(&g, &b, &loot);
    }
    printf("ten level-1 reward payouts: gold=%u; gold quest progress=%u/4\n",
           (unsigned)(g.gold - gold0), g.quests[0].progress);
    assert(g.gold - gold0 == 70 && g.quests[0].progress == 0);

    fresh(&g, 7);
    int greet_days = 0;
    for (int day = 1; day <= 365; day++) {
        rpg_roll_day(&g, rpg_day_of(t0) + (uint32_t)day);
        for (int q = 0; q < RPG_QUESTS; q++)
            if (g.quests[q].kind == Q_GREET) { greet_days++; break; }
    }
    printf("365 generated daily sets: %d contain greet-friends quest\n", greet_days);
    assert(greet_days > 0);

    fresh(&g, 7);
    assert(rpg_dungeon_enter(&g, 0, 0, t0) == RPG_OK);
    int c = 0;
    while (!rpg_room_reachable(&g.run, 0, c)) c++;
    g.run.rooms[0][c] = ROOM_FIGHT; /* force an unresolved encounter */
    rpg_foe foes[3];
    int n;
    assert(rpg_room_enter(&g, 0, c, foes, &n) == ROOM_FIGHT && n > 0);
    g = restored(&g);
    printf("save/reload unresolved normal fight: cleared=%d; next reachable=%d\n",
           !!(g.run.cleared[0] & (1u << c)), reachable(&g));
    assert(!(g.run.cleared[0] & (1u << c)) && reachable(&g) > 0);

    g.run.floor = 5;
    rpg_dungeon_floor(&g.run);
    g.run.row = RPG_ROWS - 1;
    for (c = 0; c < RPG_COLS; c++)
        if (g.run.rooms[RPG_ROWS - 1][c]) break;
    g.run.col = (int8_t)c;
    assert(rpg_room_enter(&g, RPG_ROWS, 1, foes, &n) == ROOM_BOSS && n == 1);
    g = restored(&g);
    printf("save/reload unresolved boss: active=%u; row=%d; reachable=%d\n",
           g.run.active, g.run.row, reachable(&g));
    assert(g.run.active && g.run.row == RPG_ROWS && reachable(&g) == 0);

    fresh(&g, 7);
    g.hero.level = 6;
    for (int s = 0; s < ST_COUNT; s++) g.hero.stat[s] = 25;
    for (int i = 0; i < RPG_BAG; i++) g.bag[i] = g.hero.equip[SLOT_WEAPON];
    assert(rpg_hunt_start(&g, 0, t0) == RPG_OK);
    rpg_haul haul;
    rpg_hunt_preview(&g, t0 + 12u * 3600u, &haul);
    int found = haul.nitems;
    assert(found > 0);
    assert(rpg_hunt_collect(&g, t0 + 12u * 3600u, &haul) == RPG_OK);
    printf("idle hunt with full bag: reported items=%d; bag remains %d/%d; all gear unchanged=",
           found, rpg_bag_count(&g), RPG_BAG);
    int unchanged = 1;
    for (int i = 0; i < RPG_BAG; i++)
        unchanged &= !memcmp(&g.bag[i], &g.hero.equip[SLOT_WEAPON], sizeof(rpg_item));
    printf("%d\n", unchanged);
    assert(unchanged && rpg_bag_count(&g) == RPG_BAG);
    puts("audit probes: all observations reproduced");
    return 0;
}
