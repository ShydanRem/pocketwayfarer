#pragma once
/* Immutable avatar images: raw descriptors can be scene-pinned, while widgets
 * acquire/release a reference so changing a draft does not retain every look. */
#include <stddef.h>
#include <stdint.h>
#include "lvgl.h"

/* RGB565 and A8 are separate tightly packed planes, each w*h pixels. The
 * callback runs immediately on a new entry and must not keep either pointer. */
typedef void (*hero_cache_render)(uint16_t *rgb, uint8_t *alpha, int w, int h, void *ctx);

/* key must be non-NULL/nonempty and cover every input affecting the render.
 * Its bytes are copied; ctx is never retained or used as a cache identity.
 * Dimensions and render-function identity are also part of the cache key.
 * Invalid arguments or allocation failure return NULL; existing entries remain
 * unchanged. This function pins the result until scene release. Call from the
 * LVGL task, like the rest of the scene toolkit. */
const lv_image_dsc_t *hero_cache_image(const void *key, size_t key_size, int w, int h,
                                      hero_cache_render render, void *ctx);

/* Same identity/render contract, with one acquired reference instead of a scene
 * pin. Shareable by multiple widgets. Set a widget's NEW source before unref'ing
 * its OLD descriptor; unref the current one when the owning widget is deleted. */
const lv_image_dsc_t *hero_cache_acquire(const void *key, size_t key_size, int w, int h,
                                        hero_cache_render render, void *ctx);
/* Releases one acquisition. Frees only when no refs AND no raw scene pin remain.
 * NULL/unknown descriptors and an already-zero reference count are ignored. */
void hero_cache_unref(const lv_image_dsc_t *image);

/* Only after all widgets using the descriptors are deleted. The toolkit calls
 * this automatically at scene release; calling it again on an empty cache is safe. */
void hero_cache_release(void);
