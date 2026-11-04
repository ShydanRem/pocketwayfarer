/* UI state, routing, onboarding, autosave and link events. */
#include <stdio.h>
#include <string.h>
#include "ui_internal.h"
#include "save.h"

uint8_t wf_lang;

wf_ui_state ui;

const wf_sprite *icon(int id) { return &wf_ui_icons[id]; }
lv_obj_t *pet_screen(int scene) { return kit_screen(scene >= 0 ? &wf_scenes[scene] : NULL); }
lv_obj_t *kit_back_button(lv_obj_t *parent, lv_event_cb_t cb) {
    return kit_round_button(parent, icon(WF_ICON_BACK), NULL, CX, 420, 60, COL_PANEL_HI, cb, 0);
}


static uint32_t last_tick_s;
static uint32_t save_due_s;
static bool radio_on;

uint32_t now_s(void) { return wf_platform_now_s(); }
int ui_hour(void) { return (int)((now_s() / 3600u + ui.g.hour_offset) % 24u); }

void mark_dirty(void) {
    ui.dirty = true;
    uint32_t t = now_s() + 3;
    if (!save_due_s || t < save_due_s) save_due_s = t;
}

void wf_ui_flush_save(void) {
    if (!ui.loaded) return;
    static uint8_t buf[WF_SAVE_BYTES];
    static uint32_t seq;
    size_t n = wf_save_encode(&ui.g, ++seq, buf, sizeof buf);
    if (n) wf_platform_save(buf, n);
    ui.dirty = false;
    save_due_s = 0;
    ui.last_save_s = now_s();
}

wf_game *wf_ui_game(void) { return &ui.g; }
int wf_ui_screen(void) { return ui.screen; }

static void set_radio(bool on) {
    if (on == radio_on) return;
    radio_on = on;
    ui.link.beacon_on = on;
    wf_platform_radio(on);
}

void go(int screen) {
    ui.screen = screen;
    switch (screen) {
    case WF_SCR_ONBOARD: scr_onboard(); break;
    case WF_SCR_HOME: scr_home(); break;
    case WF_SCR_MENU: scr_menu(); break;
    case WF_SCR_FEED: scr_feed(); break;
    case WF_SCR_PLAY: scr_play(); break;
    case WF_SCR_TRAIN: scr_train(); break;
    case WF_SCR_MAP: scr_map(); break;
    case WF_SCR_ZONE: scr_zone(); break;
    case WF_SCR_BATTLE: scr_battle(); break;
    case WF_SCR_DUEL: scr_duel(); break;
    case WF_SCR_LINK: scr_link(); break;
    case WF_SCR_BAG: case WF_SCR_SHOP: ui.bag_tab = screen == WF_SCR_SHOP; scr_bag(); break;
    case WF_SCR_DIARY: scr_diary(); break;
    default: scr_home(); break;
    }
    set_radio(screen == WF_SCR_LINK || screen == WF_SCR_DUEL || ui.link.duel.state != WF_DUEL_IDLE);
}

void wf_ui_goto(int screen) { go(screen); }

void go_home_anim(int anim) {
    ui.pending_anim = anim;
    go(WF_SCR_HOME);
}

/* ---- Onboarding ------------------------------------------------------------ */
static char draft_name[WF_NAME_LEN + 1];
static lv_obj_t *name_label;

static void random_name(void) {
    static const char *const a[] = {"Lu", "Pi", "Ka", "Mo", "Ri", "Ze", "Ta", "No", "Bri", "Fi", "Lo", "Sa",
                                    "Ve", "Ni", "Pu", "Do", "Ama", "Eli", "Oro", "Ste"};
    static const char *const b[] = {"mi", "na", "lo", "ko", "ra", "pa", "ri", "to", "ne", "be", "ssi", "llo"};
    static const char *const c[] = {"", "", "", "n", "x", "l", "o", "a"};
    uint32_t r = wf_platform_random();
    snprintf(draft_name, sizeof draft_name, "%s%s%s", a[r % 20], b[(r >> 8) % 12], c[(r >> 16) % 8]);
}

static void on_reroll(lv_event_t *e) {
    (void)e;
    random_name();
    lv_label_set_text(name_label, draft_name);
}

static void on_begin(lv_event_t *e) {
    (void)e;
    wf_game_new(&ui.g, draft_name, now_s(), wf_platform_random());
    ui.loaded = true;
    wf_ui_flush_save();
    go(WF_SCR_HOME);
    kit_toast("Tocca il seme per scaldarlo!", icon(WF_ICON_EGG));
}

void scr_onboard(void) {
    lv_obj_t *s = pet_screen(WF_SCENE_HOME_NIGHT);
    kit_dim(s, 90);
    lv_obj_t *t = kit_label(s, &wf_font_huge, COL_GOLD, "Pocket\nWayfarer", CX, 92);
    lv_obj_set_style_text_line_space(t, 4, 0);
    lv_obj_t *seed = kit_img(s, &wf_pet_sprites[WF_FORM_SEED][0], PIX, CX, 228);
    kit_bob(seed, 8, 1600);
    kit_label(s, &wf_font_small, COL_TEXT, "Un seme d'astro è caduto\nproprio qui. Come lo chiami?", CX, 325);
    if (!draft_name[0]) random_name();
    lv_obj_t *pill = kit_panel(s, CX - 30, 378, 190, 50);
    name_label = lv_label_create(pill);
    lv_obj_set_style_text_font(name_label, &wf_font_big, 0);
    lv_obj_set_style_text_color(name_label, lv_color_hex(COL_TEXT), 0);
    lv_label_set_text(name_label, draft_name);
    lv_obj_center(name_label);
    kit_round_button(s, icon(WF_ICON_SPARKLE), NULL, CX + 98, 378, 52, COL_BTN_ALT, on_reroll, 0);
    kit_text_button(s, "Inizia!", CX, 434, 150, 46, COL_BTN_OK, on_begin, 0);
}

/* ---- Link events ----------------------------------------------------------- */
static lv_obj_t *invite_dialog;

static void on_invite(lv_event_t *e) {
    bool accept = (intptr_t)lv_event_get_user_data(e) != 0;
    kit_dialog_close(invite_dialog);
    invite_dialog = NULL;
    if (!wf_link_duel_respond(&ui.link, &ui.g, accept, lv_tick_get(), now_s())) return;
    if (accept) go(WF_SCR_DUEL);
}

static void handle_link_events(void) {
    wf_link_event ev;
    char buf[64];
    while (wf_link_next_event(&ui.link, &ev)) {
        switch (ev.kind) {
        case WF_LE_GREETED:
            snprintf(buf, sizeof buf, ev.value ? "%s ti saluta! +Umore" : "%s ti saluta di nuovo", ev.name);
            kit_toast(buf, icon(WF_ICON_HEART));
            mark_dirty();
            break;
        case WF_LE_GREET_DONE:
            snprintf(buf, sizeof buf, ev.value ? "Saluto ricevuto da %s! +Legame" : "Vi siete già salutati oggi", ev.name);
            kit_toast(buf, icon(WF_ICON_HEART));
            mark_dirty();
            break;
        case WF_LE_GIFT_IN:
            snprintf(buf, sizeof buf, "Dono da %s: %s", ev.name, wf_item_get(ev.item)->name);
            kit_toast(buf, &wf_item_icons[ev.item]);
            mark_dirty();
            break;
        case WF_LE_GIFT_DONE:
            kit_toast("Dono consegnato!", icon(WF_ICON_GIFT));
            mark_dirty();
            break;
        case WF_LE_SEND_FAILED:
            kit_toast(ev.value == WF_PKT_GIFT ? "Dono non consegnato (restituito)" : "Nessuna risposta", NULL);
            break;
        case WF_LE_DUEL_INVITED:
            snprintf(buf, sizeof buf, "%s ti sfida a duello!", ev.name);
            invite_dialog = kit_dialog("Sfida!", buf, icon(WF_ICON_SWORD));
            kit_dialog_button(invite_dialog, "Accetta", COL_BTN_OK, on_invite, 1);
            kit_dialog_button(invite_dialog, "Rifiuta", COL_BAD, on_invite, 0);
            break;
        case WF_LE_DUEL_DECLINED:
            kit_toast(ev.value ? "Nessuna risposta alla sfida" : "Sfida rifiutata", NULL);
            if (ui.screen == WF_SCR_DUEL) go(WF_SCR_LINK);
            break;
        case WF_LE_DUEL_STARTED:
            if (ui.screen != WF_SCR_DUEL) go(WF_SCR_DUEL);
            break;
        case WF_LE_DUEL_ROUND:
            if (ui.screen == WF_SCR_DUEL) duel_on_event(&ev);
            break;
        case WF_LE_DUEL_LEFT:
            if (invite_dialog) { kit_dialog_close(invite_dialog); invite_dialog = NULL; }
            if (ui.screen == WF_SCR_DUEL) {
                kit_toast(ev.value ? "Connessione persa" : "L'avversario ha lasciato", NULL);
                go(WF_SCR_LINK);
            }
            break;
        default:
            break;
        }
    }
}

/* ---- Platform API ----------------------------------------------------------- */
void wf_ui_start(const wf_game *loaded) {
    memset(&ui, 0, sizeof ui);
    wf_link_init(&ui.link, wf_platform_link_send, NULL, wf_platform_random());
    ui.link.beacon_on = 0;
    if (loaded) {
        ui.g = *loaded;
        ui.loaded = true;
        wf_game_tick(&ui.g, now_s());
        wf_platform_brightness(ui.g.brightness);
        go(WF_SCR_HOME);
    } else {
        go(WF_SCR_ONBOARD);
    }
    last_tick_s = now_s();
}

void wf_ui_tick(void) {
    uint32_t t = now_s();
    if (ui.loaded && t != last_tick_s) {
        last_tick_s = t;
        if (wf_game_tick(&ui.g, t)) ui.dirty = true;
        if (ui.dirty && (!save_due_s || t >= save_due_s)) {
            if (save_due_s || t - ui.last_save_s >= 300) wf_ui_flush_save();
        }
    }
    if (radio_on) wf_link_poll(&ui.link, &ui.g, lv_tick_get(), t);
    handle_link_events();
    switch (ui.screen) {
    case WF_SCR_HOME: home_tick(); break;
    case WF_SCR_BATTLE: battle_tick(); break;
    case WF_SCR_PLAY: play_tick(); break;
    case WF_SCR_TRAIN: train_tick(); break;
    case WF_SCR_LINK: link_tick(); break;
    case WF_SCR_DUEL: duel_tick(); break;
    default: break;
    }
}

bool wf_ui_key(int key) {
    if (key == WF_KEY_PWR_SHORT) return false;   /* platform handles screen off */
    if ((ui.screen == WF_SCR_BATTLE || ui.screen == WF_SCR_DUEL) && battle_key(key)) return true;
    if (key == WF_KEY_BOOT_SHORT) {
        if (ui.screen == WF_SCR_HOME) go(WF_SCR_MENU);
        else if (ui.screen == WF_SCR_ONBOARD) on_reroll(NULL);
        else if (ui.screen == WF_SCR_ZONE) go(WF_SCR_MAP);
        else if (ui.screen != WF_SCR_BATTLE && ui.screen != WF_SCR_DUEL) go(WF_SCR_HOME);
        return true;
    }
    if (key == WF_KEY_BOOT_LONG && ui.screen != WF_SCR_ONBOARD) {
        go(WF_SCR_HOME);
        return true;
    }
    return false;
}

void wf_ui_link_rx(const uint8_t from[6], const uint8_t *data, size_t len) {
    if (!ui.loaded) return;
    wf_link_receive(&ui.link, &ui.g, from, data, len, lv_tick_get(), now_s());
}

/* ---- App interface (common/platform.h) -------------------------------------- */
const char *wf_app_save_key(void) { return "pet"; }
size_t wf_app_save_max(void) { return WF_SAVE_BYTES; }
void wf_app_start(const uint8_t *save, size_t len) {
    static wf_game loaded;
    bool ok = save && wf_save_decode(save, len, &loaded, NULL);
    wf_ui_start(ok ? &loaded : NULL);
}
void wf_app_tick(void) { wf_ui_tick(); }
bool wf_app_key(int key) { return wf_ui_key(key); }
void wf_app_link_rx(const uint8_t from[6], const uint8_t *data, size_t len) { wf_ui_link_rx(from, data, len); }
void wf_app_flush_save(void) { wf_ui_flush_save(); }
bool wf_app_busy(void) {
    int s = ui.screen;
    return s == WF_SCR_BATTLE || s == WF_SCR_DUEL || s == WF_SCR_PLAY || s == WF_SCR_TRAIN || s == WF_SCR_LINK;
}
void wf_app_wake(void) { go(WF_SCR_HOME); }
