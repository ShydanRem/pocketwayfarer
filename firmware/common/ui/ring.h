#pragma once
/* Timing widgets drawn around the round display. */
#include "kit.h"
#include "timing.h"

typedef struct {
    lv_obj_t *root, *spark, *zones[3];
    wf_ring ring;
    uint32_t start;
    bool active;
} ring_widget;

typedef struct {
    lv_obj_t *root, *wave, *guard;
    wf_parry parry;
    uint32_t start;
    bool active;
    uint32_t color;
} parry_widget;

/* Both widgets cover the whole screen and capture taps (LV_EVENT_PRESSED is
 * forwarded to `cb`). */
void ring_open(ring_widget *w, lv_obj_t *parent, const wf_ring *r, lv_event_cb_t cb);
void ring_update(ring_widget *w);
int ring_judge(ring_widget *w);
bool ring_expired(const ring_widget *w);
void ring_close(ring_widget *w);

void parry_open(parry_widget *w, lv_obj_t *parent, const wf_parry *p, uint32_t color, const wf_sprite *shield,
                lv_event_cb_t cb);
void parry_update(parry_widget *w);
int parry_judge(parry_widget *w);
bool parry_expired(const parry_widget *w);
void parry_close(parry_widget *w);

void quality_popup(lv_obj_t *parent, int quality, int x, int y);
