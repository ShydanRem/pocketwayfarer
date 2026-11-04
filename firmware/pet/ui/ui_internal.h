#pragma once
/* Shared between the UI translation units. Not part of the platform API. */
#include "lvgl.h"
#include "ui.h"
#include "kit.h"
#include "art/art.h"
#include "battle.h"
#include "timing.h"
#include "world.h"

#include "kit.h"

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

const wf_sprite *icon(int id);
lv_obj_t *kit_back_button(lv_obj_t *parent, lv_event_cb_t cb);
lv_obj_t *pet_screen(int scene);

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
