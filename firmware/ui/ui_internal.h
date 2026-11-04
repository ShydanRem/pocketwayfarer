#pragma once
/* Shared between the UI translation units. Not part of the platform API. */
#include "lvgl.h"
#include "ui.h"
#include "art/art.h"
#include "battle.h"
#include "timing.h"
#include "world.h"

#define CX 233
#define CY 233
#define PIX 3   /* art pixel size on screen */

LV_FONT_DECLARE(wf_font_small)
LV_FONT_DECLARE(wf_font_big)
LV_FONT_DECLARE(wf_font_huge)

/* Palette */
#define COL_INK      0x1d1430
#define COL_PANEL    0x161b38
#define COL_PANEL_HI 0x2a3266
#define COL_TEXT     0xfff6e0
#define COL_DIM      0xa8b0d8
#define COL_GOLD     0xffd23f
#define COL_HUNGER   0xff9a3a
#define COL_ENERGY   0x5ad8ff
#define COL_MOOD     0xff6aa8
#define COL_XP       0x8ee86a
#define COL_HP       0x6ce06a
#define COL_BAD      0xff4f5a
#define COL_BTN      0x3a5ae8
#define COL_BTN_ALT  0xe8508a
#define COL_BTN_OK   0x3aba6a

/* ---- Global UI state --------------------------------------------------- */
typedef struct {
    wf_game g;
    wf_link link;
    int screen;
    bool loaded;
    uint32_t last_save_s;
    bool dirty;
    /* Cross-screen context */
    int zone;               /* selected zone */
    wf_step step;           /* last exploration step */
    bool battle_boss;
    int feed_index;
    int bag_tab;
    int link_peer;          /* selected nearby peer index */
    uint8_t link_peer_id[6];
    char toast[48];
    int pending_anim;       /* home: animation to play on entry */
} wf_ui_state;

extern wf_ui_state ui;

enum { ANIM_NONE, ANIM_EAT, ANIM_HAPPY, ANIM_SAD, ANIM_EVOLVE, ANIM_HATCH, ANIM_CLEAN };

uint32_t now_s(void);
int ui_hour(void);   /* local hour for day/night */
void mark_dirty(void);

/* ---- Kit ------------------------------------------------------------------ */
const lv_image_dsc_t *gfx_sprite(const wf_sprite *s, int scale);
void gfx_release_screen(void);      /* frees scene/creature expansions */
lv_obj_t *kit_screen(int scene);    /* new active screen; scene < 0 = plain dark */
lv_obj_t *kit_img(lv_obj_t *parent, const wf_sprite *s, int scale, int x, int y);
void kit_img_set(lv_obj_t *img, const wf_sprite *s, int scale);
lv_obj_t *kit_label(lv_obj_t *parent, const lv_font_t *font, uint32_t color, const char *text, int x, int y);
lv_obj_t *kit_pill(lv_obj_t *parent, const char *text, int x, int y);
lv_obj_t *kit_panel(lv_obj_t *parent, int x, int y, int w, int h);
lv_obj_t *kit_round_button(lv_obj_t *parent, const wf_sprite *icon, const char *caption, int x, int y, int size,
                           uint32_t color, lv_event_cb_t cb, intptr_t user);
lv_obj_t *kit_text_button(lv_obj_t *parent, const char *text, int x, int y, int w, int h, uint32_t color,
                          lv_event_cb_t cb, intptr_t user);
lv_obj_t *kit_bar(lv_obj_t *parent, int x, int y, int w, int h, uint32_t color, int value, int max);
void kit_bar_set(lv_obj_t *bar, int value, int max, bool anim);
lv_obj_t *kit_arc(lv_obj_t *parent, int radius, int width, int start, int end, uint32_t color, int value, int max);
void kit_toast(const char *text, const wf_sprite *icon);
void kit_float_text(lv_obj_t *parent, const char *text, uint32_t color, const lv_font_t *font, int x, int y);
void kit_burst(lv_obj_t *parent, const wf_sprite *icon, int x, int y, int count);
lv_obj_t *kit_dialog(const char *title, const char *body, const wf_sprite *icon);
lv_obj_t *kit_dialog_button(lv_obj_t *dialog, const char *text, uint32_t color, lv_event_cb_t cb, intptr_t user);
void kit_dialog_close(lv_obj_t *dialog);
void kit_bob(lv_obj_t *obj, int amount, int period_ms);
void kit_shake(lv_obj_t *obj, int amount);
void kit_flash(lv_obj_t *img, uint32_t color, int ms);
void kit_fade_in(lv_obj_t *obj, int ms, int delay);
lv_obj_t *kit_back_button(lv_obj_t *parent, lv_event_cb_t cb);
void kit_pos(lv_obj_t *obj, int x, int y);
void kit_clear_handlers(void);
const wf_sprite *icon(int id);
const char *fmt_duration(uint32_t seconds, char *buf, size_t cap);
lv_obj_t *kit_dim(lv_obj_t *parent, int opa);

/* ---- Screens -------------------------------------------------------------- */
void scr_onboard(void);
void scr_home(void);
void scr_menu(void);
void scr_feed(void);
void scr_play(void);
void scr_train(void);
void scr_map(void);
void scr_zone(void);
void scr_battle(void);
void scr_duel(void);
void scr_link(void);
void scr_bag(void);
void scr_diary(void);
void home_tick(void);
void battle_tick(void);
void play_tick(void);
void train_tick(void);
void link_tick(void);
void duel_tick(void);
void duel_on_event(const wf_link_event *e);
bool battle_key(int key);
void go(int screen);
void train_reset(void);
void go_home_anim(int anim);
void start_battle(int zone, const wf_step *step);
