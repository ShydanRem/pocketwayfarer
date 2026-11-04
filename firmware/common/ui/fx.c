#include <math.h>
#include <string.h>
#include "fx.h"

/* ---- Generated images ---------------------------------------------------------- */
typedef struct {
    uint16_t w, h;
    uint32_t color;
    uint8_t kind;           /* 0 glow, 1 shadow */
    lv_image_dsc_t dsc;
    uint8_t *buf;
} fx_img;

#define FX_IMGS 32
static fx_img imgs[FX_IMGS];

static const lv_image_dsc_t *make(int kind, int w, int h, uint32_t color) {
    for (int i = 0; i < FX_IMGS; i++)
        if (imgs[i].buf && imgs[i].kind == kind && imgs[i].w == w && imgs[i].h == h && imgs[i].color == color)
            return &imgs[i].dsc;
    fx_img *e = NULL;
    for (int i = 0; i < FX_IMGS && !e; i++) if (!imgs[i].buf) e = &imgs[i];
    if (!e) e = &imgs[(unsigned)(w * 7 + h + color) % FX_IMGS];     /* full: recycle (rare) */
    if (e->buf) wf_platform_big_free(e->buf);
    size_t px = (size_t)w * (size_t)h;
    e->buf = wf_platform_big_alloc(px * 3);
    uint16_t *rgb = (uint16_t *)e->buf;
    uint8_t *a = e->buf + px * 2;
    lv_color_t c = lv_color_hex(color);
    uint16_t c565 = (uint16_t)(((c.red >> 3) << 11) | ((c.green >> 2) << 5) | (c.blue >> 3));
    float cx = (w - 1) / 2.0f, cy = (h - 1) / 2.0f;
    for (int y = 0; y < h; y++)
        for (int x = 0; x < w; x++) {
            float dx = (x - cx) / (w / 2.0f), dy = (y - cy) / (h / 2.0f);
            float d = sqrtf(dx * dx + dy * dy);
            float v = d >= 1 ? 0 : (1 - d);
            int alpha = kind == 0 ? (int)(v * v * 255) : (int)(fminf(1, v * 1.8f) * 150);
            rgb[y * w + x] = kind == 0 ? c565 : 0;
            a[y * w + x] = (uint8_t)alpha;
        }
    memset(&e->dsc, 0, sizeof e->dsc);
    e->dsc.header.magic = LV_IMAGE_HEADER_MAGIC;
    e->dsc.header.cf = LV_COLOR_FORMAT_RGB565A8;
    e->dsc.header.w = (uint32_t)w;
    e->dsc.header.h = (uint32_t)h;
    e->dsc.header.stride = (uint32_t)w * 2u;
    e->dsc.data = e->buf;
    e->dsc.data_size = (uint32_t)(px * 3);
    e->w = (uint16_t)w;
    e->h = (uint16_t)h;
    e->color = color;
    e->kind = (uint8_t)kind;
    return &e->dsc;
}

lv_obj_t *fx_glow(lv_obj_t *parent, int x, int y, int r, uint32_t color, int opa) {
    if (r < 2) r = 2;
    lv_obj_t *img = lv_image_create(parent);
    lv_image_set_src(img, make(0, r * 2, r * 2, color));
    lv_obj_set_style_opa(img, (lv_opa_t)opa, 0);
    lv_obj_remove_flag(img, LV_OBJ_FLAG_CLICKABLE);
    kit_pos(img, x, y);
    return img;
}

lv_obj_t *fx_shadow(lv_obj_t *parent, int x, int y, int w) {
    lv_obj_t *img = lv_image_create(parent);
    lv_image_set_src(img, make(1, w, w / 4 > 4 ? w / 4 : 4, 0));
    lv_obj_remove_flag(img, LV_OBJ_FLAG_CLICKABLE);
    kit_pos(img, x, y);
    return img;
}

/* ---- Ambient scene ---------------------------------------------------------------- */
#define MAX_P 22
#define MAX_L 12
typedef struct {
    lv_obj_t *obj;
    float x, y, vx, vy, phase, speed;
    uint8_t size;
} particle;

typedef struct {
    lv_timer_t *timer;
    lv_obj_t *scr;
    int kind;
    particle p[MAX_P];
    int np;
    lv_obj_t *light[MAX_L];
    uint8_t flicker[MAX_L];
    uint8_t base_opa[MAX_L];
    int nl;
    uint32_t t;
} ambient;

static ambient amb;

static float frand(void) { return (float)(wf_platform_random() % 10000) / 10000.0f; }

static void respawn(particle *q, int kind, bool anywhere) {
    q->x = 30 + frand() * (WF_RES - 60);
    switch (kind) {
    case FX_EMBER: q->y = anywhere ? 120 + frand() * 330 : WF_RES - 20; q->vx = (frand() - 0.5f) * 0.6f; q->vy = -0.9f - frand(); break;
    case FX_STARDUST: q->y = anywhere ? frand() * 400 : 10; q->vx = (frand() - 0.5f) * 0.3f; q->vy = 0.25f + frand() * 0.3f; break;
    case FX_POLLEN: q->y = anywhere ? 60 + frand() * 380 : WF_RES - 30; q->vx = (frand() - 0.5f) * 0.3f; q->vy = -0.18f - frand() * 0.2f; break;
    default: q->y = 60 + frand() * 360; q->vx = (frand() - 0.5f) * 0.4f; q->vy = (frand() - 0.5f) * 0.3f; break;
    }
    q->phase = frand() * 6.28f;
    q->speed = 0.6f + frand();
}

static void ambient_tick(lv_timer_t *t) {
    (void)t;
    amb.t += 1;
    for (int i = 0; i < amb.np; i++) {
        particle *q = &amb.p[i];
        q->phase += 0.06f * q->speed;
        q->x += q->vx + sinf(q->phase) * (amb.kind == FX_FIREFLY ? 0.7f : 0.25f);
        q->y += q->vy;
        if (q->y < -10 || q->y > WF_RES + 10 || q->x < -10 || q->x > WF_RES + 10) respawn(q, amb.kind, false);
        int opa;
        switch (amb.kind) {
        case FX_SPARKLE: opa = (int)(fmaxf(0, sinf(q->phase * 2)) * 255); break;
        case FX_FIREFLY: opa = 90 + (int)((sinf(q->phase * 1.5f) + 1) * 80); break;
        case FX_EMBER: opa = 120 + (int)(sinf(q->phase * 3) * 100); break;
        default: opa = 110 + (int)(sinf(q->phase) * 70); break;
        }
        lv_obj_set_pos(q->obj, (int)q->x - q->size / 2, (int)q->y - q->size / 2);
        lv_obj_set_style_opa(q->obj, (lv_opa_t)(opa < 0 ? 0 : opa > 255 ? 255 : opa), 0);
    }
    for (int i = 0; i < amb.nl; i++) {
        if (!amb.flicker[i]) continue;
        int o = amb.base_opa[i];
        if (amb.flicker[i] == 1) o = o - 40 + (int)(wf_platform_random() % 60);
        else o = o + (int)(sinf((float)amb.t * 0.05f + (float)i) * 50);
        lv_obj_set_style_opa(amb.light[i], (lv_opa_t)(o < 0 ? 0 : o > 255 ? 255 : o), 0);
    }
}

static void ambient_stop(lv_event_t *e) {
    (void)e;
    if (amb.timer) lv_timer_delete(amb.timer);
    memset(&amb, 0, sizeof amb);
}

void fx_scene(lv_obj_t *scr, const fx_light *lights, int nlights, int kind, uint32_t color, int count) {
    if (amb.timer) lv_timer_delete(amb.timer);
    memset(&amb, 0, sizeof amb);
    amb.scr = scr;
    amb.kind = kind;
    for (int i = 0; i < nlights && i < MAX_L; i++) {
        const fx_light *l = &lights[i];
        amb.base_opa[i] = l->flicker == 1 ? 200 : 170;
        amb.light[i] = fx_glow(scr, l->x, l->y, l->r, l->color, amb.base_opa[i]);
        amb.flicker[i] = l->flicker;
        amb.nl++;
    }
    if (kind != FX_NONE) {
        amb.np = count > MAX_P ? MAX_P : count;
        for (int i = 0; i < amb.np; i++) {
            particle *q = &amb.p[i];
            q->size = (uint8_t)(kind == FX_FIREFLY ? 12 : kind == FX_SPARKLE ? 8 : 6 + wf_platform_random() % 5);
            q->obj = lv_image_create(scr);
            lv_image_set_src(q->obj, make(0, q->size, q->size, color));
            lv_obj_remove_flag(q->obj, LV_OBJ_FLAG_CLICKABLE);
            respawn(q, kind, true);
        }
    }
    if (amb.np || amb.nl) {
        amb.timer = lv_timer_create(ambient_tick, 40, NULL);
        lv_obj_add_event_cb(scr, ambient_stop, LV_EVENT_DELETE, NULL);
        ambient_tick(NULL);
    }
}

/* ---- One-shot effects ----------------------------------------------------------------- */
static void anim_x(void *o, int32_t v) { lv_obj_set_style_translate_x(o, v, 0); }
static void anim_y(void *o, int32_t v) { lv_obj_set_style_translate_y(o, v, 0); }
static void anim_opa(void *o, int32_t v) { lv_obj_set_style_opa(o, (lv_opa_t)v, 0); }
static void anim_del(lv_anim_t *a) { lv_obj_delete_async(a->var); }

void fx_sparks(lv_obj_t *parent, int x, int y, uint32_t color, int count) {
    for (int i = 0; i < count; i++) {
        int s = 8 + (int)(wf_platform_random() % 8);
        lv_obj_t *img = lv_image_create(parent);
        lv_image_set_src(img, make(0, s, s, color));
        lv_obj_remove_flag(img, LV_OBJ_FLAG_CLICKABLE);
        kit_pos(img, x, y);
        float a = frand() * 6.283f, d = 26 + frand() * 44;
        lv_anim_t an;
        lv_anim_init(&an);
        lv_anim_set_var(&an, img);
        lv_anim_set_duration(&an, 320 + (uint32_t)(frand() * 200));
        lv_anim_set_path_cb(&an, lv_anim_path_ease_out);
        lv_anim_set_exec_cb(&an, anim_x);
        lv_anim_set_values(&an, 0, (int32_t)(cosf(a) * d));
        lv_anim_start(&an);
        lv_anim_set_exec_cb(&an, anim_y);
        lv_anim_set_values(&an, 0, (int32_t)(sinf(a) * d));
        lv_anim_start(&an);
        lv_anim_set_exec_cb(&an, anim_opa);
        lv_anim_set_values(&an, 255, 0);
        lv_anim_set_path_cb(&an, lv_anim_path_ease_in);
        lv_anim_set_completed_cb(&an, anim_del);
        lv_anim_start(&an);
    }
    lv_obj_t *burst = fx_glow(parent, x, y, 40, color, 230);
    lv_anim_t b;
    lv_anim_init(&b);
    lv_anim_set_var(&b, burst);
    lv_anim_set_exec_cb(&b, anim_opa);
    lv_anim_set_values(&b, 230, 0);
    lv_anim_set_duration(&b, 280);
    lv_anim_set_completed_cb(&b, anim_del);
    lv_anim_start(&b);
}

void fx_flash(uint32_t color, int opa, int ms) {
    lv_obj_t *f = lv_obj_create(lv_layer_top());
    lv_obj_remove_style_all(f);
    lv_obj_set_size(f, WF_RES, WF_RES);
    lv_obj_set_style_bg_color(f, lv_color_hex(color), 0);
    lv_obj_set_style_bg_opa(f, LV_OPA_COVER, 0);
    lv_obj_remove_flag(f, LV_OBJ_FLAG_CLICKABLE);
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, f);
    lv_anim_set_exec_cb(&a, anim_opa);
    lv_anim_set_values(&a, opa, 0);
    lv_anim_set_duration(&a, (uint32_t)ms);
    lv_anim_set_completed_cb(&a, anim_del);
    lv_anim_start(&a);
}

static int shake_strength;
static void shake_exec(void *o, int32_t v) {
    /* v runs 1000 -> 0: decaying wobble */
    int s = shake_strength * v / 1000;
    lv_obj_set_style_translate_x(o, (int32_t)(sinf((float)v * 0.09f) * (float)s), 0);
    lv_obj_set_style_translate_y(o, (int32_t)(cosf((float)v * 0.13f) * (float)s * 0.6f), 0);
}

void fx_shake(lv_obj_t *obj, int strength, int ms) {
    shake_strength = strength;
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, obj);
    lv_anim_set_exec_cb(&a, shake_exec);
    lv_anim_set_values(&a, 1000, 0);
    lv_anim_set_duration(&a, (uint32_t)ms);
    lv_anim_start(&a);
}
