#include "battle.h"
#include <string.h>

static const int quality_mult[4] = {35, 100, 130, 175};
static const int parry_taken[4] = {100, 70, 45, 0};

const char *wf_element_name(int element) {
    switch (element) {
    case WF_EL_EMBER: return "Brace";
    case WF_EL_TIDE: return "Marea";
    case WF_EL_GROVE: return "Fronda";
    default: return "Astro";
    }
}

int wf_element_mult(int attacker, int defender) {
    if (!attacker || !defender) return 100;
    if ((attacker == WF_EL_EMBER && defender == WF_EL_GROVE) ||
        (attacker == WF_EL_GROVE && defender == WF_EL_TIDE) ||
        (attacker == WF_EL_TIDE && defender == WF_EL_EMBER)) return 150;
    if ((defender == WF_EL_EMBER && attacker == WF_EL_GROVE) ||
        (defender == WF_EL_GROVE && attacker == WF_EL_TIDE) ||
        (defender == WF_EL_TIDE && attacker == WF_EL_EMBER)) return 67;
    return 100;
}

void wf_fighter_from_pet(wf_fighter *out, const wf_game *g) {
    memset(out, 0, sizeof *out);
    wf_stats s = wf_pet_stats(g);
    wf_copy_name(out->name, g->pet.name);
    out->element = (uint8_t)wf_pet_element(g);
    out->level = g->pet.level;
    out->sprite = g->pet.form;
    out->is_pet = 1;
    out->max_hp = out->hp = (int16_t)s.hp;
    out->atk = (int16_t)s.atk;
    out->def = (int16_t)s.def;
    out->spd = (int16_t)s.spd;
    out->focus = 1;
}

void wf_battle_start(wf_battle *b, const wf_fighter *a, const wf_fighter *foe,
                     uint32_t seed, bool boss, bool duel) {
    memset(b, 0, sizeof *b);
    b->f[0] = *a;
    b->f[1] = *foe;
    b->f[0].focus = 1;
    b->f[1].focus = duel ? 1 : 0;
    wf_rng_seed(&b->rng, seed);
    b->boss = boss;
    b->duel = duel;
    b->state = WF_B_PLAYER;
}

bool wf_battle_over(const wf_battle *b) { return b->state >= WF_B_WON; }

static void push(wf_battle *b, int kind, int actor, int quality, int effect, int skill, int value) {
    if (b->ev_count >= WF_EVENTS) return;
    wf_event *e = &b->ev[b->ev_count++];
    e->kind = (uint8_t)kind;
    e->actor = (uint8_t)actor;
    e->quality = (uint8_t)quality;
    e->effect = (uint8_t)effect;
    e->skill = (uint8_t)skill;
    e->value = (int16_t)value;
}

int wf_damage(const wf_fighter *att, const wf_fighter *def, int power, int quality, int roll) {
    if (quality < 0) quality = 0;
    if (quality > 3) quality = 3;
    int base = att->atk * power / 100;
    int d = base * 50 / (50 + def->def);
    d = d * quality_mult[quality] / 100;
    d = d * roll / 100;
    return d < 1 ? 1 : d;
}

static void hurt(wf_fighter *f, int dmg) {
    int hp = f->hp - dmg;
    f->hp = (int16_t)(hp < 0 ? 0 : hp);
}

static void heal(wf_fighter *f, int amount) {
    int hp = f->hp + amount;
    f->hp = (int16_t)(hp > f->max_hp ? f->max_hp : hp);
}

static void gain_focus(wf_battle *b, int side, int amount) {
    wf_fighter *f = &b->f[side];
    int v = f->focus + amount;
    f->focus = (uint8_t)(v > WF_FOCUS_MAX ? WF_FOCUS_MAX : v);
    push(b, WF_EV_FOCUS, side, 0, 0, 0, f->focus);
}

/* One attack from `side` against the other. `taken` (x100) scales damage on
 * the receiver (parry/guard). Returns damage dealt. */
static int attack(wf_battle *b, int side, bool skill, int quality, int taken) {
    wf_fighter *att = &b->f[side];
    wf_fighter *def = &b->f[!side];
    int power = 100, effect = 0;
    if (skill) {
        power = att->element ? 170 : 150;
        int m = wf_element_mult(att->element, def->element);
        power = power * m / 100;
        effect = m > 100 ? 1 : m < 100 ? 2 : 0;
    }
    if (att->rooted) {
        power = power * 60 / 100;
        att->rooted = 0;
    }
    int dmg = wf_damage(att, def, power, quality, wf_rng_range(&b->rng, 92, 108));
    if (def->guarding) dmg = dmg / 2;
    dmg = dmg * taken / 100;
    if (taken > 0 && dmg < 1) dmg = 1;
    hurt(def, dmg);
    push(b, WF_EV_HIT, side, quality, effect, skill, dmg);
    if (skill && dmg > 0) {
        switch (att->element) {
        case WF_EL_EMBER:
            def->burn = 2;
            push(b, WF_EV_IGNITE, side, 0, 0, 1, 0);
            break;
        case WF_EL_TIDE: {
            int h = dmg * 40 / 100;
            if (h < 1) h = 1;
            heal(att, h);
            push(b, WF_EV_HEAL, side, 0, 0, 1, h);
            break;
        }
        case WF_EL_GROVE:
            def->rooted = 1;
            push(b, WF_EV_ROOT, side, 0, 0, 1, 0);
            break;
        default:
            break;
        }
    }
    return dmg;
}

static bool check_ko(wf_battle *b) {
    if (b->f[1].hp <= 0) {
        push(b, WF_EV_KO, 1, 0, 0, 0, 0);
        b->state = WF_B_WON;
        return true;
    }
    if (b->f[0].hp <= 0) {
        push(b, WF_EV_KO, 0, 0, 0, 0, 0);
        b->state = WF_B_LOST;
        return true;
    }
    return false;
}

static void burn_ticks(wf_battle *b) {
    for (int side = 0; side < 2; side++) {
        wf_fighter *f = &b->f[side];
        if (!f->burn || f->hp <= 0) continue;
        int d = f->max_hp * 8 / 100;
        if (d < 1) d = 1;
        hurt(f, d);
        f->burn--;
        push(b, WF_EV_BURN, side, 0, 0, 0, d);
    }
}

/* Performs a non-item action for `side`. Returns true when it was an attack. */
static void perform(wf_battle *b, int side, int action, int quality, int taken) {
    wf_fighter *f = &b->f[side];
    if (action == WF_ACT_SKILL && f->focus < WF_SKILL_COST) {
        push(b, WF_EV_NO_FOCUS, side, 0, 0, 0, 0);
        action = WF_ACT_STRIKE;
    }
    if (action == WF_ACT_SKILL) {
        f->focus = (uint8_t)(f->focus - WF_SKILL_COST);
        attack(b, side, true, quality, taken);
    } else {
        attack(b, side, false, quality, taken);
        gain_focus(b, side, 1);
    }
}

static void choose_intent(wf_battle *b) {
    wf_fighter *foe = &b->f[1];
    int skill_chance = b->boss ? 65 : 50;
    b->foe_intent = (foe->focus >= WF_SKILL_COST && wf_rng_chance(&b->rng, skill_chance))
                  ? WF_ACT_SKILL : WF_ACT_STRIKE;
}

void wf_battle_player(wf_battle *b, int action, int quality) {
    b->ev_count = 0;
    if (b->state != WF_B_PLAYER) return;
    wf_fighter *me = &b->f[0];
    switch (action) {
    case WF_ACT_GUARD:
        me->guarding = 1;
        push(b, WF_EV_GUARD, 0, 0, 0, 0, 0);
        gain_focus(b, 0, 1);
        break;
    case WF_ACT_POTION: {
        int h = me->max_hp * 40 / 100;
        heal(me, h);
        push(b, WF_EV_HEAL, 0, 0, 0, 0, h);
        break;
    }
    case WF_ACT_TONIC:
        me->focus = WF_FOCUS_MAX;
        push(b, WF_EV_FOCUS, 0, 0, 0, 0, me->focus);
        break;
    case WF_ACT_FLEE: {
        int chance = 50 + (me->spd - b->f[1].spd) * 3;
        if (chance < 20) chance = 20;
        if (chance > 90) chance = 90;
        if (!b->boss && wf_rng_chance(&b->rng, chance)) {
            push(b, WF_EV_FLEE_OK, 0, 0, 0, 0, 0);
            b->state = WF_B_FLED;
            return;
        }
        push(b, WF_EV_FLEE_FAIL, 0, 0, 0, 0, 0);
        break;
    }
    default:
        perform(b, 0, action == WF_ACT_SKILL ? WF_ACT_SKILL : WF_ACT_STRIKE, quality, 100);
        break;
    }
    if (check_ko(b)) return;
    b->state = WF_B_FOE;
    choose_intent(b);
}

void wf_battle_foe(wf_battle *b, int parry) {
    b->ev_count = 0;
    if (b->state != WF_B_FOE) return;
    if (parry < 0) parry = 0;
    if (parry > 3) parry = 3;
    perform(b, 1, b->foe_intent, WF_Q_GOOD, parry_taken[parry]);
    if (parry == WF_Q_PERFECT && b->f[1].hp > 0) {
        /* A perfect parry ripostes with half a strike. */
        int dmg = wf_damage(&b->f[0], &b->f[1], 50, WF_Q_GOOD, wf_rng_range(&b->rng, 92, 108));
        hurt(&b->f[1], dmg);
        push(b, WF_EV_COUNTER, 0, WF_Q_PERFECT, 0, 0, dmg);
    }
    b->f[0].guarding = 0;
    if (check_ko(b)) return;
    burn_ticks(b);
    if (check_ko(b)) return;
    b->round++;
    b->state = WF_B_PLAYER;
}

void wf_duel_round(wf_battle *b, int act_a, int q_a, int act_b, int q_b) {
    b->ev_count = 0;
    if (b->state != WF_B_PLAYER) return;
    int acts[2] = {act_a, act_b}, qs[2] = {q_a, q_b};
    for (int s = 0; s < 2; s++) {
        if (acts[s] != WF_ACT_STRIKE && acts[s] != WF_ACT_SKILL) acts[s] = WF_ACT_GUARD;
        if (acts[s] == WF_ACT_GUARD) {
            b->f[s].guarding = 1;
            push(b, WF_EV_GUARD, s, 0, 0, 0, 0);
            gain_focus(b, s, 1);
        }
    }
    int first = b->f[1].spd > b->f[0].spd ? 1 : 0;
    for (int i = 0; i < 2; i++) {
        int s = i ? !first : first;
        if (acts[s] == WF_ACT_GUARD || b->f[s].hp <= 0) continue;
        perform(b, s, acts[s], qs[s], 100);
        if (check_ko(b)) return;
    }
    b->f[0].guarding = b->f[1].guarding = 0;
    burn_ticks(b);
    if (check_ko(b)) return;
    if (++b->round >= 30) {
        /* Time limit: higher remaining health share wins; ties go to B. */
        int a = b->f[0].hp * 1000 / b->f[0].max_hp, c = b->f[1].hp * 1000 / b->f[1].max_hp;
        b->state = a > c ? WF_B_WON : WF_B_LOST;
    }
}

uint32_t wf_battle_xp(const wf_battle *b) {
    uint32_t xp = (uint32_t)b->f[1].level * 6u + 4u;
    return b->boss ? xp * 3u : xp;
}

uint32_t wf_battle_coins(const wf_battle *b) {
    uint32_t c = (uint32_t)b->f[1].level * 2u + 3u + (uint32_t)(b->rng.s % (b->f[1].level + 1u));
    return b->boss ? c * 3u : c;
}
