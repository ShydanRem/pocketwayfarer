#pragma once
#include "rui.h"
bool hero_has_combat_rig(const hero_look *look);
/* Returns opaque pixels clipped by the logical frame (zero for admitted art). */
unsigned hero_rig_compose(const hero_look *look, int expr, int pose, uint16_t *rgb, uint8_t *alpha);
