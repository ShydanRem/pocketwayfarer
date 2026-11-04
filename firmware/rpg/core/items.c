#include <stdio.h>
#include <string.h>
#include "rpg.h"

static const uint8_t tier_level[RPG_TIERS] = {1, 12, 25, 40, 55};

bool rpg_item_empty(const rpg_item *it) { return it->type >= IT_TYPE_COUNT; }

/* Base stat scaled by tier, forge level and rarity. */
static int scaled(const rpg_item *it, int base) {
    if (!base) return 0;
    /* Grows smoothly with the item level; the tier only names the material. */
    int v = base * (100 + it->level * 8) / 100;
    v = v * (100 + it->plus * 9) / 100;
    v = v * (100 + it->rarity * 6) / 100;
    return v;
}
int rpg_item_atk(const rpg_item *it) { return rpg_item_empty(it) ? 0 : scaled(it, rpg_item_type_get(it->type)->atk); }
int rpg_item_matk(const rpg_item *it) { return rpg_item_empty(it) ? 0 : scaled(it, rpg_item_type_get(it->type)->matk); }
int rpg_item_def(const rpg_item *it) { return rpg_item_empty(it) ? 0 : scaled(it, rpg_item_type_get(it->type)->def); }
int rpg_item_mdef(const rpg_item *it) { return rpg_item_empty(it) ? 0 : scaled(it, rpg_item_type_get(it->type)->mdef); }
int rpg_item_hp(const rpg_item *it) { return rpg_item_empty(it) ? 0 : scaled(it, rpg_item_type_get(it->type)->hp); }

const char *rpg_item_name(const rpg_item *it, char *buf, int cap) {
    if (rpg_item_empty(it)) { snprintf(buf, (size_t)cap, "-"); return buf; }
    const rpg_item_type *t = rpg_item_type_get(it->type);
    wf_text m = rpg_material_name(t->material, it->tier);
    char plus[8] = "";
    if (it->plus) snprintf(plus, sizeof plus, " +%d", it->plus);
    if (wf_lang) snprintf(buf, (size_t)cap, "%s %s%s", m.en, t->name.en, plus);
    else snprintf(buf, (size_t)cap, "%s di %s%s", t->name.it, m.it, plus);
    return buf;
}

uint32_t rpg_item_value(const rpg_item *it) {
    if (rpg_item_empty(it)) return 0;
    uint32_t v = 8u + (uint32_t)it->level * 4u;
    v = v * (uint32_t)(1 + it->rarity * it->rarity) * (uint32_t)(10 + it->plus * 3) / 10u;
    return v;
}

uint32_t rpg_item_price(const rpg_item *it) { return rpg_item_value(it) * 4u; }

int rpg_rarity_roll(wf_rng *r, int bonus) {
    int roll = wf_rng_range(r, 0, 999) + bonus * 10;
    if (roll >= 995) return RAR_LEGENDARY;
    if (roll >= 960) return RAR_EPIC;
    if (roll >= 850) return RAR_RARE;
    if (roll >= 600) return RAR_UNCOMMON;
    return RAR_COMMON;
}

static int affix_value(wf_rng *r, int kind, int level) {
    switch (kind) {
    case AF_STR: case AF_AGI: case AF_VIT: case AF_INT: case AF_DEX: case AF_LUK:
        return wf_rng_range(r, 1, 2 + level / 8);
    case AF_ATK_PCT: case AF_MATK_PCT: case AF_HP_PCT: return wf_rng_range(r, 3, 6 + level / 7);
    case AF_DEF: return wf_rng_range(r, 2, 4 + level / 4);
    case AF_CRIT: return wf_rng_range(r, 1, 3 + level / 15);
    case AF_LIFESTEAL: return wf_rng_range(r, 1, 2 + level / 20);
    case AF_SP_REGEN: return wf_rng_range(r, 1, 1 + level / 20);
    case AF_GOLD: return wf_rng_range(r, 5, 10 + level / 4);
    default: return wf_rng_range(r, 5, 10 + level / 5);   /* elemental damage % */
    }
}

void rpg_item_roll(rpg_item *it, wf_rng *r, int level, int rarity_bonus, int families) {
    memset(it, 0, sizeof *it);
    /* Prefer gear the hero can use (2 out of 3), otherwise anything. */
    int type;
    for (int tries = 0;; tries++) {
        type = wf_rng_range(r, 0, IT_TYPE_COUNT - 1);
        const rpg_item_type *t = rpg_item_type_get(type);
        if (tries > 30 || t->families == 0xff || (t->families & families) || wf_rng_chance(r, 33)) break;
    }
    it->type = (uint8_t)type;
    int tier = 0;
    for (int t = RPG_TIERS - 1; t >= 0; t--) if (level >= tier_level[t]) { tier = t; break; }
    it->tier = (uint8_t)tier;
    it->level = (uint8_t)(level < 1 ? 1 : level > RPG_MAX_LEVEL ? RPG_MAX_LEVEL : level);
    if (it->level < tier_level[tier]) it->level = tier_level[tier];
    it->rarity = (uint8_t)rpg_rarity_roll(r, rarity_bonus);
    int naff = it->rarity == RAR_LEGENDARY ? 3 : it->rarity;
    for (int a = 0; a < naff; a++) {
        int kind;
        bool dup;
        do {
            kind = wf_rng_range(r, AF_STR, AF_COUNT - 1);
            dup = false;
            for (int b = 0; b < a; b++) dup |= it->aff[b] == kind;
        } while (dup);
        it->aff[a] = (uint8_t)kind;
        int v = affix_value(r, kind, level);
        if (it->rarity == RAR_LEGENDARY) v = v * 3 / 2;
        it->aff_val[a] = (uint8_t)(v > 255 ? 255 : v);
    }
    int sock = wf_rng_range(r, 0, 99);
    it->sockets = (uint8_t)(sock < 10 + it->rarity * 12 ? (sock < 3 + it->rarity * 4 ? 2 : 1) : 0);
}

bool rpg_can_equip(const rpg_game *g, const rpg_item *it) {
    if (rpg_item_empty(it)) return false;
    const rpg_item_type *t = rpg_item_type_get(it->type);
    if (it->level > g->hero.level) return false;
    if (t->families != 0xff && !(t->families & (1u << rpg_class_family(g->hero.cls)))) return false;
    return true;
}

int rpg_bag_count(const rpg_game *g) {
    int n = 0;
    for (int i = 0; i < RPG_BAG; i++) n += !rpg_item_empty(&g->bag[i]);
    return n;
}

bool rpg_bag_add(rpg_game *g, const rpg_item *it) {
    for (int i = 0; i < RPG_BAG; i++)
        if (rpg_item_empty(&g->bag[i])) { g->bag[i] = *it; return true; }
    return false;
}

void rpg_bag_remove(rpg_game *g, int i) {
    if ((unsigned)i >= RPG_BAG) return;
    memmove(&g->bag[i], &g->bag[i + 1], sizeof(rpg_item) * (size_t)(RPG_BAG - 1 - i));
    g->bag[RPG_BAG - 1].type = 0xff;
}

static void clamp_vitals(rpg_game *g) {
    rpg_stats st;
    rpg_stats_get(g, &st);
    if (g->hero.hp > st.max_hp) g->hero.hp = st.max_hp;
    if (g->hero.sp > st.max_sp) g->hero.sp = st.max_sp;
}

int rpg_equip(rpg_game *g, int i) {
    if ((unsigned)i >= RPG_BAG || rpg_item_empty(&g->bag[i])) return RPG_ERR_NONE;
    rpg_item it = g->bag[i];
    const rpg_item_type *t = rpg_item_type_get(it.type);
    if (it.level > g->hero.level) return RPG_ERR_LEVEL;
    if (!rpg_can_equip(g, &it)) return RPG_ERR_CLASS;
    int slot = t->slot;
    if (slot == SLOT_ACC1 && !rpg_item_empty(&g->hero.equip[SLOT_ACC1]) && rpg_item_empty(&g->hero.equip[SLOT_ACC2]))
        slot = SLOT_ACC2;
    rpg_item old = g->hero.equip[slot];
    g->hero.equip[slot] = it;
    if (rpg_item_empty(&old)) rpg_bag_remove(g, i);
    else g->bag[i] = old;
    clamp_vitals(g);
    return RPG_OK;
}

int rpg_unequip(rpg_game *g, int slot) {
    if ((unsigned)slot >= SLOT_COUNT || rpg_item_empty(&g->hero.equip[slot])) return RPG_ERR_NONE;
    if (!rpg_bag_add(g, &g->hero.equip[slot])) return RPG_ERR_BAG;
    g->hero.equip[slot].type = 0xff;
    clamp_vitals(g);
    return RPG_OK;
}

uint32_t rpg_sell(rpg_game *g, int i) {
    if ((unsigned)i >= RPG_BAG || rpg_item_empty(&g->bag[i])) return 0;
    uint32_t v = rpg_item_value(&g->bag[i]);
    g->gold += v;
    for (int k = 0; k < 2; k++)                       /* gems come back out */
        if (g->bag[i].gems[k] && g->gems[g->bag[i].gems[k] - 1] < 99) g->gems[g->bag[i].gems[k] - 1]++;
    rpg_bag_remove(g, i);
    return v;
}

/* ---- Forge --------------------------------------------------------------------------- */
int rpg_forge_chance(int plus) {
    static const uint8_t c[10] = {100, 100, 90, 80, 70, 60, 50, 40, 30, 20};
    return plus >= 0 && plus < 10 ? c[plus] : 0;
}
uint32_t rpg_forge_gold(const rpg_item *it) { return 20u + (uint32_t)it->level * 6u * (uint32_t)(it->plus + 1); }
int rpg_forge_ore(const rpg_item *it) { return 1 + it->plus / 3; }

int rpg_forge(rpg_game *g, rpg_item *it, wf_rng *r) {
    if (rpg_item_empty(it) || it->plus >= 10) return -2;
    uint32_t gold = rpg_forge_gold(it);
    int ore = rpg_forge_ore(it);
    if (g->gold < gold) return -3;
    if (g->cons[CO_ORE] < ore) return -4;
    g->gold -= gold;
    g->cons[CO_ORE] = (uint8_t)(g->cons[CO_ORE] - ore);
    rpg_quest_bump(g, Q_FORGE, 1);
    if (wf_rng_range(r, 1, 100) <= rpg_forge_chance(it->plus)) {
        it->plus++;
        clamp_vitals(g);
        return 1;
    }
    if (it->plus >= 6) {
        it->plus--;
        clamp_vitals(g);
        return -1;
    }
    return 0;
}

int rpg_socket(rpg_game *g, rpg_item *it, int sp) {
    if (rpg_item_empty(it) || (unsigned)sp >= SP_COUNT || !g->gems[sp]) return RPG_ERR_NONE;
    for (int k = 0; k < it->sockets && k < 2; k++) {
        if (!it->gems[k]) {
            it->gems[k] = (uint8_t)(sp + 1);
            g->gems[sp]--;
            return RPG_OK;
        }
    }
    return RPG_ERR_FULL;
}

void rpg_shop_stock(const rpg_game *g, rpg_item out[4]) {
    /* Same stock all day: seeded by the day and the hero level band. */
    wf_rng r;
    wf_rng_seed(&r, g->day * 7919u + g->hero.level / 5u + g->rng_state % 97u);
    int fam = 1 << rpg_class_family(g->hero.cls);
    for (int i = 0; i < 4; i++) {
        int lv = g->hero.level + wf_rng_range(&r, -2, 3);
        if (lv < 1) lv = 1;
        rpg_item_roll(&out[i], &r, lv, 0, fam);
        if (out[i].rarity > RAR_RARE) out[i].rarity = RAR_RARE;
        if (out[i].rarity < RAR_UNCOMMON && i < 2) {
            out[i].rarity = RAR_UNCOMMON;
            if (!out[i].aff[0]) { out[i].aff[0] = AF_VIT; out[i].aff_val[0] = 2; }
        }
        if (out[i].level > g->hero.level) out[i].level = g->hero.level;
    }
}
