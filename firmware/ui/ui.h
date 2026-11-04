#pragma once
/* Pocket Wayfarer user interface (LVGL 9).
 *
 * The UI owns the game state and the link session. The platform (firmware
 * main.c or the host preview tool) provides time, randomness, storage and the
 * radio through the hooks below, and forwards physical buttons. All calls must
 * come from the LVGL task. */
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "game.h"
#include "link.h"

#define WF_RES 466

/* ---- Hooks implemented by the platform ------------------------------- */
uint32_t wf_platform_now_s(void);          /* wall clock (RTC) seconds */
uint32_t wf_platform_random(void);
void *wf_platform_big_alloc(size_t bytes); /* PSRAM on the device */
void wf_platform_big_free(void *p);
void wf_platform_save(const wf_game *g);   /* persist (called sparingly) */
void wf_platform_brightness(int percent);
void wf_platform_radio(bool on);           /* ESP-NOW on/off */
void wf_platform_link_send(void *ctx, const uint8_t *peer, const uint8_t *data, size_t len);

/* ---- UI API ---------------------------------------------------------- */
/* `loaded` = NULL starts the onboarding (new companion). */
void wf_ui_start(const wf_game *loaded);
/* Call every few ms from the LVGL task, after lv_timer_handler(). */
void wf_ui_tick(void);
enum { WF_KEY_BOOT_SHORT = 0, WF_KEY_BOOT_LONG, WF_KEY_PWR_SHORT };
/* Returns true when the UI consumed the key. */
bool wf_ui_key(int key);
/* Radio frame received (from the ESP-NOW callback, marshalled to the LVGL task). */
void wf_ui_link_rx(const uint8_t from[6], const uint8_t *data, size_t len);
/* Saves now (e.g. before sleep). */
void wf_ui_flush_save(void);
wf_game *wf_ui_game(void);
/* For previews/tests: jump straight to a screen. */
enum {
    WF_SCR_ONBOARD, WF_SCR_HOME, WF_SCR_MENU, WF_SCR_FEED, WF_SCR_PLAY, WF_SCR_TRAIN, WF_SCR_MAP,
    WF_SCR_ZONE, WF_SCR_BATTLE, WF_SCR_LINK, WF_SCR_BAG, WF_SCR_SHOP, WF_SCR_DIARY, WF_SCR_DUEL,
    WF_SCR_COUNT
};
void wf_ui_goto(int screen);
int wf_ui_screen(void);
