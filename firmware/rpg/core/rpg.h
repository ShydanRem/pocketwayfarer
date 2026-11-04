#pragma once
/* Pocket Wayfarer RPG — game state and static data.
 *
 * Plain C11, integer arithmetic only (deterministic on every device), no LVGL.
 * Design: docs/RPG-DESIGN.md. */
#include <stdbool.h>
#include <stdint.h>
#include "i18n.h"
#include "rng.h"

#define RPG_NAME_LEN 12
#define RPG_MAX_LEVEL 70
#define RPG_BAG 40
#define RPG_NEED_MAX 100000
#define RPG_QUESTS 3
#define RPG_FRIENDS 12

/* ---- Basics ----------------------------------------------------------------- */
enum { ST_STR, ST_AGI, ST_VIT, ST_INT, ST_DEX, ST_LUK, ST_COUNT };
enum { EL_NEUTRAL, EL_FIRE, EL_WATER, EL_EARTH, EL_WIND, EL_HOLY, EL_DARK, EL_COUNT };
enum { RACE_HUMAN, RACE_ELF, RACE_DWARF, RACE_BEAST, RACE_COUNT };
enum { STS_NONE, STS_BURN, STS_POISON, STS_FREEZE, STS_STUN, STS_BLEED, STS_WEAK, STS_BLIND, STS_COUNT };

typedef struct {
    wf_text name;
    wf_text blurb;
    int8_t stat[ST_COUNT];
    uint8_t xp_bonus;   /* percent */
} rpg_race_info;
const rpg_race_info *rpg_race(int race);
wf_text rpg_stat_name(int stat);
wf_text rpg_element_name(int el);
wf_text rpg_status_name(int st);
/* Multiplier x100 of `attack` element against `defense` element. */
int rpg_element_mult(int attack, int defense);

/* ---- Classes ------------------------------------------------------------------ */
enum {
    FAM_WARRIOR, FAM_MAGE, FAM_ARCHER, FAM_ROGUE, FAM_CLERIC, FAM_MONK, FAM_ARTIFICER, FAM_OCCULTIST, FAM_COUNT
};
#define RPG_CLASS_COUNT 40
/* ids: 0..7 base (= family), 8..23 advanced (8 + fam*2 + branch), 24..39 master (adv + 16) */
static inline int rpg_class_tier(int c) { return c < 8 ? 1 : c < 24 ? 2 : 3; }
static inline int rpg_class_family(int c) { return c < 8 ? c : c < 24 ? (c - 8) / 2 : (c - 24) / 2; }
static inline int rpg_class_parent(int c) { return c < 8 ? -1 : c < 24 ? (c - 8) / 2 : c - 16; }
/* Next classes reachable from c (0, 1 or 2 entries). */
int rpg_class_next(int c, int out[2]);
int rpg_job_max(int c);

typedef struct {
    wf_text name;
    wf_text blurb;
    uint8_t main_stat, second_stat;
    uint8_t skills[3];       /* 0xff = none */
} rpg_class_info;
const rpg_class_info *rpg_class(int c);

/* ---- Skills -------------------------------------------------------------------- */
typedef enum {
    SK_STRIKE,      /* physical, one target          */
    SK_MULTI,       /* physical, `hits` on one target */
    SK_CLEAVE,      /* physical, all enemies          */
    SK_BOLT,        /* magical, one target            */
    SK_NOVA,        /* magical, all enemies           */
    SK_HEAL,        /* restore own HP                 */
    SK_BUFF,        /* self buff                      */
    SK_DEBUFF,      /* status on one (or all if `hits`=9) */
    SK_DRAIN,       /* magical, heals part of damage  */
    SK_SUMMON,      /* ally that strikes every round  */
    SK_PASSIVE
} rpg_skill_kind;

enum { BF_NONE, BF_ATK, BF_DEF, BF_MATK, BF_CRIT, BF_DODGE, BF_REGEN, BF_SHIELD, BF_RAGE, BF_COUNT };
enum {
    PS_NONE, PS_HP, PS_SP, PS_ATK, PS_MATK, PS_DEF, PS_CRIT, PS_DODGE, PS_LIFESTEAL, PS_DOUBLE,
    PS_SP_REGEN, PS_HEAL, PS_SUMMON, PS_GOLD
};

typedef struct {
    wf_text name;
    uint8_t kind;
    uint8_t element;
    uint16_t power;         /* % at level 1 (damage/heal), or buff/passive value */
    uint8_t power_lv;       /* added per skill level */
    uint8_t hits;
    uint8_t sp, sp_lv;
    uint8_t cooldown;
    uint8_t status, chance; /* status inflicted and chance % */
    uint8_t buff;           /* BF_* or PS_* for passives */
    uint8_t turns;
} rpg_skill_info;

#define RPG_SKILL_COUNT 104
#define RPG_SKILL_MAX_LV 5
const rpg_skill_info *rpg_skill(int s);
/* Effective power/cost at a given skill level (1..5). */
int rpg_skill_power(int s, int lv);
int rpg_skill_cost(int s, int lv);

/* ---- Items --------------------------------------------------------------------- */
enum { SLOT_WEAPON, SLOT_OFFHAND, SLOT_HEAD, SLOT_BODY, SLOT_FEET, SLOT_ACC1, SLOT_ACC2, SLOT_COUNT };
enum {
    IT_SWORD, IT_AXE, IT_STAFF, IT_BOW, IT_DAGGER, IT_MACE, IT_KNUCKLE, IT_HAMMER, IT_SCYTHE,
    IT_SHIELD, IT_QUIVER, IT_TOME, IT_CHARM,
    IT_HELM, IT_HOOD, IT_CIRCLET,
    IT_PLATE, IT_LEATHER, IT_ROBE,
    IT_BOOTS,
    IT_RING, IT_AMULET,
    IT_TYPE_COUNT
};
#define RPG_TIERS 5
enum { RAR_COMMON, RAR_UNCOMMON, RAR_RARE, RAR_EPIC, RAR_LEGENDARY, RAR_COUNT };
enum {
    AF_NONE, AF_STR, AF_AGI, AF_VIT, AF_INT, AF_DEX, AF_LUK, AF_ATK_PCT, AF_MATK_PCT, AF_DEF, AF_HP_PCT,
    AF_CRIT, AF_LIFESTEAL, AF_SP_REGEN, AF_FIRE, AF_WATER, AF_EARTH, AF_WIND, AF_HOLY, AF_DARK, AF_GOLD,
    AF_COUNT
};

typedef struct {
    wf_text name;
    uint8_t slot;
    uint8_t material;       /* 0 metal, 1 wood, 2 leather, 3 cloth, 4 gem */
    uint8_t families;       /* bit mask of FAM_*; 0xff = everyone */
    uint8_t look;           /* sprite layer variant */
    uint8_t magic;          /* weapon scales MATK instead of ATK */
    uint16_t atk, matk, def, mdef, hp;   /* at tier 0; scaled per tier */
} rpg_item_type;

typedef struct {
    uint8_t type;           /* IT_*; 0xff = empty */
    uint8_t tier;
    uint8_t rarity;
    uint8_t plus;           /* forge level 0..10 */
    uint8_t level;          /* required level */
    uint8_t sockets;
    uint8_t gems[2];        /* species + 1, 0 = empty */
    uint8_t aff[3];
    uint8_t aff_val[3];
} rpg_item;

const rpg_item_type *rpg_item_type_get(int t);
wf_text rpg_material_name(int material, int tier);
wf_text rpg_rarity_name(int r);
wf_text rpg_affix_name(int a);
bool rpg_item_empty(const rpg_item *it);
/* Writes the localised display name ("Spada di Ferro +3"). */
const char *rpg_item_name(const rpg_item *it, char *buf, int cap);
int rpg_item_atk(const rpg_item *it);
int rpg_item_matk(const rpg_item *it);
int rpg_item_def(const rpg_item *it);
int rpg_item_mdef(const rpg_item *it);
int rpg_item_hp(const rpg_item *it);
uint32_t rpg_item_value(const rpg_item *it);  /* sell price */

/* Consumables and materials kept as counts. */
enum {
    CO_POTION, CO_HIPOTION, CO_ETHER, CO_ANTIDOTE, CO_BREAD, CO_STEW, CO_PIE, CO_ORE, CO_RETURN,
    CO_COUNT
};
typedef struct {
    wf_text name;
    wf_text blurb;
    uint16_t price;
    uint8_t hp_pct, sp_pct;
    int32_t hunger;
    uint8_t morale, stamina;
} rpg_consumable_info;
const rpg_consumable_info *rpg_consumable(int c);

/* ---- Monsters ------------------------------------------------------------------- */
enum {
    /* Lumen Meadows / Lantern Crypt */
    SP_SLIME, SP_SHROOM, SP_WOLF, SP_WISP, SP_SKELETON, SP_BAT, SP_GHOST, SP_MIMIC, SP_SLIME_KING, SP_BONE_KING,
    /* Glass Shore / Sunken Temple */
    SP_CRAB, SP_JELLY, SP_BLUE_SLIME, SP_GOBLIN, SP_SERPENT, SP_DEEP_GHOST, SP_CRAB_KING, SP_LEVIATHAN,
    /* Whispering Wood / Elder Roots */
    SP_TREANT, SP_OWL, SP_DIRE_WOLF, SP_SPIDER, SP_TOXIC_SHROOM, SP_DARK_FAIRY, SP_ELDER_STUMP, SP_SPIDER_QUEEN,
    /* Ember Caves / Infernal Forge */
    SP_SALAMANDER, SP_LAVA_GOLEM, SP_FIRE_BAT, SP_GOBLIN_MINER, SP_IMP, SP_HELL_KNIGHT, SP_ASH_DRAGON, SP_DEMON_LORD,
    /* Sky Spires / Celestial Citadel */
    SP_STORM_WISP, SP_GRIFFIN, SP_CRYSTAL, SP_HARPY, SP_SKY_KNIGHT, SP_CRYSTAL_GOLEM, SP_GUARDIAN, SP_STAR_DRAGON,
    SP_COUNT
};

enum { FOE_HIT, FOE_HEAVY, FOE_MULTI, FOE_BREATH, FOE_HEAL, FOE_CURSE };

typedef struct {
    wf_text name;
    uint8_t element;
    uint8_t m_hp, m_atk, m_def, m_agi;   /* x10 on the level curve */
    uint8_t special;                     /* FOE_* move used besides normal hits */
    uint8_t status, chance;              /* status of the special */
    uint8_t magic;                       /* special uses MATK vs MDEF */
    uint8_t boss;
    uint8_t gem_affix, gem_value;        /* soul gem bonus */
} rpg_species_info;
const rpg_species_info *rpg_species(int sp);

/* ---- World ------------------------------------------------------------------------ */
#define RPG_ZONES 5
#define RPG_FLOORS 10
typedef struct {
    wf_text field_name, dungeon_name, blurb;
    uint8_t min_level, max_level;
    uint8_t field_foes[4];
    uint8_t dungeon_foes[4];
    uint8_t mid_boss, final_boss;
    uint8_t field_scene, dungeon_scene;
} rpg_zone_info;
const rpg_zone_info *rpg_zone(int z);

/* ---- Hero & game ------------------------------------------------------------------ */
typedef struct {
    char name[RPG_NAME_LEN + 1];
    uint8_t race, body, skin, hair, hair_color, eyes;
    uint8_t cls;
    uint8_t level, job_level;
    uint32_t xp, job_xp;
    uint8_t stat[ST_COUNT];
    uint16_t stat_points, skill_points;
    uint8_t skill[RPG_SKILL_COUNT];
    int32_t hp, sp;
    int32_t hunger;                 /* 0..RPG_NEED_MAX */
    int16_t stamina;                /* 0..100 */
    int16_t morale;                 /* 0..100 */
    uint16_t stamina_acc;           /* minute accumulator */
    rpg_item equip[SLOT_COUNT];
} rpg_hero;

typedef enum { ROOM_NONE, ROOM_FIGHT, ROOM_ELITE, ROOM_CHEST, ROOM_TRAP, ROOM_FOUNTAIN, ROOM_MERCHANT, ROOM_EVENT,
               ROOM_BOSS, ROOM_STAIRS, ROOM_KIND_COUNT } rpg_room_kind;
#define RPG_ROWS 4
#define RPG_COLS 3
typedef struct {
    uint8_t active;
    uint8_t zone;
    uint8_t floor;                  /* 1..RPG_FLOORS */
    int8_t row, col;                /* current position; row -1 = entrance */
    uint8_t rooms[RPG_ROWS + 1][RPG_COLS];     /* last row: boss/stairs at col 1 */
    uint8_t links[RPG_ROWS + 1][RPG_COLS];     /* bit c: link to column c of the next row */
    uint8_t cleared[RPG_ROWS + 1];             /* bit per column */
    uint32_t seed;
    uint32_t gold;                  /* found in this descent */
    uint16_t kills;
} rpg_run;

typedef struct {
    uint8_t active;
    uint8_t zone;
    uint32_t since;
    uint32_t seed;
    int32_t hunger0;        /* needs when the hunt started */
    int16_t stamina0;
} rpg_hunt;

enum { Q_KILL, Q_ROOMS, Q_FORGE, Q_EAT, Q_GOLD, Q_SKILL, Q_BOSS, Q_GREET, Q_KIND_COUNT };
typedef struct {
    uint8_t kind, goal, progress, claimed;
    uint16_t gold;
    uint8_t reward;   /* consumable */
} rpg_quest;

typedef struct {
    uint8_t used;
    uint8_t id[6];
    char name[RPG_NAME_LEN + 1];
    uint8_t cls, level;
    uint32_t last_greet_day;
} rpg_friend;

typedef struct {
    rpg_hero hero;
    uint32_t gold;
    rpg_item bag[RPG_BAG];
    uint8_t cons[CO_COUNT];
    uint8_t gems[SP_COUNT];
    uint8_t zone_unlocked;          /* highest field/dungeon open */
    uint8_t best_floor[RPG_ZONES];
    rpg_run run;
    rpg_hunt hunt;
    uint32_t day;
    uint8_t streak, daily_claimed;
    rpg_quest quests[RPG_QUESTS];
    rpg_friend friends[RPG_FRIENDS];
    uint32_t last_update;
    uint32_t created;
    uint32_t rng_state;
    uint32_t kills, deaths, bosses;
    uint8_t lang, brightness, hour_offset, auto_battle;
} rpg_game;

/* ---- Lifecycle (hero.c) ------------------------------------------------------------- */
typedef struct {
    char name[RPG_NAME_LEN + 1];
    uint8_t race, body, skin, hair, hair_color, eyes, cls, lang;
} rpg_new_hero;
void rpg_game_new(rpg_game *g, const rpg_new_hero *h, uint32_t now, uint32_t seed);
/* Real-time simulation (needs, regeneration, new day). Returns true if anything changed. */
bool rpg_tick(rpg_game *g, uint32_t now);
void rpg_roll_day(rpg_game *g, uint32_t day);

typedef struct {
    int32_t max_hp, max_sp;
    int32_t atk, matk, def, mdef;
    int stat[ST_COUNT];             /* including race, items and gems */
    int crit;                       /* % */
    int dodge;                      /* % */
    int double_act;                 /* % */
    int lifesteal;                  /* % */
    int sp_regen;                   /* per round */
    int heal_bonus;                 /* % */
    int summon_bonus;               /* % */
    int gold_bonus;                 /* % */
    int elem_bonus[EL_COUNT];       /* % damage by element */
    uint8_t weapon_element;
    int timing;                     /* ring width bonus from DEX */
} rpg_stats;
void rpg_stats_get(const rpg_game *g, rpg_stats *out);
int rpg_power(const rpg_game *g);   /* rough combat rating */
uint32_t rpg_xp_next(int level);
uint32_t rpg_job_xp_next(int cls, int job_level);
/* Returns levels gained (base) and writes job levels gained. */
int rpg_gain_xp(rpg_game *g, uint32_t xp, uint32_t job_xp, int *job_levels);
bool rpg_raise_stat(rpg_game *g, int stat);
int rpg_stat_cost(int value);
bool rpg_learn_skill(rpg_game *g, int skill);
/* Skills currently usable/learnable by the hero's class line. */
int rpg_class_skills(int cls, int out[9]);
bool rpg_can_change_class(const rpg_game *g, int target);
bool rpg_change_class(rpg_game *g, int target);

/* Needs */
enum { RPG_OK = 0, RPG_ERR_GOLD, RPG_ERR_NONE, RPG_ERR_FULL, RPG_ERR_STAMINA, RPG_ERR_LEVEL, RPG_ERR_CLASS,
       RPG_ERR_BAG, RPG_ERR_BUSY, RPG_ERR_HUNGRY, RPG_ERR_LOCKED, RPG_ERR_DEAD };
const char *rpg_err(int e);
int rpg_use_consumable(rpg_game *g, int c);   /* outside battle */
int rpg_inn(rpg_game *g);
uint32_t rpg_inn_price(const rpg_game *g);
int rpg_buy(rpg_game *g, int c, int count);

/* Quests & daily */
void rpg_quest_bump(rpg_game *g, int kind, int amount);
int rpg_quest_claim(rpg_game *g, int i);
const char *rpg_quest_text(const rpg_quest *q, char *buf, int cap);
int rpg_daily_claim(rpg_game *g, int *item);
bool rpg_greet(rpg_game *g, const uint8_t id[6], const char *name, int cls, int level, uint32_t now);
uint32_t rpg_day_of(uint32_t now);

/* ---- Items (items.c) ---------------------------------------------------------------- */
void rpg_item_roll(rpg_item *out, wf_rng *r, int level, int rarity_bonus, int families);
int rpg_rarity_roll(wf_rng *r, int bonus);
bool rpg_can_equip(const rpg_game *g, const rpg_item *it);
int rpg_equip(rpg_game *g, int bag_index);      /* swaps with the slot */
int rpg_unequip(rpg_game *g, int slot);
bool rpg_bag_add(rpg_game *g, const rpg_item *it);
void rpg_bag_remove(rpg_game *g, int index);
int rpg_bag_count(const rpg_game *g);
uint32_t rpg_sell(rpg_game *g, int bag_index);
int rpg_forge_chance(int plus);
uint32_t rpg_forge_gold(const rpg_item *it);
int rpg_forge_ore(const rpg_item *it);
/* Returns 1 success, 0 failure (no change), -1 failure (level lost), or an error < -1. */
int rpg_forge(rpg_game *g, rpg_item *it, wf_rng *r);
int rpg_socket(rpg_game *g, rpg_item *it, int species);
/* Equipment in the shop for the hero's level. */
void rpg_shop_stock(const rpg_game *g, rpg_item out[4]);
uint32_t rpg_item_price(const rpg_item *it);

/* ---- Battle (battle.c) ---------------------------------------------------------------- */
#define RPG_MAX_FOES 3
#define RPG_EVENTS 24
typedef struct {
    uint8_t species, level, elite, boss;
    int32_t hp, max_hp;
    int32_t atk, matk, def, mdef;
    int16_t agi;
    uint8_t element;
    uint8_t status, status_turns;
    uint8_t weak_turns;
    uint8_t charge;      /* builds toward the special move */
} rpg_foe;

enum { ACT_ATTACK, ACT_SKILL, ACT_ITEM, ACT_GUARD, ACT_FLEE };
typedef struct {
    uint8_t kind;
    uint8_t skill;      /* skill id */
    uint8_t item;       /* consumable id */
    uint8_t target;     /* foe index */
} rpg_action;

enum {
    EV_HIT, EV_CRIT_HIT, EV_MISS, EV_HEAL, EV_SP, EV_BUFF, EV_STATUS, EV_DOT, EV_DODGE, EV_PARRY, EV_COUNTER,
    EV_KO, EV_SUMMON, EV_SUMMON_HIT, EV_GUARD, EV_FLEE_OK, EV_FLEE_FAIL, EV_EXTRA, EV_SKILL, EV_NO_SP,
    EV_FOE_SKILL, EV_FROZEN
};
typedef struct {
    uint8_t kind;
    int8_t actor;       /* -1 hero, 0..2 foe, 3 summon */
    int8_t target;      /* -1 hero, 0..2 foe */
    uint8_t quality;
    uint8_t element;
    uint8_t effect;     /* 1 super effective, 2 resisted; for EV_STATUS the status */
    uint8_t skill;
    int32_t value;
} rpg_event;

enum { B_HERO, B_FOES, B_WON, B_LOST, B_FLED };

typedef struct {
    rpg_stats st;
    int32_t hp, sp;
    uint8_t status, status_turns;
    uint8_t buff[BF_COUNT];         /* turns left */
    int16_t buff_val[BF_COUNT];
    int32_t shield;
    uint8_t cooldown[9];            /* per entry of class_skills */
    int skills[9];
    int nskills;
    uint8_t guarding;
    int32_t summon_hp, summon_atk;
    uint8_t summon_turns;
} rpg_hero_b;

typedef struct {
    rpg_hero_b h;
    rpg_foe foe[RPG_MAX_FOES];
    uint8_t nfoes;
    wf_rng rng;
    uint8_t state;
    uint8_t flags;              /* B_FLAG_* */
    uint8_t acting;             /* foe about to act */
    uint8_t foe_move;           /* FOE_* chosen for the acting foe */
    uint8_t foe_dodge;          /* pre-rolled: the hero dodges this attack */
    uint8_t extra;              /* hero got a double action */
    uint16_t round;
    rpg_event ev[RPG_EVENTS];
    uint8_t ev_count;
} rpg_battle;
enum { B_FLAG_BOSS = 1, B_FLAG_DUNGEON = 2, B_FLAG_NOFLEE = 4 };

void rpg_foe_make(rpg_foe *f, int species, int level, bool elite);
void rpg_battle_start(rpg_battle *b, const rpg_game *g, const rpg_foe *foes, int n, uint32_t seed, int flags);
bool rpg_battle_over(const rpg_battle *b);
int rpg_battle_target(const rpg_battle *b, int preferred);   /* first alive foe */
/* Hero acts; quality is the ring timing (ignored for heals/buffs/items). */
void rpg_battle_hero(rpg_battle *b, rpg_game *g, const rpg_action *a, int quality);
/* Picks the next foe to act (state B_FOES); returns false when the round ended. */
bool rpg_battle_next_foe(rpg_battle *b);
/* The acting foe attacks; parry = defensive timing (ignored when dodged). */
void rpg_battle_foe(rpg_battle *b, int parry);
/* Simple automatic choice for AUTO mode and idle simulations. */
void rpg_battle_auto(const rpg_battle *b, const rpg_game *g, rpg_action *a);
bool rpg_skill_ready(const rpg_battle *b, int index);

typedef struct {
    uint32_t xp, job_xp, gold;
    rpg_item items[3];
    uint8_t nitems;
    uint8_t cons[CO_COUNT];
    uint8_t gem;                /* species + 1 */
    uint8_t levels, job_levels;
} rpg_loot;
/* Applies the outcome to the game (HP/SP back to the hero, rewards, quests). */
void rpg_battle_finish(rpg_game *g, const rpg_battle *b, rpg_loot *loot);

/* ---- Dungeons & fields (world.c) --------------------------------------------------- */
/* checkpoint 1 starts at floor 6 once the mid boss was beaten. */
int rpg_dungeon_enter(rpg_game *g, int zone, int checkpoint, uint32_t now);
bool rpg_dungeon_checkpoint(const rpg_game *g, int zone);
void rpg_dungeon_floor(rpg_run *run);            /* generates the current floor */
bool rpg_room_reachable(const rpg_run *run, int row, int col);
/* Moves into a room; for fights fills foes/n (returns ROOM_* kind). */
int rpg_room_enter(rpg_game *g, int row, int col, rpg_foe *foes, int *n);
typedef struct {
    int kind;
    int32_t value;      /* hp lost/healed, gold */
    rpg_item item;
    uint8_t got_item;
    uint8_t cons;       /* consumable id + 1 */
    uint8_t mimic;      /* chest was a mimic: fight */
} rpg_room_result;
/* Resolves a non-fight room. */
void rpg_room_resolve(rpg_game *g, rpg_room_result *res, rpg_foe *foes, int *n);
int rpg_dungeon_descend(rpg_game *g);            /* after stairs/boss */
void rpg_dungeon_leave(rpg_game *g, bool defeated);

int rpg_field_encounter(rpg_game *g, int zone, rpg_foe *foes, int *n);
int rpg_hunt_start(rpg_game *g, int zone, uint32_t now);
typedef struct {
    uint32_t minutes, kills, xp, job_xp, gold;
    rpg_item items[4];
    uint8_t nitems;
    uint8_t cons[CO_COUNT];
    uint8_t gems;
    uint8_t levels;
    uint8_t stopped;      /* 0 time, 1 stamina, 2 hunger, 3 too strong foes */
} rpg_haul;
void rpg_hunt_preview(const rpg_game *g, uint32_t now, rpg_haul *out);
int rpg_hunt_collect(rpg_game *g, uint32_t now, rpg_haul *out);

/* ---- Save (save.c) -------------------------------------------------------------------- */
#define RPG_SAVE_MAGIC 0x31475052u   /* "RPG1" */
#define RPG_SAVE_VERSION 1u
size_t rpg_save_size(void);
size_t rpg_save_encode(const rpg_game *g, uint8_t *out, size_t cap);
bool rpg_save_decode(const uint8_t *in, size_t len, rpg_game *g);
