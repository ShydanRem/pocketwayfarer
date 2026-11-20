/* Model-level acceptance tests. Timing quality is injected at its real timestamp,
 * not by changing damage or restoring health between encounters. */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "rpg.h"
#include "timing.h"

static unsigned snapshots, steps, discovered;
static uint32_t crc32(const uint8_t *p, size_t n) {
    uint32_t c = ~0u;
    while (n--) {
        c ^= *p++;
        for (int k = 0; k < 8; k++) c = (c >> 1) ^ (0xedb88320u & (uint32_t)-(int32_t)(c & 1));
    }
    return ~c;
}
static void fresh(rpg_game *g) {
    rpg_new_hero n = {0};
    strcpy(n.name, "Lantern");
    rpg_game_new(g, &n, 846756000u, 71);
}
static void roundtrip(rpg_game *g) {
    uint8_t buf[4096];
    rpg_game back;
    size_t size = rpg_save_encode(g, buf, sizeof buf);
    assert(size && rpg_save_decode(buf, size, &back));
    if (memcmp(g, &back, sizeof *g)) {
        fprintf(stderr, "roundtrip mismatch state=%u next=%u acting=%u n=%u\n", g->lantern.state,
                g->lantern.next_state, g->lantern.acting, g->lantern.nfoes);
        abort();
    }
    *g = back;
    snapshots++;
}
static unsigned tick_at_quality(rpg_lantern *l, int quality) {
    static const unsigned offset[] = {250, 95, 60, 0};
    return (l->state == LN_ATTACK ? 700u : 1000u) + offset[quality];
}
static int play(int archetype, uint32_t seed, int quality, int branch, int starter, int policy) {
    rpg_game g;
    fresh(&g);
    if (starter >= 0) { g.lantern.blueprints = (uint8_t)(1u << starter); g.lantern.equipped = (uint8_t)(starter+1); }
    ln_start(&g.lantern, archetype, seed);
    for (int guard = 0; guard < 5000 && ln_active(&g.lantern); guard++) {
        rpg_lantern *l = &g.lantern;
        switch (l->state) {
        case LN_ROUTE: assert(ln_enter(l, l->room == 4 ? 0 : branch)); break;
        case LN_CHOOSE:
            assert(ln_action(l, l->sp >= 6 ? LN_POWER : LN_BASIC, 0)); break;
        case LN_ATTACK: case LN_PARRY:
            l->elapsed_ms = (uint16_t)tick_at_quality(l, quality);
            assert(ln_quality(l) == quality);
            assert(ln_tap(l)); break;
        case LN_TELEGRAPH: case LN_IMPACT: assert(ln_tick(l, 1000)); break;
        case LN_RELIC:
            assert(l->offers[0] != l->offers[1]);
            assert(!(l->relics & (1u << l->offers[0])) && !(l->relics & (1u << l->offers[1])));
            if (policy == 2) assert(ln_skip_relic(l));
            else if (ln_relic_count(l) < 3) assert(ln_choose_relic(l, (int)(seed & 1u)));
            else if (!policy) assert(ln_skip_relic(l));
            else {
                int outgoing = 0;
                while (!(l->relics & (1u << outgoing))) outgoing++;
                assert(ln_replace_relic(l, (int)(seed & 1u), outgoing));
            }
            assert(ln_relic_count(l) <= 3); break;
        case LN_EVENT: assert(ln_event(l, 0)); break;
        case LN_CLEARED: assert(ln_continue(l)); break;
        case LN_BLUEPRINT: {
            int r = 0;
            while (!(l->relics & (1u << r))) r++;
            assert(ln_keep_blueprint(l, r));
            assert(l->blueprints & (1u << r));
            if (starter >= 0) assert(l->blueprints & (1u << starter));
            break;
        }
        default: assert(0);
        }
        assert(l->hp >= 0 && l->hp <= l->max_hp && l->sp >= 0 && l->sp <= l->max_sp);
        assert(l->currency <= 80 && l->resonance <= 3);
        roundtrip(&g);
        steps++;
    }
    assert(!ln_active(&g.lantern));
    discovered |= g.lantern.seen;
    int won = g.lantern.state == LN_WON;
    unsigned reward = ln_bank(&g.lantern);
    assert(reward <= 80 && ln_bank(&g.lantern) == 0);
    roundtrip(&g);
    assert(ln_bank(&g.lantern) == 0);
    if (won) assert(reward == 80 && g.lantern.wins == 1);
    else assert(g.lantern.state == LN_LOST);
    if (policy == 2) assert(!g.lantern.relics && !g.lantern.blueprints);
    return won;
}
static void migration(void) {
    rpg_game g, back;
    fresh(&g);
    g.gold = 4321; g.hero.level = 19; g.gems[1] = 3;
    const size_t old_size = offsetof(rpg_game, lantern);
    assert(old_size == 1356); /* actual v1 ABI, not a size inferred from the new struct */
    uint8_t buf[4096];
    uint32_t magic = RPG_SAVE_MAGIC, size = (uint32_t)old_size;
    uint16_t version = 1, reserved = 0;
    memcpy(buf, &magic, 4); memcpy(buf + 4, &version, 2); memcpy(buf + 6, &reserved, 2);
    memcpy(buf + 8, &size, 4); memcpy(buf + 16, &g, old_size);
    uint32_t crc = crc32(buf + 16, old_size);
    memcpy(buf + 12, &crc, 4);
    assert(rpg_save_decode(buf, old_size + 16, &back));
    assert(!memcmp(&g, &back, old_size) && back.lantern.state == LN_NONE && back.lantern.wins == 0);
    roundtrip(&back);
    assert(!rpg_save_decode(buf, old_size + 15, &back));
    buf[23] ^= 1;
    assert(!rpg_save_decode(buf, old_size + 16, &back));
}
static void timing_pause_and_boundaries(void) {
    rpg_game g;
    fresh(&g); ln_start(&g.lantern, LN_SENTINEL, 11);
    rpg_lantern *l = &g.lantern;
    assert(ln_enter(l, 1) && ln_action(l, LN_POWER, 1));
    int sp = l->sp;
    assert(!ln_action(l, LN_POWER, 0) && l->sp == sp);
    assert(!ln_choose_relic(l, 0) && !ln_continue(l) && !ln_retreat(l));
    l->elapsed_ms = 650; l->paused = 1;
    roundtrip(&g);
    assert(!ln_tick(l, 1000) && l->elapsed_ms == 650 && !ln_tap(l));
    l->paused = 0; l->elapsed_ms = 700;
    assert(ln_tap(l) && l->sp == sp);
    assert(!ln_tap(l));
    /* Full timing windows and assist apply equally to attack and parry. */
    l->state = LN_ATTACK; l->elapsed_ms = 746;
    assert(ln_quality(l) == WF_Q_GREAT);
    l->assist = 1;
    assert(ln_quality(l) == WF_Q_PERFECT);
    l->assist = 0; l->elapsed_ms = 2795;
    assert(ln_tick(l, 10) && l->quality == WF_Q_MISS);
    l->state = LN_CLEARED; l->currency = 30;
    assert(ln_retreat(l) && ln_bank(l) == 30 && ln_bank(l) == 0);
    ln_start(l, LN_ROGUE, 18);
    assert(l->hp == l->max_hp && l->currency == 0 && l->relics == 0 && l->resonance == 0);
    l->state = LN_LOST; l->currency = 30;
    assert(ln_bank(l) == 23); /* lose floor(30 / 4) */
    assert(ln_bank(l) == 0);
}
static void relics(void) {
    rpg_lantern a = {0}, b;
    ln_start(&a, 0, 3); a.relics = 1u << LR_BELL;
    assert(ln_enter(&a, 0) && a.shield == 12);
    a.state = LN_CLEARED; a.hp = 40; a.relics = 1u << LR_COMPASS;
    assert(ln_continue(&a) && a.hp == 46);
    ln_start(&a, 0, 3); a.relics = 1u << LR_GLASS;
    assert(ln_enter(&a, 0) && ln_action(&a, LN_GUARD, 0) && a.focused == 8);
    ln_start(&a, 1, 4); assert(ln_enter(&a, 0));
    a.relics = 1u << LR_COAL;
    assert(ln_action(&a, LN_BASIC, 0)); a.elapsed_ms = 700; assert(ln_tap(&a));
    assert(a.foes[0].burn == 2 && a.foes[0].burn_turns == 2);
    ln_start(&a, 1, 7); assert(ln_enter(&a, 0));
    a.foes[0].hp--; b = a; b.relics = 1u << LR_THORN;
    assert(ln_action(&a, LN_BASIC, 0) && ln_action(&b, LN_BASIC, 0));
    a.elapsed_ms = b.elapsed_ms = 700; assert(ln_tap(&a) && ln_tap(&b));
    assert(b.last_damage == a.last_damage + 4);
    ln_start(&a, 0, 8); assert(ln_enter(&a, 0));
    a.state = LN_PARRY; a.elapsed_ms = 1000; a.sp = 0; a.relics = 1u << LR_HEART;
    assert(ln_tap(&a) && a.sp == 4 && a.resonance == 1); /* +3 relic, +1 end turn */
}
static void relic_exchanges(void) {
    rpg_game g;
    for (unsigned mask = 0; mask < (1u << LR_COUNT); mask++) {
        fresh(&g); ln_start(&g.lantern, LN_MAGE, 31);
        rpg_lantern *l = &g.lantern;
        l->relics = (uint8_t)mask;
        if (ln_relic_count(l) != 3) continue;
        l->state = LN_RELIC; l->room = 3; l->currency = 30;
        int n = 0;
        for (int r = 0; r < LR_COUNT && n < 2; r++)
            if (!(mask & (1u << r))) l->offers[n++] = (uint8_t)r;
        roundtrip(&g);
        rpg_lantern before = *l;
        assert(!ln_choose_relic(l, 0) && !ln_choose_relic(l, 1));
        assert(!ln_replace_relic(l, -1, 0) && !ln_replace_relic(l, 2, 0));
        assert(!ln_replace_relic(l, 0, -1) && !ln_replace_relic(l, 0, LR_COUNT));
        assert(!memcmp(l, &before, sizeof before));
        for (int choice = 0; choice < 2; choice++) for (int outgoing = 0; outgoing < LR_COUNT; outgoing++) {
            *l = before;
            bool owned = (mask & (1u << outgoing)) != 0;
            assert(ln_replace_relic(l, choice, outgoing) == owned);
            rpg_lantern expected = before;
            if (owned) {
                expected.relics = (uint8_t)((mask & ~(1u << outgoing)) | (1u << before.offers[choice]));
                expected.state = LN_CLEARED;
                assert(ln_relic_count(l) == 3);
            }
            assert(!memcmp(l, &expected, sizeof expected));
            roundtrip(&g);
            if (owned) assert(!ln_replace_relic(l, choice, outgoing)); /* no repeated exchange */
        }
        *l = before;
        assert(ln_skip_relic(l) && l->relics == mask && l->rng == before.rng && l->currency == 30);
        roundtrip(&g);
        assert(!ln_skip_relic(l));
    }
    fresh(&g); ln_start(&g.lantern, LN_SENTINEL, 72);
    rpg_lantern *l = &g.lantern;
    l->state = LN_RELIC; l->relics = 1u << LR_BELL; l->offers[0] = LR_HEART;
    rpg_lantern before = *l;
    assert(!ln_replace_relic(l, 0, LR_BELL) && !memcmp(l, &before, sizeof before));
    l->offers[0] = LR_BELL; before = *l;
    assert(!ln_choose_relic(l, 0) && !memcmp(l, &before, sizeof before));
    l->offers[0] = LR_COUNT; before = *l;
    assert(!ln_choose_relic(l, 0) && !memcmp(l, &before, sizeof before));
    assert(ln_skip_relic(l) && l->relics == (1u << LR_BELL));
    l->state = LN_BLUEPRINT; before = *l;
    assert(!ln_skip_relic(l) && !memcmp(l, &before, sizeof before));
    puts("relic exchange: all 20 three-slot loadouts, both offers, owned/unowned targets, atomic failures, skip and save/reload: passed");
}
static void coaching_and_feedback(void) {
    rpg_game g; fresh(&g);
    rpg_lantern *l = &g.lantern, before;
    ln_start(l, LN_ROGUE, 31); assert(ln_enter(l, 1));
    assert(ln_coach_tip(&g.coach, l, false) == LNT_ATTACK);
    before = *l;
    assert(ln_coach_tip(&g.coach, l, true) == LNT_SKILL && !memcmp(&before, l, sizeof before));
    assert(ln_action(l, LN_BASIC, 0)); before = *l;
    assert(ln_coach_tip(&g.coach, l, false) == LNT_ATTACK_TIMING);
    l->elapsed_ms = 100; assert(ln_tap(l)); ln_coach_observe(&g.coach, &before, l);
    assert(!(g.coach.learned & LNC_ATTACK));
    ln_start(l, LN_ROGUE, 31); assert(ln_enter(l, 1) && ln_action(l, LN_POWER, 1));
    l->resonance = 3; before = *l; l->elapsed_ms = 700;
    int hp = l->foes[1].hp;
    assert(ln_tap(l)); ln_coach_observe(&g.coach, &before, l);
    assert(l->effect_actor == 1 && (l->effect_flags & LN_EFFECT_RESONANCE));
    assert(l->last_damage == hp-l->foes[1].hp && l->resonance == 0);
    assert((g.coach.learned & (LNC_ATTACK|LNC_RESONANCE)) == (LNC_ATTACK|LNC_RESONANCE));
    roundtrip(&g);
    assert(l->effect_actor == 1 && (l->effect_flags & LN_EFFECT_RESONANCE));
    /* The attacking foe index survives incrementing acting to the next foe. */
    ln_start(l, LN_SENTINEL, 32); assert(ln_enter(l, 1));
    l->state = LN_PARRY; l->acting = 0; l->elapsed_ms = 1000; before = *l;
    assert(ln_tap(l)); ln_coach_observe(&g.coach, &before, l);
    assert(l->effect_actor == 0 && l->acting == 1 && (g.coach.learned & LNC_PARRY));
    g.coach.learned = LNC_ATTACK|LNC_PARRY; l->state = LN_CHOOSE; l->sp = 4;
    assert(ln_coach_tip(&g.coach, l, false) == LNT_RESONANCE);
    ln_coach_ack(&g.coach, LNT_RESONANCE);
    assert(ln_coach_tip(&g.coach, l, false) == LNT_GUARD);
    before = *l; assert(ln_action(l, LN_GUARD, 0)); ln_coach_observe(&g.coach, &before, l);
    assert(g.coach.learned & LNC_GUARD);
    l->state = LN_CHOOSE; l->sp = l->max_sp; before = *l;
    assert(ln_action(l, LN_SUPPORT, 0)); ln_coach_observe(&g.coach, &before, l);
    assert(g.coach.learned == LNC_ALL);
    g.coach.disabled = 1; l->state = LN_CHOOSE;
    assert(ln_coach_tip(&g.coach, l, true) == LNT_NONE);
    roundtrip(&g); ln_start(l, LN_MAGE, 19);
    assert(g.coach.learned == LNC_ALL && g.coach.disabled);
    /* A foe burning out before it attacks is not shown as an incoming hit. */
    assert(ln_enter(l, 1)); l->state = LN_PARRY; l->foes[0].hp = 2;
    l->foes[0].burn = 5; l->foes[0].burn_turns = 1; l->elapsed_ms = 1000;
    hp = l->hp; assert(ln_tap(l));
    assert(l->feedback == LN_FEEDBACK_BURN && l->last_damage == 2 && l->hp == hp && !l->resonance);
    puts("coach/feedback: actual learning, miss/retry, opt-out, exact actor/damage, resonance and burn: passed");
}

static void finish_bastion_encounter(rpg_game *g, int quality) {
    rpg_lantern *l = &g->lantern;
    for (int i = 0; i < 150 && l->state >= LN_CHOOSE && l->state <= LN_IMPACT; i++) {
        switch (l->state) {
        case LN_CHOOSE: assert(ln_action(l, LN_BASIC, 0)); break;
        case LN_ATTACK: case LN_PARRY:
            l->elapsed_ms = (uint16_t)tick_at_quality(l, quality);
            assert(ln_quality(l) == quality && ln_tap(l));
            break;
        default: assert(ln_tick(l, 1000)); break;
        }
        roundtrip(g);
    }
    assert(l->state == LN_RELIC || l->state == LN_LOST);
}

static void bastion(void) {
    rpg_game g;
    fresh(&g);
    rpg_lantern *l = &g.lantern;
    ln_start(l, LN_SENTINEL, 3294095628u);
    ln_route_info info;
    rpg_lantern before = *l;
    assert(!ln_preview_route(l, LN_PATH_BASTION, &info) && !ln_enter(l, LN_PATH_BASTION));
    assert(!memcmp(l, &before, sizeof *l));

    l->room = LN_BASTION_ROOM;
    l->currency = 30;
    l->hp = 12;
    before = *l;
    assert(ln_preview_route(l, LN_PATH_TRAIL, &info) && !info.hp_cost && info.gold == 10);
    assert(ln_preview_route(l, LN_PATH_BASTION, &info) && info.hp_cost == 12 && info.gold == 25);
    assert(info.count == 1 && info.foes[0].kind == LN_CRAB && info.foes[0].max_hp == 88);
    assert(!ln_enter(l, LN_PATH_BASTION) && !memcmp(l, &before, sizeof *l));
    roundtrip(&g);

    l->hp = 90;
    assert(ln_enter(l, LN_PATH_BASTION));
    assert(l->hp == 78 && l->currency == 30 && l->branch == LN_PATH_BASTION);
    assert(l->nfoes == 1 && l->foes[0].kind == LN_CRAB && l->foes[0].intent == LN_HEAVY);
    assert(l->foes[0].hp == 88 && (l->seen & (1u << LN_CRAB)));
    roundtrip(&g);
    assert(!ln_enter(l, LN_PATH_BASTION) && l->hp == 78);
    finish_bastion_encounter(&g, WF_Q_PERFECT);
    assert(l->state == LN_RELIC && l->currency == 55 && !ln_bank(l));
    assert(ln_skip_relic(l) && ln_retreat(l) && ln_bank(l) == 55);
    roundtrip(&g);
    assert(!ln_bank(l));

    ln_start(l, LN_SENTINEL, 2224073057u);
    l->room = LN_BASTION_ROOM;
    l->currency = 30;
    l->hp = 13;
    assert(ln_enter(l, LN_PATH_BASTION) && l->hp == 1);
    finish_bastion_encounter(&g, WF_Q_MISS);
    assert(l->state == LN_LOST && l->currency == 30 && ln_bank(l) == 23);
    roundtrip(&g);
    assert(!ln_bank(l));

    /* The old Pier branch in saved fourth-room encounters keeps its original payoff. */
    ln_start(l, LN_SENTINEL, 2224073057u);
    l->room = LN_BASTION_ROOM;
    l->currency = 30;
    assert(ln_enter(l, LN_PATH_PIER) && l->hp == 120 && l->nfoes == 2);
    roundtrip(&g);
    finish_bastion_encounter(&g, WF_Q_PERFECT);
    assert(l->state == LN_RELIC && l->currency == 40);
    puts("Bastion: pure preview, exact entry cost, low HP guard, legacy Pier, v4 resume and one-time win/loss payouts passed");
}

int main(void) {
    migration(); timing_pause_and_boundaries(); relics(); relic_exchanges(); coaching_and_feedback();
    bastion();
    int any_loss = 0;
    for (int q = 0; q <= 3; q++) {
        printf("lantern %s:", q == 0 ? "miss" : q == 1 ? "good" : q == 2 ? "great" : "perfect");
        for (int a = 0; a < 3; a++) {
            int wins = 0;
            for (uint32_t seed = 1; seed <= 24; seed++) wins += play(a, seed, q, (int)(seed & 1u), -1, false);
            if (q == 3) assert(wins == 24);
            if (q == 0) any_loss |= wins < 24;
            printf(" %d/24", wins);
        }
        puts("");
    }
    assert(any_loss);
    for (int relic = 0; relic < LR_COUNT; relic++) for (int a = 0; a < LN_ARCHETYPES; a++)
        for (uint32_t seed = 1; seed <= 8; seed++) {
            assert(play(a, seed, WF_Q_GREAT, (int)(seed&1u), relic, false));
            assert(play(a, seed, WF_Q_GREAT, (int)(seed&1u), relic, true));
        }
    for (int a = 0; a < LN_ARCHETYPES; a++) for (uint32_t seed = 1; seed <= 8; seed++)
        assert(play(a, seed, WF_Q_PERFECT, (int)(seed&1u), -1, 2));
    assert(discovered == (1u << LN_ENEMIES)-1u);
    puts("600 runs: baseline, all starting relics x loadouts x eight seeds x skip/replace, and 24 no-relic wins; all bestiary entries: passed");
    printf("lantern: %u save/resume snapshots, %u rule transitions passed\n", snapshots, steps);
    return 0;
}
