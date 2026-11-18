/* Stable avatar descriptors under real LVGL widget/scene lifetimes. */
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "hero_cache.h"
#include "kit.h"

enum { LIVE_IMAGES = 40, MAX_ALLOCATIONS = 128 };
static struct { void *ptr; size_t bytes; } allocations[MAX_ALLOCATIONS];
static unsigned live_allocations, allocations_done, frees_done, renders, deleted_widgets;
static unsigned expected_deleted;
static size_t live_bytes;
static bool reject_big_alloc, check_delete_order;
uint8_t wf_lang;

void *wf_platform_big_alloc(size_t bytes) {
    if (reject_big_alloc) return NULL;
    void *ptr = malloc(bytes);
    assert(ptr);
    for (unsigned i = 0; i < MAX_ALLOCATIONS; i++) if (!allocations[i].ptr) {
        allocations[i].ptr = ptr; allocations[i].bytes = bytes;
        live_allocations++; allocations_done++; live_bytes += bytes;
        return ptr;
    }
    assert(0); return NULL;
}
void wf_platform_big_free(void *ptr) {
    if (check_delete_order) assert(deleted_widgets == expected_deleted);
    for (unsigned i = 0; i < MAX_ALLOCATIONS; i++) if (allocations[i].ptr == ptr) {
        live_bytes -= allocations[i].bytes; allocations[i].ptr = NULL;
        live_allocations--; frees_done++; free(ptr); return;
    }
    assert(0);
}
uint32_t wf_platform_random(void) { return 19; }

static void fill(uint16_t *rgb, uint8_t *alpha, int w, int h, void *ctx) {
    const uint16_t value = *(const uint16_t *)ctx;
    renders++;
    for (int i = 0; i < w*h; i++) { rgb[i] = value; alpha[i] = (uint8_t)(i % 251); }
}
static void alternate_fill(uint16_t *rgb, uint8_t *alpha, int w, int h, void *ctx) {
    fill(rgb, alpha, w, h, ctx);
}
static void image_deleted(lv_event_t *event) { (void)event; deleted_widgets++; }
static void acquired_image_deleted(lv_event_t *event) {
    hero_cache_unref(lv_image_get_src(lv_event_get_target_obj(event)));
}
static void flush(lv_display_t *display, const lv_area_t *area, uint8_t *pixels) {
    (void)area; (void)pixels; lv_display_flush_ready(display);
}
static void check_pixels(const lv_image_dsc_t *image, int w, int h, uint16_t value) {
    assert(image->header.magic == LV_IMAGE_HEADER_MAGIC);
    assert(image->header.cf == LV_COLOR_FORMAT_RGB565A8);
    assert(image->header.w == (uint32_t)w && image->header.h == (uint32_t)h);
    assert(image->header.stride == (uint32_t)w*2 && image->data_size == (uint32_t)w*h*3);
    const uint16_t *rgb = (const uint16_t *)image->data;
    const uint8_t *alpha = image->data + (size_t)w*h*2;
    for (int i = 0; i < w*h; i++) assert(rgb[i] == value && alpha[i] == (uint8_t)(i % 251));
}
int main(void) {
    lv_init();
    lv_display_t *display = lv_display_create(WF_RES, WF_RES);
    static uint16_t draw_buffer[WF_RES*20];
    lv_display_set_buffers(display, draw_buffer, NULL, sizeof draw_buffer, LV_DISPLAY_RENDER_MODE_PARTIAL);
    lv_display_set_flush_cb(display, flush);
    lv_obj_t *screen = kit_screen(NULL);
    const lv_image_dsc_t *images[LIVE_IMAGES];
    lv_obj_t *widgets[LIVE_IMAGES];
    for (uint32_t key = 0; key < LIVE_IMAGES; key++) {
        uint16_t value = (uint16_t)(0x100 + key);
        images[key] = hero_cache_image(&key, sizeof key, 12, 16, fill, &value);
        assert(images[key]);
        widgets[key] = lv_image_create(screen);
        lv_image_set_src(widgets[key], images[key]);
        lv_obj_add_event_cb(widgets[key], image_deleted, LV_EVENT_DELETE, NULL);
    }
    assert(renders == LIVE_IMAGES && live_allocations == LIVE_IMAGES);
    for (uint32_t key = 0; key < LIVE_IMAGES; key++) {
        uint16_t ignored_context = 0xffff;
        const lv_image_dsc_t *reused = hero_cache_image(&key, sizeof key, 12, 16, fill, &ignored_context);
        assert(reused == images[key] && lv_image_get_src(widgets[key]) == images[key]);
        check_pixels(images[key], 12, 16, (uint16_t)(0x100 + key));
        for (unsigned j = 0; j < key; j++) assert(images[j] != images[key]);
    }
    assert(renders == LIVE_IMAGES); /* Different stack context does not rerender/overwrite. */

    uint32_t zero_key = 0;
    uint16_t value = 0x3553;
    const lv_image_dsc_t *wide = hero_cache_image(&zero_key, sizeof zero_key, 14, 16, fill, &value);
    const lv_image_dsc_t *tall = hero_cache_image(&zero_key, sizeof zero_key, 12, 18, fill, &value);
    const lv_image_dsc_t *other_renderer = hero_cache_image(&zero_key, sizeof zero_key, 12, 16, alternate_fill, &value);
    assert(wide && tall && other_renderer && wide != images[0] && tall != images[0] && other_renderer != images[0]);
    check_pixels(wide,14,16,value); check_pixels(tall,12,18,value); check_pixels(other_renderer,12,16,value);
    const uint8_t short_key[] = {0x42}, long_key[] = {0x42,0};
    const lv_image_dsc_t *short_image = hero_cache_image(short_key,sizeof short_key,12,16,fill,&value);
    const lv_image_dsc_t *long_image = hero_cache_image(long_key,sizeof long_key,12,16,fill,&value);
    assert(short_image && long_image && short_image != long_image);

    uint32_t stack_key = 700;
    const lv_image_dsc_t *copied_key = hero_cache_image(&stack_key,sizeof stack_key,12,16,fill,&value);
    stack_key = 701;
    assert(hero_cache_image(&stack_key,sizeof stack_key,12,16,fill,&value) != copied_key);
    stack_key = 700;
    assert(hero_cache_image(&stack_key,sizeof stack_key,12,16,fill,&value) == copied_key);

    unsigned before_failure = live_allocations;
    uint32_t new_key = 900;
    reject_big_alloc = true;
    assert(!hero_cache_image(&new_key,sizeof new_key,12,16,fill,&value));
    reject_big_alloc = false;
    assert(live_allocations == before_failure);
    assert(!hero_cache_image(NULL,1,12,16,fill,&value));
    assert(!hero_cache_image(&new_key,0,12,16,fill,&value));
    assert(!hero_cache_image(&new_key,sizeof new_key,12,16,NULL,&value));
    assert(!hero_cache_image(&new_key,sizeof new_key,0,16,fill,&value));
    assert(!hero_cache_image(&new_key,sizeof new_key,-1,16,fill,&value));
    assert(!hero_cache_image(&new_key,sizeof new_key,UINT16_MAX,16,fill,&value));
    assert(!hero_cache_image(&new_key,sizeof new_key,12,65536,fill,&value));
    assert(!hero_cache_image(&new_key,SIZE_MAX,12,16,fill,&value));
    assert(!hero_cache_image(&new_key,sizeof new_key,32767,65535,fill,&value));
    assert(live_allocations == before_failure);
    for (unsigned i = 0; i < LIVE_IMAGES; i++) check_pixels(images[i],12,16,(uint16_t)(0x100+i));

    /* Draft edits must not retain all 1000 discarded looks just because the
     * screen also contains pinned raw portraits. Only the live refs survive. */
    size_t pinned_bytes = live_bytes;
    unsigned pinned_allocations = live_allocations;
    uint32_t changing_key = 10000;
    uint16_t current_value = 0x4000;
    const lv_image_dsc_t *current = hero_cache_acquire(&changing_key,sizeof changing_key,12,16,fill,&current_value);
    assert(current && live_bytes == pinned_bytes+12u*16u*3u);
    lv_obj_t *changing_widget = lv_image_create(screen); lv_image_set_src(changing_widget,current);
    lv_obj_add_event_cb(changing_widget,acquired_image_deleted,LV_EVENT_DELETE,NULL);
    for (unsigned i = 1; i <= 1000; i++) {
        changing_key++;
        uint16_t next_value = (uint16_t)(0x4000+i);
        const lv_image_dsc_t *next = hero_cache_acquire(&changing_key,sizeof changing_key,12,16,fill,&next_value);
        assert(next && next != current && live_allocations == pinned_allocations+2);
        assert(live_bytes == pinned_bytes+2u*12u*16u*3u);
        check_pixels(current,12,16,current_value);
        lv_image_set_src(changing_widget,next); /* Source swap precedes last unref. */
        hero_cache_unref(current);
        current = next; current_value = next_value;
        assert(live_allocations == pinned_allocations+1 && live_bytes == pinned_bytes+12u*16u*3u);
        if (i % 100 == 0) { lv_obj_update_layout(screen); lv_refr_now(display); }
    }
    check_pixels(current,12,16,current_value);
    lv_obj_delete(changing_widget); /* Delete callback releases the final reference. */
    assert(live_allocations == pinned_allocations && live_bytes == pinned_bytes);

    uint32_t shared_key = 20000;
    unsigned before_shared = renders;
    const lv_image_dsc_t *shared = hero_cache_acquire(&shared_key,sizeof shared_key,12,16,fill,&value);
    assert(shared && hero_cache_acquire(&shared_key,sizeof shared_key,12,16,fill,&value) == shared);
    assert(renders == before_shared+1 && live_allocations == pinned_allocations+1);
    lv_obj_t *shared_a = lv_image_create(screen), *shared_b = lv_image_create(screen);
    lv_image_set_src(shared_a,shared); lv_image_set_src(shared_b,shared);
    lv_obj_add_event_cb(shared_a,acquired_image_deleted,LV_EVENT_DELETE,NULL);
    lv_obj_add_event_cb(shared_b,acquired_image_deleted,LV_EVENT_DELETE,NULL);
    lv_obj_delete(shared_a);
    assert(live_allocations == pinned_allocations+1);
    check_pixels(lv_image_get_src(shared_b),12,16,value);
    lv_obj_delete(shared_b);
    assert(live_allocations == pinned_allocations && live_bytes == pinned_bytes);

    /* Acquiring a raw entry must not remove its existing scene pin. */
    assert(hero_cache_acquire(&zero_key,sizeof zero_key,12,16,fill,&value) == images[0]);
    hero_cache_unref(images[0]); hero_cache_unref(images[0]);
    assert(live_allocations == pinned_allocations);
    check_pixels(images[0],12,16,0x100);
    /* Conversely, a raw lookup can add a scene pin to a live acquired entry. */
    const lv_image_dsc_t *promoted_pin = hero_cache_acquire(&shared_key,sizeof shared_key,12,16,fill,&value);
    assert(promoted_pin && hero_cache_image(&shared_key,sizeof shared_key,12,16,fill,&value) == promoted_pin);
    hero_cache_unref(promoted_pin);
    assert(live_allocations == pinned_allocations+1);
    check_pixels(promoted_pin,12,16,value);
    hero_cache_unref(NULL);
    uint32_t fail_acquire_key = 30000;
    reject_big_alloc = true;
    assert(!hero_cache_acquire(&fail_acquire_key,sizeof fail_acquire_key,12,16,fill,&value));
    reject_big_alloc = false;
    assert(live_allocations == pinned_allocations+1);

    /* The scene release must delete BOTH normal and modal/top-layer image
     * objects before dropping any descriptor or freeing a pixel plane. */
    lv_obj_t *overlay_image = lv_image_create(lv_layer_top());
    lv_image_set_src(overlay_image,images[0]);
    lv_obj_add_event_cb(overlay_image,image_deleted,LV_EVENT_DELETE,NULL);
    lv_obj_update_layout(screen); lv_refr_now(display); /* Populate LVGL decoder caches too. */
    expected_deleted = LIVE_IMAGES + 1; check_delete_order = true;
    screen = kit_screen(NULL);
    assert(deleted_widgets == expected_deleted && live_allocations == 0 && live_bytes == 0);
    assert(allocations_done == frees_done);
    unsigned before_fresh_scene = renders;
    const lv_image_dsc_t *fresh = hero_cache_image(&zero_key,sizeof zero_key,12,16,fill,&value);
    assert(fresh && renders == before_fresh_scene+1);
    check_pixels(fresh,12,16,value);
    lv_obj_t *fresh_widget = lv_image_create(screen); lv_image_set_src(fresh_widget,fresh);
    lv_obj_add_event_cb(fresh_widget,image_deleted,LV_EVENT_DELETE,NULL);
    expected_deleted++;
    kit_screen(NULL);
    hero_cache_release(); /* Idempotent after the automatic scene cleanup. */
    assert(live_allocations == 0 && live_bytes == 0 && allocations_done == frees_done);
    puts("hero cache: 40 stable pinned descriptors,1000 bounded live swaps,shared refs,pin promotion,copied keys,allocation failure,scene cleanup and fresh render: passed");
    return 0;
}
