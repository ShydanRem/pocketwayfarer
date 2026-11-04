/* Display/touch/key initialisation adapted from PocketD-D (same author), which
 * in turn adapts the Waveshare ESP32-S3-Touch-AMOLED-1.75 BSP (Apache-2.0).
 * Pins and the CO5300 init sequence are the ones verified on hardware there.
 * Additions: PCF85063 RTC (read; written only when its clock is invalid). */
#include "board.h"
#include <string.h>
#include "lvgl.h"
#include "esp_lcd_co5300.h"
#include "esp_lcd_touch_cst9217.h"
#include "esp_lcd_panel_ops.h"
#include "driver/spi_master.h"
#include "driver/i2c_master.h"
#include "driver/gpio.h"
#include "driver/usb_serial_jtag.h"
#include "esp_heap_caps.h"
#include "esp_timer.h"
#include "esp_random.h"
#include "esp_sleep.h"
#include "bootloader_random.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

#define RES 466
#define LINES 48

static esp_lcd_panel_handle_t panel;
static esp_lcd_touch_handle_t touch;
static SemaphoreHandle_t flushed;
static bool transfer_pending, panel_off;
static int64_t last_tick;
static i2c_master_bus_handle_t i2c;
static i2c_master_dev_handle_t expander, rtc;
static bool pwr_ready, rtc_valid;
static uint8_t pwr_polarity;
static int pwr_level = -1;
static int64_t buttons_read_at = -100;
static uint32_t rtc_base;          /* RTC seconds at rtc_base_us */
static int64_t rtc_base_us;

static const co5300_lcd_init_cmd_t commands[] = {
    {0xFE, (uint8_t[]){0x20}, 1, 0}, {0x19, (uint8_t[]){0x10}, 1, 0}, {0x1C, (uint8_t[]){0xA0}, 1, 0},
    {0xFE, (uint8_t[]){0x00}, 1, 0}, {0xC4, (uint8_t[]){0x80}, 1, 0}, {0x3A, (uint8_t[]){0x55}, 1, 0},
    {0x35, (uint8_t[]){0x00}, 1, 0}, {0x53, (uint8_t[]){0x20}, 1, 0}, {0x51, (uint8_t[]){0x80}, 1, 0},
    {0x63, (uint8_t[]){0xFF}, 1, 0}, {0x2A, (uint8_t[]){0x00, 0x06, 0x01, 0xD7}, 4, 0},
    {0x2B, (uint8_t[]){0x00, 0x00, 0x01, 0xD1}, 4, 600}, {0x11, NULL, 0, 600}, {0x29, NULL, 0, 0},
};

static bool color_done(esp_lcd_panel_io_handle_t io, esp_lcd_panel_io_event_data_t *ev, void *ctx) {
    BaseType_t wake = pdFALSE;
    xSemaphoreGiveFromISR(flushed, &wake);
    return wake == pdTRUE;
}

static void flush(lv_display_t *d, const lv_area_t *a, uint8_t *px) {
    lv_draw_sw_rgb565_swap(px, lv_area_get_size(a));
    transfer_pending = true;
    ESP_ERROR_CHECK(esp_lcd_panel_draw_bitmap(panel, a->x1, a->y1, a->x2 + 1, a->y2 + 1, px));
}

static void flush_wait(lv_display_t *d) {
    if (transfer_pending) {
        xSemaphoreTake(flushed, portMAX_DELAY);
        transfer_pending = false;
    }
}

/* The CO5300 needs even-aligned windows. */
static void round_area(lv_event_t *e) {
    lv_area_t *a = lv_event_get_param(e);
    a->x1 &= ~1;
    a->y1 &= ~1;
    a->x2 |= 1;
    a->y2 |= 1;
}

static void read_touch(lv_indev_t *indev, lv_indev_data_t *data) {
    esp_lcd_touch_point_data_t p = {0};
    uint8_t count = 0;
    data->state = LV_INDEV_STATE_RELEASED;
    if (esp_lcd_touch_read_data(touch) != ESP_OK) return;
    if (esp_lcd_touch_get_data(touch, &p, &count, 1) == ESP_OK && count && p.x < RES && p.y < RES) {
        data->state = LV_INDEV_STATE_PRESSED;
        data->point.x = p.x;
        data->point.y = p.y;
    }
}

static esp_err_t expander_read(uint8_t reg, uint8_t *v) {
    return i2c_master_transmit_receive(expander, &reg, 1, v, 1, 5);
}

/* ---- PCF85063 RTC -------------------------------------------------------- */
static uint8_t bcd(uint8_t v) { return (uint8_t)((v >> 4) * 10 + (v & 15)); }
static uint8_t tobcd(int v) { return (uint8_t)(((v / 10) << 4) | (v % 10)); }

static uint32_t days_from_2000(int y, int m, int d) {
    static const uint16_t before[12] = {0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334};
    uint32_t days = (uint32_t)(y * 365 + (y + 3) / 4);
    days += before[(m - 1) % 12] + (uint32_t)(d - 1);
    if (m > 2 && y % 4 == 0) days++;
    return days;
}

static bool rtc_read(uint32_t *out) {
    uint8_t reg = 0x04, r[7];
    if (!rtc || i2c_master_transmit_receive(rtc, &reg, 1, r, sizeof r, 20) != ESP_OK) return false;
    if (r[0] & 0x80) return false;                      /* oscillator stopped: time invalid */
    int sec = bcd(r[0] & 0x7f), min = bcd(r[1] & 0x7f), hour = bcd(r[2] & 0x3f);
    int day = bcd(r[3] & 0x3f), month = bcd(r[5] & 0x1f), year = bcd(r[6]);
    if (month < 1 || month > 12 || day < 1 || day > 31) return false;
    *out = days_from_2000(year, month, day) * 86400u + (uint32_t)(hour * 3600 + min * 60 + sec);
    return true;
}

static void rtc_set_default(void) {
    /* 1 Nov 2026, 10:00. The player can shift the day/night hour from the diary. */
    uint8_t w[8] = {0x04, tobcd(0), tobcd(0), tobcd(10), tobcd(1), 0, tobcd(11), tobcd(26)};
    if (rtc) i2c_master_transmit(rtc, w, sizeof w, 20);
}

static void rtc_init(void) {
    i2c_device_config_t cfg = {.dev_addr_length = I2C_ADDR_BIT_LEN_7, .device_address = 0x51, .scl_speed_hz = 400000};
    if (i2c_master_probe(i2c, 0x51, 20) != ESP_OK || i2c_master_bus_add_device(i2c, &cfg, &rtc) != ESP_OK) {
        rtc = NULL;
        return;
    }
    uint32_t t;
    if (!rtc_read(&t)) {
        rtc_set_default();
        if (!rtc_read(&t)) { rtc = NULL; return; }
    }
    rtc_valid = true;
    rtc_base = t;
    rtc_base_us = esp_timer_get_time();
}

bool board_rtc_ok(void) { return rtc_valid; }

uint32_t board_rtc_now(void) {
    int64_t now = esp_timer_get_time();
    /* Re-read the RTC every 10 minutes; in between, extrapolate with esp_timer
     * (which keeps counting through light sleep). */
    if (rtc_valid && now - rtc_base_us > 600LL * 1000000LL) {
        uint32_t t;
        if (rtc_read(&t)) { rtc_base = t; rtc_base_us = now; }
    }
    if (!rtc_valid && !rtc_base) {
        rtc_base = days_from_2000(26, 11, 1) * 86400u + 10u * 3600u;
        rtc_base_us = now;
    }
    return rtc_base + (uint32_t)((now - rtc_base_us) / 1000000LL);
}

uint32_t board_random(void) { return esp_random(); }

/* ---- Init ------------------------------------------------------------------ */
void board_init(void) {
    bootloader_random_enable();
    flushed = xSemaphoreCreateBinary();
    spi_bus_config_t bus = CO5300_PANEL_BUS_QSPI_CONFIG(38, 4, 5, 6, 7, RES * LINES * 2);
    ESP_ERROR_CHECK(spi_bus_initialize(SPI2_HOST, &bus, SPI_DMA_CH_AUTO));
    esp_lcd_panel_io_spi_config_t io_cfg = CO5300_PANEL_IO_QSPI_CONFIG(12, color_done, NULL);
    io_cfg.trans_queue_depth = 1;
    esp_lcd_panel_io_handle_t io;
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)SPI2_HOST, &io_cfg, &io));
    co5300_vendor_config_t vendor = {.init_cmds = commands, .init_cmds_size = sizeof commands / sizeof commands[0],
                                     .flags.use_qspi_interface = 1};
    esp_lcd_panel_dev_config_t cfg = {.reset_gpio_num = 39, .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_RGB,
                                      .bits_per_pixel = 16, .vendor_config = &vendor};
    ESP_ERROR_CHECK(esp_lcd_new_panel_co5300(io, &cfg, &panel));
    ESP_ERROR_CHECK(esp_lcd_panel_set_gap(panel, 6, 0));
    ESP_ERROR_CHECK(esp_lcd_panel_reset(panel));
    ESP_ERROR_CHECK(esp_lcd_panel_init(panel));
    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel, true));

    i2c_master_bus_config_t i2c_cfg = {.i2c_port = 1, .sda_io_num = 15, .scl_io_num = 14,
                                       .clk_source = I2C_CLK_SRC_DEFAULT, .flags.enable_internal_pullup = 1};
    ESP_ERROR_CHECK(i2c_new_master_bus(&i2c_cfg, &i2c));
    esp_lcd_panel_io_i2c_config_t tio = ESP_LCD_TOUCH_IO_I2C_CST9217_CONFIG();
    tio.scl_speed_hz = 400000;
    esp_lcd_panel_io_handle_t tp_io;
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_i2c(i2c, &tio, &tp_io));
    esp_lcd_touch_config_t tp = {.x_max = RES, .y_max = RES, .rst_gpio_num = 40, .int_gpio_num = 11,
                                 .levels = {.reset = 0, .interrupt = 0}, .flags = {.mirror_x = 1, .mirror_y = 1}};
    ESP_ERROR_CHECK(esp_lcd_touch_new_i2c_cst9217(tp_io, &tp, &touch));

    /* PWR key through the TCA9554 expander (P4). Only its direction bit changes. */
    i2c_device_config_t ex = {.dev_addr_length = I2C_ADDR_BIT_LEN_7, .device_address = 0x20, .scl_speed_hz = 400000};
    if (i2c_master_bus_add_device(i2c, &ex, &expander) == ESP_OK) {
        uint8_t dir = 0;
        if (expander_read(3, &dir) == ESP_OK && expander_read(2, &pwr_polarity) == ESP_OK) {
            uint8_t w[] = {3, (uint8_t)(dir | 0x10)};
            pwr_ready = (dir & 0x10) || i2c_master_transmit(expander, w, sizeof w, 5) == ESP_OK;
        }
    }
    gpio_config_t key = {.pin_bit_mask = 1ULL << 0, .mode = GPIO_MODE_INPUT};
    ESP_ERROR_CHECK(gpio_config(&key));
    rtc_init();

    lv_init();
    lv_display_t *d = lv_display_create(RES, RES);
    uint8_t *buf = heap_caps_malloc(RES * LINES * 2, MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);
    assert(d && buf);
    lv_display_set_color_format(d, LV_COLOR_FORMAT_RGB565);
    lv_display_set_buffers(d, buf, NULL, RES * LINES * 2, LV_DISPLAY_RENDER_MODE_PARTIAL);
    lv_display_set_flush_cb(d, flush);
    lv_display_set_flush_wait_cb(d, flush_wait);
    lv_display_add_event_cb(d, round_area, LV_EVENT_INVALIDATE_AREA, NULL);
    lv_indev_t *in = lv_indev_create();
    lv_indev_set_type(in, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(in, read_touch);
    /* Timing minigames want low input latency. */
    lv_timer_set_period(lv_indev_get_read_timer(in), 10);
    last_tick = esp_timer_get_time() / 1000;
}

void board_tick(void) {
    int64_t now = esp_timer_get_time() / 1000;
    lv_tick_inc((uint32_t)(now - last_tick));
    last_tick = now;
    if (!panel_off) lv_timer_handler();
}

void board_buttons(int levels[2]) {
    int64_t now = esp_timer_get_time() / 1000;
    if (now - buttons_read_at >= 20) {
        buttons_read_at = now;
        uint8_t raw;
        pwr_level = pwr_ready && expander_read(0, &raw) == ESP_OK ? (((raw ^ pwr_polarity) & 0x10) ? 1 : 0) : -1;
    }
    levels[0] = pwr_level;
    levels[1] = gpio_get_level(GPIO_NUM_0) == 0;
}

bool board_brightness(unsigned percent) {
    if (percent < 10 || percent > 100) return false;
    flush_wait(NULL);
    return esp_lcd_panel_co5300_set_brightness(panel, percent) == ESP_OK;
}

bool board_panel(bool on) {
    flush_wait(NULL);
    if (esp_lcd_panel_disp_on_off(panel, on) != ESP_OK) return false;
    panel_off = !on;
    if (on) lv_obj_invalidate(lv_screen_active());
    return true;
}

bool board_wait_wake(void) {
    int levels[2];
    board_buttons(levels);
    esp_lcd_touch_point_data_t p = {0};
    uint8_t count = 0;
    if (esp_lcd_touch_read_data(touch) == ESP_OK) esp_lcd_touch_get_data(touch, &p, &count, 1);
    if (count || levels[0] == 1 || levels[1] == 1) return true;
    if (usb_serial_jtag_is_connected()) {
        vTaskDelay(pdMS_TO_TICKS(30));
        return false;
    }
    gpio_wakeup_enable(GPIO_NUM_0, GPIO_INTR_LOW_LEVEL);
    gpio_wakeup_enable(GPIO_NUM_11, GPIO_INTR_LOW_LEVEL);
    esp_sleep_enable_gpio_wakeup();
    esp_sleep_enable_timer_wakeup(40000);
    bootloader_random_disable();
    esp_light_sleep_start();
    bootloader_random_enable();
    bool gpio = esp_sleep_get_wakeup_cause() == ESP_SLEEP_WAKEUP_GPIO;
    gpio_wakeup_disable(GPIO_NUM_0);
    gpio_wakeup_disable(GPIO_NUM_11);
    esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_GPIO);
    esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_TIMER);
    return gpio;
}
