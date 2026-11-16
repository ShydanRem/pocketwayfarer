/* Interactive desktop host for the exact native game UI and game rules.
 * SDL2 only supplies a window, framebuffer, pointer and physical-button keys. */
#define _POSIX_C_SOURCE 200809L
#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include "SDL.h"
#include "lvgl.h"
#include "rui.h"

static uint16_t fb[WF_RES * WF_RES], visible[WF_RES * WF_RES], drawbuf[WF_RES * 40];
static int px, py;
static bool pressed, screen_on = true, dirty = true;
static const char *save_path = "build/desktop-save.bin";
static uint8_t saved[4096];
static size_t saved_len;
static uint32_t random_state;

uint32_t wf_platform_now_s(void) { return (uint32_t)(time(NULL) - 946684800); }
uint32_t wf_platform_random(void) {
    random_state ^= random_state << 13; random_state ^= random_state >> 17; random_state ^= random_state << 5;
    return random_state;
}
void *wf_platform_big_alloc(size_t n) { return malloc(n); }
void wf_platform_big_free(void *p) { free(p); }
void wf_platform_brightness(int percent) { (void)percent; }
void wf_platform_radio(bool on) { (void)on; }
void wf_platform_link_send(void *c, const uint8_t *p, const uint8_t *d, size_t n) { (void)c; (void)p; (void)d; (void)n; }
void wf_platform_save(const void *data, size_t n) {
    if (n > sizeof saved) { fprintf(stderr, "Save exceeds desktop buffer\n"); return; }
    char temporary[1024];
    if (snprintf(temporary, sizeof temporary, "%s.tmp", save_path) >= (int)sizeof temporary) return;
    FILE *f = fopen(temporary, "wb");
    if (!f) { fprintf(stderr, "Cannot open save: %s\n", strerror(errno)); return; }
    bool ok = fwrite(data, 1, n, f) == n && fflush(f) == 0 && fsync(fileno(f)) == 0;
    if (fclose(f) != 0) ok = false;
    if (!ok || rename(temporary, save_path) != 0) { fprintf(stderr, "Cannot commit save: %s\n", strerror(errno)); return; }
    memcpy(saved, data, n); saved_len = n;
}
static uint32_t ticks(void) { return SDL_GetTicks(); }
static void flush(lv_display_t *d, const lv_area_t *a, uint8_t *pixels) {
    const uint16_t *src = (const uint16_t *)pixels;
    for (int y = a->y1; y <= a->y2; y++) for (int x = a->x1; x <= a->x2; x++) fb[y * WF_RES + x] = *src++;
    dirty = true; lv_display_flush_ready(d);
}
static void read_pointer(lv_indev_t *i, lv_indev_data_t *d) {
    (void)i; d->point.x = px; d->point.y = py;
    bool in_circle = (px - 233) * (px - 233) + (py - 233) * (py - 233) < 233 * 233;
    d->state = pressed && screen_on && in_circle ? LV_INDEV_STATE_PRESSED : LV_INDEV_STATE_RELEASED;
}
static void load(void) {
    FILE *f = fopen(save_path, "rb");
    if (f) { saved_len = fread(saved, 1, sizeof saved, f); fclose(f); }
    wf_app_start(saved_len ? saved : NULL, saved_len);
}
int main(int argc, char **argv) {
    bool smoke = false;
    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "--smoke")) smoke = true;
        else if (!strcmp(argv[i], "--save") && i + 1 < argc) save_path = argv[++i];
        else { fprintf(stderr, "Usage: %s [--save PATH] [--smoke]\n", argv[0]); return 2; }
    }
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0) { fprintf(stderr, "%s\n", SDL_GetError()); return 1; }
    SDL_Window *window = SDL_CreateWindow("Pocket Wayfarer | mouse = touch | Space = BOOT | P = power | F5 = reload", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 699, 699, SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
    SDL_Renderer *renderer = window ? SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE) : NULL;
    SDL_Texture *texture = renderer ? SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGB565, SDL_TEXTUREACCESS_STREAMING, WF_RES, WF_RES) : NULL;
    if (!texture) { fprintf(stderr, "Desktop initialization: %s\n", SDL_GetError()); SDL_Quit(); return 1; }
    SDL_RenderSetLogicalSize(renderer, WF_RES, WF_RES);
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "nearest");
    lv_init(); lv_tick_set_cb(ticks);
    lv_display_t *display = lv_display_create(WF_RES, WF_RES);
    lv_display_set_buffers(display, drawbuf, NULL, sizeof drawbuf, LV_DISPLAY_RENDER_MODE_PARTIAL);
    lv_display_set_flush_cb(display, flush);
    lv_indev_t *input = lv_indev_create(); lv_indev_set_type(input, LV_INDEV_TYPE_POINTER); lv_indev_set_read_cb(input, read_pointer);
    lv_timer_set_period(lv_indev_get_read_timer(input), 10);
    random_state = (uint32_t)time(NULL) | 1u;
    load();
    bool running = true;
    uint32_t started = SDL_GetTicks();
    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) running = false;
            else if (event.type == SDL_MOUSEMOTION || event.type == SDL_MOUSEBUTTONDOWN || event.type == SDL_MOUSEBUTTONUP) {
                int x = event.type == SDL_MOUSEMOTION ? event.motion.x : event.button.x;
                int y = event.type == SDL_MOUSEMOTION ? event.motion.y : event.button.y;
                /* SDL_RenderSetLogicalSize already transforms these events.
                 * Applying RenderWindowToLogical again breaks letterboxed input. */
                px = x; py = y;
                if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT) pressed = true;
                if (event.type == SDL_MOUSEBUTTONUP && event.button.button == SDL_BUTTON_LEFT) pressed = false;
                /* Preserve short clicks even if down/up arrive in one event poll. */
                if (event.type != SDL_MOUSEMOTION && screen_on) lv_indev_read(input);
            } else if (event.type == SDL_KEYDOWN && !event.key.repeat) {
                SDL_Keycode key = event.key.keysym.sym;
                if (key == SDLK_p) {
                    if (screen_on) wf_app_flush_save(); else wf_app_wake();
                    screen_on = !screen_on; dirty = true;
                } else if (key == SDLK_F5) { wf_app_flush_save(); wf_app_start(saved_len ? saved : NULL, saved_len); }
                else if (screen_on && (key == SDLK_SPACE || key == SDLK_RETURN)) wf_app_key(WF_KEY_BOOT_SHORT);
                else if (screen_on && key == SDLK_ESCAPE) wf_app_key(WF_KEY_BOOT_LONG);
            }
        }
        if (screen_on) { lv_timer_handler(); wf_app_tick(); }
        if (dirty) {
            for (int y = 0; y < WF_RES; y++) for (int x = 0; x < WF_RES; x++) {
                bool inside = (x - 233) * (x - 233) + (y - 233) * (y - 233) < 233 * 233;
                visible[y * WF_RES + x] = screen_on && inside ? fb[y * WF_RES + x] : 0x0841;
            }
            SDL_UpdateTexture(texture, NULL, visible, WF_RES * 2);
            SDL_RenderClear(renderer); SDL_RenderCopy(renderer, texture, NULL, NULL); SDL_RenderPresent(renderer); dirty = false;
        }
        if (smoke && SDL_GetTicks() - started > 500) running = false;
        SDL_Delay(4);
    }
    wf_app_flush_save();
    SDL_DestroyTexture(texture); SDL_DestroyRenderer(renderer); SDL_DestroyWindow(window); SDL_Quit();
    puts("Native desktop closed cleanly");
    return 0;
}
