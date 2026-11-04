#pragma once
/* Shared drawing toolkit for the round 466x466 display (LVGL 9). */
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "lvgl.h"
#include "platform.h"
#include "i18n.h"
#include "sprite.h"

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

/* ---- Kit ------------------------------------------------------------------ */
const lv_image_dsc_t *gfx_sprite(const wf_sprite *s, int scale);
void gfx_release_screen(void);      /* frees scene/creature expansions */
/* New active screen with an optional full-screen scene (156x156 art shown at 3x). */
lv_obj_t *kit_screen(const wf_sprite *scene);
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
void kit_pos(lv_obj_t *obj, int x, int y);
const char *kit_last_toast(void);
const char *fmt_duration(uint32_t seconds, char *buf, size_t cap);
lv_obj_t *kit_dim(lv_obj_t *parent, int opa);

