#pragma once
#include "lvgl.h"
#include "lantern.h"

typedef struct {
    lv_obj_t *layer, *slashes[2], *rings[2], *motes[8], *pips[3], *target;
    lv_point_precise_t strokes[2][3];
    int foe_x[2], foe_y[2];
} lantern_fx;

enum { LN_HERO_X = 126, LN_HERO_Y = 201, LN_POSE_CONTACT_MS = 150, LN_POSE_RECOVERY_END_MS = 320 };

/* One shared layout for rendering, effects and non-overlapping touch boxes. */
static inline int ln_foe_x(const rpg_lantern *l, int i) { return l->nfoes == 2 ? 270 + i*100 : 316; }
static inline int ln_foe_y(const rpg_lantern *l, int i) {
    return l->nfoes == 2 ? 220 + i*16 : l->foes[i].kind == LN_BOSS ? 198 : 212;
}
static inline int ln_foe_scale(const rpg_lantern *l) { return l->nfoes == 2 ? 1 : 2; }

void lantern_fx_create(lantern_fx *fx, lv_obj_t *parent, const rpg_lantern *l);
void lantern_fx_update(lantern_fx *fx, const rpg_lantern *l, lv_obj_t *hero, lv_obj_t *foes[2], bool preparing);
