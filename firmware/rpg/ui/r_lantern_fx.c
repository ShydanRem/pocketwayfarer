/* Original procedural effects. No independent timers: saved phase time drives
 * all motion, so pausing/reloading cannot consume an action or replay a cue. */
#include <math.h>
#include <string.h>
#include "kit.h"
#include "timing.h"
#include "r_lantern_fx.h"

#define AMBER 0xe6b461
#define IVORY 0xeee7d6
#define JADE 0x7ba994
#define EMBER 0xe28c51
#define DANGER 0xd06b5d

static void visible(lv_obj_t *o, bool on) {
    if (on) lv_obj_remove_flag(o, LV_OBJ_FLAG_HIDDEN);
    else lv_obj_add_flag(o, LV_OBJ_FLAG_HIDDEN);
}
static lv_obj_t *shape(lv_obj_t *p, int size, bool ring) {
    lv_obj_t *o = lv_obj_create(p); lv_obj_remove_style_all(o);
    lv_obj_set_size(o, size, size);
    lv_obj_remove_flag(o, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    if (ring) {
        lv_obj_set_style_radius(o, LV_RADIUS_CIRCLE, 0);
        lv_obj_set_style_border_width(o, 3, 0);
    } else lv_obj_set_style_bg_opa(o, 255, 0);
    return o;
}
void lantern_fx_create(lantern_fx *fx, lv_obj_t *parent, const rpg_lantern *l) {
    memset(fx, 0, sizeof *fx);
    fx->layer = lv_obj_create(parent); lv_obj_remove_style_all(fx->layer);
    lv_obj_set_size(fx->layer, WF_RES, WF_RES); lv_obj_set_pos(fx->layer, 0, 0);
    lv_obj_remove_flag(fx->layer, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    for (int i = 0; i < l->nfoes; i++) {
        fx->foe_x[i] = ln_foe_x(l, i);
        fx->foe_y[i] = ln_foe_y(l, i);
    }
    for (int i = 0; i < 2; i++) {
        fx->slashes[i] = lv_line_create(fx->layer);
        lv_obj_remove_flag(fx->slashes[i], LV_OBJ_FLAG_CLICKABLE);
        lv_obj_set_style_line_width(fx->slashes[i], i ? 3 : 5, 0);
        lv_obj_set_style_line_rounded(fx->slashes[i], true, 0);
        fx->rings[i] = shape(fx->layer, 72 + i * 18, true);
    }
    for (int i = 0; i < 8; i++) fx->motes[i] = shape(fx->layer, i % 3 ? 4 : 6, false);
    for (int i = 0; i < 3; i++) {
        fx->pips[i] = shape(fx->layer, 8, false);
        kit_pos(fx->pips[i], 211 + i * 22, 438);
        lv_obj_set_style_radius(fx->pips[i], 2, 0);
        lv_obj_set_style_border_width(fx->pips[i], 1, 0);
        lv_obj_set_style_border_color(fx->pips[i], lv_color_hex(JADE), 0);
    }
    fx->target = shape(fx->layer, 64, true);
    lv_obj_set_size(fx->target, 64, 14);
    lv_obj_set_style_radius(fx->target, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_border_width(fx->target, 2, 0);
    lv_obj_set_style_border_color(fx->target, lv_color_hex(IVORY), 0);
}
static void ring(lv_obj_t *o, int x, int y, int size, uint32_t color, int opacity) {
    lv_obj_set_size(o, size, size); kit_pos(o, x, y);
    lv_obj_set_style_border_color(o, lv_color_hex(color), 0);
    lv_obj_set_style_opa(o, opacity, 0); visible(o, true);
}
static void slash(lantern_fx *fx, int i, int x, int y, bool reverse, uint32_t color, int opacity) {
    fx->strokes[i][0] = (lv_point_precise_t){0, reverse ? 0 : 58};
    fx->strokes[i][1] = (lv_point_precise_t){27, 28};
    fx->strokes[i][2] = (lv_point_precise_t){62, reverse ? 58 : 0};
    lv_line_set_points(fx->slashes[i], fx->strokes[i], 3);
    lv_obj_set_pos(fx->slashes[i], x - 31, y - 29);
    lv_obj_set_style_line_color(fx->slashes[i], lv_color_hex(color), 0);
    lv_obj_set_style_opa(fx->slashes[i], opacity, 0); visible(fx->slashes[i], true);
}
void lantern_fx_update(lantern_fx *fx, const rpg_lantern *l, lv_obj_t *hero, lv_obj_t *foes[2], bool preparing) {
    if (!fx->layer) return;
    for (int i = 0; i < 2; i++) { visible(fx->slashes[i], false); visible(fx->rings[i], false); }
    for (int i = 0; i < 8; i++) visible(fx->motes[i], false);
    for (int i = 0; i < 3; i++) {
        lv_obj_set_style_bg_color(fx->pips[i], lv_color_hex(i < l->resonance ? l->resonance == 3 ? AMBER : JADE : 0x203b40), 0);
    }
    bool target_visible = l->nfoes == 2 && l->state == LN_CHOOSE && !l->paused && !preparing;
    visible(fx->target, target_visible);
    if (target_visible) {
        kit_pos(fx->target, fx->foe_x[l->target], fx->foe_y[l->target] + 40);
        lv_obj_set_style_bg_color(fx->target, lv_color_hex(IVORY), 0);
    }
    int hero_dx = 0, foe_dx[2] = {0, 0};
    int actor = l->effect_actor < l->nfoes ? l->effect_actor : 0;
    float p = fminf(1.0f, l->elapsed_ms / 420.0f);
    bool impact = l->state == LN_IMPACT;
    bool hero_hit = impact && l->feedback == LN_FEEDBACK_HERO;
    bool enemy_hit = impact && l->feedback == LN_FEEDBACK_FOE;
    bool empowered = hero_hit && (l->effect_flags & LN_EFFECT_RESONANCE);
    bool perfect = enemy_hit && l->quality == WF_Q_PERFECT;
    uint32_t color = hero_hit ? l->archetype == LN_MAGE ? EMBER : l->archetype == LN_ROGUE ? JADE : AMBER
                             : perfect || l->feedback == LN_FEEDBACK_HEAL ? JADE : enemy_hit && !l->last_damage ? AMBER : DANGER;
    /* Freeze at the exact saved phase pose behind pause/preparation panels. */
    if (impact && !l->reduced_motion) {
        int kick = (int)lroundf(sinf(p * 3.14159265f) * (l->action == LN_POWER ? 18 : 12));
        if (hero_hit) { hero_dx = kick; foe_dx[actor] = kick / 2; }
        else if (enemy_hit) { hero_dx = perfect ? 0 : -kick / 2; foe_dx[actor] = -kick; }
    }
    /* A paused/preparation scene has no moving effects over the pause surface. */
    if (!l->paused && !preparing && impact) {
        int x = hero_hit || l->feedback == LN_FEEDBACK_HEAL || l->feedback == LN_FEEDBACK_BURN ? fx->foe_x[actor] : LN_HERO_X;
        int y = hero_hit || l->feedback == LN_FEEDBACK_HEAL || l->feedback == LN_FEEDBACK_BURN ? fx->foe_y[actor] : LN_HERO_Y;
        int opacity = l->reduced_motion ? 210 : (int)(245 * (1.0f - p * .8f));
        if (hero_hit) {
            if (l->archetype == LN_MAGE) {
                ring(fx->rings[0], x, y, l->reduced_motion ? 52 : 32 + (int)(p * 60), EMBER, opacity);
                ring(fx->rings[1], x, y, 24, IVORY, opacity);
            } else {
                slash(fx, 0, x, y, false, color, opacity);
                if (l->archetype == LN_ROGUE) slash(fx, 1, x + 6, y + 4, true, IVORY, opacity);
                else slash(fx, 1, x + 4, y, false, IVORY, opacity);
            }
            if (empowered) ring(fx->rings[1], x, y, l->reduced_motion ? 90 : 76 + (int)(p * 40), AMBER, opacity);
        } else if (perfect || !l->last_damage || l->feedback == LN_FEEDBACK_HEAL) {
            ring(fx->rings[0], x, y, l->reduced_motion ? 70 : 62 + (int)(p * 20), color, opacity);
            if (perfect) ring(fx->rings[1], x, y, 46, IVORY, opacity);
        } else slash(fx, 0, x, y, true, l->feedback == LN_FEEDBACK_BURN ? EMBER : DANGER, opacity);
        if (!l->reduced_motion) for (int i = 0; i < 8; i++) {
            float a = i * .78539816f;
            int radius = 18 + (int)(p * (empowered ? 46 : 30));
            kit_pos(fx->motes[i], x + (int)lroundf(cosf(a) * radius), y + (int)lroundf(sinf(a) * radius));
            lv_obj_set_style_bg_color(fx->motes[i], lv_color_hex(i & 1 ? IVORY : color), 0);
            lv_obj_set_style_opa(fx->motes[i], opacity, 0); visible(fx->motes[i], true);
        }
    } else if (!l->paused && !preparing && l->state == LN_TELEGRAPH && l->acting == 0 &&
               (l->action == LN_GUARD || l->action == LN_SUPPORT) && l->elapsed_ms < 320) {
        ring(fx->rings[0], LN_HERO_X, LN_HERO_Y, l->reduced_motion ? 70 : 56 + l->elapsed_ms / 10, JADE, 180);
    }
    kit_pos(hero, LN_HERO_X + hero_dx, LN_HERO_Y);
    lv_obj_set_style_image_recolor(hero, lv_color_hex(perfect ? JADE : DANGER), 0);
    lv_obj_set_style_image_recolor_opa(hero, enemy_hit && l->last_damage && !l->paused && !preparing ? l->reduced_motion ? 30 : (int)(70 * (1 - p)) : 0, 0);
    for (int i = 0; i < l->nfoes; i++) if (foes[i]) {
        kit_pos(foes[i], fx->foe_x[i] + foe_dx[i], fx->foe_y[i]);
        int tint = hero_hit && i == actor && !l->paused && !preparing ? l->reduced_motion ? 30 : (int)(95 * (1 - p)) : 0;
        lv_obj_set_style_image_recolor(foes[i], lv_color_hex(IVORY), 0);
        lv_obj_set_style_image_recolor_opa(foes[i], tint, 0);
        lv_obj_set_style_image_opa(foes[i], !l->foes[i].hp && impact && i == actor && !l->reduced_motion ? (int)(255 * (1 - p)) : 255, 0);
    }
}
