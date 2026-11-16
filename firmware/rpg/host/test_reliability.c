/* Regressions converted from the original audit reproductions. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "rpg.h"

static const uint32_t T0 = 9800u * 86400u + 10u * 3600u;
static void fresh(rpg_game *g) {
    rpg_new_hero n = {0}; strcpy(n.name, "Recovery"); n.cls = FAM_ARCHER;
    rpg_game_new(g, &n, T0, 7);
}
static rpg_game reload(const rpg_game *g) {
    uint8_t data[4096]; rpg_game back;
    size_t n = rpg_save_encode(g, data, sizeof data);
    assert(n && rpg_save_decode(data, n, &back)); return back;
}
static uint32_t crc32(const uint8_t *p, size_t n) {
    uint32_t c = ~0u;
    while (n--) { c ^= *p++; for (int k = 0; k < 8; k++) c = (c >> 1) ^ (0xedb88320u & (uint32_t)-(int32_t)(c & 1)); }
    return ~c;
}
static rpg_game legacy(const rpg_game *g, uint16_t version, uint32_t size) {
    uint8_t data[4096] = {0}; rpg_game back;
    uint32_t magic = RPG_SAVE_MAGIC, crc = crc32((const uint8_t *)g, size);
    memcpy(data, &magic, 4); memcpy(data+4, &version, 2); memcpy(data+8, &size, 4); memcpy(data+12, &crc, 4);
    memcpy(data+16, g, size); assert(rpg_save_decode(data, size+16, &back)); return back;
}
int main(void) {
    rpg_game g, a, b;
    fresh(&g); g.hero.hunger = 0; g.hero.morale = 100; a = b = g;
    for (int i = 1; i <= 60; i++) rpg_tick(&a, T0 + (uint32_t)i * 60);
    rpg_tick(&b, T0 + 3600);
    assert(a.hero.morale == 98 && b.hero.morale == 98);
    puts("morale: minute updates and offline hour agree (98/98)");

    fresh(&g); memset(g.quests, 0, sizeof g.quests);
    g.quests[0] = (rpg_quest){.kind=Q_GOLD,.goal=4};
    uint32_t before = g.gold;
    for (int i = 0; i < 10; i++) {
        rpg_foe f; rpg_battle battle; rpg_loot loot;
        rpg_foe_make(&f, SP_SLIME, 1, false); rpg_battle_start(&battle, &g, &f, 1, (uint32_t)i+1, 0);
        battle.state = B_WON; battle.foe[0].hp = 0; rpg_battle_finish(&g, &battle, &loot);
    }
    assert(g.gold == before+70 && g.quests[0].progress == 1 && g.gold_remainder == 20);
    g = reload(&g); rpg_gold_add(&g, 30);
    assert(g.quests[0].progress == 2 && g.gold_remainder == 0);
    g.gold = UINT32_MAX - 2; rpg_gold_add(&g, 100); assert(g.gold == UINT32_MAX);
    puts("gold: fractional payouts survive reload, accumulate and saturate safely");

    fresh(&g);
    for (uint32_t day = 1; day <= 365; day++) {
        rpg_roll_day(&g, g.day + 1);
        for (int q = 0; q < RPG_QUESTS; q++) assert(g.quests[q].kind != Q_GREET);
    }
    g.quests[0] = (rpg_quest){.kind=Q_GREET,.goal=2,.gold=44};
    a = reload(&g); assert(a.quests[0].kind != Q_GREET && a.quests[0].gold == 44);
    puts("quests: unavailable social goals excluded and existing unclaimed goals repaired");

    fresh(&g); assert(rpg_dungeon_enter(&g, 0, 0, T0) == RPG_OK);
    int c = 0; while (!rpg_room_reachable(&g.run, 0, c)) c++;
    g.run.rooms[0][c] = ROOM_FIGHT;
    a = g; rpg_classic_begin(&g);
    rpg_foe foes[3]; int n;
    assert(rpg_room_enter(&g, 0, c, foes, &n) == ROOM_FIGHT && n > 0);
    g.cons[CO_POTION]--; g.hero.hp = 1; g.gold_remainder = 17;
    g.lantern.seen = 1u << LN_ROOT;
    b = reload(&g);
    assert(!memcmp(&a, &b, RPG_V1_PROFILE_BYTES));
    assert(b.classic.active == 0 && b.run.row == -1 && b.lantern.seen == (1u << LN_ROOT));
    assert(rpg_room_reachable(&b.run, 0, c));
    puts("classic fight: interrupted transaction restores consumables, vitals and actual parent room");

    fresh(&g); assert(rpg_dungeon_enter(&g, 0, 0, T0) == RPG_OK);
    g.run.floor = 5; rpg_dungeon_floor(&g.run); g.run.row = RPG_ROWS - 1;
    for (c = 0; c < RPG_COLS && !g.run.rooms[RPG_ROWS-1][c]; c++) {}
    g.run.col = (int8_t)c; a = g; rpg_classic_begin(&g);
    assert(rpg_room_enter(&g, RPG_ROWS, 1, foes, &n) == ROOM_BOSS);
    b = reload(&g); assert(b.run.row == a.run.row && b.run.col == a.run.col);
    assert(rpg_room_reachable(&b.run, RPG_ROWS, 1));
    b = legacy(&g, 2, RPG_V2_PROFILE_BYTES);
    assert(b.run.row == RPG_ROWS-1 && rpg_room_reachable(&b.run, RPG_ROWS, 1));
    puts("boss: v3 transaction and v2 orphan migration both leave the encounter reachable");

    fresh(&g); g.hero.level = 6;
    for (int s = 0; s < ST_COUNT; s++) g.hero.stat[s] = 25;
    for (int i = 0; i < RPG_BAG; i++) g.bag[i] = g.hero.equip[SLOT_WEAPON];
    assert(rpg_hunt_start(&g, 0, T0) == RPG_OK); a = g;
    rpg_haul h; assert(rpg_hunt_collect(&g, T0+12*3600, &h) == RPG_ERR_BAG);
    assert(h.nitems > 0 && !memcmp(&a, &g, sizeof g));
    int items = h.nitems;
    for (int i = 0; i < items; i++) rpg_bag_remove(&g, 0);
    assert(rpg_hunt_collect(&g, T0+12*3600, &h) == RPG_OK && h.nitems == items);
    assert(rpg_bag_count(&g) == RPG_BAG && !g.hunt.active);
    before = g.gold; assert(rpg_hunt_collect(&g, T0+12*3600, &h) == RPG_ERR_NONE && g.gold == before);
    puts("idle haul: full bag rejects atomically, gear remains claimable and cannot be claimed twice");

    fresh(&g); ln_start(&g.lantern, LN_MAGE, 91);
    assert(ln_enter(&g.lantern, 0) && ln_action(&g.lantern, LN_BASIC, 0));
    g.lantern.elapsed_ms = 550;
    a = legacy(&g, 2, RPG_V2_PROFILE_BYTES);
    assert(a.lantern.state == LN_ATTACK && a.lantern.elapsed_ms == 550 && a.lantern.equipped == 0);
    a.lantern.blueprints = (1u << LR_HEART) | (1u << LR_GLASS);
    a.lantern.equipped = LR_HEART+1; a.lantern.seen = 0x7f;
    ln_start(&a.lantern, LN_ROGUE, 12);
    assert(a.lantern.relics == (1u << LR_HEART) && a.lantern.seen == 0x7f);
    b = reload(&a); assert(b.lantern.equipped == LR_HEART+1);
    puts("v2 run migration and equipped blueprint/bestiary persistence: passed");
    fresh(&g); ln_start(&g.lantern, LN_SENTINEL, 55); assert(ln_enter(&g.lantern, 1));
    g.gold = 173; g.gold_remainder = 17; g.coach.learned = LNC_ALL;
    g.lantern.effect_flags = 0xff; /* former v3 padding is not trusted */
    a = legacy(&g, 3, RPG_V3_PROFILE_BYTES);
    assert(a.gold == 173 && a.gold_remainder == 17 && a.lantern.state == LN_CHOOSE);
    assert(!a.coach.learned && !a.coach.disabled && !a.lantern.effect_flags);
    g.lantern.wins = 1; a = legacy(&g, 3, RPG_V3_PROFILE_BYTES);
    assert(a.coach.learned == LNC_ALL); /* existing winners are not retrained */
    a.coach.learned = LNC_ATTACK; a.coach.disabled = 1; b = reload(&a);
    assert(b.coach.learned == LNC_ATTACK && b.coach.disabled && b.lantern.wins == 1);
    puts("v3 migration: exact profile/run retained; v4 coaching and effects roundtrip: passed");
    puts("reliability: all audit regressions passed");
    return 0;
}
