#pragma once
/* RPG user interface — internal header shared by the r_*.c screens. */
#include "kit.h"
#include "ring.h"
#include "fx.h"
#include "rpg.h"
#include "timing.h"
#include "art/rpg_art.h"

enum {
    R_TITLE, R_CREATE, R_TOWN, R_MAP, R_ZONE, R_BATTLE, R_DUNGEON, R_HERO, R_SKILLS, R_CLASS,
    R_EQUIP, R_BAG, R_SHOP, R_FORGE, R_QUESTS, R_SCREEN_COUNT
};

typedef struct {
    rpg_game g;
    bool loaded;
    int screen;
    uint32_t last_tick_s, last_save_s, save_due_s;
    bool dirty;
    /* context */
    int zone;
    int shop_tab;
    int bag_tab;
    int forge_pick;          /* -1 none, 0..6 equipped slot, 100+ bag index */
    bool battle_dungeon;
    bool shop_in_dungeon;
    int return_screen;
} rpg_ui_state;

extern rpg_ui_state R;

uint32_t r_now(void);
int r_hour(void);
void r_dirty(void);
void r_save_now(void);
void r_go(int screen);
const wf_sprite *ri(int id);
/* Scene background plus its ambient lights and particles. */
lv_obj_t *r_scene(int scene);

/* Hero picture --------------------------------------------------------------- */
typedef struct {
    uint8_t race, skin, eyes, hair, hair_color, body;
    uint8_t fam, tier;
    uint8_t weapon, weapon_tier;      /* IT_* weapon or 0xff */
    uint8_t off, off_tier;            /* IT_* off-hand or 0xff */
    uint8_t head, head_tier;          /* IT_* head item or 0xff */
} hero_look;
enum { HX_IDLE, HX_BLINK, HX_HURT, HX_HAPPY };
void hero_look_from_game(hero_look *l, const rpg_game *g);
const lv_image_dsc_t *hero_image(const hero_look *l, int expr, int scale);
lv_obj_t *hero_obj(lv_obj_t *parent, const hero_look *l, int expr, int scale, int x, int y);
void hero_obj_set(lv_obj_t *img, const hero_look *l, int expr, int scale);

/* Shared bits --------------------------------------------------------------- */
uint32_t rarity_color(int r);
const wf_sprite *item_icon(const rpg_item *it);
lv_obj_t *r_list(lv_obj_t *parent, int x, int y, int w, int h);
lv_obj_t *r_row(lv_obj_t *list, int h);
lv_obj_t *r_text(lv_obj_t *parent, const char *t, uint32_t color, const lv_font_t *font);
void r_item_dialog(const rpg_item *it, const char *title_extra, lv_obj_t **dlg);
const char *r_item_lines(const rpg_item *it, char *buf, int cap);
void r_vitals_arcs(lv_obj_t *s, lv_obj_t **hp, lv_obj_t **sp);
void r_vitals_update(lv_obj_t *hp, lv_obj_t *sp, int32_t hpv, int32_t hpmax, int32_t spv, int32_t spmax);
const char *r_fmt_time(uint32_t s, char *buf, int cap);
lv_obj_t *r_back(lv_obj_t *parent, int screen);

/* Screens -------------------------------------------------------------------- */
void scr_title(void);
void scr_create(void);
void scr_town(void);
void scr_map(void);
void scr_zone(void);
void scr_battle(void);
void scr_dungeon(void);
void scr_hero(void);
void scr_skills(void);
void scr_class(void);
void scr_equip(void);
void scr_bag(void);
void scr_shop(void);
void scr_forge(void);
void scr_quests(void);
void town_tick(void);
void battle_tick(void);
void create_tick(void);
bool battle_key(int key);
/* Starts a battle; `dungeon` makes it part of the current descent. */
void r_start_battle(const rpg_foe *foes, int n, bool dungeon, bool boss);
void r_after_dungeon_battle(int state);
