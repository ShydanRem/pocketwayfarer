#pragma once
#include <stdint.h>
/* 8-bit indexed sprite; with `alpha` set, index 0 is transparent. */
typedef struct {
    uint8_t w, h;
    uint8_t alpha;
    uint16_t colors;
    const uint16_t *pal;   /* RGB565 */
    const uint8_t *px;
} wf_sprite;
