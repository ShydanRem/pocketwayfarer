/* Host preview: runs the real UI with a simulated clock, touch and a second
 * "virtual" Pocket for linked play, and writes screenshots (PPM) of the
 * main screens. scripts/previews.py turns them into round PNGs.
 *
 *   preview <out_dir>
 */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lvgl.h"
#include "ui.h"
#include "ui_internal.h"
#include "save.h"

static uint16_t fb[WF_RES * WF_RES];
static uint32_t fake_ms = 1000;
static uint32_t fake_s = 20000u * 86400u + 10u * 3600u;
static const char *out_dir = ".";
static int shot_no;
static uint32_t rnd = 12345;

/* ---- Platform hooks ------------------------------------------------------- */
uint32_t wf_platform_now_s(void) { return fake_s; }
uint32_t wf_platform_random(void) { rnd = rnd * 1664525u + 1013904223u; return rnd >> 3; }
void *wf_platform_big_alloc(size_t n) { return malloc(n); }
void wf_platform_big_free(void *p) { free(p); }
static int saves;
void wf_platform_save(const void *data, size_t len) {
    wf_game back;
    if (!wf_save_decode(data, len, &back, NULL)) { fprintf(stderr, "save roundtrip failed\n"); exit(1); }
    saves++;
}
void wf_platform_brightness(int p) { (void)p; }
static bool radio;
void wf_platform_radio(bool on) { radio = on; }

/* ---- Virtual second Pocket -------------------------------------------------- */
typedef struct { uint8_t data[WF_PACKET_BYTES]; int to_ui; } frame;
static frame air[256];
static int air_n;
static wf_link peer;
static wf_game peer_game;
static const uint8_t ui_mac[6] = {0x24, 0x6f, 0x28, 1, 2, 3};
static const uint8_t peer_mac[6] = {0x24, 0x6f, 0x28, 9, 8, 7};

void wf_platform_link_send(void *ctx, const uint8_t *to, const uint8_t *data, size_t len) {
    (void)ctx; (void)to;
    if (!radio || air_n >= 256) return;
    memcpy(air[air_n].data, data, len);
    air[air_n++].to_ui = 0;
}
static void peer_send(void *ctx, const uint8_t *to, const uint8_t *data, size_t len) {
    (void)ctx; (void)to;
    if (air_n >= 256) return;
    memcpy(air[air_n].data, data, len);
    air[air_n++].to_ui = 1;
}
static bool peer_on;
static void pump_air(void) {
    for (int guard = 0; guard < 4 && air_n; guard++) {
        frame batch[256];
        int n = air_n;
        memcpy(batch, air, sizeof(frame) * (size_t)n);
        air_n = 0;
        for (int i = 0; i < n; i++) {
            if (batch[i].to_ui) wf_ui_link_rx(peer_mac, batch[i].data, WF_PACKET_BYTES);
            else if (peer_on) wf_link_receive(&peer, &peer_game, ui_mac, batch[i].data, WF_PACKET_BYTES, fake_ms, fake_s);
        }
    }
}

/* ---- Display & input --------------------------------------------------------- */
static void flush(lv_display_t *d, const lv_area_t *a, uint8_t *px) {
    const uint16_t *src = (const uint16_t *)px;
    for (int y = a->y1; y <= a->y2; y++)
        for (int x = a->x1; x <= a->x2; x++) fb[y * WF_RES + x] = *src++;
    lv_display_flush_ready(d);
}
static int touch_x, touch_y, touch_down;
static void read_touch(lv_indev_t *i, lv_indev_data_t *d) {
    (void)i;
    d->point.x = touch_x;
    d->point.y = touch_y;
    d->state = touch_down ? LV_INDEV_STATE_PRESSED : LV_INDEV_STATE_RELEASED;
}
static uint32_t tick_cb(void) { return fake_ms; }

static void run(uint32_t ms) {
    for (uint32_t t = 0; t < ms; t += 5) {
        fake_ms += 5;
        if (fake_ms % 1000 < 5) fake_s++;
        lv_timer_handler();
        wf_ui_tick();
        if (peer_on) {
            wf_link_poll(&peer, &peer_game, fake_ms, fake_s);
            wf_link_event ev;
            while (wf_link_next_event(&peer, &ev)) {
                if (ev.kind == WF_LE_DUEL_INVITED) wf_link_duel_respond(&peer, &peer_game, true, fake_ms, fake_s);
            }
            /* The virtual opponent always strikes with good timing. */
            if (peer.duel.state == WF_DUEL_ACTIVE && !peer.duel.my_ready && fake_ms % 700 < 5)
                wf_link_duel_submit(&peer, peer.duel.battle.f[1].focus >= 2 ? WF_ACT_SKILL : WF_ACT_STRIKE, 1, fake_ms);
        }
        pump_air();
    }
}

static void click(int x, int y) {
    touch_x = x; touch_y = y; touch_down = 1;
    run(60);
    touch_down = 0;
    run(80);
}

static void shot(const char *name) {
    lv_refr_now(NULL);
    char path[512];
    snprintf(path, sizeof path, "%s/%02d-%s.ppm", out_dir, ++shot_no, name);
    FILE *f = fopen(path, "wb");
    assert(f);
    fprintf(f, "P6\n%d %d\n255\n", WF_RES, WF_RES);
    for (int i = 0; i < WF_RES * WF_RES; i++) {
        uint16_t c = fb[i];
        uint8_t rgb[3] = {(uint8_t)((c >> 11) << 3 | (c >> 13)), (uint8_t)(((c >> 5) & 63) << 2 | ((c >> 9) & 3)),
                          (uint8_t)((c & 31) << 3 | ((c >> 2) & 7))};
        fwrite(rgb, 1, 3, f);
    }
    fclose(f);
    printf("  %s (screen %d)\n", path, wf_ui_screen());
}

/* Finds the first visible object whose label text contains `text`, clicks its centre. */
static lv_obj_t *find_text(lv_obj_t *o, const char *text) {
    if (lv_obj_has_flag(o, LV_OBJ_FLAG_HIDDEN)) return NULL;
    if (lv_obj_check_type(o, &lv_label_class) && strstr(lv_label_get_text(o), text)) return o;
    for (uint32_t i = 0; i < lv_obj_get_child_count(o); i++) {
        lv_obj_t *r = find_text(lv_obj_get_child(o, (int32_t)i), text);
        if (r) return r;
    }
    return NULL;
}
static bool click_text(const char *text) {
    lv_obj_t *o = find_text(lv_layer_top(), text);
    if (!o) o = find_text(lv_screen_active(), text);
    if (!o) { printf("  (text not found: %s)\n", text); return false; }
    lv_area_t a;
    lv_obj_get_coords(o, &a);
    click((a.x1 + a.x2) / 2, (a.y1 + a.y2) / 2);
    return true;
}

int main(int argc, char **argv) {
    if (argc > 1) out_dir = argv[1];
    lv_init();
    lv_tick_set_cb(tick_cb);
    lv_display_t *d = lv_display_create(WF_RES, WF_RES);
    static uint16_t buf[WF_RES * 40];
    lv_display_set_buffers(d, buf, NULL, sizeof buf, LV_DISPLAY_RENDER_MODE_PARTIAL);
    lv_display_set_flush_cb(d, flush);
    lv_indev_t *in = lv_indev_create();
    lv_indev_set_type(in, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(in, read_touch);

    /* 1. Onboarding and hatching */
    wf_ui_start(NULL);
    run(800);
    shot("benvenuto");
    click_text("Inizia");
    run(2500);
    shot("seme");
    for (int i = 0; i < 24; i++) { click(233, 268); run(60); }
    run(1300);
    shot("nascita");
    click_text("Ciao");
    run(500);

    /* 2. A few hours later: hungry, a little mess, gift to claim */
    wf_game *g = wf_ui_game();
    fake_s += 3 * 3600;
    run(1500);
    g->inv[WF_IT_CHILI] = 3;
    g->inv[WF_IT_STEW] = 1;
    go(WF_SCR_HOME);
    run(600);
    shot("casa");
    click(98, 250);           /* daily gift */
    run(600);
    shot("regalo");
    click_text("Grazie");
    run(400);
    go(WF_SCR_MENU);
    run(500);
    shot("menu");
    go(WF_SCR_FEED);
    run(300);
    shot("cibo");
    click_text("Dai da mangiare");
    run(700);
    shot("pappa");

    /* 3. Battle */
    g->pet.level = 3;
    g->pet.energy = 90000;
    g->pet.mood = 90000;
    wf_step st = {.kind = WF_STEP_BATTLE, .species = WF_SP_FUNGOTTO, .level = 3};
    start_battle(0, &st);
    run(600);
    shot("lotta-incontro");
    run(900);
    shot("lotta-scelta");
    click(400, 175);          /* Attacca */
    run(450);
    shot("lotta-anello");
    /* wait for the spark to reach the golden arc, then tap */
    for (int i = 0; i < 400; i++) {
        extern int battle_ring_quality_now(void);
        if (battle_ring_quality_now() == 3) break;
        run(5);
    }
    click(233, 233);
    run(250);
    shot("lotta-colpo");
    run(2200);
    shot("lotta-parata");
    for (int i = 0; i < 300; i++) {
        extern int battle_parry_quality_now(void);
        if (battle_parry_quality_now() == 3) break;
        run(5);
    }
    click(233, 233);
    run(300);
    shot("lotta-contrattacco");
    for (int round = 0; round < 12 && wf_ui_screen() == WF_SCR_BATTLE; round++) {
        run(3000);
        if (!find_text(lv_layer_top(), "Continua")) {
            click(400, 175);
            run(300);
            for (int i = 0; i < 400; i++) {
                extern int battle_ring_quality_now(void);
                if (battle_ring_quality_now() >= 2) break;
                run(5);
            }
            click(233, 233);
            run(2500);
            for (int i = 0; i < 300; i++) {
                extern int battle_parry_quality_now(void);
                if (battle_parry_quality_now() >= 2) break;
                run(5);
            }
            click(233, 233);
        } else {
            shot("lotta-vittoria");
            click_text("Continua");
            run(500);
            break;
        }
    }

    /* 4. World */
    go(WF_SCR_MAP);
    run(400);
    shot("mappa");
    ui.zone = 0;
    g->zone_progress[0] = 4;
    go(WF_SCR_ZONE);
    run(400);
    shot("zona");

    /* 5. Minigame */
    go(WF_SCR_PLAY);
    run(4200);
    shot("stelle");
    run(17000);
    click_text("Continua");
    run(500);

    /* 6. Training */
    train_reset();
    go(WF_SCR_TRAIN);
    run(300);
    shot("allenamento");

    /* 7. Linked play with a virtual Pocket nearby */
    wf_game_new(&peer_game, "Bea", fake_s, 99);
    while (!wf_warm_seed(&peer_game, fake_s)) {}
    peer_game.pet.form = WF_FORM_SPRUZZO;
    peer_game.pet.level = 6;
    wf_link_init(&peer, peer_send, NULL, 4242);
    peer_on = true;
    go(WF_SCR_LINK);
    run(3500);
    shot("legami");
    click(233 + 130, 233 + 75);   /* first peer slot (30 degrees) */
    run(400);
    shot("legami-amico");
    click_text("Sfida");
    run(1500);
    shot("duello");
    run(1200);
    click(400, 175);
    run(300);
    for (int i = 0; i < 400; i++) {
        extern int battle_ring_quality_now(void);
        if (battle_ring_quality_now() == 3) break;
        run(5);
    }
    click(233, 233);
    run(900);
    shot("duello-round");
    peer_on = false;
    wf_link_duel_leave(&ui.link, fake_ms);

    /* 8. Diary and evolved forms at night */
    g->quests[0].progress = g->quests[0].goal;
    go(WF_SCR_DIARY);
    run(300);
    shot("diario");
    g->pet.form = WF_FORM_PIROLUME;
    g->pet.level = 14;
    g->pet.sleeping = 0;
    g->pet.mess = 2;
    g->pet.hunger = g->pet.energy = g->pet.mood = 90000;
    g->hour_offset = (uint8_t)((22 - (int)(fake_s / 3600u % 24u) + 48) % 24);
    go(WF_SCR_HOME);
    run(600);
    shot("notte");
    g->hour_offset = (uint8_t)((19 - (int)(fake_s / 3600u % 24u) + 48) % 24);
    g->pet.form = WF_FORM_CORALBA;
    g->pet.mess = 0;
    go(WF_SCR_HOME);
    run(600);
    shot("tramonto");
    g->pet.form = WF_FORM_FOGLIOLO;
    g->hour_offset = (uint8_t)((11 - (int)(fake_s / 3600u % 24u) + 48) % 24);
    g->pet.away_since = 0;
    wf_expedition_start(g, 1 < g->zone_unlocked ? 1 : 0, 1, fake_s);
    go(WF_SCR_HOME);
    run(600);
    shot("spedizione");
    printf("preview: %d screenshots, %d saves\n", shot_no, saves);
    return 0;
}
