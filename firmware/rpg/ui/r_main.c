/* RPG UI: state, routing, saving, title, hero creation and the town hub. */
#include <math.h>
#include <stdio.h>
#include <string.h>
#include "rui.h"

rpg_ui_state R;

uint32_t r_now(void) { return wf_platform_now_s(); }
int r_hour(void) { return (int)((r_now() / 3600u + R.g.hour_offset) % 24u); }
const wf_sprite *ri(int id) { return &rpg_ui_icons[id]; }

lv_obj_t *r_scene(int scene) {
    lv_obj_t *s = kit_screen(&rpg_scenes[scene]);
    const rpg_scene_fx *f = &rpg_scene_fxs[scene];
    fx_scene(s, f->lights, f->nlights, f->particles, f->color, 16);
    return s;
}

void r_dirty(void) {
    R.dirty = true;
    uint32_t t = r_now() + 3;
    if (!R.save_due_s || t < R.save_due_s) R.save_due_s = t;
}

void r_save_now(void) {
    if (!R.loaded) return;
    static uint8_t buf[4096];
    size_t n = rpg_save_encode(&R.g, buf, sizeof buf);
    if (n) wf_platform_save(buf, n);
    R.dirty = false;
    R.save_due_s = 0;
    R.last_save_s = r_now();
}

void r_go(int screen) {
    R.screen = screen;
    switch (screen) {
    case R_TITLE: scr_title(); break;
    case R_CREATE: scr_create(); break;
    case R_TOWN: scr_town(); break;
    case R_MAP: scr_map(); break;
    case R_ZONE: scr_zone(); break;
    case R_BATTLE: scr_battle(); break;
    case R_DUNGEON: scr_dungeon(); break;
    case R_HERO: scr_hero(); break;
    case R_SKILLS: scr_skills(); break;
    case R_CLASS: scr_class(); break;
    case R_EQUIP: scr_equip(); break;
    case R_BAG: scr_bag(); break;
    case R_SHOP: scr_shop(); break;
    case R_FORGE: scr_forge(); break;
    case R_QUESTS: scr_quests(); break;
    default: scr_town(); break;
    }
}

static void on_go(lv_event_t *e) { r_go((int)(intptr_t)lv_event_get_user_data(e)); }

lv_obj_t *r_back(lv_obj_t *parent, int screen) {
    return kit_round_button(parent, ri(RI_BACK), NULL, CX, 428, 50, COL_PANEL_HI, on_go, screen);
}

/* ---- Title ----------------------------------------------------------------------- */
static lv_obj_t *dlg;
static void close_dlg(lv_event_t *e) { (void)e; kit_dialog_close(dlg); dlg = NULL; }

static void on_lang(lv_event_t *e) {
    wf_lang = (uint8_t)(intptr_t)lv_event_get_user_data(e);
    r_go(R_TITLE);
}

void scr_title(void) {
    lv_obj_t *s = r_scene(RS_TITLE);
    lv_obj_t *t = kit_label(s, &wf_font_huge, COL_GOLD, "Pocket\nWayfarer", CX, 116);
    lv_obj_set_style_text_line_space(t, 4, 0);
    kit_label(s, &wf_font_small, COL_TEXT, TR("Un RPG tascabile", "A pocket RPG"), CX, 182);
    kit_text_button(s, TR("Nuova avventura", "New adventure"), CX, 330, 230, 54, COL_BTN_OK, on_go, R_CREATE);
    kit_text_button(s, "Italiano", CX - 64, 396, 116, 44, wf_lang ? COL_PANEL_HI : COL_BTN, on_lang, 0);
    kit_text_button(s, "English", CX + 64, 396, 116, 44, wf_lang ? COL_BTN : COL_PANEL_HI, on_lang, 1);
}

/* ---- Hero creation ------------------------------------------------------------------- */
enum { CS_RACE, CS_SKIN, CS_HAIR, CS_COLOR, CS_EYES, CS_BODY, CS_CLASS, CS_NAME, CS_COUNT };
static rpg_new_hero draft;
static int cstep;
static lv_obj_t *c_hero, *c_title, *c_value, *c_blurb, *c_next;
static uint32_t c_blink;

static const uint8_t start_weapon[FAM_COUNT] = {IT_SWORD, IT_STAFF, IT_BOW, IT_DAGGER, IT_MACE, IT_KNUCKLE, IT_HAMMER, IT_SCYTHE};
static const uint8_t start_off[FAM_COUNT] = {IT_SHIELD, IT_TOME, IT_QUIVER, IT_CHARM, IT_SHIELD, IT_CHARM, IT_SHIELD, IT_TOME};

static void draft_look(hero_look *l) {
    memset(l, 0, sizeof *l);
    l->race = draft.race; l->skin = draft.skin; l->eyes = draft.eyes; l->hair = draft.hair;
    l->hair_color = draft.hair_color; l->body = draft.body; l->fam = draft.cls; l->tier = 0;
    l->weapon = start_weapon[draft.cls]; l->off = start_off[draft.cls]; l->head = 0xff;
}

static void random_name(void) {
    static const char *const a[] = {"Ar", "Bel", "Cor", "Dan", "El", "Fen", "Gal", "Il", "Kael", "Lor", "Mir", "Nym",
                                    "Or", "Ser", "Tal", "Val", "Wyn", "Zel", "Ae", "Ro"};
    static const char *const b[] = {"an", "ia", "or", "en", "is", "ar", "wyn", "eth", "us", "a", "ion", "dra"};
    uint32_t r = wf_platform_random();
    snprintf(draft.name, sizeof draft.name, "%s%s", a[r % 20], b[(r >> 8) % 12]);
}

static void create_refresh(void) {
    static const wf_text steps[CS_COUNT] = {{"Razza", "Race"}, {"Carnagione", "Skin"}, {"Capelli", "Hair"},
                                            {"Colore capelli", "Hair colour"}, {"Occhi", "Eyes"},
                                            {"Viso", "Face"}, {"Classe", "Class"}, {"Nome", "Name"}};
    static const wf_text hair[6] = {{"Corti", "Short"}, {"A punta", "Spiky"}, {"Lunghi", "Long"},
                                    {"Coda", "Ponytail"}, {"Caschetto", "Bob"}, {"Trecce", "Braids"}};
    static const wf_text eyes[4] = {{"Nocciola", "Hazel"}, {"Azzurri", "Blue"}, {"Verdi", "Green"}, {"Viola", "Violet"}};
    char buf[96];
    snprintf(buf, sizeof buf, "%d/%d  %s", cstep + 1, CS_COUNT, TX(steps[cstep]));
    lv_label_set_text(c_title, buf);
    const char *blurb = "";
    switch (cstep) {
    case CS_RACE: lv_label_set_text(c_value, TX(rpg_race(draft.race)->name)); blurb = TX(rpg_race(draft.race)->blurb); break;
    case CS_SKIN: snprintf(buf, sizeof buf, "%d / 5", draft.skin + 1); lv_label_set_text(c_value, buf); break;
    case CS_HAIR: lv_label_set_text(c_value, TX(hair[draft.hair])); break;
    case CS_COLOR: snprintf(buf, sizeof buf, "%d / 8", draft.hair_color + 1); lv_label_set_text(c_value, buf); break;
    case CS_EYES: lv_label_set_text(c_value, TX(eyes[draft.eyes])); break;
    case CS_BODY: lv_label_set_text(c_value, draft.body ? TR("Ciglia lunghe", "Long lashes") : TR("Semplice", "Simple")); break;
    case CS_CLASS:
        lv_label_set_text(c_value, TX(rpg_class(draft.cls)->name));
        blurb = TX(rpg_class(draft.cls)->blurb);
        break;
    default: lv_label_set_text(c_value, draft.name); blurb = TR("Tocca le frecce per un altro nome", "Tap the arrows for another name"); break;
    }
    lv_label_set_text(c_blurb, blurb);
    lv_label_set_text(lv_obj_get_child(c_next, 0), cstep == CS_COUNT - 1 ? TR("Inizia!", "Begin!") : TR("Avanti", "Next"));
    hero_look l;
    draft_look(&l);
    hero_obj_set(c_hero, &l, HX_IDLE, 3);
}

static void on_cycle(lv_event_t *e) {
    int d = (int)(intptr_t)lv_event_get_user_data(e);
    switch (cstep) {
    case CS_RACE: draft.race = (uint8_t)((draft.race + RACE_COUNT + d) % RACE_COUNT); break;
    case CS_SKIN: draft.skin = (uint8_t)((draft.skin + 5 + d) % 5); break;
    case CS_HAIR: draft.hair = (uint8_t)((draft.hair + 6 + d) % 6); break;
    case CS_COLOR: draft.hair_color = (uint8_t)((draft.hair_color + 8 + d) % 8); break;
    case CS_EYES: draft.eyes = (uint8_t)((draft.eyes + 4 + d) % 4); break;
    case CS_BODY: draft.body ^= 1; break;
    case CS_CLASS: draft.cls = (uint8_t)((draft.cls + FAM_COUNT + d) % FAM_COUNT); break;
    default: random_name(); break;
    }
    create_refresh();
}

static void on_step(lv_event_t *e) {
    int d = (int)(intptr_t)lv_event_get_user_data(e);
    if (d < 0 && cstep == 0) { r_go(R_TITLE); return; }
    if (d > 0 && cstep == CS_COUNT - 1) {
        draft.lang = wf_lang;
        rpg_game_new(&R.g, &draft, r_now(), wf_platform_random());
        R.loaded = true;
        r_save_now();
        r_go(R_TOWN);
        char body[160];
        snprintf(body, sizeof body,
                 TR("Benvenuto a Borgo Lanterna, %s! Esplora i campi, scendi nei dungeon e diventa leggenda.",
                    "Welcome to Lantern Hollow, %s! Roam the fields, delve into dungeons and become a legend."),
                 R.g.hero.name);
        dlg = kit_dialog(TX(rpg_class(R.g.hero.cls)->name), body, ri(RI_CROWN));
        kit_dialog_button(dlg, TR("Andiamo!", "Let's go!"), COL_BTN_OK, close_dlg, 0);
        return;
    }
    cstep += d;
    create_refresh();
}

void scr_create(void) {
    lv_obj_t *s = r_scene(RS_TOWN);
    kit_dim(s, 120);
    if (!draft.name[0]) {
        random_name();
        draft.skin = 1;
        draft.hair_color = (uint8_t)(wf_platform_random() % 8);
    }
    cstep = 0;
    c_title = kit_label(s, &wf_font_small, COL_GOLD, "", CX, 58);
    hero_look l;
    draft_look(&l);
    c_hero = hero_obj(s, &l, HX_IDLE, 3, CX, 168);
    kit_round_button(s, ri(RI_BACK), NULL, 92, 300, 56, COL_PANEL_HI, on_cycle, -1);
    lv_obj_t *nx = kit_round_button(s, ri(RI_BACK), NULL, 374, 300, 56, COL_PANEL_HI, on_cycle, 1);
    lv_image_set_rotation(lv_obj_get_child(nx, 0), 1800);
    c_value = kit_label(s, &wf_font_big, COL_TEXT, "", CX, 286);
    c_blurb = kit_label(s, &wf_font_small, COL_DIM, "", CX, 324);
    lv_obj_set_width(c_blurb, 250);
    lv_label_set_long_mode(c_blurb, LV_LABEL_LONG_WRAP);
    kit_text_button(s, TR("Indietro", "Back"), CX - 72, 392, 128, 48, COL_PANEL_HI, on_step, -1);
    c_next = kit_text_button(s, "", CX + 72, 392, 128, 48, COL_BTN_OK, on_step, 1);
    c_blink = lv_tick_get() + 2000;
    create_refresh();
}

void create_tick(void) {
    if ((int32_t)(lv_tick_get() - c_blink) < 0) return;
    hero_look l;
    draft_look(&l);
    static bool closed;
    closed = !closed;
    hero_obj_set(c_hero, &l, closed ? HX_BLINK : HX_IDLE, 3);
    c_blink = lv_tick_get() + (closed ? 140 : 2600);
}

/* ---- Town ---------------------------------------------------------------------------- */
static lv_obj_t *t_hero, *t_arcs[3], *t_hp, *t_sp, *t_info, *t_hunt, *t_gift;
static uint32_t t_refresh, t_blink;

static void town_refresh(void) {
    rpg_game *g = &R.g;
    rpg_stats st;
    rpg_stats_get(g, &st);
    lv_arc_set_value(t_arcs[0], g->hero.hunger / 1000);
    lv_arc_set_value(t_arcs[1], g->hero.stamina);
    lv_arc_set_value(t_arcs[2], g->hero.morale);
    r_vitals_update(t_hp, t_sp, g->hero.hp, st.max_hp, g->hero.sp, st.max_sp);
    char buf[96];
    snprintf(buf, sizeof buf, "%s · Lv %d\n%s · %s %d", g->hero.name, g->hero.level, TX(rpg_class(g->hero.cls)->name),
             TR("Cl", "Job"), g->hero.job_level);
    lv_label_set_text(t_info, buf);
    if (g->hunt.active) {
        char d[24];
        rpg_haul h;
        rpg_hunt_preview(g, r_now(), &h);
        snprintf(buf, sizeof buf, TR("Caccia: %s · %lu mostri", "Hunting: %s · %lu kills"),
                 r_fmt_time(h.minutes * 60u, d, sizeof d), (unsigned long)h.kills);
        lv_label_set_text(lv_obj_get_child(t_hunt, 0), buf);
        lv_obj_remove_flag(t_hunt, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_flag(t_hunt, LV_OBJ_FLAG_HIDDEN);
    }
    if (g->daily_claimed) lv_obj_add_flag(t_gift, LV_OBJ_FLAG_HIDDEN);
    else lv_obj_remove_flag(t_gift, LV_OBJ_FLAG_HIDDEN);
}

static void on_gift(lv_event_t *e) {
    (void)e;
    int item;
    int gold = rpg_daily_claim(&R.g, &item);
    if (!gold) return;
    char body[128];
    snprintf(body, sizeof body, TR("Giorno %d di fila!\n+%d oro, %s", "Day %d in a row!\n+%d gold, %s"), R.g.streak, gold,
             TX(rpg_consumable(item)->name));
    dlg = kit_dialog(TR("Regalo del giorno", "Daily gift"), body, ri(RI_GIFT));
    kit_dialog_button(dlg, TR("Grazie!", "Thanks!"), COL_BTN_OK, close_dlg, 0);
    r_dirty();
    town_refresh();
}

static void on_collect(lv_event_t *e) {
    (void)e;
    rpg_haul h;
    if (rpg_hunt_collect(&R.g, r_now(), &h) != RPG_OK) return;
    char body[220], d[24];
    int n = snprintf(body, sizeof body, TR("%s di caccia, %lu mostri.\n+%lu PE  +%lu oro", "%s of hunting, %lu kills.\n+%lu XP  +%lu gold"),
                     r_fmt_time(h.minutes * 60u, d, sizeof d), (unsigned long)h.kills, (unsigned long)h.xp, (unsigned long)h.gold);
    if (h.nitems) n += snprintf(body + n, sizeof body - (size_t)n, TR("\n%d oggetti trovati", "\n%d items found"), h.nitems);
    if (h.gems) n += snprintf(body + n, sizeof body - (size_t)n, TR("\n%d gemme d'anima!", "\n%d soul gems!"), h.gems);
    if (h.levels) n += snprintf(body + n, sizeof body - (size_t)n, TR("\nLivello %d!", "\nLevel %d!"), R.g.hero.level);
    static const wf_text why[4] = {{"", ""}, {"\n(stamina finita)", "\n(out of stamina)"}, {"\n(aveva fame)", "\n(got hungry)"},
                                   {"\n(nemici troppo forti)", "\n(foes too strong)"}};
    if (h.stopped) snprintf(body + n, sizeof body - (size_t)n, "%s", TX(why[h.stopped & 3]));
    dlg = kit_dialog(TR("Bentornato!", "Welcome back!"), body, ri(RI_TROPHY));
    kit_dialog_button(dlg, "Ok", COL_BTN_OK, close_dlg, 0);
    r_save_now();
    town_refresh();
}

static void on_inn_rest(lv_event_t *e) {
    kit_dialog_close(dlg);
    dlg = NULL;
    int r = rpg_inn(&R.g);
    kit_toast(r == RPG_OK ? TR("Riposato! PV e PM pieni", "Rested! HP and SP full") : rpg_err(r), ri(RI_BED));
    if (r == RPG_OK) r_dirty();
    town_refresh();
    (void)e;
}

static void on_eat(lv_event_t *e) {
    int c = (int)(intptr_t)lv_event_get_user_data(e);
    kit_dialog_close(dlg);
    dlg = NULL;
    int r = rpg_use_consumable(&R.g, c);
    if (r == RPG_ERR_NONE) {
        r = rpg_buy(&R.g, c, 1);
        if (r == RPG_OK) r = rpg_use_consumable(&R.g, c);
    }
    kit_toast(r == RPG_OK ? TR("Gnam!", "Yum!") : rpg_err(r), &rpg_cons_icons[c]);
    if (r == RPG_OK) {
        r_dirty();
        hero_look l;
        hero_look_from_game(&l, &R.g);
        hero_obj_set(t_hero, &l, HX_HAPPY, 3);
        t_blink = lv_tick_get() + 1500;
    }
    town_refresh();
}

static void on_inn(lv_event_t *e) {
    (void)e;
    char body[160];
    snprintf(body, sizeof body,
             TR("Letto caldo: PV/PM pieni, +50 stamina (%lu oro).\nOppure un pasto (se non ne hai, lo compri).",
                "A warm bed: full HP/SP, +50 stamina (%lu gold).\nOr a meal (bought if you have none)."),
             (unsigned long)rpg_inn_price(&R.g));
    dlg = kit_dialog(TR("Locanda", "Inn"), body, ri(RI_BED));
    kit_dialog_button(dlg, TR("Dormi", "Sleep"), COL_BTN, on_inn_rest, 0);
    kit_dialog_button(dlg, TR("Pane", "Bread"), COL_BTN_ALT, on_eat, CO_BREAD);
    kit_dialog_button(dlg, TR("Stufato", "Stew"), COL_BTN_ALT, on_eat, CO_STEW);
    kit_dialog_button(dlg, TR("Torta", "Pie"), COL_BTN_ALT, on_eat, CO_PIE);
}

static void on_soon(lv_event_t *e) {
    (void)e;
    kit_toast(TR("Legami: in arrivo (party, duelli, scambi)", "Bonds: coming soon (party, duels, trades)"), ri(RI_LINK));
}

void scr_town(void) {
    if (R.g.run.active) { r_go(R_DUNGEON); return; }
    bool night = r_hour() < 7 || r_hour() >= 20;
    lv_obj_t *s = r_scene(night ? RS_TOWN_NIGHT : RS_TOWN);
    static const uint32_t colors[3] = {COL_HUNGER, 0xffd23f, COL_MOOD};
    static const int icons_[3] = {RI_FOOD, RI_STAMINA, RI_MORALE};
    static const float mids[3] = {226.0f, 270.0f, 314.0f};
    for (int i = 0; i < 3; i++) {
        t_arcs[i] = kit_arc(s, 226, 16, (int)mids[i] - 19, (int)mids[i] + 19, colors[i], 0, 100);
        float a = mids[i] * 3.14159265f / 180.0f;
        kit_img(s, ri(icons_[i]), 2, CX + (int)lroundf(192 * cosf(a)), CY + (int)lroundf(192 * sinf(a)));
    }
    r_vitals_arcs(s, &t_hp, &t_sp);
    t_info = kit_label(s, &wf_font_small, COL_TEXT, "", CX, 100);
    lv_obj_set_style_bg_color(t_info, lv_color_hex(COL_PANEL), 0);
    lv_obj_set_style_bg_opa(t_info, 210, 0);
    lv_obj_set_style_pad_hor(t_info, 10, 0);
    lv_obj_set_style_pad_ver(t_info, 4, 0);
    lv_obj_set_style_radius(t_info, 10, 0);
    hero_look l;
    hero_look_from_game(&l, &R.g);
    fx_shadow(s, CX, 352, 90);
    t_hero = hero_obj(s, &l, HX_IDLE, 3, CX, 286);
    kit_bob(t_hero, 3, 1500);
    /* Hub buttons around the hero. */
    static const struct { int icon; wf_text name; int screen; int x, y; uint32_t color; } hub[] = {
        {RI_MAP, {"Mappa", "Map"}, R_MAP, 92, 196, 0x3aba6a},
        {RI_CROWN, {"Eroe", "Hero"}, R_HERO, 374, 196, 0x8a5ae8},
        {RI_BAG, {"Equip", "Gear"}, R_EQUIP, 92, 296, 0xc8803a},
        {RI_COIN, {"Bottega", "Shop"}, R_SHOP, 374, 296, 0xd8a03a},
        {RI_ANVIL, {"Forgia", "Forge"}, R_FORGE, 150, 380, 0x6a6a8a},
        {RI_BED, {"Locanda", "Inn"}, -1, 316, 380, 0x3a5ae8},
        {RI_SCROLL, {"Missioni", "Quests"}, R_QUESTS, 233, 410, 0x8a7a5a},
    };
    for (unsigned i = 0; i < sizeof hub / sizeof hub[0]; i++) {
        lv_event_cb_t cb = hub[i].screen < 0 ? on_inn : on_go;
        lv_obj_t *b = kit_round_button(s, ri(hub[i].icon), NULL, hub[i].x, hub[i].y, 62, hub[i].color, cb, hub[i].screen);
        lv_obj_t *cap = kit_label(s, &wf_font_small, COL_TEXT, TX(hub[i].name), hub[i].x, hub[i].y + 42);
        lv_obj_set_style_bg_color(cap, lv_color_hex(COL_PANEL), 0);
        lv_obj_set_style_bg_opa(cap, 200, 0);
        lv_obj_set_style_pad_hor(cap, 4, 0);
        lv_obj_set_style_radius(cap, 5, 0);
        if (i == 6) lv_obj_add_flag(cap, LV_OBJ_FLAG_HIDDEN);
        (void)b;
    }
    kit_round_button(s, ri(RI_LINK), NULL, 160, 168, 40, COL_PANEL_HI, on_soon, 0);
    t_gift = kit_round_button(s, ri(RI_GIFT), NULL, 306, 168, 46, COL_BTN_ALT, on_gift, 0);
    kit_bob(t_gift, 5, 900);
    t_hunt = kit_text_button(s, "", CX, 146, 270, 38, COL_BTN_OK, on_collect, 0);
    t_refresh = 0;
    t_blink = lv_tick_get() + 2500;
    town_refresh();
}

void town_tick(void) {
    uint32_t t = lv_tick_get();
    if ((int32_t)(t - t_blink) >= 0) {
        hero_look l;
        hero_look_from_game(&l, &R.g);
        static bool closed;
        closed = !closed;
        hero_obj_set(t_hero, &l, closed ? HX_BLINK : HX_IDLE, 3);
        t_blink = t + (closed ? 140 : 2800);
    }
    if (t - t_refresh > 1000) {
        t_refresh = t;
        town_refresh();
    }
}

/* ---- App interface ---------------------------------------------------------------- */
const char *wf_app_save_key(void) { return "rpg"; }
size_t wf_app_save_max(void) { return rpg_save_size(); }

void wf_app_start(const uint8_t *save, size_t len) {
    memset(&R, 0, sizeof R);
    if (save && rpg_save_decode(save, len, &R.g)) {
        R.loaded = true;
        rpg_tick(&R.g, r_now());
        wf_platform_brightness(R.g.brightness);
        r_go(R.g.run.active ? R_DUNGEON : R_TOWN);
    } else {
        r_go(R_TITLE);
    }
    R.last_tick_s = r_now();
}

void wf_app_tick(void) {
    uint32_t t = r_now();
    if (R.loaded && t != R.last_tick_s) {
        R.last_tick_s = t;
        if (rpg_tick(&R.g, t)) R.dirty = true;
        if (R.dirty && ((R.save_due_s && t >= R.save_due_s) || t - R.last_save_s >= 300)) r_save_now();
    }
    switch (R.screen) {
    case R_TOWN: town_tick(); break;
    case R_BATTLE: battle_tick(); break;
    case R_CREATE: create_tick(); break;
    default: break;
    }
}

bool wf_app_key(int key) {
    if (R.screen == R_BATTLE && battle_key(key)) return true;
    if (key == WF_KEY_BOOT_SHORT || key == WF_KEY_BOOT_LONG) {
        if (!R.loaded) return false;
        if (R.screen == R_DUNGEON || R.screen == R_BATTLE || R.screen == R_TOWN) return true;
        if (R.screen == R_SKILLS || R.screen == R_CLASS) r_go(R_HERO);
        else if (R.screen == R_BAG) r_go(R_EQUIP);
        else if (R.screen == R_ZONE) r_go(R_MAP);
        else if (R.screen == R_SHOP && R.shop_in_dungeon) r_go(R_DUNGEON);
        else r_go(R_TOWN);
        return true;
    }
    return false;
}

void wf_app_link_rx(const uint8_t from[6], const uint8_t *data, size_t len) { (void)from; (void)data; (void)len; }
void wf_app_flush_save(void) { if (R.dirty) r_save_now(); }
bool wf_app_busy(void) { return R.screen == R_BATTLE || R.screen == R_CREATE; }
void wf_app_wake(void) {
    if (R.screen == R_TOWN) town_refresh();
}
