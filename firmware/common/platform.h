#pragma once
/* Hooks every game uses, implemented by the firmware (main/) or a host tool. */
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define WF_RES 466

uint32_t wf_platform_now_s(void);          /* wall clock (RTC) seconds */
uint32_t wf_platform_random(void);
void *wf_platform_big_alloc(size_t bytes); /* PSRAM on the device */
void wf_platform_big_free(void *p);
void wf_platform_save(const void *data, size_t len);
void wf_platform_brightness(int percent);
void wf_platform_radio(bool on);           /* ESP-NOW on/off */
void wf_platform_link_send(void *ctx, const uint8_t *peer, const uint8_t *data, size_t len);

/* ---- Implemented by the game (pet/ or rpg/) ------------------------------ */
enum { WF_KEY_BOOT_SHORT = 0, WF_KEY_BOOT_LONG, WF_KEY_PWR_SHORT };
const char *wf_app_save_key(void);            /* NVS key */
size_t wf_app_save_max(void);
/* `save` NULL (or invalid) starts a new game. */
void wf_app_start(const uint8_t *save, size_t len);
void wf_app_tick(void);
bool wf_app_key(int key);
void wf_app_link_rx(const uint8_t from[6], const uint8_t *data, size_t len);
void wf_app_flush_save(void);
/* True while the screen must not turn itself off (battles, minigames, radio). */
bool wf_app_busy(void);
/* Called after the screen turns back on. */
void wf_app_wake(void);
