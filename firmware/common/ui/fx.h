#pragma once
/* HD-2D style effects layered over pixel-art scenes: additive lights,
 * ambient particles, soft shadows, sparks, flashes and camera shake.
 * Everything is generated at runtime (no art) and tied to the active screen. */
#include "kit.h"

enum {
    FX_NONE = 0,
    FX_POLLEN,      /* drifts up and sways          */
    FX_SPARKLE,     /* twinkles in place             */
    FX_FIREFLY,     /* wanders and pulses            */
    FX_EMBER,       /* rises quickly                 */
    FX_STARDUST,    /* falls slowly                  */
    FX_DUST,        /* floats lazily                 */
};

typedef struct {
    int16_t x, y;       /* screen pixels */
    uint16_t r;
    uint32_t color;
    uint8_t flicker;    /* 0 steady, 1 fire, 2 slow pulse */
} fx_light;

/* Adds lights and particles to `scr`; they stop when the screen is deleted. */
void fx_scene(lv_obj_t *scr, const fx_light *lights, int nlights, int particles, uint32_t particle_color, int count);
lv_obj_t *fx_glow(lv_obj_t *parent, int x, int y, int r, uint32_t color, int opa);
lv_obj_t *fx_shadow(lv_obj_t *parent, int x, int y, int w);
void fx_sparks(lv_obj_t *parent, int x, int y, uint32_t color, int count);
void fx_flash(uint32_t color, int opa, int ms);
void fx_shake(lv_obj_t *obj, int strength, int ms);
