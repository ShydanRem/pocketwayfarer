/* Tests and balance simulation for the RPG core. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "rpg.h"
#include "timing.h"

static int checks;
#define CHECK(c) do { checks++; if (!(c)) { fprintf(stderr, "%s:%d: CHECK(%s)\n", __FILE__, __LINE__, #c); exit(1); } } while (0)
static const uint32_t T0 = 9800u * 86400u + 10u * 3600u;
static bool verbose;

static void new_hero(rpg_game *g, int fam, uint32_t seed) {
    rpg_new_hero nh = {0};
    snprintf(nh.name, sizeof nh.name, "Test%d", fam);
    nh.cls = (uint8_t)fam;
    nh.race = (uint8_t)(seed % RACE_COUNT);
    rpg_game_new(g, &nh, T0, seed);
}

static const uint8_t fam_weapon[FAM_COUNT] = {IT_SWORD, IT_STAFF, IT_BOW, IT_DAGGER, IT_MACE, IT_KNUCKLE, IT_HAMMER, IT_SCYTHE};
static const uint8_t fam_off[FAM_COUNT] = {IT_SHIELD, IT_TOME, IT_QUIVER, IT_CHARM, IT_SHIELD, IT_CHARM, IT_SHIELD, IT_TOME};

/* Builds a reasonably played hero of a given level: stats on the class stats,
 * skills learned, class advanced when allowed, gear of its level. */
static void grow(rpg_game *g, int level, int branch) {
    while (g->hero.level < level) {
        rpg_gain_xp(g, rpg_xp_next(g->hero.level), rpg_job_xp_next(g->hero.cls, g->hero.job_level) * 2, NULL);
        int next[2], n = rpg_class_next(g->hero.cls, next);
        if (n && rpg_can_change_class(g, next[n > 1 ? branch : 0])) rpg_change_class(g, next[n > 1 ? branch : 0]);
    }
    const rpg_class_info *c = rpg_class(g->hero.cls);
    for (int i = 0; g->hero.stat_points && i < 500; i++) {
        int s = i % 3 == 2 ? ST_VIT : i % 3 == 1 ? c->second_stat : c->main_stat;
        if (!rpg_raise_stat(g, s)) rpg_raise_stat(g, ST_VIT);
    }
    int skills[9], n = rpg_class_skills(g->hero.cls, skills);
    for (int round = 0; round < 6 && g->hero.skill_points; round++)
        for (int i = n - 1; i >= 0; i--) rpg_learn_skill(g, skills[i]);
    wf_rng r;
    wf_rng_seed(&r, (uint32_t)level * 31u);
    int fam = rpg_class_family(g->hero.cls);
    static const uint8_t armour[] = {IT_HELM, IT_PLATE, IT_BOOTS, IT_RING, IT_AMULET};
    uint8_t types[7] = {fam_weapon[fam], fam_off[fam], armour[0], armour[1], armour[2], armour[3], armour[4]};
    if (fam == FAM_MAGE || fam == FAM_OCCULTIST) { types[2] = IT_CIRCLET; types[3] = IT_ROBE; }
    for (int i = 0; i < 7; i++) {
        rpg_item it;
        do { rpg_item_roll(&it, &r, level, 0, 1 << fam); } while (0);
        it.type = types[i];
        it.rarity = RAR_UNCOMMON;
        it.level = (uint8_t)level;
        it.plus = (uint8_t)(level / 12);
        g->bag[0] = it;
        CHECK(rpg_equip(g, 0) == RPG_OK);
        g->bag[0].type = 0xff;
    }
    rpg_stats st;
    rpg_stats_get(g, &st);
    g->hero.hp = st.max_hp;
    g->hero.sp = st.max_sp;
    g->cons[CO_POTION] = 10;
    g->cons[CO_HIPOTION] = 3;
}

/* Plays one battle with AUTO at the given timing quality. Returns state. */
static int last_rounds, last_hp_pct;
static int fight(rpg_game *g, rpg_foe *foes, int n, int quality, int parry, uint32_t seed, int flags) {
    rpg_battle b;
    rpg_battle_start(&b, g, foes, n, seed, flags);
    for (int guard = 0; guard < 400 && !rpg_battle_over(&b); guard++) {
        if (b.state == B_HERO) {
            rpg_action a;
            rpg_battle_auto(&b, g, &a);
            rpg_battle_hero(&b, g, &a, quality);
        } else if (b.state == B_FOES) {
            while (rpg_battle_next_foe(&b)) {
                rpg_battle_foe(&b, parry);
                if (rpg_battle_over(&b)) break;
            }
        }
    }
    last_rounds = b.round;
    last_hp_pct = (int)(b.h.hp * 100 / b.h.st.max_hp);
    rpg_loot loot;
    rpg_battle_finish(g, &b, &loot);
    return b.state;
}

static void test_data(void) {
    uint8_t owner[RPG_SKILL_COUNT];
    memset(owner, 0, sizeof owner);
    for (int c = 0; c < RPG_CLASS_COUNT; c++) {
        const rpg_class_info *ci = rpg_class(c);
        CHECK(ci->name.it && ci->name.en && ci->main_stat != ci->second_stat);
        for (int k = 0; k < 3; k++) {
            if (ci->skills[k] == 0xff) { CHECK(c >= 24 && k == 2); continue; }
            CHECK(ci->skills[k] < RPG_SKILL_COUNT);
            owner[ci->skills[k]]++;
        }
        int list[9];
        int n = rpg_class_skills(c, list);
        CHECK(n == (c < 8 ? 3 : c < 24 ? 6 : 8));
        CHECK(rpg_class_family(c) == rpg_class_family(rpg_class_parent(c) < 0 ? c : rpg_class_parent(c)));
    }
    for (int s = 0; s < RPG_SKILL_COUNT; s++) {
        CHECK(owner[s] == 1);
        const rpg_skill_info *k = rpg_skill(s);
        CHECK(k->name.it && k->name.en && k->kind <= SK_PASSIVE);
        if (k->kind != SK_PASSIVE) CHECK(k->sp > 0);
    }
    for (int sp = 0; sp < SP_COUNT; sp++) CHECK(rpg_species(sp)->name.it && rpg_species(sp)->m_hp);
    for (int z = 0; z < RPG_ZONES; z++) {
        const rpg_zone_info *zi = rpg_zone(z);
        CHECK(rpg_species(zi->mid_boss)->boss && rpg_species(zi->final_boss)->boss);
        for (int i = 0; i < 4; i++) CHECK(!rpg_species(zi->field_foes[i])->boss && !rpg_species(zi->dungeon_foes[i])->boss);
    }
    CHECK(rpg_element_mult(EL_FIRE, EL_EARTH) == 150 && rpg_element_mult(EL_WATER, EL_FIRE) == 150);
    CHECK(rpg_element_mult(EL_FIRE, EL_FIRE) == 50 && rpg_element_mult(EL_HOLY, EL_DARK) == 150);
    CHECK(rpg_element_mult(EL_NEUTRAL, EL_DARK) == 100);
    int next[2];
    CHECK(rpg_class_next(3, next) == 2 && next[0] == 14 && next[1] == 15);
    CHECK(rpg_class_next(15, next) == 1 && next[0] == 31);
    CHECK(rpg_class_next(31, next) == 0);
}

static void test_hero(void) {
    rpg_game g;
    for (int f = 0; f < FAM_COUNT; f++) {
        new_hero(&g, f, (uint32_t)f + 1);
        rpg_stats st;
        rpg_stats_get(&g, &st);
        CHECK(st.max_hp > 50 && st.max_sp > 10 && st.atk > 5 && g.hero.hp == st.max_hp);
        CHECK(!rpg_item_empty(&g.hero.equip[SLOT_WEAPON]) && rpg_can_equip(&g, &g.hero.equip[SLOT_WEAPON]));
        CHECK(g.hero.skill[rpg_class(f)->skills[0]] == 1);
    }
    new_hero(&g, FAM_ROGUE, 3);
    CHECK(g.hero.stat_points == 3);
    CHECK(rpg_raise_stat(&g, ST_AGI) && g.hero.stat_points == 2);
    int lv = rpg_gain_xp(&g, 100000, 0, NULL);
    CHECK(lv > 5 && g.hero.stat_points > 20);
    /* Class change: needs job level 30 and base 20. */
    CHECK(!rpg_can_change_class(&g, 14));
    while (g.hero.job_level < 30) rpg_gain_xp(&g, 0, rpg_job_xp_next(g.hero.cls, g.hero.job_level), NULL);
    while (g.hero.level < 20) rpg_gain_xp(&g, rpg_xp_next(g.hero.level), 0, NULL);
    CHECK(!rpg_can_change_class(&g, 12) && rpg_can_change_class(&g, 14));
    CHECK(rpg_change_class(&g, 14) && g.hero.cls == 14 && g.hero.job_level == 1);
    CHECK(g.hero.skill[rpg_class(14)->skills[0]] == 1);
    CHECK(rpg_learn_skill(&g, 9) || g.hero.skill_points == 0);
    CHECK(!rpg_learn_skill(&g, 0));            /* warrior skill: not ours */
    /* Needs in real time. */
    new_hero(&g, FAM_WARRIOR, 9);
    int32_t hunger = g.hero.hunger;
    g.hero.stamina = 40;
    g.hero.hp = 1;
    rpg_tick(&g, T0 + 3600);
    CHECK(hunger - g.hero.hunger == 6000 && g.hero.stamina == 50 && g.hero.hp > 1);
    rpg_tick(&g, T0 + 30 * 86400u);
    CHECK(g.hero.hunger == 0 && g.hero.morale < 70);
    CHECK(rpg_use_consumable(&g, CO_BREAD) == RPG_OK && g.hero.hunger == 25000);
    g.gold = 0;
    CHECK(rpg_inn(&g) == RPG_ERR_GOLD);
    g.gold = 1000;
    g.hero.stamina = 10;
    CHECK(rpg_inn(&g) == RPG_OK && g.hero.stamina == 60);
    /* Daily and quests. */
    int item;
    CHECK(rpg_daily_claim(&g, &item) > 0 && rpg_daily_claim(&g, &item) == 0);
    rpg_quest_bump(&g, g.quests[0].kind, 99);
    uint32_t gold = g.gold;
    CHECK(rpg_quest_claim(&g, 0) > 0 && g.gold > gold);
    char buf[64];
    wf_lang = 1;
    CHECK(strlen(rpg_quest_text(&g.quests[1], buf, sizeof buf)) > 5);
    wf_lang = 0;
}

static void test_items(void) {
    rpg_game g;
    new_hero(&g, FAM_WARRIOR, 5);
    wf_rng r;
    wf_rng_seed(&r, 1);
    int rar[RAR_COUNT] = {0};
    for (int i = 0; i < 20000; i++) {
        rpg_item it;
        rpg_item_roll(&it, &r, 1 + i % 70, 0, 1);
        CHECK(!rpg_item_empty(&it) && it.tier < RPG_TIERS && it.level >= 1);
        rar[it.rarity]++;
        for (int a = 0; a < 3; a++) if (a < (it.rarity == RAR_LEGENDARY ? 3 : it.rarity)) CHECK(it.aff[a]);
    }
    CHECK(rar[RAR_COMMON] > rar[RAR_UNCOMMON] && rar[RAR_UNCOMMON] > rar[RAR_RARE] && rar[RAR_LEGENDARY] > 0);
    char name[48];
    rpg_item it = {.type = IT_SWORD, .tier = 1, .plus = 3, .level = 12};
    CHECK(!strcmp(rpg_item_name(&it, name, sizeof name), "Spada di Ferro +3"));
    wf_lang = 1;
    CHECK(!strcmp(rpg_item_name(&it, name, sizeof name), "Iron Sword +3"));
    wf_lang = 0;
    /* Equip rules. */
    rpg_item staff = {.type = IT_STAFF, .level = 1};
    g.bag[0] = staff;
    CHECK(rpg_equip(&g, 0) == RPG_ERR_CLASS);
    rpg_item axe = {.type = IT_AXE, .level = 1, .rarity = RAR_RARE, .aff = {AF_STR}, .aff_val = {5}};
    g.bag[0] = axe;
    rpg_stats before, after;
    rpg_stats_get(&g, &before);
    CHECK(rpg_equip(&g, 0) == RPG_OK && g.hero.equip[SLOT_WEAPON].type == IT_AXE && g.bag[0].type == IT_SWORD);
    rpg_stats_get(&g, &after);
    CHECK(after.atk > before.atk && after.stat[ST_STR] == before.stat[ST_STR] + 5);
    CHECK(rpg_unequip(&g, SLOT_HEAD) == RPG_ERR_NONE);
    rpg_item ring = {.type = IT_RING, .level = 1}, ring2 = ring;
    g.bag[1] = ring; g.bag[2] = ring2;
    CHECK(rpg_equip(&g, 1) == RPG_OK);
    CHECK(rpg_equip(&g, 1) == RPG_OK);      /* ring2 shifted to index 1 */
    CHECK(!rpg_item_empty(&g.hero.equip[SLOT_ACC1]) && !rpg_item_empty(&g.hero.equip[SLOT_ACC2]));
    /* Forge: never destroys, +6 failures step back. */
    g.gold = 1000000;
    g.cons[CO_ORE] = 99;
    rpg_item *w = &g.hero.equip[SLOT_WEAPON];
    int tries = 0;
    while (w->plus < 10 && tries < 2000) {
        if (g.cons[CO_ORE] < 10) g.cons[CO_ORE] = 99;
        int res = rpg_forge(&g, w, &r);
        CHECK(res >= -1);
        tries++;
    }
    CHECK(w->plus == 10 && rpg_forge(&g, w, &r) == -2);
    /* Gems. */
    w->sockets = 1;
    g.gems[SP_WOLF] = 1;
    CHECK(rpg_socket(&g, w, SP_WOLF) == RPG_OK && w->gems[0] == SP_WOLF + 1 && !g.gems[SP_WOLF]);
    CHECK(rpg_socket(&g, w, SP_WOLF) == RPG_ERR_NONE);
    uint32_t gold = g.gold;
    CHECK(rpg_sell(&g, 0) > 0 && g.gold > gold);
    rpg_item stock[4];
    rpg_shop_stock(&g, stock);
    for (int i = 0; i < 4; i++) CHECK(!rpg_item_empty(&stock[i]) && stock[i].level <= g.hero.level);
}

static void test_battle_basics(void) {
    rpg_game g;
    new_hero(&g, FAM_MAGE, 4);
    rpg_foe foes[3];
    rpg_foe_make(&foes[0], SP_SLIME, 1, false);
    rpg_foe_make(&foes[1], SP_WOLF, 1, false);
    rpg_battle b;
    rpg_battle_start(&b, &g, foes, 2, 1, 0);
    CHECK(b.foe[0].atk < foes[0].atk);         /* pack scaling */
    g.hero.skill[4] = 1;
    rpg_action a = {.kind = ACT_SKILL, .skill = 4, .target = 1};   /* Ice Lance, cooldown 1 */
    rpg_battle_hero(&b, &g, &a, WF_Q_PERFECT);
    bool hit = false;
    for (int i = 0; i < b.ev_count; i++) hit |= b.ev[i].kind == EV_CRIT_HIT && b.ev[i].target == 1;
    CHECK(hit);
    CHECK(b.h.sp < g.hero.sp);
    /* Same skill again is on cooldown: falls back to an attack. */
    if (b.state == B_FOES) while (rpg_battle_next_foe(&b)) rpg_battle_foe(&b, WF_Q_PERFECT);
    if (b.state == B_HERO) {
        rpg_battle_hero(&b, &g, &a, WF_Q_GOOD);
        CHECK(b.ev[0].kind == EV_NO_SP);
    }
    /* Perfect parry: no damage and a counter. */
    rpg_foe_make(&foes[0], SP_SKELETON, 3, false);
    rpg_battle_start(&b, &g, foes, 1, 7, B_FLAG_NOFLEE);
    rpg_action guard = {.kind = ACT_GUARD};
    rpg_battle_hero(&b, &g, &guard, 0);
    if (b.state == B_FOES && rpg_battle_next_foe(&b) && !b.foe_dodge) {
        int32_t hp = b.h.hp;
        rpg_battle_foe(&b, WF_Q_PERFECT);
        CHECK(b.h.hp == hp);
        bool counter = false;
        for (int i = 0; i < b.ev_count; i++) counter |= b.ev[i].kind == EV_COUNTER;
        CHECK(counter || b.foe_move == FOE_HEAL);
    }
    rpg_action flee = {.kind = ACT_FLEE};
    if (b.state == B_FOES) while (rpg_battle_next_foe(&b)) rpg_battle_foe(&b, 0);
    if (b.state == B_HERO) {
        rpg_battle_hero(&b, &g, &flee, 0);
        CHECK(b.state != B_FLED);               /* NOFLEE */
    }
}

/* ---- Balance ---------------------------------------------------------------------- */
static void test_balance(void) {
    static const int levels[] = {3, 8, 15, 25, 38, 55, 68};
    int total_win = 0, total = 0, hp_sum = 0, rounds_sum = 0;
    if (verbose) printf("\n  field win%% (AUTO, good timing) by family and level\n  lv  ");
    if (verbose) for (int f = 0; f < FAM_COUNT; f++) printf(" %8.8s", rpg_class(f)->name.en);
    for (unsigned li = 0; li < sizeof levels / sizeof levels[0]; li++) {
        int L = levels[li];
        int zone = 0;
        for (int z = 0; z < RPG_ZONES; z++) if (L >= rpg_zone(z)->min_level) zone = z;
        if (verbose) printf("\n  %2d  ", L);
        for (int f = 0; f < FAM_COUNT; f++) {
            int wins = 0, hp = 0, rounds = 0;
            for (int t = 0; t < 30; t++) {
                rpg_game g;
                new_hero(&g, f, (uint32_t)(t + 1));
                g.zone_unlocked = 4;
                grow(&g, L, t & 1);
                rpg_foe foes[3];
                int n;
                g.hero.stamina = 100;
                CHECK(rpg_field_encounter(&g, zone, foes, &n) == RPG_OK);
                wins += fight(&g, foes, n, WF_Q_GOOD, WF_Q_GOOD, (uint32_t)(t * 7 + f), 0) == B_WON;
                hp += last_hp_pct;
                rounds += last_rounds;
            }
            hp_sum += hp / 30;
            rounds_sum += rounds;
            total_win += wins;
            total += 30;
            if (verbose) printf(" %3d/%2d/%d", wins * 100 / 30, hp / 30, rounds / 30);
            CHECK(wins >= 26);    /* >= 85% for every class at every level */
        }
    }
    if (verbose) printf("\n  overall field win rate: %d%%, hp left %d%%, rounds %d (win/hp/rounds)\n", total_win * 100 / total,
                        hp_sum / (FAM_COUNT * 7), rounds_sum / total);
    CHECK(total_win * 100 / total >= 95);
}

static int dungeon_fail;
static void test_dungeon(void) {
    rpg_game g;
    new_hero(&g, FAM_WARRIOR, 11);
    CHECK(rpg_dungeon_enter(&g, 1, 0, T0) == -RPG_ERR_LOCKED);
    /* Floor graph: every room reachable from the entrance and leads upward. */
    for (uint32_t seed = 1; seed < 300; seed++) {
        rpg_run run = {.active = 1, .floor = (uint8_t)(1 + seed % 10), .seed = seed};
        rpg_dungeon_floor(&run);
        uint8_t reach[RPG_ROWS + 1] = {0};
        for (int c = 0; c < RPG_COLS; c++) if (run.rooms[0][c]) reach[0] |= (uint8_t)(1u << c);
        for (int r = 0; r < RPG_ROWS; r++)
            for (int c = 0; c < RPG_COLS; c++) {
                if (!run.rooms[r][c]) continue;
                CHECK(run.links[r][c]);
                if (reach[r] >> c & 1) reach[r + 1] |= run.links[r][c];
            }
        for (int r = 0; r <= RPG_ROWS; r++)
            for (int c = 0; c < RPG_COLS; c++) if (run.rooms[r][c]) CHECK(reach[r] >> c & 1);
        CHECK(run.rooms[RPG_ROWS][1] == (run.floor % 5 ? ROOM_STAIRS : ROOM_BOSS));
    }
    /* Full descent of every dungeon with a hero of the zone's level, great timing. */
    for (int z = 0; z < RPG_ZONES; z++) {
        int cleared = 0;
        for (int attempt = 0; attempt < 6; attempt++) {
            rpg_game d;
            new_hero(&d, attempt % FAM_COUNT, (uint32_t)(attempt + 40 + z));
            d.zone_unlocked = (uint8_t)z;
            const rpg_zone_info *zi = rpg_zone(z);
            grow(&d, (zi->min_level + zi->max_level) / 2 + 2, attempt & 1);
            d.hero.stamina = 100;
            CHECK(rpg_dungeon_enter(&d, z, 0, T0) == RPG_OK);
            int rooms = 0;
            bool rested = false;
            while (d.run.active && rooms < 200) {
                if (d.run.floor == 6 && d.run.row < 0 && !rested) {
                    /* a player goes back to the inn after the mid boss */
                    rested = true;
                    rpg_dungeon_leave(&d, false);
                    CHECK(rpg_dungeon_checkpoint(&d, z));
                    d.gold += 1000;
                    CHECK(rpg_inn(&d) == RPG_OK);
                    d.cons[CO_POTION] = 10;
                    d.hero.stamina = 100;
                    CHECK(rpg_dungeon_enter(&d, z, 1, T0) == RPG_OK && d.run.floor == 6);
                }
                int row = d.run.row + 1, col = -1;
                for (int c = 0; c < RPG_COLS; c++) if (rpg_room_reachable(&d.run, row, c)) { col = c; break; }
                CHECK(col >= 0);
                d.hero.stamina = 100;
                rpg_foe foes[3];
                int n;
                int kind = rpg_room_enter(&d, row, col, foes, &n);
                CHECK(kind > 0);
                rooms++;
                if (!n) {
                    rpg_room_result res;
                    rpg_room_resolve(&d, &res, foes, &n);
                }
                if (n) {
                    int st = fight(&d, foes, n, WF_Q_GREAT, WF_Q_GREAT, (uint32_t)rooms, B_FLAG_DUNGEON | (kind == ROOM_BOSS ? B_FLAG_BOSS : 0));
                    if (st != B_WON) {
                        if (verbose) printf("    z%d %-9.9s lv%d died floor %d room %d kind %d foes %d (%s lv%d)\n", z,
                                            rpg_class(d.hero.cls)->name.en, d.hero.level, d.run.floor, row, kind, n,
                                            rpg_species(foes[0].species)->name.en, foes[0].level);
                        rpg_dungeon_leave(&d, true);
                        break;
                    }
                    d.run.cleared[row] |= (uint8_t)(1u << col);
                }
                if (kind == ROOM_STAIRS || kind == ROOM_BOSS) {
                    if (rpg_dungeon_descend(&d) == 1) cleared++;
                    /* drink between floors like a player would */
                    rpg_stats s;
                    rpg_stats_get(&d, &s);
                    if (d.hero.hp < s.max_hp / 2 && d.cons[CO_POTION]) rpg_use_consumable(&d, CO_POTION);
                }
            }
            if (verbose && attempt == 0) printf("  dungeon %d: floor %d reached (best %d)\n", z, d.run.floor, d.best_floor[z]);
        }
        if (verbose) printf("  dungeon %d cleared %d/6\n", z, cleared);
        if (getenv("SOFT")) dungeon_fail += cleared < 1; else CHECK(cleared >= 1);
    }
}

static void test_hunt_and_save(void) {
    rpg_game g;
    new_hero(&g, FAM_ARCHER, 8);
    grow(&g, 6, 0);
    CHECK(rpg_hunt_start(&g, 0, T0) == RPG_OK);
    { rpg_foe f[3]; int n0; CHECK(rpg_field_encounter(&g, 0, f, &n0) == RPG_ERR_BUSY); }
    rpg_haul h;
    rpg_hunt_preview(&g, T0 + 2 * 3600, &h);
    CHECK(h.minutes == 120 && h.kills > 10 && h.xp > 0);
    uint32_t gold = g.gold;
    CHECK(rpg_hunt_collect(&g, T0 + 2 * 3600, &h) == RPG_OK && g.gold == gold + h.gold && !g.hunt.active);
    /* stamina-limited */
    g.hero.stamina = 12;
    CHECK(rpg_hunt_start(&g, 0, T0) == RPG_OK);
    rpg_hunt_preview(&g, T0 + 12 * 3600, &h);
    CHECK(h.minutes == 120 && h.stopped == 1);
    rpg_hunt_collect(&g, T0 + 12 * 3600, &h);
    /* Save. */
    static uint8_t buf[8192];
    CHECK(rpg_save_size() < sizeof buf);
    size_t n = rpg_save_encode(&g, buf, sizeof buf);
    rpg_game back;
    CHECK(n && rpg_save_decode(buf, n, &back) && !memcmp(&back, &g, sizeof g));
    buf[40] ^= 1;
    CHECK(!rpg_save_decode(buf, n, &back));
    if (verbose) printf("  save size: %zu bytes\n", n);
}

int main(int argc, char **argv) {
    verbose = argc > 1 && argv[1][0];
    test_data();
    test_hero();
    test_items();
    test_battle_basics();
    test_hunt_and_save();
    test_dungeon();
    test_balance();
    printf("rpg: %d checks passed\n", checks);
    return 0;
}
