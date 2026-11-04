#pragma once
/* Waveshare ESP32-S3-Touch-AMOLED-1.75: 466x466 CO5300 AMOLED (QSPI),
 * CST9217 touch, TCA9554 expander (PWR key), AXP2101 PMIC, PCF85063 RTC.
 * All functions must be called from the LVGL/main task. */
#include <stdbool.h>
#include <stdint.h>

void board_init(void);
/* lv_tick + lv_timer_handler (skipped while the panel is off). */
void board_tick(void);
/* PWR and BOOT levels: -1 unavailable, 0 released, 1 pressed. */
void board_buttons(int levels[2]);
bool board_brightness(unsigned percent);
/* Turns the panel off/on. */
bool board_panel(bool on);
/* While the panel is off: sleeps briefly; returns true when touch/BOOT/PWR
 * asks to wake up. Light sleep only on battery. */
bool board_wait_wake(void);
/* RTC seconds since 2000-01-01 (falls back to uptime-based seconds). */
uint32_t board_rtc_now(void);
bool board_rtc_ok(void);
uint32_t board_random(void);
