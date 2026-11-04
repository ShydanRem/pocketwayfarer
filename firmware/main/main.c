/* Pocket Wayfarer firmware entry point: storage, ESP-NOW radio, keys and power. */
#include <string.h>
#include "board.h"
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
#include "save.h"
#include "ui.h"

static const char *TAG = "wayfarer";
#define NVS_PART "wf_nvs"
#define RADIO_CHANNEL 1
#define IDLE_OFF_MS 45000u

/* ---- Storage ------------------------------------------------------------------ */
static uint32_t save_seq;

static bool load_game(wf_game *g) {
    nvs_handle_t h;
    if (nvs_open_from_partition(NVS_PART, "wayfarer", NVS_READONLY, &h) != ESP_OK) return false;
    static uint8_t buf[WF_SAVE_BYTES];
    size_t len = sizeof buf;
    bool ok = nvs_get_blob(h, "save", buf, &len) == ESP_OK && wf_save_decode(buf, len, g, &save_seq);
    nvs_close(h);
    return ok;
}

void wf_platform_save(const wf_game *g) {
    static uint8_t buf[WF_SAVE_BYTES];
    size_t n = wf_save_encode(g, ++save_seq, buf, sizeof buf);
    nvs_handle_t h;
    if (!n || nvs_open_from_partition(NVS_PART, "wayfarer", NVS_READWRITE, &h) != ESP_OK) return;
    /* NVS commits a blob atomically: a power cut keeps the previous save. */
    if (nvs_set_blob(h, "save", buf, n) != ESP_OK || nvs_commit(h) != ESP_OK) ESP_LOGW(TAG, "save failed");
    nvs_close(h);
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
    uint8_t data[WF_PACKET_BYTES];
} rx_frame;

static QueueHandle_t rx_queue;
static bool wifi_ready, radio_on;
static const uint8_t broadcast[6] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};

static void on_recv(const esp_now_recv_info_t *info, const uint8_t *data, int len) {
    if (len != WF_PACKET_BYTES || !rx_queue) return;
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
typedef struct {
    int level;
    int64_t down_at;
    bool long_sent;
} key_state;

static void keys(key_state k[2], bool *pwr_click, bool *boot_click, bool *boot_long) {
    int levels[2];
    board_buttons(levels);
    int64_t now = lv_tick_get();
    *pwr_click = *boot_click = *boot_long = false;
    for (int i = 0; i < 2; i++) {
        int lv = levels[i] < 0 ? 0 : levels[i];
        if (lv && !k[i].level) { k[i].down_at = now; k[i].long_sent = false; }
        if (lv && i == 1 && !k[i].long_sent && now - k[i].down_at > 700) { k[i].long_sent = true; *boot_long = true; }
        if (!lv && k[i].level && !k[i].long_sent && now - k[i].down_at > 30) {
            if (i == 0) *pwr_click = true;
            else *boot_click = true;
        }
        k[i].level = lv;
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
    static wf_game saved;
    bool have = load_game(&saved);
    wf_ui_start(have ? &saved : NULL);

    key_state k[2] = {0};
    bool screen_on = true;
    for (;;) {
        bool pwr, boot, boot_long;
        keys(k, &pwr, &boot, &boot_long);
        if (screen_on) {
            board_tick();
            wf_ui_tick();
            rx_frame f;
            while (xQueueReceive(rx_queue, &f, 0) == pdTRUE) wf_ui_link_rx(f.from, f.data, f.len);
            if (boot) wf_ui_key(WF_KEY_BOOT_SHORT);
            if (boot_long) wf_ui_key(WF_KEY_BOOT_LONG);
            int scr = wf_ui_screen();
            bool busy = scr == WF_SCR_BATTLE || scr == WF_SCR_DUEL || scr == WF_SCR_PLAY || scr == WF_SCR_TRAIN ||
                        scr == WF_SCR_LINK;
            if (pwr || (!busy && lv_display_get_inactive_time(NULL) > IDLE_OFF_MS)) {
                wf_ui_flush_save();
                wf_platform_radio(false);
                board_panel(false);
                screen_on = false;
                continue;
            }
            vTaskDelay(pdMS_TO_TICKS(4));
        } else {
            if (board_wait_wake()) {
                board_panel(true);
                lv_display_trigger_activity(NULL);
                screen_on = true;
                /* Swallow the key/touch that woke us up. */
                while (k[0].level || k[1].level) { keys(k, &pwr, &boot, &boot_long); vTaskDelay(pdMS_TO_TICKS(20)); }
                wf_ui_goto(WF_SCR_HOME);
            }
        }
    }
}
