/* Drawing toolkit: sprite expansion cache and the few widgets every screen uses. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "kit.h"

/* ---- Sprite cache --------------------------------------------------------
 * Art is stored as 8-bit indexed pixels; on first use it is expanded to
 * RGB565 (+A8) at the requested integer scale. Icons stay cached forever;
 * scenes and creatures are released whenever the screen changes. */
typedef struct {
    const wf_sprite *s;
    uint8_t scale;
    uint8_t persistent;
    lv_image_dsc_t dsc;
    void *buf;
} gfx_entry;

#define GFX_SLOTS 160
static gfx_entry cache[GFX_SLOTS];

static void expand(const wf_sprite *s, int scale, gfx_entry *e) {
    int w = s->w * scale, h = s->h * scale;
    size_t px = (size_t)w * (size_t)h;
    size_t bytes = px * (s->alpha ? 3u : 2u);
    uint8_t *buf = wf_platform_big_alloc(bytes);
    LV_ASSERT_MALLOC(buf);
    uint16_t *rgb = (uint16_t *)buf;
    uint8_t *alpha = buf + px * 2u;
    for (int y = 0; y < s->h; y++) {
        for (int x = 0; x < s->w; x++) {
            uint8_t idx = s->px[y * s->w + x];
            uint16_t c = s->pal[idx];
            uint8_t a = (s->alpha && idx == 0) ? 0 : 255;
            for (int dy = 0; dy < scale; dy++) {
                size_t row = (size_t)(y * scale + dy) * (size_t)w + (size_t)(x * scale);
                for (int dx = 0; dx < scale; dx++) {
                    rgb[row + (size_t)dx] = c;
                    if (s->alpha) alpha[row + (size_t)dx] = a;
                }
            }
        }
    }
    memset(&e->dsc, 0, sizeof e->dsc);
    e->dsc.header.magic = LV_IMAGE_HEADER_MAGIC;
    e->dsc.header.cf = s->alpha ? LV_COLOR_FORMAT_RGB565A8 : LV_COLOR_FORMAT_RGB565;
    e->dsc.header.w = (uint32_t)w;
    e->dsc.header.h = (uint32_t)h;
    e->dsc.header.stride = (uint32_t)w * 2u;
    e->dsc.data = buf;
    e->dsc.data_size = (uint32_t)bytes;
    e->buf = buf;
}

const lv_image_dsc_t *gfx_sprite(const wf_sprite *s, int scale) {
    gfx_entry *free_slot = NULL;
    for (int i = 0; i < GFX_SLOTS; i++) {
        if (cache[i].s == s && cache[i].scale == scale) return &cache[i].dsc;
        if (!cache[i].s && !free_slot) free_slot = &cache[i];
    }
    if (!free_slot) {
        /* Should not happen within one screen; recycle the first non-persistent slot. */
        for (int i = 0; i < GFX_SLOTS && !free_slot; i++)
            if (!cache[i].persistent) free_slot = &cache[i];
        wf_platform_big_free(free_slot->buf);
        free_slot->s = NULL;
    }
    expand(s, scale, free_slot);
    free_slot->s = s;
    free_slot->scale = (uint8_t)scale;
    free_slot->persistent = s->w <= 16;
    return &free_slot->dsc;
}

void gfx_release_screen(void) {
    for (int i = 0; i < GFX_SLOTS; i++) {
        if (cache[i].s && !cache[i].persistent) {
            wf_platform_big_free(cache[i].buf);
            memset(&cache[i], 0, sizeof cache[i]);
        }
    }
}


/* ---- Basics ---------------------------------------------------------------- */
void kit_pos(lv_obj_t *obj, int x, int y) { lv_obj_align(obj, LV_ALIGN_CENTER, x - CX, y - CY); }

lv_obj_t *kit_screen(const wf_sprite *scene) {
    lv_obj_t *old = lv_screen_active();
    lv_obj_t *scr = lv_obj_create(NULL);
    lv_obj_remove_style_all(scr);
    lv_obj_set_size(scr, WF_RES, WF_RES);
    lv_obj_set_style_bg_color(scr, lv_color_hex(0x0b0d1c), 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);
    lv_obj_remove_flag(scr, LV_OBJ_FLAG_SCROLLABLE);
    lv_screen_load(scr);
    lv_obj_clean(lv_layer_top());
    if (old && old != scr) lv_obj_delete(old);
    gfx_release_screen();
    if (scene) {
        lv_obj_t *bg = lv_image_create(scr);
        lv_image_set_src(bg, gfx_sprite(scene, PIX));
        lv_obj_set_pos(bg, -1, -1);
        lv_obj_remove_flag(bg, LV_OBJ_FLAG_CLICKABLE);
    }
    return scr;
}

lv_obj_t *kit_img(lv_obj_t *parent, const wf_sprite *s, int scale, int x, int y) {
    lv_obj_t *img = lv_image_create(parent);
    lv_image_set_src(img, gfx_sprite(s, scale));
    lv_obj_remove_flag(img, LV_OBJ_FLAG_CLICKABLE);
    kit_pos(img, x, y);
    return img;
}

void kit_img_set(lv_obj_t *img, const wf_sprite *s, int scale) {
    const lv_image_dsc_t *d = gfx_sprite(s, scale);
    if (lv_image_get_src(img) != d) lv_image_set_src(img, d);
}

lv_obj_t *kit_label(lv_obj_t *parent, const lv_font_t *font, uint32_t color, const char *text, int x, int y) {
    lv_obj_t *l = lv_label_create(parent);
    lv_obj_set_style_text_font(l, font, 0);
    lv_obj_set_style_text_color(l, lv_color_hex(color), 0);
    lv_obj_set_style_text_align(l, LV_TEXT_ALIGN_CENTER, 0);
    lv_label_set_text(l, text);
    kit_pos(l, x, y);
    return l;
}

static void panel_style(lv_obj_t *o, uint32_t color, int opa) {
    lv_obj_remove_style_all(o);
    lv_obj_set_style_bg_color(o, lv_color_hex(color), 0);
    lv_obj_set_style_bg_opa(o, (lv_opa_t)opa, 0);
    lv_obj_set_style_border_color(o, lv_color_hex(COL_INK), 0);
    lv_obj_set_style_border_width(o, 3, 0);
    lv_obj_set_style_radius(o, 12, 0);
    lv_obj_remove_flag(o, LV_OBJ_FLAG_SCROLLABLE);
}

lv_obj_t *kit_pill(lv_obj_t *parent, const char *text, int x, int y) {
    lv_obj_t *p = lv_obj_create(parent);
    panel_style(p, COL_PANEL, 220);
    lv_obj_set_style_radius(p, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_pad_hor(p, 14, 0);
    lv_obj_set_style_pad_ver(p, 6, 0);
    lv_obj_set_size(p, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_remove_flag(p, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_t *l = lv_label_create(p);
    lv_obj_set_style_text_font(l, &wf_font_small, 0);
    lv_obj_set_style_text_color(l, lv_color_hex(COL_TEXT), 0);
    lv_label_set_text(l, text);
    kit_pos(p, x, y);
    return p;
}

lv_obj_t *kit_panel(lv_obj_t *parent, int x, int y, int w, int h) {
    lv_obj_t *p = lv_obj_create(parent);
    panel_style(p, COL_PANEL, 235);
    lv_obj_set_size(p, w, h);
    lv_obj_remove_flag(p, LV_OBJ_FLAG_CLICKABLE);
    kit_pos(p, x, y);
    return p;
}

static void button_style(lv_obj_t *b, uint32_t color) {
    lv_color_t base = lv_color_hex(color);
    lv_obj_remove_style_all(b);
    lv_obj_set_style_bg_opa(b, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(b, lv_color_lighten(base, 40), 0);
    lv_obj_set_style_bg_grad_color(b, base, 0);
    lv_obj_set_style_bg_grad_dir(b, LV_GRAD_DIR_VER, 0);
    lv_obj_set_style_border_color(b, lv_color_hex(COL_INK), 0);
    lv_obj_set_style_border_width(b, 3, 0);
    lv_obj_set_style_bg_color(b, lv_color_darken(base, 40), LV_STATE_PRESSED);
    lv_obj_set_style_bg_grad_color(b, lv_color_darken(base, 70), LV_STATE_PRESSED);
    lv_obj_set_style_translate_y(b, 2, LV_STATE_PRESSED);
    lv_obj_set_style_bg_color(b, lv_color_hex(0x4a4a5a), LV_STATE_DISABLED);
    lv_obj_set_style_bg_grad_color(b, lv_color_hex(0x34344a), LV_STATE_DISABLED);
    lv_obj_add_flag(b, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_remove_flag(b, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_ext_click_area(b, 8);
}

lv_obj_t *kit_round_button(lv_obj_t *parent, const wf_sprite *ic, const char *caption, int x, int y, int size,
                           uint32_t color, lv_event_cb_t cb, intptr_t user) {
    lv_obj_t *b = lv_obj_create(parent);
    button_style(b, color);
    lv_obj_set_style_radius(b, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_size(b, size, size);
    kit_pos(b, x, y);
    if (ic) {
        int scale = size >= 76 ? 3 : 2;
        lv_obj_t *img = lv_image_create(b);
        lv_image_set_src(img, gfx_sprite(ic, scale));
        lv_obj_center(img);
        lv_obj_remove_flag(img, LV_OBJ_FLAG_CLICKABLE);
    }
    if (caption) {
        lv_obj_t *l = kit_label(parent, &wf_font_small, COL_TEXT, caption, x, y + size / 2 + 13);
        lv_obj_set_style_bg_color(l, lv_color_hex(COL_PANEL), 0);
        lv_obj_set_style_bg_opa(l, 200, 0);
        lv_obj_set_style_pad_hor(l, 5, 0);
        lv_obj_set_style_pad_ver(l, 2, 0);
        lv_obj_set_style_radius(l, 6, 0);
    }
    if (cb) lv_obj_add_event_cb(b, cb, LV_EVENT_CLICKED, (void *)user);
    return b;
}

lv_obj_t *kit_text_button(lv_obj_t *parent, const char *text, int x, int y, int w, int h, uint32_t color,
                          lv_event_cb_t cb, intptr_t user) {
    lv_obj_t *b = lv_obj_create(parent);
    button_style(b, color);
    lv_obj_set_style_radius(b, 14, 0);
    lv_obj_set_size(b, w, h);
    kit_pos(b, x, y);
    lv_obj_t *l = lv_label_create(b);
    lv_obj_set_style_text_font(l, &wf_font_small, 0);
    lv_obj_set_style_text_color(l, lv_color_hex(COL_TEXT), 0);
    lv_label_set_text(l, text);
    lv_obj_center(l);
    if (cb) lv_obj_add_event_cb(b, cb, LV_EVENT_CLICKED, (void *)user);
    return b;
}

lv_obj_t *kit_bar(lv_obj_t *parent, int x, int y, int w, int h, uint32_t color, int value, int max) {
    lv_obj_t *bar = lv_bar_create(parent);
    lv_obj_remove_style_all(bar);
    lv_obj_set_size(bar, w, h);
    lv_obj_set_style_bg_color(bar, lv_color_hex(COL_INK), 0);
    lv_obj_set_style_bg_opa(bar, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(bar, h / 2, 0);
    lv_obj_set_style_pad_all(bar, 2, 0);
    lv_obj_set_style_bg_color(bar, lv_color_hex(color), LV_PART_INDICATOR);
    lv_obj_set_style_bg_opa(bar, LV_OPA_COVER, LV_PART_INDICATOR);
    lv_obj_set_style_radius(bar, h / 2, LV_PART_INDICATOR);
    lv_bar_set_range(bar, 0, max > 0 ? max : 1);
    lv_bar_set_value(bar, value, LV_ANIM_OFF);
    kit_pos(bar, x, y);
    return bar;
}

void kit_bar_set(lv_obj_t *bar, int value, int max, bool anim) {
    lv_bar_set_range(bar, 0, max > 0 ? max : 1);
    lv_obj_set_style_anim_duration(bar, 450, 0);
    lv_bar_set_value(bar, value, anim ? LV_ANIM_ON : LV_ANIM_OFF);
}

lv_obj_t *kit_arc(lv_obj_t *parent, int radius, int width, int start, int end, uint32_t color, int value, int max) {
    lv_obj_t *a = lv_arc_create(parent);
    lv_obj_remove_style_all(a);
    lv_obj_set_size(a, radius * 2, radius * 2);
    kit_pos(a, CX, CY);
    lv_arc_set_bg_angles(a, (lv_value_precise_t)start, (lv_value_precise_t)end);
    lv_arc_set_range(a, 0, max > 0 ? max : 1);
    lv_arc_set_value(a, value);
    lv_obj_set_style_arc_width(a, width, LV_PART_MAIN);
    lv_obj_set_style_arc_color(a, lv_color_hex(COL_INK), LV_PART_MAIN);
    lv_obj_set_style_arc_opa(a, 200, LV_PART_MAIN);
    lv_obj_set_style_arc_rounded(a, true, LV_PART_MAIN);
    lv_obj_set_style_arc_width(a, width - 6, LV_PART_INDICATOR);
    lv_obj_set_style_arc_color(a, lv_color_hex(color), LV_PART_INDICATOR);
    lv_obj_set_style_arc_rounded(a, true, LV_PART_INDICATOR);
    lv_obj_set_style_pad_all(a, 3, LV_PART_INDICATOR);
    lv_obj_remove_flag(a, LV_OBJ_FLAG_CLICKABLE);
    return a;
}

lv_obj_t *kit_dim(lv_obj_t *parent, int opa) {
    lv_obj_t *d = lv_obj_create(parent);
    lv_obj_remove_style_all(d);
    lv_obj_set_size(d, WF_RES, WF_RES);
    lv_obj_set_style_bg_color(d, lv_color_hex(0x05060f), 0);
    lv_obj_set_style_bg_opa(d, (lv_opa_t)opa, 0);
    lv_obj_add_flag(d, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_remove_flag(d, LV_OBJ_FLAG_SCROLLABLE);
    return d;
}

/* ---- Effects ---------------------------------------------------------------- */
static void anim_y(void *o, int32_t v) { lv_obj_set_style_translate_y(o, v, 0); }
static void anim_x(void *o, int32_t v) { lv_obj_set_style_translate_x(o, v, 0); }
static void anim_opa(void *o, int32_t v) { lv_obj_set_style_opa(o, (lv_opa_t)v, 0); }
static void anim_recolor(void *o, int32_t v) { lv_obj_set_style_image_recolor_opa(o, (lv_opa_t)v, 0); }
static void anim_delete(lv_anim_t *a) { lv_obj_delete_async(a->var); }

void kit_bob(lv_obj_t *obj, int amount, int period_ms) {
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, obj);
    lv_anim_set_exec_cb(&a, anim_y);
    lv_anim_set_values(&a, 0, -amount);
    lv_anim_set_duration(&a, (uint32_t)period_ms / 2);
    lv_anim_set_playback_duration(&a, (uint32_t)period_ms / 2);
    lv_anim_set_repeat_count(&a, LV_ANIM_REPEAT_INFINITE);
    lv_anim_set_path_cb(&a, lv_anim_path_ease_in_out);
    lv_anim_start(&a);
}

void kit_shake(lv_obj_t *obj, int amount) {
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, obj);
    lv_anim_set_exec_cb(&a, anim_x);
    lv_anim_set_values(&a, -amount, amount);
    lv_anim_set_duration(&a, 50);
    lv_anim_set_playback_duration(&a, 50);
    lv_anim_set_repeat_count(&a, 3);
    lv_anim_set_completed_cb(&a, NULL);
    lv_anim_start(&a);
}

void kit_flash(lv_obj_t *img, uint32_t color, int ms) {
    lv_obj_set_style_image_recolor(img, lv_color_hex(color), 0);
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, img);
    lv_anim_set_exec_cb(&a, anim_recolor);
    lv_anim_set_values(&a, 230, 0);
    lv_anim_set_duration(&a, (uint32_t)ms);
    lv_anim_start(&a);
}

void kit_fade_in(lv_obj_t *obj, int ms, int delay) {
    lv_obj_set_style_opa(obj, 0, 0);
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, obj);
    lv_anim_set_exec_cb(&a, anim_opa);
    lv_anim_set_values(&a, 0, 255);
    lv_anim_set_duration(&a, (uint32_t)ms);
    lv_anim_set_delay(&a, (uint32_t)delay);
    lv_anim_start(&a);
}

void kit_float_text(lv_obj_t *parent, const char *text, uint32_t color, const lv_font_t *font, int x, int y) {
    lv_obj_t *l = kit_label(parent, font, color, text, x, y);
    lv_obj_set_style_bg_color(l, lv_color_hex(COL_INK), 0);
    lv_obj_set_style_bg_opa(l, 170, 0);
    lv_obj_set_style_pad_hor(l, 6, 0);
    lv_obj_set_style_radius(l, 6, 0);
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, l);
    lv_anim_set_exec_cb(&a, anim_y);
    lv_anim_set_values(&a, 0, -46);
    lv_anim_set_duration(&a, 900);
    lv_anim_set_path_cb(&a, lv_anim_path_ease_out);
    lv_anim_start(&a);
    lv_anim_set_exec_cb(&a, anim_opa);
    lv_anim_set_values(&a, 255, 0);
    lv_anim_set_delay(&a, 600);
    lv_anim_set_duration(&a, 400);
    lv_anim_set_path_cb(&a, lv_anim_path_linear);
    lv_anim_set_completed_cb(&a, anim_delete);
    lv_anim_start(&a);
}

void kit_burst(lv_obj_t *parent, const wf_sprite *ic, int x, int y, int count) {
    for (int i = 0; i < count; i++) {
        lv_obj_t *img = kit_img(parent, ic, 2, x, y);
        int dx = (int)(wf_platform_random() % 120) - 60;
        int dy = -30 - (int)(wf_platform_random() % 60);
        lv_anim_t a;
        lv_anim_init(&a);
        lv_anim_set_var(&a, img);
        lv_anim_set_duration(&a, 700 + (uint32_t)(i * 60));
        lv_anim_set_path_cb(&a, lv_anim_path_ease_out);
        lv_anim_set_exec_cb(&a, anim_x);
        lv_anim_set_values(&a, 0, dx);
        lv_anim_start(&a);
        lv_anim_set_exec_cb(&a, anim_y);
        lv_anim_set_values(&a, 0, dy);
        lv_anim_start(&a);
        lv_anim_set_exec_cb(&a, anim_opa);
        lv_anim_set_values(&a, 255, 0);
        lv_anim_set_path_cb(&a, lv_anim_path_ease_in);
        lv_anim_set_completed_cb(&a, anim_delete);
        lv_anim_start(&a);
    }
}

static lv_obj_t *toast_obj;
static char last_toast[64];
const char *kit_last_toast(void) { return last_toast; }
static void toast_gone(lv_anim_t *a) {
    if (a->var == toast_obj) toast_obj = NULL;
    lv_obj_delete_async(a->var);
}

void kit_toast(const char *text, const wf_sprite *ic) {
    if (toast_obj && lv_obj_is_valid(toast_obj)) lv_obj_delete(toast_obj);
    lv_obj_t *p = lv_obj_create(lv_layer_top());
    panel_style(p, COL_PANEL, 240);
    lv_obj_set_style_radius(p, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_border_color(p, lv_color_hex(COL_GOLD), 0);
    lv_obj_set_style_pad_hor(p, 16, 0);
    lv_obj_set_style_pad_ver(p, 8, 0);
    lv_obj_set_style_pad_column(p, 8, 0);
    lv_obj_set_size(p, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(p, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(p, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_remove_flag(p, LV_OBJ_FLAG_CLICKABLE);
    if (ic) {
        lv_obj_t *img = lv_image_create(p);
        lv_image_set_src(img, gfx_sprite(ic, 2));
    }
    lv_obj_t *l = lv_label_create(p);
    lv_obj_set_style_text_font(l, &wf_font_small, 0);
    lv_obj_set_style_text_color(l, lv_color_hex(COL_TEXT), 0);
    lv_label_set_text(l, text);
    kit_pos(p, CX, 128);
    toast_obj = p;
    snprintf(last_toast, sizeof last_toast, "%s", text);
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, p);
    lv_anim_set_exec_cb(&a, anim_opa);
    lv_anim_set_values(&a, 255, 0);
    lv_anim_set_delay(&a, 1800);
    lv_anim_set_duration(&a, 350);
    lv_anim_set_completed_cb(&a, toast_gone);
    lv_anim_start(&a);
}

lv_obj_t *kit_dialog(const char *title, const char *body, const wf_sprite *ic) {
    lv_obj_t *dim = kit_dim(lv_layer_top(), 170);
    lv_obj_t *p = lv_obj_create(dim);
    panel_style(p, COL_PANEL, 250);
    lv_obj_set_style_border_color(p, lv_color_hex(COL_GOLD), 0);
    lv_obj_set_style_radius(p, 22, 0);
    lv_obj_set_width(p, 330);
    lv_obj_set_height(p, LV_SIZE_CONTENT);
    lv_obj_set_style_pad_all(p, 16, 0);
    lv_obj_set_style_pad_row(p, 10, 0);
    lv_obj_set_flex_flow(p, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(p, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_center(p);
    if (ic) {
        lv_obj_t *img = lv_image_create(p);
        lv_image_set_src(img, gfx_sprite(ic, ic->w > 16 ? 2 : 3));
    }
    if (title) {
        lv_obj_t *t = lv_label_create(p);
        lv_obj_set_style_text_font(t, &wf_font_big, 0);
        lv_obj_set_style_text_color(t, lv_color_hex(COL_GOLD), 0);
        lv_label_set_text(t, title);
    }
    if (body) {
        lv_obj_t *b = lv_label_create(p);
        lv_obj_set_style_text_font(b, &wf_font_small, 0);
        lv_obj_set_style_text_color(b, lv_color_hex(COL_TEXT), 0);
        lv_obj_set_style_text_align(b, LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_set_width(b, 290);
        lv_label_set_long_mode(b, LV_LABEL_LONG_WRAP);
        lv_label_set_text(b, body);
    }
    lv_obj_t *row = lv_obj_create(p);
    lv_obj_remove_style_all(row);
    lv_obj_set_size(row, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_set_flex_align(row, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_max_width(row, 298, 0);
    lv_obj_set_style_pad_column(row, 8, 0);
    lv_obj_set_style_pad_row(row, 8, 0);
    lv_obj_set_style_pad_top(row, 4, 0);
    lv_obj_set_user_data(dim, row);
    return dim;
}

lv_obj_t *kit_dialog_button(lv_obj_t *dialog, const char *text, uint32_t color, lv_event_cb_t cb, intptr_t user) {
    lv_obj_t *row = lv_obj_get_user_data(dialog);
    lv_obj_t *b = kit_text_button(row, text, 0, 0, 0, 50, color, cb, user);
    lv_obj_set_align(b, LV_ALIGN_DEFAULT);
    lv_obj_set_pos(b, 0, 0);
    lv_obj_set_width(b, LV_SIZE_CONTENT);
    lv_obj_set_style_min_width(b, 64, 0);
    lv_obj_set_style_pad_hor(b, 14, 0);
    return b;
}

void kit_dialog_close(lv_obj_t *dialog) {
    if (dialog && lv_obj_is_valid(dialog)) lv_obj_delete_async(dialog);
}


const char *fmt_duration(uint32_t s, char *buf, size_t cap) {
    if (s >= 3600) snprintf(buf, cap, "%luh %02lum", (unsigned long)(s / 3600), (unsigned long)(s / 60 % 60));
    else snprintf(buf, cap, "%lum %02lus", (unsigned long)(s / 60), (unsigned long)(s % 60));
    return buf;
}
