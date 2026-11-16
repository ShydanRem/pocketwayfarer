/* Real-LVGL scripted touch/BOOT acceptance and native-resolution evidence.
 * No browser mockup and no screenshots composed from the design board. */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lvgl.h"
#include "rui.h"

static uint16_t fb[WF_RES * WF_RES], drawbuf[WF_RES * 40];
static uint16_t incremental_fb[WF_RES * WF_RES];
static uint32_t ms = 1000, seconds = 846756000u, rnd = 17;
static int tx, ty, down, shots, saves;
static const char *dest;
static uint8_t saved[4096];
static size_t saved_len;
static size_t image_bytes, peak_image_bytes;
static struct { void *ptr; size_t size; } allocations[256];
LV_FONT_DECLARE(ln_font_body)
LV_FONT_DECLARE(ln_font_title)
extern int rpg_battle_phase(void);
extern int rpg_battle_ring_quality_now(void);

uint32_t wf_platform_now_s(void) { return seconds; }
uint32_t wf_platform_random(void) { rnd = rnd * 1664525u + 1013904223u; return rnd; }
void *wf_platform_big_alloc(size_t n) {
    void *p = malloc(n); assert(p);
    for (int i = 0; i < 256; i++) if (!allocations[i].ptr) {
        allocations[i].ptr = p; allocations[i].size = n;
        image_bytes += n; if (image_bytes > peak_image_bytes) peak_image_bytes = image_bytes;
        return p;
    }
    assert(0); return NULL;
}
void wf_platform_big_free(void *p) {
    if (!p) return;
    for (int i = 0; i < 256; i++) if (allocations[i].ptr == p) {
        image_bytes -= allocations[i].size; allocations[i].ptr = NULL; free(p); return;
    }
    assert(0);
}
void wf_platform_save(const void *data, size_t n) {
    assert(n <= sizeof saved);
    memcpy(saved, data, n); saved_len = n; saves++;
    rpg_game back; assert(rpg_save_decode(saved, n, &back));
}
void wf_platform_brightness(int p) { (void)p; }
void wf_platform_radio(bool on) { (void)on; }
void wf_platform_link_send(void *c, const uint8_t *p, const uint8_t *d, size_t n) { (void)c; (void)p; (void)d; (void)n; }
static uint32_t tick(void) { return ms; }
static void flush(lv_display_t *d, const lv_area_t *a, uint8_t *pixels) {
    const uint16_t *src = (const uint16_t *)pixels;
    for (int y = a->y1; y <= a->y2; y++) for (int x = a->x1; x <= a->x2; x++) fb[y * WF_RES + x] = *src++;
    lv_display_flush_ready(d);
}
static void read_touch(lv_indev_t *i, lv_indev_data_t *d) {
    (void)i; d->point.x = tx; d->point.y = ty;
    d->state = down ? LV_INDEV_STATE_PRESSED : LV_INDEV_STATE_RELEASED;
}
static void run(unsigned amount) {
    for (unsigned i = 0; i < amount; i += 5) {
        ms += 5;
        if (ms % 1000 < 5) seconds++;
        lv_timer_handler(); wf_app_tick();
    }
}
static void click(int x, int y) { tx = x; ty = y; down = 1; run(40); down = 0; run(50); }
static lv_obj_t *find(lv_obj_t *o, const char *s) {
    if (lv_obj_has_flag(o, LV_OBJ_FLAG_HIDDEN)) return NULL;
    if (lv_obj_check_type(o, &lv_label_class) && !strcmp(lv_label_get_text(o), s)) return o;
    for (uint32_t i = 0; i < lv_obj_get_child_count(o); i++) {
        lv_obj_t *p = find(lv_obj_get_child(o, (int32_t)i), s); if (p) return p;
    }
    return NULL;
}
static void press(const char *s) {
    lv_obj_update_layout(lv_screen_active());
    lv_obj_update_layout(lv_layer_top());
    lv_obj_t *o = find(lv_layer_top(), s);
    if (!o) o = find(lv_screen_active(), s);
    if (!o) fprintf(stderr, "missing UI label: %s (screen=%d run=%u room=%u)\n", s, R.screen, R.g.lantern.state, R.g.lantern.room);
    assert(o);
    lv_area_t a; lv_obj_get_coords(o, &a);
    int x = (a.x1 + a.x2) / 2, y = (a.y1 + a.y2) / 2;
    if ((x - 233) * (x - 233) + (y - 233) * (y - 233) >= 233 * 233)
        fprintf(stderr, "outside circle: %s at %d,%d rectangle %d,%d..%d,%d\n", s, x, y, a.x1, a.y1, a.x2, a.y2);
    assert((x - 233) * (x - 233) + (y - 233) * (y - 233) < 233 * 233);
    click(x, y);
}
static void check_buttons(lv_obj_t *o) {
    if (lv_obj_has_flag(o, LV_OBJ_FLAG_HIDDEN)) return;
    if (lv_obj_check_type(o, &lv_button_class)) {
        lv_obj_t *text = lv_obj_get_child(o, 0);
        if (text && lv_obj_check_type(text, &lv_label_class)) {
            lv_area_t a, b; lv_obj_get_coords(text, &a); lv_obj_get_coords(o, &b);
            if (a.x1 < b.x1+3 || a.x2 > b.x2-3 || a.y1 < b.y1+3 || a.y2 > b.y2-3)
                fprintf(stderr, "clipped button text: %s\n", lv_label_get_text(text));
            assert(a.x1 >= b.x1+3 && a.x2 <= b.x2-3 && a.y1 >= b.y1+3 && a.y2 <= b.y2-3);
        }
    }
    for (uint32_t i = 0; i < lv_obj_get_child_count(o); i++) check_buttons(lv_obj_get_child(o, (int)i));
}
static void shot(const char *name) {
    lv_obj_update_layout(lv_screen_active());
    if (R.screen != R_DUNGEON) check_buttons(lv_screen_active());
    lv_refr_now(NULL);
    memcpy(incremental_fb, fb, sizeof fb);
    lv_obj_invalidate(lv_screen_active());
    lv_refr_now(NULL);
    if (memcmp(incremental_fb, fb, sizeof fb)) fprintf(stderr, "incremental/full render mismatch: %s\n", name);
    assert(!memcmp(incremental_fb, fb, sizeof fb));
    char path[512]; snprintf(path, sizeof path, "%s/%02d-%s.ppm", dest, ++shots, name);
    FILE *f = fopen(path, "wb"); assert(f);
    fprintf(f, "P6\n466 466\n255\n");
    for (int i = 0; i < WF_RES * WF_RES; i++) {
        uint16_t c = fb[i];
        uint8_t p[3] = {(uint8_t)(((c >> 11) << 3) | (c >> 13)),
                        (uint8_t)((((c >> 5) & 63) << 2) | ((c >> 9) & 3)),
                        (uint8_t)(((c & 31) << 3) | ((c >> 2) & 7))};
        fwrite(p, 1, 3, f);
    }
    fclose(f);
}
static void play_battle(void) {
    rpg_lantern *l = &R.g.lantern;
    for (int guard = 0; guard < 15000 && l->state >= LN_CHOOSE && l->state <= LN_IMPACT; guard++) {
        if (l->state == LN_CHOOSE) { press("Attacco"); }
        else if ((l->state == LN_ATTACK || l->state == LN_PARRY) && ln_quality(l) == WF_Q_PERFECT) {
            assert(wf_app_key(WF_KEY_BOOT_SHORT));
        }
        run(5);
    }
    assert(l->state < LN_CHOOSE || l->state > LN_IMPACT);
}
static void judge_at(int quality) {
    for (int i = 0; i < 600 && ln_quality(&R.g.lantern) != quality; i++) run(5);
    assert(ln_quality(&R.g.lantern) == quality);
    assert(wf_app_key(WF_KEY_BOOT_SHORT));
}
static void defend_once(bool capture) {
    while (R.g.lantern.state == LN_IMPACT || R.g.lantern.state == LN_TELEGRAPH) run(5);
    assert(R.g.lantern.state == LN_PARRY);
    if (capture) shot("guida-parata-in-gioco");
    judge_at(WF_Q_PERFECT);
    if (capture) { run(100); shot("parata-perfetta-scudo"); }
    while (R.g.lantern.state == LN_IMPACT) run(5);
    assert(R.g.lantern.state == LN_CHOOSE);
}
static void return_after_hit(void) {
    while (R.g.lantern.state == LN_IMPACT) run(5);
    assert(R.g.lantern.state == LN_RELIC); press("SCEGLI"); press("Rientra"); press("Borgo");
}
int main(int argc, char **argv) {
    assert(argc == 2); dest = argv[1];
    lv_init(); lv_tick_set_cb(tick);
    const uint32_t accents[] = {0x00e0, 0x00e8, 0x00e9, 0x00f2, 0x00f9, 0x00b7};
    for (unsigned i = 0; i < sizeof accents / sizeof accents[0]; i++) {
        lv_font_glyph_dsc_t glyph;
        assert(lv_font_get_glyph_dsc(&ln_font_body, &glyph, accents[i], ' ') && glyph.box_w);
        assert(lv_font_get_glyph_dsc(&ln_font_title, &glyph, accents[i], ' ') && glyph.box_w);
    }
    lv_display_t *d = lv_display_create(WF_RES, WF_RES);
    lv_display_set_buffers(d, drawbuf, NULL, sizeof drawbuf, LV_DISPLAY_RENDER_MODE_PARTIAL);
    lv_display_set_flush_cb(d, flush);
    lv_indev_t *in = lv_indev_create(); lv_indev_set_type(in, LV_INDEV_TYPE_POINTER); lv_indev_set_read_cb(in, read_touch);
    lv_timer_set_period(lv_indev_get_read_timer(in), 10);
    wf_app_start(NULL, 0); run(150); shot("titolo");
    press("Gioca"); assert(R.loaded && R.screen == R_TOWN); run(150); shot("borgo");
    press("PARTI"); run(100); shot("assetto");
    press("Parti"); assert(R.g.lantern.state == LN_ROUTE); shot("percorso");
    lv_obj_t *route_name = find(lv_screen_active(), "Fiammella");
    assert(route_name && lv_obj_get_height(route_name) <= 30); /* never wraps into the footer */
    uint32_t route_rng = R.g.lantern.rng;
    uint8_t route_seen = R.g.lantern.seen;
    r_go(R_LANTERN); r_go(R_LANTERN);
    assert(R.g.lantern.rng == route_rng && R.g.lantern.seen == route_seen);
    press("Pontile"); assert(R.g.lantern.state == LN_CHOOSE); run(100); shot("combattimento");
    /* Living enemy selection is exercised through a real touch on its sprite. */
    click(362, 230); assert(R.g.lantern.target == 1);
    click(230, 195); assert(R.g.lantern.target == 0);
    press("II"); assert(R.g.lantern.paused);
    click(410, 220); /* sprite edge outside the pause card */
    assert(R.g.lantern.target == 0);
    press("Riprendi"); run(700);
    click(362, 230); assert(R.g.lantern.target == 1);
    press("Abilità"); run(100); shot("abilita");
    press("Torna"); press("Attacco"); assert(R.g.lantern.state == LN_ATTACK);
    /* Unannounced reboot from the last committed, unpaused action checkpoint. */
    int committed_sp = R.g.lantern.sp;
    wf_app_start(saved, saved_len); run(100);
    assert(R.g.lantern.paused && R.g.lantern.elapsed_ms == 0 && R.g.lantern.sp == committed_sp);
    press("Riprendi"); run(650);
    run(250); shot("anello");
    press("II"); assert(R.g.lantern.paused); /* visible pause wins over the tap catcher */
    press("Riprendi"); run(700);
    /* PWR path: the actual app flush serializes a paused timing phase. */
    wf_app_flush_save(); assert(R.g.lantern.paused);
    rpg_lantern suspended = R.g.lantern;
    wf_app_start(saved, saved_len); run(200);
    assert(R.g.lantern.paused && R.g.lantern.elapsed_ms == suspended.elapsed_ms);
    assert(R.g.lantern.foes[0].hp == suspended.foes[0].hp && R.g.lantern.foes[1].hp == suspended.foes[1].hp);
    shot("pausa-ripresa");
    press("Riprendi");
    run(300); assert(R.g.lantern.elapsed_ms == suspended.elapsed_ms);
    run(350);
    while (R.g.lantern.state == LN_ATTACK && ln_quality(&R.g.lantern) != WF_Q_PERFECT) run(5);
    assert(wf_app_key(WF_KEY_BOOT_SHORT));
    run(450); assert(R.g.lantern.state == LN_TELEGRAPH || R.g.lantern.state == LN_PARRY);
    while (R.g.lantern.state == LN_TELEGRAPH) run(5);
    run(300); shot("parata");
    play_battle(); assert(R.g.lantern.state == LN_RELIC);
    shot("reliquia"); press(">"); shot("reliquia-confronto"); press("SCEGLI");
    assert(R.g.lantern.state == LN_CLEARED); shot("rischio-ritorno"); press("Prosegui");
    assert(R.g.lantern.room == 1); press("Focolare"); shot("evento"); press("+25 PV"); press("Prosegui");
    for (int room = 2; room <= 4; room++) {
        assert(R.g.lantern.room == room && R.g.lantern.state == LN_ROUTE);
        press(room == 4 ? "Affrontalo" : "Sentiero");
        if (room == 4) shot("guardiano");
        play_battle();
        if (room < 4) { assert(R.g.lantern.state == LN_RELIC); press("SCEGLI"); press("Prosegui"); }
    }
    assert(R.g.lantern.state == LN_BLUEPRINT); shot("progetto");
    uint32_t gold0 = R.g.gold;
    press("CONSERVA"); assert(R.g.lantern.state == LN_WON && R.g.gold == gold0 + 80);
    shot("vittoria");
    wf_app_start(saved, saved_len); run(100); assert(R.g.gold == gold0 + 80); /* no duplicate bank */
    press("Borgo"); assert(R.screen == R_TOWN); press("Zaino"); shot("collezione");
    press("Equipaggia"); assert(R.g.lantern.equipped != 0); shot("reliquia-iniziale"); press("Torna");
    press("..."); press("Mappa"); shot("mappa-hd2d"); wf_app_key(WF_KEY_BOOT_SHORT);
    press("..."); press("Bestiario"); shot("bestiario"); press(">"); shot("bestiario-fiammella"); press("Torna");
    press("..."); press("Come si gioca"); shot("guida-intento"); press(">"); shot("guida-sinergie"); press(">"); shot("guida-ritorno"); press("Torna");
    press("Eroe"); press(">"); shot("piromante"); press(">"); shot("predone"); wf_app_key(WF_KEY_BOOT_SHORT);
    press("..."); press("Opzioni"); press("Italiano / English"); shot("options-english"); press("Back");
    shot("village-english");
    press("DEPART"); press("Depart"); press("Trail");
    /* Reach loss through real missed timing windows, without modifying HP. */
    for (int guard = 0; guard < 10000 && R.g.lantern.state <= LN_IMPACT; guard++) {
        if (R.g.lantern.state == LN_CHOOSE) press("Guard");
        run(100);
    }
    assert(R.g.lantern.state == LN_LOST); shot("defeat-english");
    press("Retry"); assert(R.g.lantern.state == LN_ROUTE && R.g.lantern.hp == R.g.lantern.max_hp);
    press("Return"); assert(R.g.lantern.state == LN_RETURNED); press("Village");
    /* Dedicated UI fixture for the second authored event; full branch behavior
     * is also exercised by the core run simulations. */
    press("DEPART"); press("Depart");
    R.g.lantern.room = 1; r_go(R_LANTERN);
    press("Mirror"); shot("mirror-event"); int hp0 = R.g.lantern.hp;
    press("Offer 12 HP"); assert(R.g.lantern.hp == hp0-12 && R.g.lantern.resonance == 3);
    press("Return"); press("Village");
    /* Verify the classic transaction contract through app restart and actual
     * UI navigation, including an orphaned stairs dialog and a settled boss. */
    wf_lang = R.g.lang = 0;
    R.g.hero.stamina = 100; R.g.hero.hp = 90;
    assert(rpg_dungeon_enter(&R.g, 0, 0, seconds) == RPG_OK);
    R.g.run.floor = 5; rpg_dungeon_floor(&R.g.run); R.g.run.row = RPG_ROWS-1;
    int col = 0; while (!R.g.run.rooms[RPG_ROWS-1][col]) col++;
    R.g.run.col = (int8_t)col; rpg_classic_begin(&R.g);
    rpg_foe legacy_foes[3]; int count;
    assert(rpg_room_enter(&R.g, RPG_ROWS, 1, legacy_foes, &count) == ROOM_BOSS);
    r_start_battle(legacy_foes, count, true, true);
    /* Real classic ring + repeated power cycles, including a second sleep
     * during the preparation countdown. Neither BOOT nor touch may spend the
     * action early, and sleeping time must not expire the timing window. */
    run(950); click(92, 280); run(120);
    assert(rpg_battle_phase() == 3); /* PH_RING */
    int classic_quality = rpg_battle_ring_quality_now();
    wf_app_flush_save(); ms += 20000; seconds += 20; wf_app_wake();
    wf_app_key(WF_KEY_BOOT_SHORT); assert(rpg_battle_phase() == 3);
    run(300); assert(rpg_battle_phase() == 3);
    wf_app_flush_save(); ms += 3000; seconds += 3; wf_app_wake();
    click(233, 230); run(550); /* 640 ms of the new 650 ms wait */
    wf_app_key(WF_KEY_BOOT_SHORT); assert(rpg_battle_phase() == 3);
    run(10); assert(rpg_battle_ring_quality_now() == classic_quality);
    wf_app_key(WF_KEY_BOOT_SHORT); assert(rpg_battle_phase() != 3);
    wf_app_flush_save(); wf_app_start(saved, saved_len); run(50);
    assert(R.screen == R_DUNGEON && R.g.run.row == RPG_ROWS-1 && rpg_room_reachable(&R.g.run, RPG_ROWS, 1));
    shot("classic-boss-recovery");
    /* Completed boss already paid: reload commits navigation without repeating reward. */
    R.g.run.row = RPG_ROWS; R.g.run.col = 1;
    R.g.classic.active = 2; R.g.classic.outcome = B_WON; R.g.classic.dungeon = 1;
    r_save_now(); gold0 = R.g.gold;
    wf_app_start(saved, saved_len); run(50);
    assert(R.g.gold == gold0 && R.g.classic.active == 0);
    press("Scendi"); assert(R.g.run.floor == 6);
    R.g.run.active = 0; r_go(R_TOWN);
    /* Unresolved stairs are recovered even without an active battle marker. */
    R.g.hero.stamina = 100; assert(rpg_dungeon_enter(&R.g, 0, 0, seconds) == RPG_OK);
    R.g.run.row = RPG_ROWS; R.g.run.col = 1; r_go(R_DUNGEON); r_save_now();
    wf_app_start(saved, saved_len); run(50); press("Scendi"); assert(R.g.run.floor == 2);
    R.g.run.active = 0; r_go(R_TOWN);
    /* A fresh lesson sequence uses real touch/BOOT. Showcases below start
     * from the existing saved hero; no damage or timing values are injected. */
    uint32_t profile_gold = R.g.gold;
    press("..."); press("Opzioni"); press("Suggerimenti: sì");
    assert(R.g.coach.disabled); press("Ripeti guida");
    assert(!R.g.coach.disabled && !R.g.coach.learned && R.g.gold == profile_gold);
    press("Torna"); press("PARTI"); press(">"); /* Predone -> Sentinella */
    assert(R.g.lantern.archetype == LN_SENTINEL); press("Parti"); press("Sentiero");
    assert(find(lv_screen_active(), TX(ln_coach_text(LNT_ATTACK)))); shot("guida-primo-attacco");
    press("Attacco"); shot("guida-timing-in-gioco"); judge_at(WF_Q_MISS);
    assert(!(R.g.coach.learned & LNC_ATTACK));
    defend_once(true);
    assert(R.g.coach.learned & LNC_PARRY);
    press("Attacco"); judge_at(WF_Q_GOOD);
    assert(R.g.coach.learned & LNC_ATTACK); run(100); shot("fendente-impatto");
    defend_once(false);
    assert(find(lv_screen_active(), TX(ln_coach_text(LNT_RESONANCE)))); shot("guida-risonanza"); press("OK");
    press("Guardia"); defend_once(false);
    assert(R.g.lantern.resonance == 3); shot("risonanza-pronta");
    press("Abilità");
    lv_obj_t *power_label = find(lv_screen_active(), "Fendente\n6 PM"); assert(power_label);
    assert(lv_obj_get_style_border_width(lv_obj_get_parent(power_label), LV_PART_MAIN) == 2);
    press("Fendente\n6 PM"); judge_at(WF_Q_PERFECT);
    assert((R.g.lantern.effect_flags & LN_EFFECT_RESONANCE) && R.g.coach.learned == LNC_ALL);
    run(100); shot("risonanza-impatto");
    press("II"); rpg_lantern frozen = R.g.lantern;
    run(500); assert(R.g.lantern.elapsed_ms == frozen.elapsed_ms && R.g.lantern.rng == frozen.rng);
    wf_app_flush_save(); wf_app_start(saved, saved_len); run(100);
    assert(R.g.coach.learned == LNC_ALL && R.g.lantern.effect_flags == frozen.effect_flags);
    press("Riprendi"); run(300); assert(R.g.lantern.elapsed_ms == frozen.elapsed_ms);
    run(350); assert(R.g.lantern.hp == frozen.hp && R.g.lantern.sp == frozen.sp && R.g.lantern.rng == frozen.rng);
    return_after_hit();
    press("PARTI"); press(">"); press("Parti"); press("Sentiero");
    assert(R.g.lantern.archetype == LN_MAGE && !find(lv_screen_active(), TX(ln_coach_text(LNT_ATTACK))));
    shot("seconda-spedizione-senza-guida"); press("Abilità"); press("Brace\n6 PM"); judge_at(WF_Q_PERFECT);
    run(100); shot("brace-impatto"); return_after_hit();
    press("PARTI"); press(">"); press("Parti"); press("Sentiero");
    assert(R.g.lantern.archetype == LN_ROGUE); press("Abilità"); check_buttons(lv_screen_active());
    press("Doppio\n6 PM"); judge_at(WF_Q_PERFECT); run(100); shot("doppio-impatto"); return_after_hit();
    press("..."); press("Opzioni"); press("Movimento ridotto: no");
    assert(R.g.lantern.reduced_motion); press("Torna");
    press("PARTI"); press("Parti"); press("Sentiero"); press("Abilità"); press("Doppio\n6 PM"); judge_at(WF_Q_PERFECT);
    run(100); shot("impatto-movimento-ridotto"); return_after_hit();
    press("..."); press("Opzioni"); press("Ripeti guida"); press("Italiano / English"); shot("options-guidance-english"); press("Back");
    press("DEPART"); press("Depart"); press("Trail");
    assert(find(lv_screen_active(), TX(ln_coach_text(LNT_ATTACK)))); shot("first-fight-guidance-english");
    press("II"); press("Abandon"); press("Village");
    size_t stable = image_bytes;
    for (int i = 0; i < 12; i++) { r_go(R_TOWN); run(20); }
    assert(image_bytes == stable);
    assert(peak_image_bytes < 2u * 1024u * 1024u);
    printf("lantern UI: %d real-LVGL screenshots, %d validated saves; image buffers peak %zu bytes\n", shots, saves, peak_image_bytes);
    printf("touch/BOOT, all five rooms, target/skill menus, pause/reload/resume, victory/defeat/retry, one-time bank, IT/EN: passed\n");
    printf("classic ring: repeated power cycles, countdown input blocking and pre-entry rollback: passed\n");
    printf("guidance/FX: learned lessons, replay/opt-out, second run, all loadouts, resonance pause/reload, reduced motion and button bounds IT/EN: passed\n");
    return 0;
}
