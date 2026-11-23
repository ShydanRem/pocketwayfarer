/* Real-LVGL scripted touch/BOOT acceptance and native-resolution evidence.
 * No browser mockup and no screenshots composed from the design board. */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lvgl.h"
#include "rui.h"
#include "art/lantern_art.h"
#include "r_lantern_fx.h"

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
bool wf_platform_save(const void *data, size_t n) {
    assert(n <= sizeof saved);
    memcpy(saved, data, n); saved_len = n; saves++;
    rpg_game back; assert(rpg_save_decode(saved, n, &back));
    return true;
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
static void new_adventure(void) {
    wf_app_start(NULL, 0);
    run(40);
    press(TR("Nuova avventura", "New adventure"));
    press(TR("Inizia", "Begin"));
    assert(R.loaded && R.screen == R_TOWN);
}
static void enter_expedition(void) {
    assert(R.screen == R_TOWN);
    press(TR("Servizi", "Services"));
    press(ln_active(&R.g.lantern) ? TR("Riprendi spedizione", "Resume expedition")
                                  : TR("Spedizione extra", "Side expedition"));
}
static void restore_expedition(void) {
    wf_app_start(saved, saved_len);
    enter_expedition();
    assert(R.screen == R_LANTERN);
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
static void check_hero_clearance(const char *name) {
    bool town = R.screen == R_LANTERN_HOME;
    bool battle = R.screen == R_LANTERN && R.g.lantern.state >= LN_CHOOSE && R.g.lantern.state <= LN_IMPACT;
    if (!town && !battle) return;
    lv_obj_t *screen = lv_screen_active();
    bool found = false;
    for (unsigned i = 0; i < lv_obj_get_child_count(screen); i++) {
        lv_obj_t *o = lv_obj_get_child(screen, (int)i);
        if (!lv_obj_check_type(o, &lv_image_class) || lv_obj_get_width(o) != 176) continue;
        lv_area_t a; lv_obj_get_coords(o, &a);
        int cx = (a.x1+a.x2+1)/2;
        if (cx < (town ? 145 : 105) || cx > (town ? 175 : 150)) continue;
        const lv_image_dsc_t *image = lv_image_get_src(o);
        assert(image->header.cf == LV_COLOR_FORMAT_RGB565A8);
        const uint8_t *alpha = image->data + image->header.stride * image->header.h;
        for (unsigned y = 0; y < image->header.h; y++) for (unsigned x = 0; x < image->header.w; x++) {
            if (!alpha[y*image->header.w+x]) continue;
            /* Test actual expanded sprite pixels against the rendered HUD,
             * rather than its padded transparent canvas. Feet used to fail. */
            assert(a.y1+(int)y > 106 && a.y1+(int)y < (town ? 279 : 285));
            if (battle) {
                int dx = a.x1 + (int)x - CX, dy = a.y1 + (int)y - CY;
                assert(dx * dx + dy * dy < CX * CY && a.x1 + (int)x < CX);
            }
        }
        found = true;
    }
    /* Town subpages share R_LANTERN_HOME but intentionally have no hero. */
    if (!strcmp(name, "borgo") || !strcmp(name, "village-english") || !strcmp(name, "combattimento")) assert(found);
}
static void shot(const char *name) {
    lv_obj_update_layout(lv_screen_active());
    if (R.screen != R_DUNGEON) check_buttons(lv_screen_active());
    check_hero_clearance(name);
    lv_refr_now(NULL);
    memcpy(incremental_fb, fb, sizeof fb);
    lv_obj_invalidate(lv_screen_active());
    lv_refr_now(NULL);
    if (memcmp(incremental_fb, fb, sizeof fb)) {
        int left = WF_RES, top = WF_RES, right = 0, bottom = 0, changed = 0;
        for (int y = 0; y < WF_RES; y++) for (int x = 0; x < WF_RES; x++) {
            unsigned pixel = (unsigned)(y * WF_RES + x);
            if (incremental_fb[pixel] == fb[pixel]) continue;
            if (x < left) left = x;
            if (x > right) right = x;
            if (y < top) top = y;
            if (y > bottom) bottom = y;
            changed++;
        }
        fprintf(stderr, "incremental/full render mismatch: %s (%d pixels at %d,%d..%d,%d)\n",
                name, changed, left, top, right, bottom);
    }
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
        if (l->state == LN_CHOOSE) { press(TR("Attacco", "Attack")); }
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
    assert(R.g.lantern.state == LN_RELIC); press("SCEGLI"); press("Rientra"); press("Borgo"); enter_expedition();
}
static void check_relic_comparison(int incoming, int outgoing) {
    lv_obj_update_layout(lv_screen_active());
    const int relics[] = {incoming, outgoing};
    for (int i = 0; i < 2; i++) {
        lv_obj_t *name = find(lv_screen_active(), TX(ln_relic_name(relics[i])));
        lv_obj_t *text = find(lv_screen_active(), TX(ln_relic_text(relics[i])));
        assert(name && text);
        lv_area_t a, b; lv_obj_get_coords(name, &a); lv_obj_get_coords(text, &b);
        if (a.y2 >= b.y1 || b.y2 > 234 + i * 122)
            fprintf(stderr, "relic comparison overflow: %s name=%d..%d text=%d..%d\n",
                    TX(ln_relic_name(relics[i])), a.y1, a.y2, b.y1, b.y2);
        assert(a.y1 >= 114 + i * 122 && a.y2 < b.y1 && b.y2 <= 234 + i * 122);
        assert(a.x1 >= 166 && a.x2 <= 394 && b.x1 >= 166 && b.x2 <= 394);
    }
    check_buttons(lv_screen_active());
}
static int selected_offer(void) {
    return find(lv_screen_active(), TX(ln_relic_name(R.g.lantern.offers[0]))) ? 0 : 1;
}
static void relic_exchange_ui(void) {
    for (int language = 0; language < 2; language++) for (int first = 0; first < LR_COUNT; first++) {
        wf_lang = R.g.lang = (uint8_t)language;
        rpg_lantern *l = &R.g.lantern;
        ln_start(l, LN_SENTINEL, 882u + (unsigned)first);
        l->state = LN_RELIC; l->room = 3; l->currency = 30;
        l->relics = (uint8_t)((1u << first) | (1u << ((first+1) % LR_COUNT)) | (1u << ((first+2) % LR_COUNT)));
        int n = 0;
        for (int r = 0; r < LR_COUNT && n < 2; r++) if (!(l->relics & (1u << r))) l->offers[n++] = (uint8_t)r;
        r_go(R_LANTERN); r_dirty(); wf_app_flush_save(); run(50);
        rpg_lantern before = *l;
        uint32_t gold = R.g.gold;
        if (!language && !first) {
            shot("reliquie-tre-slot");
            /* Synthetic QA profile for a separate desktop smoke test. */
            char path[512]; snprintf(path, sizeof path, "%s/relic-full-save.bin", dest);
            FILE *fixture = fopen(path, "wb"); assert(fixture);
            assert(fwrite(saved, 1, saved_len, fixture) == saved_len);
            assert(!fclose(fixture));
        }
        press(">"); int choice = selected_offer();
        press(language ? "COMPARE" : "CONFRONTA");
        int outgoing = 0;
        while (!(l->relics & (1u << outgoing))) outgoing++;
        for (int i = 0; i < 3; i++) {
            check_relic_comparison(l->offers[choice], outgoing);
            assert(!memcmp(l, &before, sizeof before));
            if (!first && !i) shot(language ? "confronto-reliquie-en" : "confronto-reliquie");
            press(">");
            do { outgoing = (outgoing + 1) % LR_COUNT; } while (!(l->relics & (1u << outgoing)));
        }
        press(language ? "Back" : "Indietro");
        assert(selected_offer() == choice && !memcmp(l, &before, sizeof before));
        press(language ? "COMPARE" : "CONFRONTA");
        wf_app_key(WF_KEY_BOOT_SHORT); run(50);
        assert(selected_offer() == choice && !memcmp(l, &before, sizeof before));
        press(language ? "COMPARE" : "CONFRONTA");
        wf_app_key(WF_KEY_PWR_SHORT); run(50); wf_app_key(WF_KEY_PWR_SHORT); run(50);
        assert(!memcmp(l, &before, sizeof before));
        check_relic_comparison(l->offers[choice], outgoing);
        wf_app_flush_save(); restore_expedition(); run(50);
        assert(!memcmp(&R.g.lantern, &before, sizeof before) && R.g.gold == gold);
        choice = selected_offer(); press(language ? "COMPARE" : "CONFRONTA");
        l = &R.g.lantern; outgoing = 0;
        while (!(l->relics & (1u << outgoing))) outgoing++;
        int incoming = l->offers[choice];
        press(language ? "Replace" : "Sostituisci");
        assert(l->state == LN_CLEARED && ln_relic_count(l) == 3);
        assert(l->relics == ((before.relics & ~(1u << outgoing)) | (1u << incoming)));
        assert(l->currency == before.currency && l->rng == before.rng && R.g.gold == gold);
        unsigned swapped = l->relics;
        wf_app_flush_save(); restore_expedition(); run(50);
        assert(R.g.lantern.state == LN_CLEARED && R.g.lantern.relics == swapped);
        /* A second reward can be declined without changing the loadout. */
        l = &R.g.lantern; l->state = LN_RELIC;
        r_go(R_LANTERN); unsigned owned = l->relics;
        press(language ? "Skip" : "Lascia");
        assert(l->state == LN_CLEARED && l->relics == owned && R.g.gold == gold);
    }
    puts("relic UI: all six effects IT/EN, three owned choices, touch/BOOT cancel, PWR/restart, atomic replace and skip: passed");
}
static void no_relic_victory_ui(void) {
    wf_lang = R.g.lang = 0;
    R.g.lantern.equipped = 0;
    unsigned projects = R.g.lantern.blueprints, wins = R.g.lantern.wins;
    uint32_t gold = R.g.gold;
    ln_start(&R.g.lantern, LN_SENTINEL, 661);
    r_go(R_LANTERN);
    for (int room = 0; room < 5; room++) {
        assert(R.g.lantern.room == room);
        if (room == 1) { press("Focolare"); press("+25 PV"); }
        else {
            press(room == 4 ? "Affrontalo" : "Sentiero"); play_battle();
            if (room < 4) { assert(R.g.lantern.state == LN_RELIC); press("Lascia"); }
        }
        assert(!R.g.lantern.relics);
        if (room < 4) press("Prosegui");
    }
    assert(R.g.lantern.state == LN_WON && R.g.lantern.wins == wins + 1);
    assert(R.g.gold == gold + 80 && R.g.lantern.blueprints == projects);
    shot("vittoria-senza-reliquie");
    wf_app_flush_save(); restore_expedition(); run(50);
    assert(R.g.lantern.state == LN_WON && R.g.gold == gold + 80 && R.g.lantern.wins == wins + 1);
    press("Borgo");
    puts("no-relic UI: all five rooms, skip every reward, win without blueprint softlock, exactly one gold payout after reload: passed");
}
static void pacing_sample(uint32_t seed, uint32_t start, const char *event, int taps) {
    const rpg_lantern *l = &R.g.lantern;
    printf("%s,%lu,%u,%s,%lu,%d,%d,%d,%u\n", wf_lang ? "en" : "it", (unsigned long)seed, l->room + 1,
           event, (unsigned long)(ms - start), taps, l->hp, l->sp, l->currency);
}

static void pacing_preview(bool bastion) {
    static const uint32_t platform_origins[] = {17, 661};
    puts("language,seed,room,event,simulated_ms,departure_taps,hp,sp,run_gold");
    uint32_t seed_seen[2] = {0};
    for (int lang = 0; lang < 2; lang++) for (int scenario = 0; scenario < 2; scenario++) {
        rpg_new_hero hero = {.lang = (uint8_t)lang};
        rpg_game_new(&R.g, &hero, seconds, 37);
        R.loaded = true;
        R.g.lantern.archetype = LN_SENTINEL;
        r_go(R_LANTERN_HOME);
        run(100);
        if (!lang && !scenario) shot("pacing-borgo");
        rnd = platform_origins[scenario];
        const uint32_t start = ms, gold = R.g.gold;
        press(TR("PARTI", "DEPART"));
        int taps = 1;
        if (R.g.lantern.state != LN_ROUTE) {
            press(TR("Parti", "Depart"));
            taps++;
        }
        assert(R.g.lantern.state == LN_ROUTE && R.g.lantern.archetype == LN_SENTINEL);
        assert(R.g.lantern.hp == 120 && R.g.lantern.sp == 24 && !R.g.lantern.relics);
        const uint32_t seed = R.g.lantern.rng;
        if (lang) assert(seed_seen[scenario] == seed);
        else seed_seen[scenario] = seed;
        pacing_sample(seed, start, "first_choice", taps);
        if (!lang && !scenario) shot("pacing-prima-scelta");
        for (int room = 0; room < 5; room++) {
            assert(R.g.lantern.room == room && R.g.lantern.state == LN_ROUTE);
            if (room == 1) {
                press(TR("Focolare", "Hearth"));
                pacing_sample(seed, start, "rest_open", taps);
                press(TR("+25 PV", "+25 HP"));
                pacing_sample(seed, start, "rest_done", taps);
            } else {
                const char *route = room == 4 ? TR("Affrontalo", "Face the guardian")
                                  : bastion && room == LN_BASTION_ROOM ? TR("Bastione", "Bastion")
                                  : TR("Sentiero", "Trail");
                press(route);
                assert(R.g.lantern.state == LN_CHOOSE);
                pacing_sample(seed, start, "battle_start", taps);
                play_battle();
                if (R.g.lantern.state == LN_LOST) {
                    pacing_sample(seed, start, "defeat", taps);
                    break;
                }
                pacing_sample(seed, start, "battle_end", taps);
                if (room < 4) {
                    assert(R.g.lantern.state == LN_RELIC);
                    press(TR("Lascia", "Skip"));
                    pacing_sample(seed, start, "reward_skipped", taps);
                }
            }
            if (room < 4) {
                press(TR("Prosegui", "Continue"));
                pacing_sample(seed, start, "next_route", taps);
            }
        }
        unsigned reward = bastion ? 95 : 80;
        assert(R.g.lantern.state == LN_WON && R.g.gold == gold + reward && R.g.lantern.wins == 1);
        pacing_sample(seed, start, "victory", taps);
        press(TR("Borgo", "Village"));
        assert(R.screen == R_TOWN && R.g.gold == gold + reward);
        pacing_sample(seed, start, "return", taps);
        if (!lang && !scenario) shot("pacing-ritorno");
    }
}

static void bastion_ui(void) {
    for (int lang = 0; lang < 2; lang++) {
        rpg_new_hero hero = {.lang = (uint8_t)lang};
        rpg_game_new(&R.g, &hero, seconds, 47);
        R.loaded = true;
        rpg_lantern *l = &R.g.lantern;
        R.g.coach.learned = LNC_ALL;
        ln_start(l, LN_SENTINEL, 3294095628u);
        l->room = LN_BASTION_ROOM; l->hp = 12; l->currency = 30;
        r_go(R_LANTERN); r_save_now();
        const char *bastion = TR("Bastione", "Bastion");
        const char *cost = TR("Costo 12 PV", "Cost 12 HP");
        const char *prize = TR("Vinci +25 oro", "Win +25 gold");
        lv_obj_t *card = find(lv_screen_active(), bastion);
        assert(card && find(lv_screen_active(), cost) && find(lv_screen_active(), prize));
        assert(lv_obj_has_state(lv_obj_get_parent(card), LV_STATE_DISABLED));
        assert(find(lv_screen_active(), TR("Servono almeno 13 PV", "Requires at least 13 HP")));
        rpg_lantern before = *l;
        press(bastion);
        assert(!memcmp(l, &before, sizeof *l));
        if (lang) shot("bastion-locked-en");

        l->hp = 90;
        r_save_now();
        restore_expedition();
        assert(l->state == LN_ROUTE && l->hp == 90 && l->currency == 30 && l->rng == before.rng);
        assert(find(lv_screen_active(), cost) && find(lv_screen_active(), prize));
        shot(lang ? "bastion-fork-en" : "bastione-bivio-it");
        const uint32_t gold = R.g.gold;
        press(bastion);
        assert(l->state == LN_CHOOSE && l->hp == 78 && l->currency == 30 && l->branch == LN_PATH_BASTION);
        assert(l->foes[0].max_hp == 88);
        if (!lang) shot("bastione-combattimento-it");
        restore_expedition();
        assert(l->paused && l->hp == 78 && l->branch == LN_PATH_BASTION && l->currency == 30);
        press(TR("Riprendi", "Resume"));
        run(650);
        play_battle();
        assert(l->state == LN_RELIC && l->currency == 55 && R.g.gold == gold);
        restore_expedition();
        assert(l->state == LN_RELIC && l->currency == 55 && R.g.gold == gold);
        press(TR("Lascia", "Skip"));
        press(TR("Rientra", "Return"));
        assert(l->state == LN_RETURNED && R.g.gold == gold + 55);
        restore_expedition();
        assert(l->state == LN_RETURNED && R.g.gold == gold + 55);
        if (!lang) shot("bastione-rientro-it");
        press(TR("Borgo", "Village"));
    }
    puts("Bastion UI: IT/EN forecast, blocked low HP, resume before/after entry and one reward on return passed");
}

static void check_label_clearance(const char *upper, const char *lower) {
    lv_obj_t *screen = lv_screen_active();
    lv_obj_update_layout(screen);
    lv_obj_t *first = find(screen, upper), *second = find(screen, lower);
    assert(first && second);
    lv_area_t a, b;
    lv_obj_get_coords(first, &a);
    lv_obj_get_coords(second, &b);
    assert(a.y2 < b.y1);
}

static void return_feedback_ui(void) {
    for (int lang = 0; lang < 2; lang++) {
        rpg_new_hero hero = {.lang = (uint8_t)lang};
        rpg_game_new(&R.g, &hero, seconds, 47);
        R.loaded = true;
        wf_lang = R.g.lang;
        R.g.coach.learned = LNC_ALL;
        rpg_lantern *l = &R.g.lantern;
        const char *relic = TX(ln_relic_name(LR_BELL));
        const char *effect = TX(ln_relic_text(LR_BELL));
        const uint32_t gold = R.g.gold;
        ln_start(l, LN_SENTINEL, 97);
        l->state = LN_BLUEPRINT;
        l->room = 4;
        l->currency = 95;
        l->relics = 1u << LR_BELL;
        l->blueprints = 1u << LR_COAL;
        r_go(R_LANTERN);
        r_save_now();
        restore_expedition();
        assert(l->state == LN_BLUEPRINT && R.g.gold == gold);
        press(TR("CONSERVA", "KEEP"));
        assert(l->state == LN_WON && R.g.gold == gold + 95 && l->settled == 96 && !l->equipped);
        restore_expedition();
        assert(R.g.gold == gold + 95 && find(lv_screen_active(), TR("Nuovo progetto", "New blueprint")));
        assert(find(lv_screen_active(), relic) && find(lv_screen_active(), effect));
        check_label_clearance(effect, TR("Equipaggia", "Equip"));
        if (!lang) shot("rientro-progetto");

        press(TR("Collezione", "Collection"));
        assert(find(lv_screen_active(), relic) && find(lv_screen_active(), effect));
        if (lang) shot("return-collection-en");
        wf_app_key(WF_KEY_BOOT_SHORT);
        assert(find(lv_screen_active(), TR("Nuovo progetto", "New blueprint")));
        press(TR("Collezione", "Collection"));
        press(TR("Torna", "Back"));
        press(TR("Equipaggia", "Equip"));
        assert(l->equipped == LR_BELL + 1);
        restore_expedition();
        assert(l->equipped == LR_BELL + 1 && R.g.gold == gold + 95);
        assert(find(lv_screen_active(), TR("Pronta alla partenza", "Ready for next run")));
        if (!lang) shot("rientro-reliquia-pronta");
        press(TR("Parti", "Depart"));
        assert(l->state == LN_ROUTE && l->relics == (1u << LR_BELL));
        press(TR("Sentiero", "Trail"));
        assert(l->state == LN_CHOOSE && l->shield == 12);

        l->currency = 55;
        press("II");
        press(TR("Abbandona", "Abandon"));
        assert(l->state == LN_LOST && R.g.gold == gold + 95 + 42);
        assert(find(lv_screen_active(), TR("+42 oro · 13 persi", "+42 gold · 13 lost")));
        if (lang) shot("return-defeat-en");
        press(TR("Riprova", "Retry"));
        assert(l->state == LN_ROUTE && l->relics == (1u << LR_BELL));

        R.g.gold = UINT32_MAX - 2;
        l->state = LN_BLUEPRINT;
        l->room = 4;
        l->currency = 95;
        r_go(R_LANTERN);
        press(TR("CONSERVA", "KEEP"));
        assert(l->state == LN_WON && R.g.gold == UINT32_MAX && l->settled == 3);
        check_label_clearance(TR("+2 oro · 4294967295 tot.", "Gold +2 / 4294967295"),
                              TR("Già in collezione", "Already in collection"));
        press(TR("Borgo", "Village"));
    }
    puts("return UI: IT/EN receipt, power cycle, collection and BOOT, equip and next run, defeat amount and gold cap passed");
}

static int expect_battle_hero(int pose) {
    lv_obj_t *screen = lv_screen_active();
    lv_obj_update_layout(screen);
    for (uint32_t i = 0; i < lv_obj_get_child_count(screen); i++) {
        lv_obj_t *object = lv_obj_get_child(screen, (int32_t)i);
        if (!lv_obj_check_type(object, &lv_image_class) || lv_obj_get_width(object) != 176) continue;
        lv_area_t area;
        lv_obj_get_coords(object, &area);
        int x = area.x1 + 88;
        if (x < 105 || x > 150) continue;
        assert(area.y1 + 88 == LN_HERO_Y);
        unsigned archetype = R.g.lantern.archetype;
        assert(lv_image_get_src(object) == gfx_sprite(&ln_loadouts[archetype][pose], 2));
        check_hero_clearance("combat-pose");
        return x;
    }
    assert(!"battle hero object missing");
    return 0;
}

static void combat_poses(void) {
    const char *names[] = {"sentinella", "piromante", "predone"};
    for (int quiet = 0; quiet < 2; quiet++) for (int a = 0; a < LN_ARCHETYPES; a++) {
        rpg_new_hero hero = {.lang = (uint8_t)quiet};
        rpg_game_new(&R.g, &hero, seconds, 47);
        R.loaded = true;
        wf_lang = R.g.lang;
        R.g.coach.learned = LNC_ALL;
        rpg_lantern *l = &R.g.lantern;
        l->reduced_motion = (uint8_t)quiet;
        ln_start(l, a, 3294095628u);
        l->room = LN_BASTION_ROOM;
        r_go(R_LANTERN);
        press(TR("Bastione", "Bastion"));
        assert(l->state == LN_CHOOSE);

        char name[64];
        press(TR("Attacco", "Attack"));
        assert(l->state == LN_ATTACK && expect_battle_hero(LN_POSE_WINDUP) == LN_HERO_X);
        if (!quiet) { snprintf(name, sizeof name, "carica-%s", names[a]); shot(name); }
        judge_at(WF_Q_PERFECT);
        run(90);
        assert(l->state == LN_IMPACT && l->quality == WF_Q_PERFECT && l->feedback == LN_FEEDBACK_HERO);
        int x = expect_battle_hero(LN_POSE_STRIKE);
        assert(quiet ? x == LN_HERO_X : x > LN_HERO_X);
        if (!quiet) { snprintf(name, sizeof name, "contatto-%s", names[a]); shot(name); }
        run(125);
        int recovery = quiet ? LN_POSE_STRIKE : LN_POSE_RECOVER;
        x = expect_battle_hero(recovery);
        assert(quiet ? x == LN_HERO_X : x > LN_HERO_X);
        if (!quiet) { snprintf(name, sizeof name, "recupero-%s", names[a]); shot(name); }

        press("II");
        rpg_lantern frozen = *l;
        assert(l->paused && l->state == LN_IMPACT);
        int frozen_x = expect_battle_hero(recovery);
        run(850);
        assert(!memcmp(l, &frozen, sizeof *l));
        assert(expect_battle_hero(recovery) == frozen_x);
        restore_expedition();
        assert(!memcmp(l, &frozen, sizeof *l));
        assert(expect_battle_hero(recovery) == frozen_x);
        press(TR("Riprendi", "Resume"));
        run(300);
        assert(l->elapsed_ms == frozen.elapsed_ms && expect_battle_hero(recovery) == frozen_x);
        run(400);
        defend_once(false);

        press(TR("Guardia", "Guard"));
        assert(l->guard && l->state == LN_TELEGRAPH);
        assert(expect_battle_hero(quiet ? LN_POSE_BRACE : LN_POSE_GUARD_ENTRY) == LN_HERO_X);
        if (!quiet) {
            snprintf(name, sizeof name, "entrata-guardia-%s", names[a]); shot(name);
        }
        run(150);
        assert(expect_battle_hero(LN_POSE_BRACE) == LN_HERO_X);
        while (l->state == LN_TELEGRAPH) run(5);
        assert(l->state == LN_PARRY && expect_battle_hero(LN_POSE_BRACE) == LN_HERO_X);
        if (!quiet) { snprintf(name, sizeof name, "parata-%s", names[a]); shot(name); }
        judge_at(WF_Q_PERFECT);
        run(60);
        assert(l->state == LN_IMPACT && l->quality == WF_Q_PERFECT && l->feedback == LN_FEEDBACK_FOE);
        press("II");
        press(TR("Abbandona", "Abandon"));
        press(TR("Borgo", "Village"));
    }
    puts("combat poses: all loadouts, IT/EN, saved phase/restart, guard/parry, reduced motion and visible alpha bounds passed");
}

static void departure_paths(void) {
    for (int lang = 0; lang < 2; lang++) {
        rpg_new_hero hero = {.lang = (uint8_t)lang};
        rpg_game_new(&R.g, &hero, seconds, 47);
        R.loaded = true;
        r_go(R_LANTERN_HOME);
        press(TR("Assetto", "Loadout"));
        press(">");
        assert(R.g.lantern.archetype == LN_MAGE && R.g.lantern.state == LN_NONE);
        if (lang) assert(wf_app_key(WF_KEY_BOOT_SHORT));
        else press("Torna");
        assert(R.screen == R_LANTERN_HOME);
        lv_obj_t *start = find(lv_screen_active(), TR("PARTI", "DEPART"));
        assert(start);
        lv_obj_t *name = find(lv_obj_get_parent(start), TX(ln_archetype_name(LN_MAGE)));
        assert(name);
        lv_obj_update_layout(lv_screen_active());
        lv_area_t action, loadout, button;
        lv_obj_get_coords(start, &action);
        lv_obj_get_coords(name, &loadout);
        lv_obj_get_coords(lv_obj_get_parent(start), &button);
        assert(action.y2 < loadout.y1 && loadout.y2 < button.y2);
        assert(loadout.x1 > button.x1 && loadout.x2 < button.x2);

        const uint32_t gold = R.g.gold;
        assert(rpg_hunt_start(&R.g, 0, seconds) == RPG_OK);
        r_go(R_LANTERN_HOME);
        const uint32_t random_before = rnd;
        press(TR("PARTI", "DEPART"));
        assert(R.screen == R_LANTERN_HOME && R.g.hunt.active && R.g.lantern.state == LN_NONE);
        assert(R.g.gold == gold && rnd == random_before);
        assert(!strcmp(kit_last_toast(), TR("Concludi prima il dungeon o la caccia",
                                             "Finish your dungeon or hunt first")));
        R.g.hunt.active = 0;
        press("...");
        press(TR("Mappa", "Map"));
        press(TR("Parti", "Depart"));
        assert(R.g.lantern.state == LN_ROUTE && R.g.lantern.archetype == LN_MAGE);
        const uint32_t seed = R.g.lantern.rng;
        restore_expedition();
        assert(R.screen == R_LANTERN && R.g.lantern.state == LN_ROUTE && R.g.lantern.rng == seed);
        assert(R.g.lantern.archetype == LN_MAGE && R.g.gold == gold);
        press(TR("Rientra", "Return"));
        press(TR("Borgo", "Village"));
    }
    puts("Direct departure: selected loadout, touch/BOOT back, hunt guard, map start and same saved route after reboot IT/EN passed");
}

int main(int argc, char **argv) {
    assert(argc == 2 || (argc == 3 && (!strcmp(argv[2], "--pacing") || !strcmp(argv[2], "--pacing-bastion"))));
    dest = argv[1];
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
    if (argc == 3) {
        pacing_preview(!strcmp(argv[2], "--pacing-bastion"));
        return 0;
    }
    wf_app_start(NULL, 0); run(150); shot("titolo");
    new_adventure(); enter_expedition(); run(150); shot("borgo");
    press("Assetto"); run(100); shot("assetto");
    press("Torna"); assert(R.screen == R_LANTERN_HOME);
    press("PARTI"); assert(R.g.lantern.state == LN_ROUTE); shot("percorso");
    lv_obj_t *route_name = find(lv_screen_active(), "Fiammella");
    assert(route_name && lv_obj_get_height(route_name) <= 30); /* never wraps into the footer */
    uint32_t route_rng = R.g.lantern.rng;
    uint8_t route_seen = R.g.lantern.seen;
    r_go(R_LANTERN); r_go(R_LANTERN);
    assert(R.g.lantern.rng == route_rng && R.g.lantern.seen == route_seen);
    press("Pontile"); assert(R.g.lantern.state == LN_CHOOSE); run(100); shot("combattimento");
    /* Living enemy selection is exercised through a real touch on its sprite. */
    click(362, 230); assert(R.g.lantern.target == 1);
    click(278, 196); assert(R.g.lantern.target == 0); /* previously intercepted by right foe's transparent margin */
    click(370, 236); assert(R.g.lantern.target == 1);
    click(270, 220); assert(R.g.lantern.target == 0);
    press("II"); assert(R.g.lantern.paused);
    click(410, 220); /* sprite edge outside the pause card */
    assert(R.g.lantern.target == 0);
    press("Riprendi"); run(700);
    click(362, 230); assert(R.g.lantern.target == 1);
    press("Abilità"); run(100); shot("abilita");
    press("Torna"); press("Attacco"); assert(R.g.lantern.state == LN_ATTACK);
    /* Unannounced reboot from the last committed, unpaused action checkpoint. */
    int committed_sp = R.g.lantern.sp;
    restore_expedition(); run(100);
    assert(R.g.lantern.paused && R.g.lantern.elapsed_ms == 0 && R.g.lantern.sp == committed_sp);
    press("Riprendi"); run(650);
    run(250); shot("anello");
    press("II"); assert(R.g.lantern.paused); /* visible pause wins over the tap catcher */
    press("Riprendi"); run(700);
    /* PWR path: the actual app flush serializes a paused timing phase. */
    wf_app_flush_save(); assert(R.g.lantern.paused);
    rpg_lantern suspended = R.g.lantern;
    restore_expedition(); run(200);
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
    restore_expedition(); run(100); assert(R.g.gold == gold0 + 80); /* no duplicate bank */
    press("Borgo"); assert(R.screen == R_TOWN); enter_expedition(); press("Reliquie"); shot("collezione");
    press("Equipaggia"); assert(R.g.lantern.equipped != 0); shot("reliquia-iniziale"); press("Torna");
    press("..."); press("Mappa"); shot("mappa-hd2d"); wf_app_key(WF_KEY_BOOT_SHORT);
    press("..."); press("Bestiario"); shot("bestiario"); press(">"); shot("bestiario-fiammella"); press("Torna");
    press("..."); press("Come si gioca"); shot("guida-intento"); press(">"); shot("guida-sinergie"); press(">"); shot("guida-ritorno"); press("Torna");
    press("Assetto"); press(">"); shot("piromante"); press(">"); shot("predone"); wf_app_key(WF_KEY_BOOT_SHORT);
    press("..."); press("Opzioni"); press("Italiano / English"); shot("options-english"); press("Back");
    shot("village-english");
    press("DEPART"); press("Trail");
    /* Reach loss through real missed timing windows, without modifying HP. */
    for (int guard = 0; guard < 10000 && R.g.lantern.state <= LN_IMPACT; guard++) {
        if (R.g.lantern.state == LN_CHOOSE) press("Guard");
        run(100);
    }
    assert(R.g.lantern.state == LN_LOST); shot("defeat-english");
    press("Retry"); assert(R.g.lantern.state == LN_ROUTE && R.g.lantern.hp == R.g.lantern.max_hp);
    press("Return"); assert(R.g.lantern.state == LN_RETURNED); press("Village"); enter_expedition();
    /* Dedicated UI fixture for the second authored event; full branch behavior
     * is also exercised by the core run simulations. */
    press("DEPART");
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
    run(950); press("Attacco"); run(120);
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
    R.g.run.active = 0; r_go(R_LANTERN_HOME);
    /* Unresolved stairs are recovered even without an active battle marker. */
    R.g.hero.stamina = 100; assert(rpg_dungeon_enter(&R.g, 0, 0, seconds) == RPG_OK);
    R.g.run.row = RPG_ROWS; R.g.run.col = 1; r_go(R_DUNGEON); r_save_now();
    wf_app_start(saved, saved_len); run(50); press("Scendi"); assert(R.g.run.floor == 2);
    R.g.run.active = 0; r_go(R_LANTERN_HOME);
    /* A fresh lesson sequence uses real touch/BOOT. Showcases below start
     * from the existing saved hero; no damage or timing values are injected. */
    uint32_t profile_gold = R.g.gold;
    press("..."); press("Opzioni"); press("Suggerimenti: sì");
    assert(R.g.coach.disabled); press("Ripeti guida");
    assert(!R.g.coach.disabled && !R.g.coach.learned && R.g.gold == profile_gold);
    press("Torna"); press("Assetto"); press(">"); /* Predone -> Sentinella */
    assert(R.g.lantern.archetype == LN_SENTINEL); press("Parti"); press("Sentiero");
    assert(find(lv_screen_active(), TX(ln_coach_caption(LNT_ATTACK)))); shot("guida-primo-attacco");
    press("Attacco"); shot("guida-timing-in-gioco"); judge_at(WF_Q_MISS);
    assert(!(R.g.coach.learned & LNC_ATTACK));
    defend_once(true);
    assert(R.g.coach.learned & LNC_PARRY);
    press("Attacco"); judge_at(WF_Q_GOOD);
    assert(R.g.coach.learned & LNC_ATTACK); run(100); shot("fendente-impatto");
    defend_once(false);
    assert(find(lv_screen_active(), TX(ln_coach_caption(LNT_RESONANCE)))); shot("guida-risonanza"); press("OK");
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
    wf_app_flush_save(); restore_expedition(); run(100);
    assert(R.g.coach.learned == LNC_ALL && R.g.lantern.effect_flags == frozen.effect_flags);
    press("Riprendi"); run(300); assert(R.g.lantern.elapsed_ms == frozen.elapsed_ms);
    run(350); assert(R.g.lantern.hp == frozen.hp && R.g.lantern.sp == frozen.sp && R.g.lantern.rng == frozen.rng);
    return_after_hit();
    press("Assetto"); press(">"); press("Parti"); press("Sentiero");
    assert(R.g.lantern.archetype == LN_MAGE && !find(lv_screen_active(), TX(ln_coach_caption(LNT_ATTACK))));
    shot("seconda-spedizione-senza-guida"); press("Abilità"); press("Brace\n6 PM"); judge_at(WF_Q_PERFECT);
    run(100); shot("brace-impatto"); return_after_hit();
    press("Assetto"); press(">"); press("Parti"); press("Sentiero");
    assert(R.g.lantern.archetype == LN_ROGUE); press("Abilità"); check_buttons(lv_screen_active());
    press("Doppio\n6 PM"); judge_at(WF_Q_PERFECT); run(100); shot("doppio-impatto"); return_after_hit();
    press("..."); press("Opzioni"); press("Movimento ridotto: no");
    assert(R.g.lantern.reduced_motion); press("Torna");
    press("PARTI"); press("Sentiero"); press("Abilità"); press("Doppio\n6 PM"); judge_at(WF_Q_PERFECT);
    run(100); shot("impatto-movimento-ridotto"); return_after_hit();
    press("..."); press("Opzioni"); press("Ripeti guida"); press("Italiano / English"); shot("options-guidance-english"); press("Back");
    press("DEPART"); press("Trail");
    assert(find(lv_screen_active(), TX(ln_coach_caption(LNT_ATTACK)))); shot("first-fight-guidance-english");
    press("II"); press("Abandon"); press("Village");
    relic_exchange_ui();
    press("Return"); press("Village");
    no_relic_victory_ui();
    departure_paths();
    bastion_ui();
    return_feedback_ui();
    combat_poses();
    r_go(R_LANTERN_HOME); run(20);
    size_t stable = image_bytes;
    for (int i = 0; i < 12; i++) { r_go(R_LANTERN_HOME); run(20); }
    assert(image_bytes == stable);
    assert(peak_image_bytes < 2u * 1024u * 1024u);
    printf("lantern UI: %d real-LVGL screenshots, %d validated saves; image buffers peak %zu bytes\n", shots, saves, peak_image_bytes);
    printf("touch/BOOT, all five rooms, target/skill menus, pause/reload/resume, victory/defeat/retry, one-time bank, IT/EN: passed\n");
    printf("classic ring: repeated power cycles, countdown input blocking and pre-entry rollback: passed\n");
    printf("guidance/FX: learned lessons, replay/opt-out, second run, all loadouts, resonance pause/reload, reduced motion and button bounds IT/EN: passed\n");
    return 0;
}
