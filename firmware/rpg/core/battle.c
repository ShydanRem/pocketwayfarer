/* Turn-based battle: one hero (+ summon) against up to three foes. */
#include <string.h>
#include "rpg.h"
#include "timing.h"

#ifndef FOE_HP0
/* Tuned with the balance simulation in rpg/host/test_rpg.c (run it with an argument to see the table). */
#define FOE_HP0 45
#define FOE_HPL 18
#define FOE_ATK0 10
#define FOE_ATKL 40
#endif
#ifndef FOE_HPQ
#define FOE_HPQ 6
#define FOE_ATKQ 50
#endif
#ifndef BOSS_HP
#define BOSS_HP 4
#define BOSS_ATK 115
#endif

static const int quality_mult[4] = {40, 100, 125, 150};
static const int parry_taken[4] = {100, 70, 45, 0};

void rpg_foe_make(rpg_foe *f, int sp, int level, bool elite) {
    const rpg_species_info *s = rpg_species(sp);
    memset(f, 0, sizeof *f);
    if (level < 1) level = 1;
    f->species = (uint8_t)sp;
    f->level = (uint8_t)level;
    f->elite = elite;
    f->boss = s->boss;
    f->element = s->element;
    /* A gentle quadratic term keeps up with stat points, gear tiers and classes. */
    f->max_hp = (FOE_HP0 + level * FOE_HPL + level * level / FOE_HPQ) * s->m_hp / 10;
    f->atk = (FOE_ATK0 + level * FOE_ATKL / 10 + level * level / FOE_ATKQ) * s->m_atk / 10;
    f->matk = f->atk;
    f->def = (3 + level * 16 / 10) * s->m_def / 10;
    f->mdef = (3 + level * 14 / 10) * s->m_def / 10;
    f->agi = (int16_t)((2 + level) * s->m_agi / 10);
    if (s->boss) {
        f->max_hp = f->max_hp * BOSS_HP;
        f->atk = f->atk * BOSS_ATK / 100;
        f->matk = f->atk;
        f->def = f->def * 12 / 10;
    } else if (elite) {
        f->max_hp = f->max_hp * 22 / 10;
        f->atk = f->atk * 13 / 10;
        f->matk = f->atk;
    }
    f->hp = f->max_hp;
}

static void push(rpg_battle *b, int kind, int actor, int target, int value) {
    if (b->ev_count >= RPG_EVENTS) return;
    rpg_event *e = &b->ev[b->ev_count++];
    memset(e, 0, sizeof *e);
    e->kind = (uint8_t)kind;
    e->actor = (int8_t)actor;
    e->target = (int8_t)target;
    e->value = value;
}
static rpg_event *last(rpg_battle *b) { return b->ev_count ? &b->ev[b->ev_count - 1] : NULL; }

void rpg_battle_start(rpg_battle *b, const rpg_game *g, const rpg_foe *foes, int n, uint32_t seed, int flags) {
    memset(b, 0, sizeof *b);
    rpg_stats_get(g, &b->h.st);
    b->h.hp = g->hero.hp > 0 ? g->hero.hp : 1;
    b->h.sp = g->hero.sp;
    if (b->h.hp > b->h.st.max_hp) b->h.hp = b->h.st.max_hp;
    b->h.nskills = rpg_class_skills(g->hero.cls, b->h.skills);
    b->nfoes = (uint8_t)(n > RPG_MAX_FOES ? RPG_MAX_FOES : n);
    for (int i = 0; i < b->nfoes; i++) {
        b->foe[i] = foes[i];
        /* Packs hit softer per member so that groups stay fair. */
        if (!foes[i].boss && !foes[i].elite && b->nfoes > 1) {
            int m = b->nfoes == 2 ? 80 : 65;
            b->foe[i].atk = b->foe[i].atk * m / 100;
            b->foe[i].matk = b->foe[i].matk * m / 100;
        }
    }
    wf_rng_seed(&b->rng, seed);
    b->flags = (uint8_t)flags;
    b->state = B_HERO;
    b->acting = 0xff;
}

bool rpg_battle_over(const rpg_battle *b) { return b->state >= B_WON; }

int rpg_battle_target(const rpg_battle *b, int preferred) {
    if (preferred >= 0 && preferred < b->nfoes && b->foe[preferred].hp > 0) return preferred;
    for (int i = 0; i < b->nfoes; i++) if (b->foe[i].hp > 0) return i;
    return -1;
}

static bool all_dead(const rpg_battle *b) { return rpg_battle_target(b, -1) < 0; }

/* ---- Hero damage ------------------------------------------------------------------- */
static int hero_offense(const rpg_battle *b, bool magic) {
    const rpg_hero_b *h = &b->h;
    int v = magic ? h->st.matk : h->st.atk;
    if (!magic && h->buff[BF_ATK]) v = v * (100 + h->buff_val[BF_ATK]) / 100;
    if (!magic && h->buff[BF_RAGE]) v = v * (100 + h->buff_val[BF_RAGE]) / 100;
    if (magic && h->buff[BF_MATK]) v = v * (100 + h->buff_val[BF_MATK]) / 100;
    if (h->status == STS_WEAK) v = v * 70 / 100;
    return v;
}

static void foe_status(rpg_battle *b, int t, int status, int chance, int turns) {
    rpg_foe *f = &b->foe[t];
    if (!status || f->hp <= 0) return;
    if (f->boss && (status == STS_STUN || status == STS_FREEZE)) chance /= 2;
    if (!wf_rng_chance(&b->rng, chance)) return;
    f->status = (uint8_t)status;
    f->status_turns = (uint8_t)(turns ? turns : 2);
    push(b, EV_STATUS, -1, t, 0);
    last(b)->effect = (uint8_t)status;
}

static int hit_foe(rpg_battle *b, int t, int power, bool magic, int element, int quality, int skill) {
    rpg_foe *f = &b->foe[t];
    if (f->hp <= 0) return 0;
    rpg_hero_b *h = &b->h;
    if (h->status == STS_BLIND && !magic && wf_rng_chance(&b->rng, 40)) {
        push(b, EV_MISS, -1, t, 0);
        return 0;
    }
    if (quality < 0) quality = 0;
    if (quality > 3) quality = 3;
    int base = hero_offense(b, magic) * power / 100;
    int d = base * 160 / (160 + (magic ? f->mdef : f->def));
    int em = rpg_element_mult(element, f->element);
    d = d * em / 100;
    d = d * (100 + h->st.elem_bonus[element]) / 100;
    d = d * quality_mult[quality] / 100;
    int crit_chance = h->st.crit + (h->buff[BF_CRIT] ? h->buff_val[BF_CRIT] : 0);
    bool crit = quality == WF_Q_PERFECT || (quality > 0 && wf_rng_chance(&b->rng, crit_chance));
    if (crit) d = d * 3 / 2;
    d = d * wf_rng_range(&b->rng, 92, 108) / 100;
    if (d < 1) d = 1;
    f->hp -= d;
    if (f->hp < 0) f->hp = 0;
    if (f->status == STS_FREEZE) f->status = 0;    /* ice shatters */
    push(b, crit ? EV_CRIT_HIT : EV_HIT, -1, t, d);
    rpg_event *e = last(b);
    e->quality = (uint8_t)quality;
    e->element = (uint8_t)element;
    e->effect = em > 100 ? 1 : em < 100 ? 2 : 0;
    e->skill = (uint8_t)skill;
    if (h->st.lifesteal && !magic) {
        int heal = d * h->st.lifesteal / 100;
        if (heal > 0) {
            h->hp += heal;
            if (h->hp > h->st.max_hp) h->hp = h->st.max_hp;
            push(b, EV_HEAL, -1, -1, heal);
        }
    }
    if (f->hp == 0) push(b, EV_KO, t, t, 0);
    return d;
}

static void heal_hero(rpg_battle *b, int amount) {
    rpg_hero_b *h = &b->h;
    if (amount < 1) amount = 1;
    h->hp += amount;
    if (h->hp > h->st.max_hp) h->hp = h->st.max_hp;
    push(b, EV_HEAL, -1, -1, amount);
}

bool rpg_skill_ready(const rpg_battle *b, int i) {
    if (i < 0 || i >= b->h.nskills) return false;
    return !b->h.cooldown[i];
}

static void after_hero(rpg_battle *b) {
    if (all_dead(b)) {
        b->state = B_WON;
        return;
    }
    if (!b->extra && wf_rng_chance(&b->rng, b->h.st.double_act)) {
        b->extra = 1;
        push(b, EV_EXTRA, -1, -1, 0);
        b->state = B_HERO;
        return;
    }
    b->state = B_FOES;
    b->acting = 0xff;
}

static void use_skill(rpg_battle *b, rpg_game *g, int index, int target, int quality) {
    rpg_hero_b *h = &b->h;
    int s = h->skills[index];
    int lv = g->hero.skill[s];
    const rpg_skill_info *k = rpg_skill(s);
    int power = rpg_skill_power(s, lv);
    int cost = rpg_skill_cost(s, lv);
    h->sp -= cost;
    h->cooldown[index] = (uint8_t)(k->cooldown + 1);
    rpg_quest_bump(g, Q_SKILL, 1);
    push(b, EV_SKILL, -1, target, s);
    last(b)->skill = (uint8_t)s;
    int turns = k->turns;
    switch (k->kind) {
    case SK_STRIKE:
        hit_foe(b, target, power, false, k->element, quality, s);
        foe_status(b, target, k->status, k->chance + lv * 3, turns);
        break;
    case SK_MULTI:
        for (int i = 0; i < k->hits; i++) {
            int t = rpg_battle_target(b, target);
            if (t < 0) break;
            hit_foe(b, t, power, false, k->element, quality, s);
            if (i == 0) foe_status(b, t, k->status, k->chance, turns);
        }
        break;
    case SK_CLEAVE:
    case SK_NOVA:
        for (int t = 0; t < b->nfoes; t++) {
            if (b->foe[t].hp <= 0) continue;
            hit_foe(b, t, power, k->kind == SK_NOVA, k->element, quality, s);
            foe_status(b, t, k->status, k->chance, turns);
        }
        break;
    case SK_BOLT:
        hit_foe(b, target, power, true, k->element, quality, s);
        foe_status(b, target, k->status, k->chance + lv * 3, turns);
        break;
    case SK_DRAIN: {
        int d = hit_foe(b, target, power, true, k->element, quality, s);
        if (d) heal_hero(b, d / 2);
        break;
    }
    case SK_HEAL:
        heal_hero(b, (h->st.matk + h->st.max_hp / 5) * power / 100 * (100 + h->st.heal_bonus) / 100);
        if (power >= 300 || s == 19) h->status = 0;            /* Miracle / Meditate cleanse */
        if (s == 19) { h->sp += h->st.max_sp / 10; if (h->sp > h->st.max_sp) h->sp = h->st.max_sp; }
        break;
    case SK_BUFF:
        h->buff[k->buff] = (uint8_t)(turns + 1);
        h->buff_val[k->buff] = (int16_t)power;
        if (k->buff == BF_SHIELD) h->shield = h->st.max_hp * power / 100;
        push(b, EV_BUFF, -1, -1, power);
        last(b)->effect = k->buff;
        break;
    case SK_DEBUFF: {
        bool magic = h->st.matk > h->st.atk;
        for (int t = 0; t < b->nfoes; t++) {
            if (k->hits != 9 && t != target) continue;
            if (b->foe[t].hp <= 0) continue;
            if (power) hit_foe(b, t, power, magic, k->element, quality, s);
            foe_status(b, t, k->status, k->chance, turns);
        }
        break;
    }
    case SK_SUMMON: {
        int base = h->st.atk > h->st.matk ? h->st.atk : h->st.matk;
        h->summon_atk = base * power / 100 * (100 + h->st.summon_bonus) / 100;
        h->summon_turns = (uint8_t)turns;
        push(b, EV_SUMMON, -1, -1, h->summon_atk);
        last(b)->skill = (uint8_t)s;
        break;
    }
    default:
        break;
    }
}

void rpg_battle_hero(rpg_battle *b, rpg_game *g, const rpg_action *a, int quality) {
    b->ev_count = 0;
    if (b->state != B_HERO) return;
    rpg_hero_b *h = &b->h;
    if (h->status == STS_STUN || h->status == STS_FREEZE) {
        push(b, EV_FROZEN, -1, -1, 0);
        last(b)->effect = h->status;
        if (!--h->status_turns) h->status = 0;
        b->state = B_FOES;
        b->acting = 0xff;
        return;
    }
    int target = rpg_battle_target(b, a->target);
    switch (a->kind) {
    case ACT_SKILL: {
        int idx = -1;
        for (int i = 0; i < h->nskills; i++) if (h->skills[i] == a->skill) idx = i;
        const rpg_skill_info *k = rpg_skill(a->skill);
        if (idx < 0 || !g->hero.skill[a->skill] || k->kind == SK_PASSIVE || h->cooldown[idx] ||
            h->sp < rpg_skill_cost(a->skill, g->hero.skill[a->skill])) {
            push(b, EV_NO_SP, -1, -1, 0);
            hit_foe(b, target, 100, false, h->st.weapon_element, quality, 0xff);
        } else {
            use_skill(b, g, idx, target, quality);
        }
        break;
    }
    case ACT_ITEM: {
        int c = a->item;
        if ((unsigned)c >= CO_COUNT || !g->cons[c]) {
            push(b, EV_NO_SP, -1, -1, 0);
            break;
        }
        g->cons[c]--;
        const rpg_consumable_info *ci = rpg_consumable(c);
        if (ci->hp_pct) heal_hero(b, h->st.max_hp * ci->hp_pct / 100);
        if (ci->sp_pct) {
            int v = h->st.max_sp * ci->sp_pct / 100;
            h->sp += v;
            if (h->sp > h->st.max_sp) h->sp = h->st.max_sp;
            push(b, EV_SP, -1, -1, v);
        }
        if (c == CO_ANTIDOTE) {
            h->status = 0;
            push(b, EV_HEAL, -1, -1, 0);
        }
        break;
    }
    case ACT_GUARD: {
        h->guarding = 1;
        int v = h->st.max_sp / 10 + 1;
        h->sp += v;
        if (h->sp > h->st.max_sp) h->sp = h->st.max_sp;
        push(b, EV_GUARD, -1, -1, v);
        break;
    }
    case ACT_FLEE: {
        int agi = h->st.stat[ST_AGI], foes = 0, n = 0;
        for (int i = 0; i < b->nfoes; i++) if (b->foe[i].hp > 0) { foes += b->foe[i].agi; n++; }
        int chance = 45 + (agi - (n ? foes / n : 0)) * 2;
        if (chance < 15) chance = 15;
        if (chance > 90) chance = 90;
        if (!(b->flags & (B_FLAG_BOSS | B_FLAG_NOFLEE)) && wf_rng_chance(&b->rng, chance)) {
            push(b, EV_FLEE_OK, -1, -1, 0);
            b->state = B_FLED;
            return;
        }
        push(b, EV_FLEE_FAIL, -1, -1, 0);
        break;
    }
    default:
        hit_foe(b, target, 100, false, h->st.weapon_element, quality, 0xff);
        break;
    }
    after_hero(b);
}

/* ---- Foes --------------------------------------------------------------------------- */
static void dot(rpg_battle *b, int who, uint8_t *status, uint8_t *turns, int32_t *hp, int32_t max_hp) {
    if (!*status || *hp <= 0) return;
    int pct = *status == STS_BURN ? 6 : *status == STS_POISON ? 5 : *status == STS_BLEED ? 4 : 0;
    if (pct) {
        int d = max_hp * pct / 100;
        if (d < 1) d = 1;
        *hp -= d;
        if (*hp < 0) *hp = 0;
        push(b, EV_DOT, who, who, d);
        last(b)->effect = *status;
        if (*hp == 0 && who >= 0) push(b, EV_KO, who, who, 0);
    }
    if (*status != STS_STUN && *status != STS_FREEZE && !--*turns) *status = 0;
}

static void end_round(rpg_battle *b) {
    rpg_hero_b *h = &b->h;
    /* Summon strikes first. */
    if (h->summon_turns) {
        int t = rpg_battle_target(b, -1);
        if (t >= 0) {
            rpg_foe *f = &b->foe[t];
            int d = h->summon_atk * 160 / (160 + f->def) * wf_rng_range(&b->rng, 92, 108) / 100;
            if (d < 1) d = 1;
            f->hp -= d;
            if (f->hp < 0) f->hp = 0;
            push(b, EV_SUMMON_HIT, 3, t, d);
            if (!f->hp) push(b, EV_KO, t, t, 0);
        }
        h->summon_turns--;
    }
    for (int i = 0; i < b->nfoes; i++) {
        rpg_foe *f = &b->foe[i];
        dot(b, i, &f->status, &f->status_turns, &f->hp, f->max_hp);
    }
    dot(b, -1, &h->status, &h->status_turns, &h->hp, h->st.max_hp);
    if (h->buff[BF_REGEN] && h->hp > 0) heal_hero(b, h->st.max_hp * h->buff_val[BF_REGEN] / 100);
    h->sp += h->st.sp_regen;
    if (h->sp > h->st.max_sp) h->sp = h->st.max_sp;
    for (int i = 0; i < BF_COUNT; i++) if (h->buff[i]) h->buff[i]--;
    if (!h->buff[BF_SHIELD]) h->shield = 0;
    for (int i = 0; i < h->nskills; i++) if (h->cooldown[i]) h->cooldown[i]--;
    h->guarding = 0;
    b->extra = 0;
    b->round++;
    if (h->hp <= 0) { push(b, EV_KO, -1, -1, 0); b->state = B_LOST; return; }
    if (all_dead(b)) { b->state = B_WON; return; }
    b->state = B_HERO;
}

bool rpg_battle_next_foe(rpg_battle *b) {
    if (b->state != B_FOES) return false;
    b->ev_count = 0;
    int start = b->acting == 0xff ? 0 : b->acting + 1;
    for (int i = start; i < b->nfoes; i++) {
        rpg_foe *f = &b->foe[i];
        if (f->hp <= 0) continue;
        if (f->status == STS_STUN || f->status == STS_FREEZE) {
            push(b, EV_FROZEN, i, i, 0);
            last(b)->effect = f->status;
            if (!--f->status_turns) f->status = 0;
            continue;
        }
        const rpg_species_info *s = rpg_species(f->species);
        f->charge++;
        int need = f->boss ? 2 : 3;
        b->foe_move = FOE_HIT;
        if (f->charge >= need && wf_rng_chance(&b->rng, 70)) {
            b->foe_move = s->special;
            f->charge = 0;
            if (b->foe_move == FOE_HEAL) {          /* heals only someone who needs it */
                bool hurt = false;
                for (int k = 0; k < b->nfoes; k++)
                    hurt |= b->foe[k].hp > 0 && b->foe[k].hp * 100 / b->foe[k].max_hp < 65;
                if (!hurt) b->foe_move = FOE_HIT;
            }
        }
        b->acting = (uint8_t)i;
        bool damaging = b->foe_move != FOE_HEAL;
        int dodge = b->h.st.dodge + (b->h.buff[BF_DODGE] ? b->h.buff_val[BF_DODGE] : 0);
        b->foe_dodge = damaging && wf_rng_chance(&b->rng, dodge);
        return true;
    }
    end_round(b);
    return false;
}

static void hurt_hero(rpg_battle *b, int d) {
    rpg_hero_b *h = &b->h;
    if (h->shield > 0) {
        int a = d < h->shield ? d : h->shield;
        h->shield -= a;
        d -= a;
    }
    h->hp -= d;
    if (h->hp < 0) h->hp = 0;
}

void rpg_battle_foe(rpg_battle *b, int parry) {
    b->ev_count = 0;
    if (b->state != B_FOES || b->acting >= b->nfoes) return;
    int i = b->acting;
    rpg_foe *f = &b->foe[i];
    rpg_hero_b *h = &b->h;
    const rpg_species_info *s = rpg_species(f->species);
    if (parry < 0) parry = 0;
    if (parry > 3) parry = 3;
    if (b->foe_move != FOE_HIT) {
        push(b, EV_FOE_SKILL, i, -1, b->foe_move);
    }
    if (b->foe_move == FOE_HEAL) {
        int t = i, worst = 1000;
        for (int k = 0; k < b->nfoes; k++) {
            if (b->foe[k].hp <= 0) continue;
            int pct = b->foe[k].hp * 100 / b->foe[k].max_hp;
            if (pct < worst) { worst = pct; t = k; }
        }
        int heal = b->foe[t].max_hp / (b->foe[t].boss ? 10 : 5);
        b->foe[t].hp += heal;
        if (b->foe[t].hp > b->foe[t].max_hp) b->foe[t].hp = b->foe[t].max_hp;
        push(b, EV_HEAL, i, t, heal);
        return;
    }
    if (b->foe_dodge) {
        push(b, EV_DODGE, i, -1, 0);
        return;
    }
    int hits = b->foe_move == FOE_MULTI ? 2 : 1;
    int power = b->foe_move == FOE_HEAVY ? 170 : b->foe_move == FOE_MULTI ? 70 : b->foe_move == FOE_BREATH ? 140
              : b->foe_move == FOE_CURSE ? 90 : 100;
    bool magic = s->magic && b->foe_move != FOE_HIT;
    for (int k = 0; k < hits && h->hp > 0; k++) {
        if (f->status == STS_BLIND && wf_rng_chance(&b->rng, 40)) {
            push(b, EV_MISS, i, -1, 0);
            continue;
        }
        int atk = magic ? f->matk : f->atk;
        if (f->status == STS_WEAK) atk = atk * 70 / 100;
        int d = atk * power / 100 * 160 / (160 + (magic ? h->st.mdef : h->st.def));
        if (magic) d = d * rpg_element_mult(f->element, EL_NEUTRAL) / 100;
        d = d * wf_rng_range(&b->rng, 90, 110) / 100;
        d = d * parry_taken[parry] / 100;
        if (h->guarding) d /= 2;
        if (h->buff[BF_DEF]) d = d * 100 / (100 + h->buff_val[BF_DEF]);
        if (h->buff[BF_RAGE]) d = d * 120 / 100;
        if (parry < 3 && d < 1) d = 1;
        hurt_hero(b, d);
        push(b, EV_HIT, i, -1, d);
        last(b)->quality = (uint8_t)parry;
    }
    if (parry == WF_Q_PERFECT) {
        int d = hero_offense(b, false) / 2 * 160 / (160 + f->def);
        if (d < 1) d = 1;
        f->hp -= d;
        if (f->hp < 0) f->hp = 0;
        push(b, EV_COUNTER, -1, i, d);
        if (!f->hp) push(b, EV_KO, i, i, 0);
    } else if (b->foe_move != FOE_HIT && s->status && h->hp > 0 && wf_rng_chance(&b->rng, s->chance)) {
        h->status = s->status;
        h->status_turns = (uint8_t)(s->status == STS_STUN || s->status == STS_FREEZE ? 1 : 3);
        push(b, EV_STATUS, i, -1, 0);
        last(b)->effect = s->status;
    }
    if (h->hp <= 0) {
        push(b, EV_KO, -1, -1, 0);
        b->state = B_LOST;
    } else if (all_dead(b)) {
        b->state = B_WON;
    }
}

/* ---- Automatic play --------------------------------------------------------------- */
void rpg_battle_auto(const rpg_battle *b, const rpg_game *g, rpg_action *a) {
    memset(a, 0, sizeof *a);
    const rpg_hero_b *h = &b->h;
    int alive = 0, weakest = -1;
    for (int i = 0; i < b->nfoes; i++) {
        if (b->foe[i].hp <= 0) continue;
        alive++;
        if (weakest < 0 || b->foe[i].hp < b->foe[weakest].hp) weakest = i;
    }
    a->target = (uint8_t)(weakest < 0 ? 0 : weakest);
    a->kind = ACT_ATTACK;
    int best = -1, best_score = 0;
    for (int i = 0; i < h->nskills; i++) {
        int s = h->skills[i];
        const rpg_skill_info *k = rpg_skill(s);
        int lv = g->hero.skill[s];
        if (!lv || k->kind == SK_PASSIVE || h->cooldown[i] || h->sp < rpg_skill_cost(s, lv)) continue;
        int score = 0;
        switch (k->kind) {
        case SK_HEAL: score = h->hp * 100 / h->st.max_hp < 45 ? 500 : 0; break;
        case SK_BUFF: score = h->buff[k->buff] ? 0 : alive >= 2 || b->flags & B_FLAG_BOSS ? 160 : 60; break;
        case SK_SUMMON: score = h->summon_turns ? 0 : 170; break;
        case SK_CLEAVE: case SK_NOVA: score = k->power * (alive >= 2 ? alive : 1) * 9 / 10; break;
        case SK_MULTI: score = k->power * k->hits; break;
        case SK_DEBUFF: score = (k->hits == 9 ? alive : 1) * 60 + k->power; break;
        default: score = k->power; break;
        }
        if (score > best_score) { best_score = score; best = s; }
    }
    if (h->hp * 100 / h->st.max_hp < 30 && (g->cons[CO_POTION] || g->cons[CO_HIPOTION])) {
        a->kind = ACT_ITEM;
        a->item = g->cons[CO_HIPOTION] && h->hp * 100 / h->st.max_hp < 15 ? CO_HIPOTION
                : g->cons[CO_POTION] ? CO_POTION : CO_HIPOTION;
        return;
    }
    if (best >= 0 && best_score > 110) {
        a->kind = ACT_SKILL;
        a->skill = (uint8_t)best;
    }
}

/* ---- Rewards ------------------------------------------------------------------------ */
void rpg_battle_finish(rpg_game *g, const rpg_battle *b, rpg_loot *loot) {
    memset(loot, 0, sizeof *loot);
    rpg_hero *hero = &g->hero;
    hero->hp = b->h.hp;
    hero->sp = b->h.sp;
    if (b->state == B_LOST) {
        rpg_stats st;
        rpg_stats_get(g, &st);
        hero->hp = st.max_hp / 10 > 0 ? st.max_hp / 10 : 1;
        hero->morale = (int16_t)(hero->morale > 10 ? hero->morale - 10 : 0);
        g->deaths++;
        return;
    }
    if (b->state != B_WON) return;
    if (hero->hp < 1) hero->hp = 1;
    wf_rng r = {g->rng_state ? g->rng_state : 77u};
    wf_rng_next(&r);
    int luck = b->h.st.stat[ST_LUK];
    int fam = 1 << rpg_class_family(hero->cls);
    int kills = 0;
    for (int i = 0; i < b->nfoes; i++) {
        const rpg_foe *f = &b->foe[i];
        uint32_t L = f->level;
        uint32_t xp = L * L / 2u + L * 6u + 8u;
        if (f->boss) xp *= 10u; else if (f->elite) xp = xp * 5u / 2u;
        if ((int)L < hero->level - 8) xp /= 2u;
        loot->xp += xp;
        uint32_t gold = (L * 2u + 5u) * (f->boss ? 8u : f->elite ? 2u : 1u);
        loot->gold += gold * (uint32_t)(100 + b->h.st.gold_bonus) / 100u;
        int item_chance = f->boss ? 1000 : f->elite ? 400 : 70 + luck;
        if (wf_rng_range(&r, 0, 999) < item_chance && loot->nitems < 3) {
            rpg_item it;
            rpg_item_roll(&it, &r, f->level, f->boss ? 60 : f->elite ? 30 : luck / 5, fam);
            if (rpg_bag_add(g, &it)) loot->items[loot->nitems++] = it;
        }
        if (wf_rng_chance(&r, 8)) loot->cons[CO_POTION]++;
        if (wf_rng_chance(&r, f->boss ? 100 : 12)) loot->cons[CO_ORE] += f->boss ? 3 : 1;
        int gem = f->boss ? 150 : f->elite ? 30 : 6 + luck / 5;
        if (!loot->gem && wf_rng_range(&r, 0, 999) < gem) loot->gem = (uint8_t)(f->species + 1);
        if (f->boss) { g->bosses++; rpg_quest_bump(g, Q_BOSS, 1); }
        kills++;
    }
    g->rng_state = r.s;
    loot->job_xp = loot->xp * 7u / 10u;
    g->gold += loot->gold;
    for (int c = 0; c < CO_COUNT; c++) {
        int v = g->cons[c] + loot->cons[c];
        g->cons[c] = (uint8_t)(v > 99 ? 99 : v);
    }
    if (loot->gem && g->gems[loot->gem - 1] < 99) g->gems[loot->gem - 1]++;
    g->kills += (uint32_t)kills;
    rpg_quest_bump(g, Q_KILL, kills);
    rpg_quest_bump(g, Q_GOLD, (int)(loot->gold / 50u));
    int m = hero->morale + 1 + (b->flags & B_FLAG_BOSS ? 4 : 0);
    hero->morale = (int16_t)(m > 100 ? 100 : m);
    int jl = 0;
    loot->levels = (uint8_t)rpg_gain_xp(g, loot->xp, loot->job_xp, &jl);
    loot->job_levels = (uint8_t)jl;
}
