/* Host tests for the portable game core. Build: see host/CMakeLists.txt. */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "game.h"
#include "battle.h"
#include "world.h"
#include "timing.h"
#include "save.h"
#include "link.h"

static int checks;
#define CHECK(c) do { checks++; if (!(c)) { fprintf(stderr, "%s:%d: CHECK(%s)\n", __FILE__, __LINE__, #c); exit(1); } } while (0)

static const uint32_t T0 = 20000u * 86400u + 9u * 3600u;   /* some day, 09:00 */

static void hatch(wf_game *g, uint32_t now) {
    int taps = 0;
    while (!wf_warm_seed(g, now)) { taps++; CHECK(taps < 100); }
    CHECK(g->pet.form == WF_FORM_LUMINO);
}

static void test_lifecycle(void) {
    wf_game g;
    wf_game_new(&g, "Pippo", T0, 42);
    CHECK(!strcmp(g.pet.name, "Pippo"));
    CHECK(wf_pet_stage(&g) == WF_STAGE_SEED);
    CHECK(wf_feed(&g, WF_IT_BERRY, T0) == WF_ERR_SEED);
    /* The seed does not starve while waiting. */
    wf_game_tick(&g, T0 + 10 * 3600);
    CHECK(g.pet.hunger == 80000);
    hatch(&g, T0 + 10 * 3600);
    uint32_t t = T0 + 10 * 3600;
    int32_t h = g.pet.hunger;
    wf_game_tick(&g, t + 3600);
    CHECK(g.pet.hunger < h);
    CHECK(h - g.pet.hunger == 60 * 208);
    /* Feeding raises hunger and bond, consumes the item. */
    int berries = g.inv[WF_IT_BERRY];
    CHECK(wf_feed(&g, WF_IT_BERRY, t + 3600) == WF_OK);
    CHECK(g.inv[WF_IT_BERRY] == berries - 1);
    g.pet.hunger = 99000;
    CHECK(wf_feed(&g, WF_IT_BREAD, t + 3600) == WF_ERR_FULL);
    /* Candy still works on a full belly (it barely fills). */
    g.inv[WF_IT_CANDY] = 1;
    CHECK(wf_feed(&g, WF_IT_CANDY, t + 3600) == WF_OK);
    /* Sleep: refuses when rested, recovers energy while sleeping, wakes itself. */
    g.pet.energy = 95000;
    CHECK(wf_sleep_toggle(&g, t + 3600) == WF_ERR_NOT_TIRED);
    g.pet.energy = 10000;
    CHECK(wf_sleep_toggle(&g, t + 3600) == WF_OK && g.pet.sleeping);
    CHECK(wf_feed(&g, WF_IT_BERRY, t + 3600) == WF_ERR_ASLEEP);
    wf_game_tick(&g, t + 3600 + 5 * 3600);
    CHECK(!g.pet.sleeping && g.pet.energy > 0);
    /* Long neglect produces care mistakes but never kills the companion. */
    uint32_t t2 = t + 6 * 3600;
    wf_game_tick(&g, t2);
    uint16_t before = g.pet.care_mistakes;
    wf_game_tick(&g, t2 + 3 * 86400);
    CHECK(g.pet.care_mistakes > before);
    CHECK(g.pet.hunger == 0 && g.pet.mess == WF_MESS_MAX);
    CHECK(wf_clean(&g, t2 + 3 * 86400) == WF_OK && g.pet.mess == WF_MESS_MAX - 1);
    /* Patting has a cooldown. */
    g.pet.sleeping = 0;
    CHECK(wf_pat(&g, t2 + 3 * 86400) == WF_OK);
    CHECK(wf_pat(&g, t2 + 3 * 86400 + 10) == WF_ERR_COOLDOWN);
    /* Catch-up is capped: a year away does not hang. */
    wf_game_tick(&g, t2 + 365u * 86400u);
}

static void test_evolution(void) {
    wf_game g;
    wf_game_new(&g, "Evo", T0, 7);
    hatch(&g, T0);
    g.pet.affinity[WF_EL_TIDE] = 30;
    g.pet.affinity[WF_EL_EMBER] = 10;
    int levels = wf_gain_xp(&g, 500);
    CHECK(levels >= 4 && g.pet.level >= 5);
    CHECK(g.pet.pending_form == WF_FORM_SPRUZZO);
    CHECK(wf_evolve(&g) == WF_FORM_SPRUZZO);
    CHECK(wf_pet_element(&g) == WF_EL_TIDE);
    /* Good care -> bright adult form. */
    g.pet.bond = 80000;
    g.pet.care_mistakes = 2;
    while (g.pet.level < 12) wf_gain_xp(&g, wf_xp_to_next(g.pet.level));
    CHECK(g.pet.pending_form == WF_FORM_CORALBA);
    wf_game other = g;
    other.pet.care_mistakes = 20;
    CHECK(wf_evolution_target(&other) == WF_FORM_ABISSINO);
    wf_evolve(&g);
    wf_stats s = wf_pet_stats(&g);
    CHECK(s.hp > 100 && s.atk > 40);
}

static int battle_sim(int pet_level, int species, int foe_level, int quality, int parry, bool boss, uint32_t seed, int *turns) {
    wf_game g;
    wf_game_new(&g, "Test", T0, seed);
    hatch(&g, T0);
    while (g.pet.level < pet_level) wf_gain_xp(&g, wf_xp_to_next(g.pet.level));
    if (g.pet.pending_form) wf_evolve(&g);
    wf_fighter me, foe;
    wf_fighter_from_pet(&me, &g);
    wf_foe_make(&foe, species, foe_level);
    wf_battle b;
    wf_battle_start(&b, &me, &foe, seed, boss, false);
    int n = 0;
    while (!wf_battle_over(&b) && n < 60) {
        int act = b.f[0].focus >= WF_SKILL_COST ? WF_ACT_SKILL : WF_ACT_STRIKE;
        wf_battle_player(&b, act, quality);
        if (b.state == WF_B_FOE) wf_battle_foe(&b, parry);
        n++;
    }
    if (turns) *turns = n;
    return b.state;
}

static void test_battle(void) {
    int turns;
    /* A fresh companion beats a level-1 slime with decent timing in a few turns. */
    CHECK(battle_sim(1, WF_SP_GELATINO, 1, WF_Q_GOOD, WF_Q_GOOD, false, 1, &turns) == WF_B_WON);
    CHECK(turns >= 2 && turns <= 6);
    /* The first boss is a real fight: careless play at level 5 loses... */
    int lost = 0, won = 0;
    for (uint32_t s = 1; s <= 20; s++) {
        lost += battle_sim(5, WF_SP_RE_GELATINO, 5, WF_Q_MISS, WF_Q_MISS, true, s, NULL) == WF_B_LOST;
        won += battle_sim(6, WF_SP_RE_GELATINO, 5, WF_Q_GREAT, WF_Q_GREAT, true, s, NULL) == WF_B_WON;
    }
    CHECK(lost >= 18);
    /* ...while good timing at level 6 usually wins. */
    CHECK(won >= 16);
    /* Element triangle. */
    CHECK(wf_element_mult(WF_EL_EMBER, WF_EL_GROVE) == 150);
    CHECK(wf_element_mult(WF_EL_GROVE, WF_EL_EMBER) == 67);
    CHECK(wf_element_mult(WF_EL_TIDE, WF_EL_EMBER) == 150);
    CHECK(wf_element_mult(WF_EL_NONE, WF_EL_EMBER) == 100);
    /* Perfect parry blocks everything and counters. */
    wf_fighter a = {.name = "A", .hp = 50, .max_hp = 50, .atk = 20, .def = 10, .spd = 10, .is_pet = 1};
    wf_fighter f = {.name = "F", .hp = 50, .max_hp = 50, .atk = 20, .def = 10, .spd = 10};
    wf_battle b;
    wf_battle_start(&b, &a, &f, 3, false, false);
    wf_battle_player(&b, WF_ACT_GUARD, 0);
    CHECK(b.state == WF_B_FOE && b.f[0].focus == 2);
    wf_battle_foe(&b, WF_Q_PERFECT);
    CHECK(b.f[0].hp == 50 && b.f[1].hp < 50);
    /* Skill without focus degrades to a strike. */
    wf_battle_start(&b, &a, &f, 3, false, false);
    b.f[0].focus = 0;
    wf_battle_player(&b, WF_ACT_SKILL, WF_Q_GOOD);
    CHECK(b.ev[0].kind == WF_EV_NO_FOCUS);
    /* Bosses cannot be fled. */
    wf_battle_start(&b, &a, &f, 3, true, false);
    wf_battle_player(&b, WF_ACT_FLEE, 0);
    CHECK(b.state == WF_B_FOE);
}

static void test_world(void) {
    wf_game g;
    wf_game_new(&g, "Explorer", T0, 99);
    hatch(&g, T0);
    wf_step st;
    CHECK(wf_explore(&g, 1, T0, &st) == WF_ERR_LOCKED);
    int battles = 0, bosses = 0;
    for (int i = 0; i < 200 && g.zone_unlocked == 0; i++) {
        g.pet.energy = WF_NEED_MAX;
        g.pet.hunger = WF_NEED_MAX;
        if (wf_boss_ready(&g, 0) && g.pet.level >= 6) CHECK(wf_boss_step(&g, 0, T0, &st) == WF_OK);
        else CHECK(wf_explore(&g, 0, T0, &st) == WF_OK);
        CHECK(st.kind != WF_STEP_BOSS || wf_boss_ready(&g, 0));
        if (st.kind == WF_STEP_BATTLE || st.kind == WF_STEP_BOSS) {
            wf_fighter me, foe;
            wf_fighter_from_pet(&me, &g);
            wf_foe_make(&foe, st.species, st.level);
            wf_battle b;
            wf_battle_start(&b, &me, &foe, (uint32_t)i, st.kind == WF_STEP_BOSS, false);
            while (!wf_battle_over(&b)) {
                wf_battle_player(&b, b.f[0].focus >= 2 ? WF_ACT_SKILL : WF_ACT_STRIKE, WF_Q_GREAT);
                if (b.state == WF_B_FOE) wf_battle_foe(&b, WF_Q_GREAT);
            }
            bool unlocked;
            wf_battle_finish(&g, 0, &b, &unlocked, NULL, NULL);
            battles++;
            if (st.kind == WF_STEP_BOSS) bosses++;
            if (st.kind == WF_STEP_BOSS && b.state == WF_B_WON) CHECK(unlocked);
        }
    }
    CHECK(g.zone_unlocked == 1);
    CHECK(battles >= WF_BOSS_AFTER + 1 && bosses >= 1);
    CHECK(g.wins >= WF_BOSS_AFTER);
    /* Expedition: away, then back with a haul. */
    g.pet.energy = WF_NEED_MAX;
    uint32_t coins = g.coins;
    CHECK(wf_expedition_start(&g, 0, 1, T0) == WF_OK);
    CHECK(wf_pet_away(&g, T0) && !wf_pet_can_act(&g, T0));
    CHECK(wf_feed(&g, WF_IT_BERRY, T0) == WF_ERR_AWAY);
    wf_haul h;
    CHECK(wf_expedition_collect(&g, T0 + 60, &h) == WF_ERR_AWAY);
    wf_game_tick(&g, T0 + 121 * 60);
    CHECK(wf_expedition_collect(&g, T0 + 121 * 60, &h) == WF_OK);
    CHECK(h.xp > 0 && g.coins == coins + h.coins && !wf_pet_away(&g, T0 + 121 * 60));
    int found = 0;
    for (int i = 0; i < WF_IT_COUNT; i++) found += h.items[i];
    CHECK(found == 4);
}

static void test_daily(void) {
    wf_game g;
    wf_game_new(&g, "Daily", T0, 5);
    hatch(&g, T0);
    int item;
    CHECK(wf_daily_claim(&g, &item) == 15 && g.streak == 1);
    CHECK(wf_daily_claim(&g, &item) == 0);
    wf_game_tick(&g, T0 + 86400);
    CHECK(!g.daily_claimed);
    CHECK(wf_daily_claim(&g, &item) == 20 && g.streak == 2);
    /* Skipping a day resets the streak. */
    wf_game_tick(&g, T0 + 3 * 86400);
    CHECK(g.streak == 0);
    /* Quests: three distinct kinds, progress and claim. */
    CHECK(g.quests[0].kind != g.quests[1].kind && g.quests[1].kind != g.quests[2].kind);
    wf_quest *q = &g.quests[0];
    CHECK(wf_quest_claim(&g, 0) == 0);
    wf_quest_bump(&g, q->kind, 50);
    CHECK(q->progress == q->goal);
    CHECK(wf_quest_claim(&g, 0) == q->coins && q->claimed);
    CHECK(wf_quest_claim(&g, 0) == 0);
    char buf[64];
    CHECK(strlen(wf_quest_text(q, buf, sizeof buf)) > 5);
}

static void test_timing(void) {
    wf_rng r;
    wf_rng_seed(&r, 11);
    wf_ring ring;
    wf_ring_make(&ring, &r, 10, 1, 70);
    CHECK(ring.perfect < ring.great && ring.great < ring.good);
    /* Find the time the spark crosses the centre: it must be perfect. */
    int hit = -1;
    for (uint32_t t = 0; t < ring.period_ms; t++)
        if (wf_ring_angle(&ring, t) == ring.center) { hit = (int)t; break; }
    CHECK(hit >= 0);
    CHECK(wf_ring_judge(&ring, (uint32_t)hit) == WF_Q_PERFECT);
    CHECK(wf_ring_judge(&ring, (uint32_t)(hit + ring.period_ms / 2)) == WF_Q_MISS);
    CHECK(wf_ring_judge(&ring, ring.period_ms * 2u + (uint32_t)hit) == WF_Q_MISS);
    CHECK(wf_angle_diff(10, 350) == 20 && wf_angle_diff(350, 10) == -20);
    wf_parry p;
    wf_parry_make(&p, &r, 10, 10, 0);
    CHECK(wf_parry_judge(&p, p.travel_ms) == WF_Q_PERFECT);
    CHECK(wf_parry_judge(&p, p.travel_ms + p.great_ms) == WF_Q_GREAT);
    CHECK(wf_parry_judge(&p, p.travel_ms + 400) == WF_Q_MISS);
}

static void test_save(void) {
    wf_game g, back;
    wf_game_new(&g, "Salvami", T0, 77);
    hatch(&g, T0);
    g.coins = 1234;
    static uint8_t buf[WF_SAVE_BYTES];
    CHECK(wf_save_encode(&g, 9, buf, sizeof buf) == WF_SAVE_BYTES);
    uint32_t seq;
    CHECK(wf_save_decode(buf, sizeof buf, &back, &seq) && seq == 9);
    CHECK(!memcmp(&g, &back, sizeof g));
    buf[sizeof(wf_save_header) + 3] ^= 0x40;
    CHECK(!wf_save_decode(buf, sizeof buf, &back, &seq));
    CHECK(!wf_save_decode(buf, 10, &back, &seq));
}

/* ---- Two devices over a lossy, duplicating radio ------------------------ */
typedef struct { uint8_t data[WF_PACKET_BYTES]; uint8_t from[6]; uint8_t to; } frame;
static frame air[512];
static int air_count;
static unsigned loss_seed = 1;
static int loss_percent;

typedef struct { int index; } radio_ctx;
static const uint8_t macs[2][6] = {{1, 2, 3, 4, 5, 6}, {9, 8, 7, 6, 5, 4}};

static int lossy(void) { loss_seed = loss_seed * 1103515245u + 12345u; return (int)((loss_seed >> 16) % 100u) < loss_percent; }

static void radio_send(void *ctx, const uint8_t *peer, const uint8_t *data, size_t len) {
    radio_ctx *c = ctx;
    (void)peer;
    for (int copy = 0; copy < 2; copy++) {           /* sometimes duplicated */
        if (lossy() || air_count >= 512) continue;
        frame *f = &air[air_count++];
        memcpy(f->data, data, len);
        memcpy(f->from, macs[c->index], 6);
        f->to = (uint8_t)!c->index;
        if (!lossy()) break;
    }
}

static void deliver(wf_link *links, wf_game *games, uint32_t ms, uint32_t s) {
    int n = air_count;
    frame batch[512];
    memcpy(batch, air, sizeof(frame) * (size_t)n);
    air_count = 0;
    /* Deliver in reverse order every other tick to exercise reordering. */
    for (int i = 0; i < n; i++) {
        frame *f = &batch[(ms / 10) % 2 ? n - 1 - i : i];
        wf_link_receive(&links[f->to], &games[f->to], f->from, f->data, WF_PACKET_BYTES, ms, s);
    }
}

static void test_packets(void) {
    wf_packet p = {0}, q;
    p.type = WF_PKT_DUEL_TURN;
    p.session = 0xDEADBEEF;
    strcpy(p.who.name, "Abcdefghijkl");
    p.who.form = 3; p.who.level = 17; p.hp = -2; p.atk = 300; p.round = 513; p.action = 1; p.quality = 3;
    uint8_t buf[WF_PACKET_BYTES];
    CHECK(wf_packet_encode(&p, buf, sizeof buf) == WF_PACKET_BYTES);
    CHECK(wf_packet_decode(buf, sizeof buf, &q));
    CHECK(q.session == p.session && !strcmp(q.who.name, p.who.name) && q.hp == -2 && q.atk == 300 && q.round == 513);
    buf[10] ^= 1;
    CHECK(!wf_packet_decode(buf, sizeof buf, &q));
}

static void test_link(int loss) {
    loss_percent = loss;
    air_count = 0;
    wf_game games[2];
    wf_link links[2];
    radio_ctx ctx[2] = {{0}, {1}};
    wf_game_new(&games[0], "Ale", T0, 1);
    wf_game_new(&games[1], "Bea", T0, 2);
    hatch(&games[0], T0);
    hatch(&games[1], T0);
    while (games[1].pet.level < 3) wf_gain_xp(&games[1], 200);
    for (int i = 0; i < 2; i++) wf_link_init(&links[i], radio_send, &ctx[i], 100u + (unsigned)i);
    uint32_t ms = 1;
    #define STEP() do { ms += 10; wf_link_poll(&links[0], &games[0], ms, T0); wf_link_poll(&links[1], &games[1], ms, T0); deliver(links, games, ms, T0); } while (0)
    for (int i = 0; i < 400 && (!wf_link_peer_count(&links[0]) || !wf_link_peer_count(&links[1])); i++) STEP();
    CHECK(wf_link_peer_count(&links[0]) == 1 && !strcmp(wf_link_peer(&links[0], 0)->who.name, "Bea"));

    /* Greeting: counted once per day on both sides. */
    int32_t mood = games[1].pet.mood = 20000;
    CHECK(wf_link_greet(&links[0], &games[0], macs[1], ms, T0));
    for (int i = 0; i < 200 && links[0].out.active; i++) STEP();
    CHECK(!links[0].out.active);
    CHECK(games[1].pet.mood > mood);
    CHECK(wf_friend_find(&games[0], macs[1]) && wf_friend_find(&games[1], macs[0]));

    /* Gift: exactly one item moves even with duplicates. */
    games[0].inv[WF_IT_STEW] = 1;
    int before = games[1].inv[WF_IT_STEW];
    CHECK(wf_link_gift(&links[0], &games[0], macs[1], WF_IT_STEW, ms));
    for (int i = 0; i < 300 && links[0].out.active; i++) STEP();
    wf_link_event ev;
    int failed = 0;
    while (wf_link_next_event(&links[0], &ev)) failed |= ev.kind == WF_LE_SEND_FAILED;
    if (!failed) CHECK(games[1].inv[WF_IT_STEW] == before + 1 && games[0].inv[WF_IT_STEW] == 0);
    while (wf_link_next_event(&links[1], &ev)) {}

    /* Duel. */
    CHECK(wf_link_duel_invite(&links[0], &games[0], macs[1], ms, T0));
    bool invited = false;
    for (int i = 0; i < 400 && !invited; i++) {
        STEP();
        while (wf_link_next_event(&links[1], &ev)) invited |= ev.kind == WF_LE_DUEL_INVITED;
    }
    CHECK(invited);
    CHECK(wf_link_duel_respond(&links[1], &games[1], true, ms, T0));
    for (int i = 0; i < 400 && links[0].duel.state != WF_DUEL_ACTIVE; i++) STEP();
    CHECK(links[0].duel.state == WF_DUEL_ACTIVE && links[1].duel.state == WF_DUEL_ACTIVE);
    unsigned pick = 7;
    for (int guard = 0; guard < 4000; guard++) {
        for (int s = 0; s < 2; s++) {
            wf_duel *d = &links[s].duel;
            if (d->state == WF_DUEL_ACTIVE && !d->my_ready) {
                pick = pick * 1664525u + 1013904223u;
                int act = (int)(pick >> 28) % 3, q = (int)(pick >> 24) % 4;
                wf_link_duel_submit(&links[s], act, q, ms);
            }
        }
        STEP();
        if (links[0].duel.state == WF_DUEL_DONE && links[1].duel.state == WF_DUEL_DONE) break;
    }
    for (int i = 0; i < 50; i++) STEP();
    CHECK(links[0].duel.state == WF_DUEL_DONE && links[1].duel.state == WF_DUEL_DONE);
    wf_battle *a = &links[0].duel.battle, *b = &links[1].duel.battle;
    CHECK(a->state == b->state && a->round == b->round);
    CHECK(a->f[0].hp == b->f[0].hp && a->f[1].hp == b->f[1].hp);
    CHECK(!strcmp(a->f[1].name, "Bea") && !strcmp(b->f[0].name, "Ale"));
    printf("  duel at %d%% loss: %u rounds (duel %u), %s won, hp %d/%d vs %d/%d\n", loss, a->round, links[0].duel.round, a->state == WF_B_WON ? "A" : "B", a->f[0].hp, a->f[0].max_hp, a->f[1].hp, a->f[1].max_hp);
    uint16_t w0 = games[0].duel_wins + games[0].duel_losses;
    wf_link_duel_record(&links[0], &games[0], T0);
    wf_link_duel_record(&links[0], &games[0], T0);
    wf_link_duel_record(&links[1], &games[1], T0);
    CHECK(games[0].duel_wins + games[0].duel_losses == w0 + 1);
    CHECK(games[0].duel_wins == games[1].duel_losses);
    wf_link_duel_leave(&links[0], ms);
    wf_link_duel_leave(&links[1], ms);
    #undef STEP
}

int main(void) {
    test_lifecycle();
    test_evolution();
    test_battle();
    test_world();
    test_daily();
    test_timing();
    test_save();
    test_packets();
    test_link(0);
    test_link(20);
    test_link(45);
    printf("core: %d checks passed\n", checks);
    return 0;
}
