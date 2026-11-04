#include <math.h>
#include "ring.h"

#define RING_R 206

static lv_obj_t *zone_arc(lv_obj_t *parent, int center, int half, int width, uint32_t color) {
    lv_obj_t *a = lv_arc_create(parent);
    lv_obj_remove_style_all(a);
    lv_obj_set_size(a, RING_R * 2 + width, RING_R * 2 + width);
    lv_obj_center(a);
    /* wf_ring angles start at 12 o'clock; LVGL starts at 3 o'clock. */
    int s = center - half - 90, e = center + half - 90;
    lv_arc_set_bg_angles(a, (lv_value_precise_t)((s % 360 + 360) % 360), (lv_value_precise_t)((e % 360 + 360) % 360));
    lv_obj_set_style_arc_width(a, width, LV_PART_MAIN);
    lv_obj_set_style_arc_color(a, lv_color_hex(color), LV_PART_MAIN);
    lv_obj_set_style_arc_rounded(a, false, LV_PART_MAIN);
    lv_obj_set_style_opa(a, 0, LV_PART_INDICATOR);
    lv_obj_remove_flag(a, LV_OBJ_FLAG_CLICKABLE);
    return a;
}

static lv_obj_t *catcher(lv_obj_t *parent, lv_event_cb_t cb, void *user) {
    lv_obj_t *root = lv_obj_create(parent);
    lv_obj_remove_style_all(root);
    lv_obj_set_size(root, WF_RES, WF_RES);
    lv_obj_add_flag(root, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_remove_flag(root, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(root, cb, LV_EVENT_PRESSED, user);
    return root;
}

void ring_open(ring_widget *w, lv_obj_t *parent, const wf_ring *r, lv_event_cb_t cb) {
    w->ring = *r;
    w->root = catcher(parent, cb, w);
    lv_obj_t *track = lv_arc_create(w->root);
    lv_obj_remove_style_all(track);
    lv_obj_set_size(track, RING_R * 2 + 30, RING_R * 2 + 30);
    lv_obj_center(track);
    lv_arc_set_bg_angles(track, 0, 360);
    lv_obj_set_style_arc_width(track, 30, LV_PART_MAIN);
    lv_obj_set_style_arc_color(track, lv_color_hex(COL_INK), LV_PART_MAIN);
    lv_obj_set_style_arc_opa(track, 210, LV_PART_MAIN);
    lv_obj_set_style_opa(track, 0, LV_PART_INDICATOR);
    lv_obj_remove_flag(track, LV_OBJ_FLAG_CLICKABLE);
    w->zones[0] = zone_arc(w->root, r->center, r->good, 24, 0x3a8a5a);
    w->zones[1] = zone_arc(w->root, r->center, r->great, 24, 0x6ce06a);
    w->zones[2] = zone_arc(w->root, r->center, r->perfect, 24, COL_GOLD);
    w->spark = lv_obj_create(w->root);
    lv_obj_remove_style_all(w->spark);
    lv_obj_set_size(w->spark, 30, 30);
    lv_obj_set_style_radius(w->spark, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(w->spark, lv_color_white(), 0);
    lv_obj_set_style_bg_opa(w->spark, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(w->spark, lv_color_hex(0xff6a3a), 0);
    lv_obj_set_style_border_width(w->spark, 5, 0);
    lv_obj_remove_flag(w->spark, LV_OBJ_FLAG_CLICKABLE);
    w->start = lv_tick_get();
    w->active = true;
    ring_update(w);
}

void ring_update(ring_widget *w) {
    if (!w->active) return;
    int a = wf_ring_angle(&w->ring, lv_tick_get() - w->start);
    float rad = (float)(a - 90) * 3.14159265f / 180.0f;
    int x = CX + (int)lroundf(RING_R * cosf(rad)), y = CY + (int)lroundf(RING_R * sinf(rad));
    lv_obj_set_pos(w->spark, x - 15, y - 15);
}

int ring_judge(ring_widget *w) { return wf_ring_judge(&w->ring, lv_tick_get() - w->start); }
bool ring_expired(const ring_widget *w) { return w->active && wf_ring_expired(&w->ring, lv_tick_get() - w->start); }

void ring_close(ring_widget *w) {
    if (w->root) lv_obj_delete(w->root);
    w->root = NULL;
    w->active = false;
}

void parry_open(parry_widget *w, lv_obj_t *parent, const wf_parry *p, uint32_t color, lv_event_cb_t cb) {
    w->parry = *p;
    w->color = color;
    w->root = catcher(parent, cb, w);
    w->guard = lv_obj_create(w->root);
    lv_obj_remove_style_all(w->guard);
    lv_obj_set_size(w->guard, 150, 150);
    lv_obj_center(w->guard);
    lv_obj_set_style_radius(w->guard, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_border_color(w->guard, lv_color_hex(0xdfe8ff), 0);
    lv_obj_set_style_border_width(w->guard, 4, 0);
    lv_obj_set_style_border_opa(w->guard, 200, 0);
    lv_obj_set_style_bg_color(w->guard, lv_color_hex(0x5aa0ff), 0);
    lv_obj_set_style_bg_opa(w->guard, 50, 0);
    lv_obj_remove_flag(w->guard, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_t *ic = lv_image_create(w->guard);
    lv_image_set_src(ic, gfx_sprite(icon(WF_ICON_SHIELD), 3));
    lv_obj_center(ic);
    w->wave = lv_obj_create(w->root);
    lv_obj_remove_style_all(w->wave);
    lv_obj_set_style_radius(w->wave, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_border_color(w->wave, lv_color_hex(color), 0);
    lv_obj_set_style_border_width(w->wave, 10, 0);
    lv_obj_remove_flag(w->wave, LV_OBJ_FLAG_CLICKABLE);
    w->start = lv_tick_get();
    w->active = true;
    parry_update(w);
}

void parry_update(parry_widget *w) {
    if (!w->active) return;
    int p = wf_parry_progress(&w->parry, lv_tick_get() - w->start);
    /* Radius goes from 230 (rim) to 75 (guard circle) at p=1000, then keeps closing. */
    int r = 230 - (230 - 75) * p / 1000;
    if (r < 8) r = 8;
    lv_obj_set_size(w->wave, r * 2, r * 2);
    lv_obj_center(w->wave);
}

int parry_judge(parry_widget *w) { return wf_parry_judge(&w->parry, lv_tick_get() - w->start); }

bool parry_expired(const parry_widget *w) {
    return w->active && lv_tick_get() - w->start > (uint32_t)w->parry.travel_ms + w->parry.good_ms + 60u;
}

void parry_close(parry_widget *w) {
    if (w->root) lv_obj_delete(w->root);
    w->root = NULL;
    w->active = false;
}

void quality_popup(lv_obj_t *parent, int q, int x, int y) {
    static const char *const words[] = {"Mancato", "Bene", "Ottimo!", "PERFETTO!"};
    static const uint32_t colors[] = {COL_DIM, COL_TEXT, COL_HP, COL_GOLD};
    kit_float_text(parent, words[q & 3], colors[q & 3], q == 3 ? &wf_font_huge : &wf_font_big, x, y);
}
