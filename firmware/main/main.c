/* Pocket Wayfarer firmware entry point: storage, ESP-NOW radio, keys and power. */
#include <string.h>
#include "board.h"
#include "device_input.h"
#include "esp_event.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "esp_netif.h"
#include "esp_now.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "lvgl.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "platform.h"

static const char *TAG = "wayfarer";
#define NVS_PART "wf_nvs"
#define RADIO_CHANNEL 1
#define IDLE_OFF_MS 45000u
#define RX_MAX 120

/* Local serial diagnostics: actual awake-loop cadence, never a guessed FPS. */
static void runtime_cadence(bool awake) {
    static uint32_t last, report_at, samples, slow, maximum;
    static uint64_t total;
    uint32_t now = lv_tick_get();
    if (!awake) { last = 0; return; }
    if (last) {
        uint32_t delta = now - last;
        samples++; total += delta;
        if (delta > 100) slow++;
        if (delta > maximum) maximum = delta;
    }
    last = now;
    if (now - report_at >= 10000 && samples) {
        ESP_LOGI(TAG, "awake loop: samples=%lu mean=%lums max=%lums over100ms=%lu",
                 (unsigned long)samples, (unsigned long)(total / samples),
                 (unsigned long)maximum, (unsigned long)slow);
        report_at = now; samples = slow = maximum = 0; total = 0;
    }
}

/* ---- Storage ------------------------------------------------------------------ */
static uint8_t *load_save(size_t *len) {
    nvs_handle_t h;
    if (nvs_open_from_partition(NVS_PART, "wayfarer", NVS_READONLY, &h) != ESP_OK) return NULL;
    size_t cap = wf_app_save_max();
    uint8_t *buf = heap_caps_malloc(cap, MALLOC_CAP_8BIT);
    *len = cap;
    if (buf && nvs_get_blob(h, wf_app_save_key(), buf, len) != ESP_OK) {
        heap_caps_free(buf);
        buf = NULL;
    }
    nvs_close(h);
    return buf;
}

bool wf_platform_save(const void *data, size_t len) {
    nvs_handle_t h;
    if (nvs_open_from_partition(NVS_PART, "wayfarer", NVS_READWRITE, &h) != ESP_OK) return false;
    /* NVS commits a blob atomically: a power cut keeps the previous save. */
    esp_err_t result = nvs_set_blob(h, wf_app_save_key(), data, len);
    if (result == ESP_OK) result = nvs_commit(h);
    if (result != ESP_OK) ESP_LOGW(TAG, "save failed: %s", esp_err_to_name(result));
    nvs_close(h);
    return result == ESP_OK;
}

/* ---- Platform hooks ---------------------------------------------------------- */
uint32_t wf_platform_now_s(void) { return board_rtc_now(); }
uint32_t wf_platform_random(void) { return board_random(); }
void *wf_platform_big_alloc(size_t n) {
    void *p = heap_caps_malloc(n, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    return p ? p : heap_caps_malloc(n, MALLOC_CAP_8BIT);
}
void wf_platform_big_free(void *p) { heap_caps_free(p); }
void wf_platform_brightness(int percent) { board_brightness((unsigned)percent); }

/* ---- ESP-NOW radio ---------------------------------------------------------- */
typedef struct {
    uint8_t from[6];
    uint8_t len;
    uint8_t data[RX_MAX];
} rx_frame;

static QueueHandle_t rx_queue;
static bool wifi_ready, radio_on;
static const uint8_t broadcast[6] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};

static void on_recv(const esp_now_recv_info_t *info, const uint8_t *data, int len) {
    if (len <= 0 || len > RX_MAX || !rx_queue) return;
    rx_frame f;
    memcpy(f.from, info->src_addr, 6);
    f.len = (uint8_t)len;
    memcpy(f.data, data, (size_t)len);
    xQueueSend(rx_queue, &f, 0);       /* drop when full: the protocol retries */
}

static void add_peer(const uint8_t *mac) {
    if (esp_now_is_peer_exist(mac)) return;
    esp_now_peer_info_t p = {0};
    memcpy(p.peer_addr, mac, 6);
    p.channel = RADIO_CHANNEL;
    p.ifidx = WIFI_IF_STA;
    if (esp_now_add_peer(&p) == ESP_ERR_ESPNOW_FULL) {
        /* Keep the table small: forget everyone but broadcast. */
        esp_now_peer_info_t it;
        bool from_head = true;
        while (esp_now_fetch_peer(from_head, &it) == ESP_OK) {
            from_head = false;
            if (memcmp(it.peer_addr, broadcast, 6)) esp_now_del_peer(it.peer_addr);
        }
        esp_now_add_peer(&p);
    }
}

void wf_platform_radio(bool on) {
    if (on == radio_on) return;
    if (on) {
        if (!wifi_ready) {
            ESP_ERROR_CHECK(esp_netif_init());
            ESP_ERROR_CHECK(esp_event_loop_create_default());
            wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
            ESP_ERROR_CHECK(esp_wifi_init(&cfg));
            ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
            ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
            wifi_ready = true;
        }
        if (esp_wifi_start() != ESP_OK) return;
        esp_wifi_set_channel(RADIO_CHANNEL, WIFI_SECOND_CHAN_NONE);
        if (esp_now_init() != ESP_OK) { esp_wifi_stop(); return; }
        esp_now_register_recv_cb(on_recv);
        add_peer(broadcast);
        radio_on = true;
        ESP_LOGI(TAG, "radio on");
    } else {
        esp_now_deinit();
        esp_wifi_stop();
        radio_on = false;
        ESP_LOGI(TAG, "radio off");
    }
}

void wf_platform_link_send(void *ctx, const uint8_t *peer, const uint8_t *data, size_t len) {
    (void)ctx;
    if (!radio_on) return;
    const uint8_t *to = peer ? peer : broadcast;
    if (peer) add_peer(peer);
    esp_now_send(to, data, len);
}

/* ---- Keys & power ----------------------------------------------------------- */
static void keys(wf_key_state k[2], bool *pwr_click, bool *boot_click, bool *boot_long, bool *boot_down) {
    int levels[2];
    board_buttons(levels);
    uint32_t now = lv_tick_get();
    *pwr_click = *boot_click = *boot_long = *boot_down = false;
    for (int i = 0; i < 2; i++) {
        if (levels[i] < 0) continue; /* unavailable is not a release */
        int lv = levels[i];
        int event = wf_key_sample(&k[i], lv, now, i == 1);
        if (i == 1 && event == WF_KEY_EVENT_DOWN) *boot_down = true;
        if (event == WF_KEY_EVENT_LONG) *boot_long = true;
        if (event == WF_KEY_EVENT_CLICK) {
            if (i == 0) *pwr_click = true;
            else *boot_click = true;
        }
    }
}

void app_main(void) {
    esp_err_t err = nvs_flash_init_partition(NVS_PART);
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_LOGW(TAG, "NVS layout changed, erasing save partition");
        nvs_flash_erase_partition(NVS_PART);
        err = nvs_flash_init_partition(NVS_PART);
    }
    ESP_ERROR_CHECK(err);
    /* Wi-Fi keeps calibration data in the default NVS partition. */
    if (nvs_flash_init() != ESP_OK) { nvs_flash_erase(); nvs_flash_init(); }
    rx_queue = xQueueCreate(24, sizeof(rx_frame));

    board_init();
    ESP_LOGI(TAG, "RTC %s, time %lu", board_rtc_ok() ? "ok" : "missing", (unsigned long)board_rtc_now());
    size_t save_len = 0;
    uint8_t *save = load_save(&save_len);
    wf_app_start(save, save_len);
    if (save) heap_caps_free(save);

    wf_key_state k[2] = {0};
    bool screen_on = true;
    for (;;) {
        runtime_cadence(screen_on);
        bool pwr, boot, boot_long, boot_down;
        keys(k, &pwr, &boot, &boot_long, &boot_down);
        if (screen_on) {
            if (pwr || boot || boot_long || boot_down) lv_display_trigger_activity(NULL);
            /* Dispatch physical timing before a possibly expensive render. */
            if (boot_down && wf_app_key(WF_KEY_BOOT_DOWN)) k[1].long_sent = true;
            if (boot) wf_app_key(WF_KEY_BOOT_SHORT);
            if (boot_long) wf_app_key(WF_KEY_BOOT_LONG);
            wf_app_tick();
            board_tick(); /* render this iteration's simulation, not the previous frame */
            rx_frame f;
            while (xQueueReceive(rx_queue, &f, 0) == pdTRUE) wf_app_link_rx(f.from, f.data, f.len);
            if (pwr || (!wf_app_busy() && lv_display_get_inactive_time(NULL) > IDLE_OFF_MS)) {
                wf_app_flush_save();
                wf_platform_radio(false);
                board_panel(false);
                screen_on = false;
                continue;
            }
            vTaskDelay(pdMS_TO_TICKS(4));
        } else {
            if (board_wait_wake()) {
                board_panel(true);
                screen_on = true;
                /* The continuous clock is already current; do not dispatch
                 * timers/input before the app shifts its timing origins. */
                lv_display_trigger_activity(NULL);
                board_input_after_wake();
                wf_key_after_wake(&k[0]); wf_key_after_wake(&k[1]);
                wf_app_wake();
            }
        }
    }
}
