/* Host preview of the RPG: plays a scripted session with the real UI and
 * writes screenshots (PPM). scripts/previews.py turns them into round PNGs. */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lvgl.h"
#include "rui.h"

static uint16_t fb[WF_RES * WF_RES];
static uint32_t fake_ms = 1000;
static uint32_t fake_s = 9800u * 86400u + 10u * 3600u;
static const char *out_dir = ".";
static int shot_no, saves;
static uint32_t rnd = 4242;

uint32_t wf_platform_now_s(void) { return fake_s; }
uint32_t wf_platform_random(void) { rnd = rnd * 1664525u + 1013904223u; return rnd >> 3; }
void *wf_platform_big_alloc(size_t n) { return malloc(n); }
void wf_platform_big_free(void *p) { free(p); }
void wf_platform_save(const void *data, size_t len) {
    rpg_game back;
    if (!rpg_save_decode(data, len, &back)) { fprintf(stderr, "save roundtrip failed\n"); exit(1); }
    saves++;
}
void wf_platform_brightness(int p) { (void)p; }
void wf_platform_radio(bool on) { (void)on; }
void wf_platform_link_send(void *c, const uint8_t *p, const uint8_t *d, size_t n) { (void)c; (void)p; (void)d; (void)n; }

static void flush(lv_display_t *d, const lv_area_t *a, uint8_t *px) {
    const uint16_t *src = (const uint16_t *)px;
    for (int y = a->y1; y <= a->y2; y++)
        for (int x = a->x1; x <= a->x2; x++) fb[y * WF_RES + x] = *src++;
    lv_display_flush_ready(d);
}
static int tx_, ty_, down;
static void read_touch(lv_indev_t *i, lv_indev_data_t *d) {
    (void)i;
    d->point.x = tx_; d->point.y = ty_;
    d->state = down ? LV_INDEV_STATE_PRESSED : LV_INDEV_STATE_RELEASED;
}
static uint32_t tick_cb(void) { return fake_ms; }

static void run(uint32_t ms) {
    for (uint32_t t = 0; t < ms; t += 5) {
        fake_ms += 5;
        if (fake_ms % 1000 < 5) fake_s++;
        lv_timer_handler();
        wf_app_tick();
    }
}
static void click(int x, int y) { tx_ = x; ty_ = y; down = 1; run(50); down = 0; run(90); }

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
    printf("  %s (screen %d)\n", path, R.screen);
}

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

extern int rpg_battle_ring_quality_now(void);
extern int rpg_battle_parry_quality_now(void);
extern int rpg_battle_phase(void);

/* Plays the current battle by hand: attack with good timing, parry well. */
static void play_battle(int max_ms) {
    for (int t = 0; t < max_ms && R.screen == R_BATTLE; t += 20) {
        if (find_text(lv_layer_top(), "Continua") || find_text(lv_layer_top(), "Continue")) break;
        int ph = rpg_battle_phase();
        if (ph == 1) click(92, 280);                                   /* attack */
        else if (rpg_battle_ring_quality_now() >= 2) click(233, 233);
        else if (rpg_battle_parry_quality_now() >= 2) click(233, 233);
        run(20);
    }
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

    wf_app_start(NULL, 0);
    run(500);
    shot("titolo");
    click_text("Nuova avventura");
    run(300);
    shot("crea-razza");
    click(374, 300);                 /* elf */
    run(100);
    click_text("Avanti");            /* skin */
    click(374, 300); click(374, 300);
    click_text("Avanti");            /* hair */
    click(374, 300); click(374, 300);
    click_text("Avanti");            /* hair colour */
    click(374, 300); click(374, 300); click(374, 300);
    shot("crea-capelli");
    click_text("Avanti");            /* eyes */
    click(374, 300);
    click_text("Avanti");            /* face */
    click_text("Avanti");            /* class */
    click(374, 300);                 /* mage */
    run(100);
    shot("crea-classe");
    click_text("Avanti");            /* name */
    click_text("Inizia");
    run(400);
    shot("benvenuto");
    click_text("Andiamo");
    run(300);
    shot("citta");

    rpg_game *g = &R.g;
    r_go(R_MAP);
    run(300);
    shot("mappa");
    R.zone = 0;
    r_go(R_ZONE);
    run(300);
    shot("zona");
    click(124, 210);
    run(700);
    shot("lotta-inizio");
    run(400);
    shot("lotta-scelta");
    click(92, 280);
    run(300);
    shot("lotta-anello");
    for (int i = 0; i < 400 && rpg_battle_ring_quality_now() != 3; i++) run(5);
    click(233, 233);
    run(300);
    shot("lotta-colpo");
    for (int i = 0; i < 600 && rpg_battle_phase() != 6 && rpg_battle_phase() != 1 && rpg_battle_phase() != 7; i++) run(10);
    if (rpg_battle_phase() == 6) { run(250); shot("lotta-parata"); }
    play_battle(60000);
    run(300);
    shot("lotta-fine");
    click_text("Continua");
    run(300);

    /* A stronger hero for the later screens. */
    for (int i = 0; i < 12; i++) rpg_gain_xp(g, rpg_xp_next(g->hero.level), rpg_job_xp_next(g->hero.cls, g->hero.job_level) * 3, NULL);
    while (g->hero.job_level < 30) rpg_gain_xp(g, 0, rpg_job_xp_next(g->hero.cls, g->hero.job_level), NULL);
    while (g->hero.level < 22) rpg_gain_xp(g, rpg_xp_next(g->hero.level), 0, NULL);
    wf_rng r;
    wf_rng_seed(&r, 3);
    for (int i = 0; i < 10; i++) {
        rpg_item it;
        rpg_item_roll(&it, &r, 20, 40, 1 << FAM_MAGE);
        rpg_bag_add(g, &it);
    }
    g->gems[SP_WOLF] = 2;
    g->cons[CO_ORE] = 10;
    g->gold = 5000;
    r_go(R_HERO);
    run(200);
    shot("eroe");
    r_go(R_SKILLS);
    run(200);
    shot("abilita");
    r_go(R_CLASS);
    run(200);
    shot("classe");
    click(153, 330);
    run(200);
    click_text("Diventa");
    run(300);
    shot("nuova-classe");
    click_text("Evviva");
    run(200);
    for (int i = 0; i < RPG_BAG; i++) if (!rpg_item_empty(&g->bag[i]) && rpg_can_equip(g, &g->bag[i])) rpg_equip(g, i);
    r_go(R_EQUIP);
    run(200);
    shot("equip");
    R.bag_tab = 0;
    r_go(R_BAG);
    run(200);
    shot("zaino");
    click(233 - 76, 170);
    run(200);
    shot("oggetto");
    r_go(R_SHOP);
    R.shop_tab = 1;
    r_go(R_SHOP);
    run(200);
    shot("bottega");
    R.forge_pick = SLOT_WEAPON;
    r_go(R_FORGE);
    run(200);
    shot("forgia");

    /* Dungeon */
    g->hero.stamina = 100;
    rpg_dungeon_enter(g, 0, 0, fake_s);
    r_go(R_DUNGEON);
    run(300);
    shot("dungeon");
    for (int c = 0; c < 3; c++) if (rpg_room_reachable(&g->run, 0, c)) { click(140 + c * 93, 360); break; }
    run(800);
    shot("stanza");
    if (R.screen == R_BATTLE) {
        g->auto_battle = 1;
        play_battle(60000);
        run(200);
        shot("dungeon-lotta");
        click_text("Continua");
        run(300);
    } else {
        click_text("Avanti");
        run(200);
    }
    shot("dungeon-avanti");

    /* Boss fight in the dungeon with three foes view and the boss. */
    g->auto_battle = 0;
    rpg_foe foes[3];
    rpg_foe_make(&foes[0], SP_SKELETON, 8, false);
    rpg_foe_make(&foes[1], SP_GHOST, 8, true);
    rpg_foe_make(&foes[2], SP_BAT, 7, false);
    r_start_battle(foes, 3, true, false);
    run(1300);
    shot("tre-nemici");
    click(126, 366);
    run(300);
    shot("abilita-menu");
    run(100);
    rpg_dungeon_leave(g, false);
    rpg_foe_make(&foes[0], SP_BONE_KING, 12, false);
    r_start_battle(foes, 1, false, true);
    run(1300);
    shot("boss");

    /* Idle hunting and town at night */
    R.screen = R_TOWN;
    rpg_hunt_start(g, 0, fake_s);
    fake_s += 3 * 3600;
    g->hour_offset = 12;
    r_go(R_TOWN);
    run(1200);
    shot("citta-notte-caccia");
    click(233, 146);
    run(300);
    shot("caccia-bottino");
    click_text("Ok");
    r_go(R_QUESTS);
    run(200);
    shot("missioni");
    wf_lang = 1;
    g->lang = 1;
    g->hour_offset = 0;
    r_go(R_TOWN);
    run(300);
    shot("town-english");
    printf("preview: %d screenshots, %d saves\n", shot_no, saves);
    return 0;
}
