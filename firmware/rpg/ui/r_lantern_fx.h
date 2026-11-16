#pragma once
#include "lvgl.h"
#include "lantern.h"

typedef struct {
    lv_obj_t *layer, *slashes[2], *rings[2], *motes[8], *pips[3], *target;
    lv_point_precise_t strokes[2][3];
    int foe_x[2], foe_y[2];
} lantern_fx;

void lantern_fx_create(lantern_fx *fx, lv_obj_t *parent, const rpg_lantern *l);
void lantern_fx_update(lantern_fx *fx, const rpg_lantern *l, lv_obj_t *hero, lv_obj_t *foes[2], bool preparing);
