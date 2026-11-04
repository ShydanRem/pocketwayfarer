#include <stdio.h>
#include <string.h>
#include "rpg.h"

uint8_t wf_lang;

static const uint8_t hp_lv[FAM_COUNT] = {14, 7, 9, 9, 10, 12, 12, 8};
static const uint8_t sp_lv[FAM_COUNT] = {2, 6, 3, 3, 5, 3, 3, 6};
static const uint8_t start_weapon[FAM_COUNT] = {IT_SWORD, IT_STAFF, IT_BOW, IT_DAGGER, IT_MACE, IT_KNUCKLE, IT_HAMMER, IT_SCYTHE};
static const uint8_t start_offhand[FAM_COUNT] = {IT_SHIELD, IT_TOME, IT_QUIVER, IT_CHARM, IT_SHIELD, IT_CHARM, IT_SHIELD, IT_TOME};
static const uint8_t start_body[FAM_COUNT] = {IT_PLATE, IT_ROBE, IT_LEATHER, IT_LEATHER, IT_PLATE, IT_LEATHER, IT_LEATHER, IT_ROBE};

uint32_t rpg_day_of(uint32_t now) { return now / 86400u; }

static void clamp_need(int32_t *v) { if (*v < 0) *v = 0; if (*v > RPG_NEED_MAX) *v = RPG_NEED_MAX; }
static void add16(int16_t *v, int d, int max) { int x = *v + d; *v = (int16_t)(x < 0 ? 0 : x > max ? max : x); }

static void basic_item(rpg_item *it, int type) {
    memset(it, 0, sizeof *it);
    it->type = (uint8_t)type;
    it->level = 1;
}

void rpg_game_new(rpg_game *g, const rpg_new_hero *nh, uint32_t now, uint32_t seed) {
    memset(g, 0, sizeof *g);
    rpg_hero *h = &g->hero;
    size_t n = 0;
    while (nh->name[n] && n < RPG_NAME_LEN) { h->name[n] = nh->name[n]; n++; }
    if (!n) strcpy(h->name, "Wayfarer");
    h->race = nh->race % RACE_COUNT;
    h->body = nh->body % 2;
    h->skin = nh->skin % 5;
    h->hair = nh->hair % 6;
    h->hair_color = nh->hair_color % 8;
    h->eyes = nh->eyes % 4;
    h->cls = (uint8_t)(nh->cls % FAM_COUNT);
    h->level = h->job_level = 1;
    for (int s = 0; s < ST_COUNT; s++) h->stat[s] = 5;
    const rpg_class_info *c = rpg_class(h->cls);
    h->stat[c->main_stat] += 3;
    h->stat[c->second_stat] += 2;
    h->stat_points = 3;
    h->skill_points = 1;
    h->skill[c->skills[0]] = 1;                  /* first active skill */
    h->hunger = 80000;
    h->stamina = 100;
    h->morale = 70;
    int fam = h->cls;
    basic_item(&h->equip[SLOT_WEAPON], start_weapon[fam]);
    basic_item(&h->equip[SLOT_OFFHAND], start_offhand[fam]);
    basic_item(&h->equip[SLOT_BODY], start_body[fam]);
    for (int s = SLOT_HEAD; s < SLOT_COUNT; s++) if (s != SLOT_BODY) h->equip[s].type = 0xff;
    for (int i = 0; i < RPG_BAG; i++) g->bag[i].type = 0xff;
    g->gold = 120;
    g->cons[CO_POTION] = 5;
    g->cons[CO_BREAD] = 3;
    g->cons[CO_ORE] = 2;
    g->cons[CO_RETURN] = 1;
    g->lang = nh->lang ? 1 : 0;
    wf_lang = g->lang;
    g->brightness = 80;
    g->last_update = now;
    g->created = now;
    wf_rng r;
    wf_rng_seed(&r, seed);
    g->rng_state = r.s;
    g->day = rpg_day_of(now);
    rpg_roll_day(g, g->day);
    rpg_stats st;
    rpg_stats_get(g, &st);
    h->hp = st.max_hp;
    h->sp = st.max_sp;
}

/* ---- Derived stats -------------------------------------------------------------- */
static int passive_value(const rpg_hero *h, int kind) {
    int skills[9], n = rpg_class_skills(h->cls, skills), v = 0;
    for (int i = 0; i < n; i++) {
        const rpg_skill_info *k = rpg_skill(skills[i]);
        int lv = h->skill[skills[i]];
        if (k->kind == SK_PASSIVE && k->buff == kind && lv) v += rpg_skill_power(skills[i], lv);
    }
    return v;
}

static void add_affix(rpg_stats *st, int *pct, int kind, int val) {
    switch (kind) {
    case AF_STR: case AF_AGI: case AF_VIT: case AF_INT: case AF_DEX: case AF_LUK:
        st->stat[kind - AF_STR] += val; break;
    case AF_ATK_PCT: pct[0] += val; break;
    case AF_MATK_PCT: pct[1] += val; break;
    case AF_DEF: st->def += val; break;
    case AF_HP_PCT: pct[2] += val; break;
    case AF_CRIT: st->crit += val; break;
    case AF_LIFESTEAL: st->lifesteal += val; break;
    case AF_SP_REGEN: st->sp_regen += val; break;
    case AF_FIRE: case AF_WATER: case AF_EARTH: case AF_WIND: case AF_HOLY: case AF_DARK:
        st->elem_bonus[EL_FIRE + kind - AF_FIRE] += val; break;
    case AF_GOLD: st->gold_bonus += val; break;
    default: break;
    }
}

void rpg_stats_get(const rpg_game *g, rpg_stats *st) {
    const rpg_hero *h = &g->hero;
    memset(st, 0, sizeof *st);
    const rpg_race_info *race = rpg_race(h->race);
    for (int s = 0; s < ST_COUNT; s++) st->stat[s] = h->stat[s] + race->stat[s];
    int pct[3] = {0, 0, 0};   /* atk, matk, hp */
    int watk = 0, wmatk = 0, def = 0, mdef = 0, hp = 0;
    for (int s = 0; s < SLOT_COUNT; s++) {
        const rpg_item *it = &h->equip[s];
        if (rpg_item_empty(it)) continue;
        watk += rpg_item_atk(it);
        wmatk += rpg_item_matk(it);
        def += rpg_item_def(it);
        mdef += rpg_item_mdef(it);
        hp += rpg_item_hp(it);
        for (int a = 0; a < 3; a++) if (it->aff[a]) add_affix(st, pct, it->aff[a], it->aff_val[a]);
        for (int k = 0; k < 2; k++)
            if (it->gems[k]) {
                const rpg_species_info *sp = rpg_species(it->gems[k] - 1);
                add_affix(st, pct, sp->gem_affix, sp->gem_value);
            }
    }
    pct[0] += passive_value(h, PS_ATK);
    pct[1] += passive_value(h, PS_MATK);
    pct[2] += passive_value(h, PS_HP);
    int fam = rpg_class_family(h->cls), tier = rpg_class_tier(h->cls);
    int tier_mult = tier == 1 ? 100 : tier == 2 ? 115 : 130;
    const rpg_item *w = &h->equip[SLOT_WEAPON];
    bool bow = !rpg_item_empty(w) && w->type == IT_BOW;
    int *S = st->stat;
    st->atk = watk * (100 + pct[0]) / 100 + (bow ? S[ST_DEX] * 2 + S[ST_STR] / 2 : S[ST_STR] * 2 + S[ST_DEX] / 2) + h->level;
    st->matk = wmatk * (100 + pct[1]) / 100 + S[ST_INT] * 2 + h->level;
    st->def = (st->def + def + S[ST_VIT] + h->level / 2) * (100 + passive_value(h, PS_DEF)) / 100;
    st->mdef = mdef + S[ST_INT] / 2 + S[ST_VIT] / 2;
    st->max_hp = (60 + h->level * hp_lv[fam]) * tier_mult / 100 * (100 + S[ST_VIT]) / 100;
    st->max_hp = (st->max_hp + hp) * (100 + pct[2]) / 100;
    st->max_sp = (15 + h->level * sp_lv[fam]) * tier_mult / 100 * (100 + S[ST_INT]) / 100;
    st->max_sp = st->max_sp * (100 + passive_value(h, PS_SP)) / 100;
    st->crit += S[ST_LUK] / 3 + S[ST_DEX] / 10 + passive_value(h, PS_CRIT);
    st->dodge = S[ST_AGI] / 4 + passive_value(h, PS_DODGE);
    if (st->dodge > 35) st->dodge = 35;
    st->double_act = S[ST_AGI] / 8 + passive_value(h, PS_DOUBLE);
    if (st->double_act > 40) st->double_act = 40;
    st->lifesteal += passive_value(h, PS_LIFESTEAL);
    st->sp_regen += 1 + passive_value(h, PS_SP_REGEN);
    st->heal_bonus = passive_value(h, PS_HEAL);
    st->summon_bonus = passive_value(h, PS_SUMMON);
    st->gold_bonus += passive_value(h, PS_GOLD) + S[ST_LUK] / 4;
    st->timing = S[ST_DEX] / 5;
    /* Low needs hurt; high morale helps. */
    if (h->hunger < 20000) { st->atk = st->atk * 80 / 100; st->matk = st->matk * 80 / 100; }
    if (h->morale >= 80) st->crit += 5;
}

int rpg_power(const rpg_game *g) {
    rpg_stats st;
    rpg_stats_get(g, &st);
    int off = st.atk > st.matk ? st.atk : st.matk;
    return off * 3 + st.def * 2 + st.max_hp / 4 + st.crit * 2;
}

/* ---- Experience -------------------------------------------------------------------- */
uint32_t rpg_xp_next(int level) { return 30u + (uint32_t)(level * level) * 8u + (uint32_t)level * 20u; }

uint32_t rpg_job_xp_next(int cls, int jl) {
    int tier = rpg_class_tier(cls);
    uint32_t base = 20u + (uint32_t)(jl * jl) * 6u + (uint32_t)jl * 15u;
    return tier == 1 ? base : tier == 2 ? base * 3u / 2u : base * 11u / 5u;
}

int rpg_gain_xp(rpg_game *g, uint32_t xp, uint32_t job_xp, int *job_levels) {
    rpg_hero *h = &g->hero;
    int bonus = rpg_race(h->race)->xp_bonus + (h->morale >= 80 ? 10 : h->morale < 25 ? -20 : 0);
    xp = xp * (uint32_t)(100 + bonus) / 100u;
    job_xp = job_xp * (uint32_t)(100 + bonus) / 100u;
    int levels = 0, jlv = 0;
    h->xp += xp;
    while (h->level < RPG_MAX_LEVEL && h->xp >= rpg_xp_next(h->level)) {
        h->xp -= rpg_xp_next(h->level);
        h->level++;
        h->stat_points += (uint16_t)(3 + h->level / 5);
        levels++;
    }
    if (h->level >= RPG_MAX_LEVEL) h->xp = 0;
    int jmax = rpg_job_max(h->cls);
    h->job_xp += job_xp;
    while (h->job_level < jmax && h->job_xp >= rpg_job_xp_next(h->cls, h->job_level)) {
        h->job_xp -= rpg_job_xp_next(h->cls, h->job_level);
        h->job_level++;
        h->skill_points++;
        jlv++;
    }
    if (h->job_level >= jmax) h->job_xp = 0;
    if (levels || jlv) {
        rpg_stats st;
        rpg_stats_get(g, &st);
        h->hp = st.max_hp;
        h->sp = st.max_sp;
    }
    if (job_levels) *job_levels = jlv;
    return levels;
}

int rpg_stat_cost(int value) { return 1 + (value - 1) / 10; }

bool rpg_raise_stat(rpg_game *g, int s) {
    rpg_hero *h = &g->hero;
    if ((unsigned)s >= ST_COUNT || h->stat[s] >= 99) return false;
    int cost = rpg_stat_cost(h->stat[s]);
    if (h->stat_points < cost) return false;
    h->stat_points = (uint16_t)(h->stat_points - cost);
    h->stat[s]++;
    return true;
}

int rpg_class_skills(int cls, int out[9]) {
    int chain[3], n = 0, c = cls;
    while (c >= 0 && n < 3) { chain[n++] = c; c = rpg_class_parent(c); }
    int k = 0;
    for (int i = n - 1; i >= 0; i--) {
        const rpg_class_info *ci = rpg_class(chain[i]);
        for (int j = 0; j < 3; j++) if (ci->skills[j] != 0xff) out[k++] = ci->skills[j];
    }
    return k;
}

bool rpg_learn_skill(rpg_game *g, int s) {
    rpg_hero *h = &g->hero;
    int list[9], n = rpg_class_skills(h->cls, list);
    bool mine = false;
    for (int i = 0; i < n; i++) mine |= list[i] == s;
    if (!mine || !h->skill_points || h->skill[s] >= RPG_SKILL_MAX_LV) return false;
    h->skill[s]++;
    h->skill_points--;
    rpg_quest_bump(g, Q_SKILL, 0);
    return true;
}

bool rpg_can_change_class(const rpg_game *g, int target) {
    const rpg_hero *h = &g->hero;
    int next[2], n = rpg_class_next(h->cls, next);
    bool ok = false;
    for (int i = 0; i < n; i++) ok |= next[i] == target;
    if (!ok) return false;
    int need_base = rpg_class_tier(target) == 2 ? 20 : 45;
    return h->job_level >= rpg_job_max(h->cls) && h->level >= need_base;
}

bool rpg_change_class(rpg_game *g, int target) {
    if (!rpg_can_change_class(g, target)) return false;
    rpg_hero *h = &g->hero;
    h->cls = (uint8_t)target;
    h->job_level = 1;
    h->job_xp = 0;
    h->skill_points += 1;
    const rpg_class_info *c = rpg_class(target);
    if (!h->skill[c->skills[0]]) h->skill[c->skills[0]] = 1;
    rpg_stats st;
    rpg_stats_get(g, &st);
    h->hp = st.max_hp;
    h->sp = st.max_sp;
    add16(&h->morale, 20, 100);
    return true;
}

/* ---- Real time ------------------------------------------------------------------------ */
bool rpg_tick(rpg_game *g, uint32_t now) {
    bool changed = false;
    uint32_t today = rpg_day_of(now);
    if (today != g->day && now > g->last_update) {
        rpg_roll_day(g, today);
        changed = true;
    }
    if (now <= g->last_update) return changed;
    uint32_t minutes = (now - g->last_update) / 60u;
    if (!minutes) return changed;
    g->last_update += minutes * 60u;
    if (minutes > 14u * 24u * 60u) minutes = 14u * 24u * 60u;
    rpg_hero *h = &g->hero;
    rpg_stats st;
    rpg_stats_get(g, &st);
    bool resting = !g->run.active && !g->hunt.active;
    for (uint32_t i = 0; i < minutes; i++) {
        h->hunger -= 100;
        clamp_need(&h->hunger);
        if (h->hunger == 0 && i % 30 == 0) add16(&h->morale, -1, 100);
        if (resting && h->hunger > 20000 && ++h->stamina_acc >= 6) {
            h->stamina_acc = 0;
            add16(&h->stamina, 1, 100);
        }
        if (resting) {
            int dh = st.max_hp * 15 / 1000, ds = st.max_sp * 15 / 1000;
            h->hp += dh > 0 ? dh : 1;
            h->sp += ds > 0 ? ds : 1;
            if (h->hp > st.max_hp) h->hp = st.max_hp;
            if (h->sp > st.max_sp) h->sp = st.max_sp;
        }
    }
    return true;
}

void rpg_roll_day(rpg_game *g, uint32_t day) {
    if (day != g->day) {
        if (day != g->day + 1 || !g->daily_claimed) g->streak = 0;
        g->daily_claimed = 0;
    }
    g->day = day;
    wf_rng r;
    wf_rng_seed(&r, day * 2654435761u ^ g->rng_state);
    static const uint8_t goals[Q_KIND_COUNT][2] = {
        [Q_KILL] = {10, 25}, [Q_ROOMS] = {4, 8}, [Q_FORGE] = {1, 2}, [Q_EAT] = {2, 3},
        [Q_GOLD] = {4, 10}, [Q_SKILL] = {5, 10}, [Q_BOSS] = {1, 1}, [Q_GREET] = {1, 2},
    };
    uint16_t used = 0;
    for (int i = 0; i < RPG_QUESTS; i++) {
        int k;
        do { k = wf_rng_range(&r, 0, Q_KIND_COUNT - 1); } while (used & (1u << k));
        used |= (uint16_t)(1u << k);
        rpg_quest *q = &g->quests[i];
        q->kind = (uint8_t)k;
        q->goal = (uint8_t)wf_rng_range(&r, goals[k][0], goals[k][1]);
        q->progress = q->claimed = 0;
        q->gold = (uint16_t)(30 + g->hero.level * 8 + q->goal * 3);
        static const uint8_t prizes[] = {CO_HIPOTION, CO_ETHER, CO_STEW, CO_PIE, CO_ORE, CO_RETURN};
        q->reward = prizes[wf_rng_range(&r, 0, (int)sizeof prizes - 1)];
    }
}

/* ---- Town --------------------------------------------------------------------------- */
int rpg_use_consumable(rpg_game *g, int c) {
    if ((unsigned)c >= CO_COUNT || !g->cons[c]) return RPG_ERR_NONE;
    const rpg_consumable_info *ci = rpg_consumable(c);
    rpg_hero *h = &g->hero;
    if (c == CO_ORE || c == CO_RETURN || c == CO_ANTIDOTE) return RPG_ERR_LOCKED;
    if (ci->hunger && h->hunger > 95000) return RPG_ERR_FULL;
    rpg_stats st;
    rpg_stats_get(g, &st);
    g->cons[c]--;
    h->hp += st.max_hp * ci->hp_pct / 100;
    h->sp += st.max_sp * ci->sp_pct / 100;
    if (h->hp > st.max_hp) h->hp = st.max_hp;
    if (h->sp > st.max_sp) h->sp = st.max_sp;
    h->hunger += ci->hunger;
    clamp_need(&h->hunger);
    add16(&h->morale, ci->morale, 100);
    add16(&h->stamina, ci->stamina, 100);
    if (ci->hunger) rpg_quest_bump(g, Q_EAT, 1);
    return RPG_OK;
}

uint32_t rpg_inn_price(const rpg_game *g) { return 10u + g->hero.level * 3u; }

int rpg_inn(rpg_game *g) {
    uint32_t p = rpg_inn_price(g);
    if (g->gold < p) return RPG_ERR_GOLD;
    g->gold -= p;
    rpg_stats st;
    rpg_stats_get(g, &st);
    g->hero.hp = st.max_hp;
    g->hero.sp = st.max_sp;
    add16(&g->hero.stamina, 50, 100);
    add16(&g->hero.morale, 5, 100);
    return RPG_OK;
}

int rpg_buy(rpg_game *g, int c, int count) {
    if ((unsigned)c >= CO_COUNT || count <= 0) return RPG_ERR_NONE;
    uint32_t cost = rpg_consumable(c)->price * (uint32_t)count;
    if (g->gold < cost) return RPG_ERR_GOLD;
    if (g->cons[c] + count > 99) return RPG_ERR_BAG;
    g->gold -= cost;
    g->cons[c] = (uint8_t)(g->cons[c] + count);
    return RPG_OK;
}

/* ---- Quests, daily, friends --------------------------------------------------------- */
void rpg_quest_bump(rpg_game *g, int kind, int amount) {
    if (kind == Q_SKILL && amount == 0) return;    /* learning is not "using" */
    for (int i = 0; i < RPG_QUESTS; i++) {
        rpg_quest *q = &g->quests[i];
        if (q->kind != kind || q->claimed) continue;
        int v = q->progress + amount;
        q->progress = (uint8_t)(v > q->goal ? q->goal : v);
    }
}

int rpg_quest_claim(rpg_game *g, int i) {
    if ((unsigned)i >= RPG_QUESTS) return 0;
    rpg_quest *q = &g->quests[i];
    if (q->claimed || q->progress < q->goal) return 0;
    q->claimed = 1;
    g->gold += q->gold;
    if (g->cons[q->reward] < 99) g->cons[q->reward]++;
    rpg_gain_xp(g, rpg_xp_next(g->hero.level) / 8u, rpg_job_xp_next(g->hero.cls, g->hero.job_level) / 8u, NULL);
    return q->gold;
}

const char *rpg_quest_text(const rpg_quest *q, char *buf, int cap) {
    static const wf_text t[Q_KIND_COUNT] = {
        {"Sconfiggi %d mostri", "Defeat %d monsters"}, {"Esplora %d stanze", "Explore %d rooms"},
        {"Potenzia %d volte alla forgia", "Upgrade %d times at the forge"}, {"Mangia %d volte", "Eat %d times"},
        {"Guadagna %d monete", "Earn %d gold"}, {"Usa %d abilità", "Use %d skills"},
        {"Sconfiggi %d boss", "Defeat %d boss"}, {"Saluta %d amici", "Greet %d friends"},
    };
    snprintf(buf, (size_t)cap, TX(t[q->kind % Q_KIND_COUNT]), q->kind == Q_GOLD ? q->goal * 50 : q->goal);
    return buf;
}

int rpg_daily_claim(rpg_game *g, int *item) {
    if (g->daily_claimed) return 0;
    g->daily_claimed = 1;
    if (g->streak < 7) g->streak++;
    int gold = 20 + g->streak * 10 + g->hero.level * 2;
    g->gold += (uint32_t)gold;
    int it = g->streak >= 7 ? CO_HIPOTION : g->streak >= 4 ? CO_PIE : CO_BREAD;
    if (g->cons[it] < 99) g->cons[it]++;
    if (g->streak >= 3 && g->cons[CO_ORE] < 99) g->cons[CO_ORE]++;
    add16(&g->hero.morale, 10, 100);
    if (item) *item = it;
    return gold;
}

bool rpg_greet(rpg_game *g, const uint8_t id[6], const char *name, int cls, int level, uint32_t now) {
    rpg_friend *f = NULL, *weak = &g->friends[0];
    for (int i = 0; i < RPG_FRIENDS; i++) {
        if (g->friends[i].used && !memcmp(g->friends[i].id, id, 6)) { f = &g->friends[i]; break; }
        if (!g->friends[i].used) weak = &g->friends[i];
    }
    if (!f) {
        f = weak;
        memset(f, 0, sizeof *f);
        f->used = 1;
        memcpy(f->id, id, 6);
    }
    snprintf(f->name, sizeof f->name, "%s", name ? name : "?");
    f->cls = (uint8_t)cls;
    f->level = (uint8_t)level;
    uint32_t today = rpg_day_of(now) + 1;
    if (f->last_greet_day == today) return false;
    f->last_greet_day = today;
    add16(&g->hero.morale, 10, 100);
    rpg_quest_bump(g, Q_GREET, 1);
    return true;
}
