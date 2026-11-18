#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include "hero_cache.h"
#include "kit.h"
#include "src/misc/cache/instance/lv_image_cache.h"

typedef struct hero_cache_entry {
    struct hero_cache_entry *next;
    lv_image_dsc_t image;
    uint8_t *buffer;
    size_t key_size;
    size_t references;
    bool scene_pinned;
    hero_cache_render render;
    uint8_t key[];
} hero_cache_entry;

static hero_cache_entry *entries;
static bool registered;

static hero_cache_entry *get_or_create(const void *key, size_t key_size, int w, int h,
                                      hero_cache_render render, void *ctx) {
    /* LVGL 9.4 image dimensions and stride are 16-bit; data_size is 32-bit.
     * Bound every multiplication before allocating on the 32-bit device. */
    if (!key || !key_size || !render || w <= 0 || h <= 0 ||
        w > UINT16_MAX / 2 || h > UINT16_MAX ||
        key_size > SIZE_MAX - sizeof(hero_cache_entry)) return NULL;
    size_t width = (size_t)w, height = (size_t)h;
    if (height > SIZE_MAX / width) return NULL;
    size_t pixels = width * height;
    if (pixels > SIZE_MAX / 3u || pixels > UINT32_MAX / 3u) return NULL;

    for (hero_cache_entry *e = entries; e; e = e->next) {
        if (e->key_size == key_size && e->image.header.w == (uint32_t)w &&
            e->image.header.h == (uint32_t)h && e->render == render &&
            !memcmp(e->key, key, key_size)) return e;
    }

    if (!registered) {
        gfx_on_scene_release(hero_cache_release);
        registered = true;
    }
    hero_cache_entry *e = calloc(1, sizeof *e + key_size);
    if (!e) return NULL;
    e->buffer = wf_platform_big_alloc(pixels * 3u);
    if (!e->buffer) { free(e); return NULL; }
    e->key_size = key_size;
    e->render = render;
    memcpy(e->key, key, key_size);
    memset(e->buffer, 0, pixels * 3u);
    render((uint16_t *)e->buffer, e->buffer + pixels * 2u, w, h, ctx);

    e->image.header.magic = LV_IMAGE_HEADER_MAGIC;
    e->image.header.cf = LV_COLOR_FORMAT_RGB565A8;
    e->image.header.w = (uint32_t)w;
    e->image.header.h = (uint32_t)h;
    e->image.header.stride = (uint32_t)w * 2u;
    e->image.data = e->buffer;
    e->image.data_size = (uint32_t)(pixels * 3u);
    e->next = entries;
    entries = e;
    return e;
}

const lv_image_dsc_t *hero_cache_image(const void *key, size_t key_size, int w, int h,
                                      hero_cache_render render, void *ctx) {
    hero_cache_entry *e = get_or_create(key, key_size, w, h, render, ctx);
    if (!e) return NULL;
    e->scene_pinned = true;
    return &e->image;
}

const lv_image_dsc_t *hero_cache_acquire(const void *key, size_t key_size, int w, int h,
                                        hero_cache_render render, void *ctx) {
    hero_cache_entry *e = get_or_create(key, key_size, w, h, render, ctx);
    if (!e || e->references == SIZE_MAX) return NULL;
    e->references++;
    return &e->image;
}

static void free_entry(hero_cache_entry *e) {
    lv_image_cache_drop(&e->image);
    wf_platform_big_free(e->buffer);
    free(e);
}

void hero_cache_unref(const lv_image_dsc_t *image) {
    if (!image) return;
    for (hero_cache_entry **link = &entries; *link; link = &(*link)->next) {
        hero_cache_entry *e = *link;
        if (&e->image != image) continue;
        if (!e->references) return;
        e->references--;
        if (!e->references && !e->scene_pinned) {
            *link = e->next;
            free_entry(e);
        }
        return;
    }
}

void hero_cache_release(void) {
    while (entries) {
        hero_cache_entry *e = entries;
        entries = e->next;
        free_entry(e);
    }
}
