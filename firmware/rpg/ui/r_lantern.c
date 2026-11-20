/* Native HD-2D slice. Artwork and actual localized LVGL controls stay separate. */
#include <math.h>
#include <stdio.h>
#include <string.h>
#include "rui.h"
#include "art/lantern_art.h"
#include "r_lantern_fx.h"

#define INK 0x171c3c
#define PANEL 0x263658
#define IVORY 0xfff4dd
#define AMBER 0xffce78
#define JADE 0x6de2b6
#define MUTED 0xc5d7e6
#define DANGER 0xff8d91
LV_FONT_DECLARE(ln_font_body)
LV_FONT_DECLARE(ln_font_title)
#define BODY (&ln_font_body)
#define TITLE (&ln_font_title)

enum { PAGE_TOWN, PAGE_BATTLE, PAGE_ROUTE, PAGE_RELIC, PAGE_EVENT, PAGE_CLEAR, PAGE_RESULT,
       PAGE_MENU, PAGE_LOADOUT, PAGE_OPTIONS, PAGE_COLLECTION, PAGE_TITLE, PAGE_MAP, PAGE_BESTIARY, PAGE_HELP, PAGE_HUNT,
       PAGE_REPLACE };
static struct {
    int page, pick, frame, skill_menu, offer_pick;
    uint32_t clock, anim_clock, ready_ms, visual_clock;
    lantern_fx effects;
    int coach_tip;
    lv_obj_t *root, *hero, *foes[2], *hp[2], *hp_text[2], *heading, *intent, *resource;
    lv_obj_t *buttons[3], *notice, *arc, *spark, *wave, *catcher, *pause, *guard_circle, *quality;
    lv_obj_t *coach_text, *coach_ok, *quality_panel, *damage_panel;
} L;

static rpg_lantern *game(void) { return &R.g.lantern; }
static void show(void);
static void battle_refresh(void);
static void advance_battle_clock(void);
static void on_click(lv_event_t *e);
static bool battle_state(int s) { return s >= LN_CHOOSE && s <= LN_IMPACT; }
static const wf_sprite *hero_sprite(int frame) { return &ln_loadouts[game()->archetype % LN_ARCHETYPES][frame & 3]; }
static const wf_sprite *foe_sprite(int kind, int frame) {
    if (kind == LN_ROOT || kind == LN_WISP) return &ln_actors[kind == LN_ROOT ? 1 : 2][frame & 3];
    int row = kind == LN_BOSS ? 4 : kind - LN_WOLF;
    return &ln_bestiary[row >= 0 && row < 5 ? row : 0][frame & 3];
}
static const char *enemy_short(int kind) {
    static const wf_text t[] = {{"Custode", "Keeper"}, {"Fiammella", "Wisp"}, {"Guardiano", "Guardian"},
        {"Lupo", "Wolf"}, {"Granchio", "Crab"}, {"Pipistrello", "Bat"}, {"Falena", "Moth"}};
    return TX(t[(unsigned)kind < LN_ENEMIES ? kind : 0]);
}

static lv_obj_t *label(lv_obj_t *p, const char *text, int x, int y, int width, uint32_t color, bool title) {
    lv_obj_t *o = kit_label(p, title ? TITLE : BODY, color, text, x, y);
    lv_obj_set_width(o, width);
    lv_label_set_long_mode(o, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_text_line_space(o, 3, 0);
    kit_pos(o, x, y);
    return o;
}
static lv_obj_t *panel(lv_obj_t *p, int x, int y, int w, int h, int opacity) {
    lv_obj_t *o = lv_obj_create(p);
    lv_obj_remove_style_all(o);
    lv_obj_set_size(o, w, h);
    kit_pos(o, x, y);
    lv_obj_set_style_bg_color(o, lv_color_hex(INK), 0);
    lv_obj_set_style_bg_grad_color(o, lv_color_hex(PANEL), 0);
    lv_obj_set_style_bg_grad_dir(o, LV_GRAD_DIR_VER, 0);
    lv_obj_set_style_bg_opa(o, opacity, 0);
    lv_obj_set_style_radius(o, 6, 0);
    lv_obj_set_style_border_width(o, 1, 0);
    lv_obj_set_style_border_color(o, lv_color_hex(0x63739b), 0);
    lv_obj_set_style_border_opa(o, 150, 0);
    lv_obj_remove_flag(o, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    return o;
}
static lv_obj_t *button(lv_obj_t *p, const char *text, int x, int y, int w, int h, int code, bool primary) {
    lv_obj_t *b = lv_button_create(p);
    lv_obj_remove_style_all(b);
    lv_obj_set_size(b, w, h);
    kit_pos(b, x, y);
    lv_obj_set_style_radius(b, 6, 0);
    lv_obj_set_style_bg_color(b, lv_color_hex(primary ? AMBER : PANEL), 0);
    lv_obj_set_style_bg_grad_color(b, lv_color_hex(primary ? 0xe9a06d : 0x171e43), 0);
    lv_obj_set_style_bg_grad_dir(b, LV_GRAD_DIR_VER, 0);
    lv_obj_set_style_bg_opa(b, 250, 0);
    lv_obj_set_style_border_width(b, 1, 0);
    lv_obj_set_style_border_color(b, lv_color_hex(primary ? IVORY : 0x8995bd), 0);
    lv_obj_set_style_bg_color(b, lv_color_hex(primary ? 0xd38d65 : 0x475b83), LV_STATE_PRESSED);
    lv_obj_set_style_shadow_color(b, lv_color_hex(0x070e2c), 0);
    lv_obj_set_style_shadow_width(b, 5, 0);
    lv_obj_set_style_shadow_offset_y(b, 2, 0);
    lv_obj_set_style_shadow_opa(b, 95, 0);
    lv_obj_set_style_border_width(b, 3, LV_STATE_FOCUSED);
    lv_obj_set_style_border_color(b, lv_color_hex(IVORY), LV_STATE_FOCUSED);
    lv_obj_set_style_opa(b, 110, LV_STATE_DISABLED);
    lv_obj_remove_flag(b, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(b, on_click, LV_EVENT_CLICKED, (void *)(intptr_t)code);
    lv_obj_t *t = lv_label_create(b);
    lv_obj_set_style_text_font(t, BODY, 0);
    lv_obj_set_style_text_color(t, lv_color_hex(primary ? INK : IVORY), 0);
    lv_obj_set_style_text_align(t, LV_TEXT_ALIGN_CENTER, 0);
    lv_label_set_text(t, text);
    lv_obj_set_width(t, w - 8);
    lv_label_set_long_mode(t, LV_LABEL_LONG_WRAP);
    lv_obj_center(t);
    return b;
}
static void ground_shadow(int x, int y, int width) {
    for (int i = 0; i < 2; i++) {
        lv_obj_t *o = lv_obj_create(L.root); lv_obj_remove_style_all(o);
        lv_obj_set_size(o, width * (i ? 7 : 10) / 10, i ? 7 : 13);
        kit_pos(o, x, y); lv_obj_set_style_radius(o, LV_RADIUS_CIRCLE, 0);
        lv_obj_set_style_bg_color(o, lv_color_hex(0x10132d), 0);
        lv_obj_set_style_bg_opa(o, i ? 100 : 45, 0);
        lv_obj_remove_flag(o, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    }
}
static void hidden(lv_obj_t *o, bool hide) {
    if (hide) lv_obj_add_flag(o, LV_OBJ_FLAG_HIDDEN);
    else lv_obj_remove_flag(o, LV_OBJ_FLAG_HIDDEN);
}
static void text_button(lv_obj_t *b, const char *t) { lv_label_set_text(lv_obj_get_child(b, 0), t); }
static lv_obj_t *scene(int page, int background, int dim) {
    memset(&L.hero, 0, (size_t)((char *)(&L.damage_panel + 1) - (char *)&L.hero));
    memset(&L.effects, 0, sizeof L.effects); L.coach_tip = LNT_NONE;
    L.page = page; L.frame = -1;
    L.root = kit_screen(&ln_scenes[background]);
    if (dim) kit_dim(L.root, dim * 2 / 3);
    /* A single restrained rim, no permanent multicolor gauges. */
    lv_obj_t *rim = lv_obj_create(L.root);
    lv_obj_remove_style_all(rim);
    lv_obj_set_size(rim, 458, 458);
    lv_obj_center(rim);
    lv_obj_set_style_radius(rim, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_border_width(rim, 1, 0);
    lv_obj_set_style_border_color(rim, lv_color_hex(0xb6d7dc), 0);
    lv_obj_set_style_border_opa(rim, 150, 0);
    lv_obj_remove_flag(rim, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    L.clock = L.anim_clock = L.visual_clock = lv_tick_get();
    return L.root;
}
static void header(const char *title, const char *subtitle) {
    lv_obj_t *back = panel(L.root, CX, 76, 290, 76, 230);
    lv_obj_t *name = label(L.root, title, CX, 76, 276, IVORY, true);
    lv_obj_update_layout(name);
    int height = lv_obj_get_height(name);
    if (subtitle) {
        /* Titles and translations can wrap: stack measured text blocks rather
         * than assuming one line and drawing the subtitle across the title. */
        kit_pos(name, CX, 38 + height / 2);
        lv_obj_t *sub = label(L.root, subtitle, CX, 94, 272, MUTED, false);
        lv_obj_update_layout(sub);
        int bottom = 38 + height + 7 + lv_obj_get_height(sub);
        kit_pos(sub, CX, bottom - lv_obj_get_height(sub) / 2);
        lv_obj_set_height(back, bottom - 30 + 8);
        kit_pos(back, CX, 30 + (bottom - 30 + 8) / 2);
    } else lv_obj_set_height(back, height + 20);
}
static void save(void) { r_dirty(); r_save_now(); }
static void changed(void) {
    rpg_lantern *l = game();
    rpg_gold_add(&R.g, ln_bank(l));
    save();
    if (L.page == PAGE_BATTLE && battle_state(l->state)) battle_refresh();
    else show();
}
static void changed_from(const rpg_lantern *before) {
    ln_coach_observe(&R.g.coach, before, game());
    changed();
    if (before->state != game()->state) L.clock = lv_tick_get();
}

/* Event codes are private to this screen: no callbacks survive a scene change. */
enum { C_DEPART = 1, C_HERO, C_BAG, C_MORE, C_HOME, C_CLASSIC, C_MAP, C_EQUIP, C_OPTIONS,
       C_LEFT, C_RIGHT, C_BEGIN, C_ROUTE0, C_ROUTE1, C_ATTACK, C_SKILLS, C_GUARD,
       C_POWER, C_SUPPORT, C_PAUSE, C_RESUME, C_TARGET0, C_TARGET1, C_RELIC,
       C_CONTINUE, C_RETREAT, C_EVENT0, C_EVENT1, C_RETRY, C_ASSIST, C_MOTION,
       C_LANGUAGE, C_QUICK, C_CUSTOM, C_COLLECTION, C_ABORT, C_EQUIP_RELIC,
       C_BESTIARY, C_HELP, C_HUNT, C_COLLECT_HUNT, C_TIPS, C_REPLAY_TIPS, C_ACK_TIP,
       C_REPLACE_RELIC, C_CANCEL_REPLACE, C_SKIP_RELIC, C_CLASSES, C_BASTION };

static void departure_button(const char *text, int y) {
    lv_obj_t *b = button(L.root, text, CX, y, 216, 72, C_DEPART, true);
    lv_obj_align(lv_obj_get_child(b, 0), LV_ALIGN_CENTER, 0, -13);
    lv_obj_t *loadout_name = lv_label_create(b);
    lv_obj_set_style_text_font(loadout_name, BODY, 0);
    lv_obj_set_style_text_color(loadout_name, lv_color_hex(INK), 0);
    lv_label_set_text(loadout_name, TX(ln_archetype_name(game()->archetype)));
    lv_obj_align(loadout_name, LV_ALIGN_CENTER, 0, 13);
}

void scr_lantern_title(void) {
    scene(PAGE_TITLE, 0, 50);
    panel(L.root, CX, 91, 330, 46, 225);
    label(L.root, "POCKET WAYFARER", CX, 91, 314, IVORY, true);
    panel(L.root, CX, 124, 238, 28, 225);
    label(L.root, "LANTERNBOUND", CX, 124, 270, AMBER, false);
    ground_shadow(CX, 282, 70);
    L.hero = kit_img(L.root, hero_sprite(0), 2, CX, 202);
    button(L.root, TR("Gioca", "Play"), CX, 323, 236, 72, C_QUICK, true);
    button(L.root, TR("Personalizza", "Customize"), CX, 397, 176, 52, C_CUSTOM, false);
    button(L.root, wf_lang ? "EN" : "IT", CX, 43, 52, 42, C_LANGUAGE, false);
}

void scr_town(void) {
    if (R.g.run.active) { r_go(R_DUNGEON); return; }
    if (ln_active(game())) { r_go(R_LANTERN); return; }
    scene(PAGE_TOWN, 0, 0);
    char sub[64];
    const char *summary = game()->wins == 1 ? TR("%lu oro  ·  %u spedizione", "%lu gold  ·  %u expedition")
                                             : TR("%lu oro  ·  %u spedizioni", "%lu gold  ·  %u expeditions");
    snprintf(sub, sizeof sub, summary,
             (unsigned long)R.g.gold, game()->wins);
    header(TR("Borgo Lanterna", "Lantern Hollow"), sub);
    ground_shadow(160, 275, 70);
    L.hero = kit_img(L.root, hero_sprite(0), 2, 160, 195);
    if (R.g.hunt.active) button(L.root, TR("Caccia", "Hunt"), 311, 238, 110, 52, C_HUNT, false);
    button(L.root, "...", 347, 146, 52, 52, C_MORE, false);
    button(L.root, TR("Eroe", "Hero"), 169, 306, 116, 54, C_HERO, false);
    button(L.root, TR("Zaino", "Relics"), 297, 306, 116, 54, C_BAG, false);
    departure_button(TR("PARTI", "DEPART"), 376);
}
static void menu(void) {
    scene(PAGE_MENU, 0, 160);
    header(TR("Il borgo", "The village"), TR("Ogni viaggio comincia qui", "Every journey begins here"));
    button(L.root, TR("Mappa", "Map"), 155, 166, 144, 58, C_MAP, false);
    button(L.root, TR("Bestiario", "Bestiary"), 311, 166, 144, 58, C_BESTIARY, false);
    button(L.root, TR("Collezione", "Collection"), 155, 234, 144, 58, C_COLLECTION, false);
    button(L.root, TR("Opzioni", "Options"), 311, 234, 144, 58, C_OPTIONS, false);
    button(L.root, TR("Come si gioca", "How to play"), 155, 302, 144, 58, C_HELP, false);
    button(L.root, TR("Classi", "Classes"), 311, 302, 144, 58, C_CLASSES, false);
    button(L.root, TR("Classico", "Classic"), 155, 370, 144, 58, C_CLASSIC, false);
    button(L.root, TR("Torna", "Back"), 311, 370, 144, 58, C_HOME, false);
}
static void loadout(void) {
    scene(PAGE_LOADOUT, 0, 130);
    header(TR("Il tuo assetto", "Your loadout"), TX(ln_archetype_name(game()->archetype)));
    L.hero = kit_img(L.root, hero_sprite(0), 2, CX, 202);
    button(L.root, "<", 105, 214, 72, 72, C_LEFT, false);
    button(L.root, ">", 361, 214, 72, 72, C_RIGHT, false);
    char b[128];
    snprintf(b, sizeof b, "%s  ·  %s", TX(ln_skill_name(game()->archetype, 0)), TX(ln_skill_name(game()->archetype, 1)));
    panel(L.root, CX, 302, 312, 66, 230);
    label(L.root, b, CX, 300, 292, IVORY, false);
    button(L.root, TR("Parti", "Depart"), CX, 371, 216, 60, C_BEGIN, true);
    button(L.root, TR("Torna", "Back"), CX, 431, 124, 42, C_HOME, false);
}
static void options(void) {
    scene(PAGE_OPTIONS, 0, 180);
    header(TR("Opzioni", "Options"), NULL);
    button(L.root, game()->assist ? TR("Timing assistito: sì", "Timing assist: on") : TR("Timing assistito: no", "Timing assist: off"), CX, 130, 296, 52, C_ASSIST, false);
    button(L.root, game()->reduced_motion ? TR("Movimento ridotto: sì", "Reduced motion: on") : TR("Movimento ridotto: no", "Reduced motion: off"), CX, 193, 296, 52, C_MOTION, false);
    button(L.root, R.g.coach.disabled ? TR("Suggerimenti: no", "Tips: off") : TR("Suggerimenti: sì", "Tips: on"), CX, 256, 296, 52, C_TIPS, false);
    button(L.root, wf_lang ? "English / Italiano" : "Italiano / English", CX, 319, 264, 52, C_LANGUAGE, false);
    button(L.root, TR("Ripeti guida", "Replay tips"), 161, 389, 128, 60, C_REPLAY_TIPS, false);
    button(L.root, TR("Torna", "Back"), 305, 389, 128, 60, C_HOME, false);
}
static void collection(void) {
    scene(PAGE_COLLECTION, 1, 155);
    header(TR("Progetti di reliquia", "Relic blueprints"), TR("Un solo progetto all'inizio", "One blueprint at the start"));
    int r = L.pick % LR_COUNT;
    bool found = (game()->blueprints & (1u << r)) != 0;
    lv_obj_t *art = kit_img(L.root, &ln_relics[r], 2, CX, 205);
    if (!found) lv_obj_set_style_image_opa(art, 70, 0);
    label(L.root, found ? TX(ln_relic_name(r)) : TR("Progetto da scoprire", "Undiscovered blueprint"), CX, 298, 300, found ? AMBER : MUTED, false);
    button(L.root, "<", 106, 216, 72, 72, C_LEFT, false);
    button(L.root, ">", 360, 216, 72, 72, C_RIGHT, false);
    bool equipped = game()->equipped == r + 1;
    lv_obj_t *equip = button(L.root, equipped ? TR("Rimuovi", "Remove") : TR("Equipaggia", "Equip"), 162, 378, 130, 64, C_EQUIP_RELIC, true);
    if (!found) lv_obj_add_state(equip, LV_STATE_DISABLED);
    button(L.root, TR("Torna", "Back"), 304, 378, 130, 64, C_HOME, false);
}
static void world_map(void) {
    scene(PAGE_MAP, 1, 145);
    header(TR("La via del Faro", "The beacon trail"), TR("Cinque stanze verso il Faro", "Five rooms to the beacon"));
    static const int xy[5][2] = {{144, 307}, {290, 267}, {154, 220}, {296, 175}, {233, 132}};
    static lv_point_precise_t points[4][2];
    for (int i = 0; i < 4; i++) {
        points[i][0] = (lv_point_precise_t){xy[i][0], xy[i][1]};
        points[i][1] = (lv_point_precise_t){xy[i+1][0], xy[i+1][1]};
        lv_obj_t *line = lv_line_create(L.root); lv_line_set_points(line, points[i], 2);
        lv_obj_set_style_line_width(line, 3, 0); lv_obj_set_style_line_color(line, lv_color_hex(0x7b876f), 0);
    }
    for (int i = 0; i < 5; i++) {
        lv_obj_t *node = panel(L.root, xy[i][0], xy[i][1], 42, 42, 255);
        lv_obj_set_style_radius(node, LV_RADIUS_CIRCLE, 0);
        lv_obj_set_style_border_width(node, 2, 0);
        lv_obj_set_style_border_color(node, lv_color_hex(ln_active(game()) && i == game()->room ? AMBER : MUTED), 0);
        char n[4]; snprintf(n, sizeof n, "%d", i+1); label(L.root, n, xy[i][0], xy[i][1], 40, IVORY, false);
    }
    departure_button(TR("Parti", "Depart"), 378);
}
static void bestiary(void) {
    scene(PAGE_BESTIARY, 1, 150);
    int k = L.pick % LN_ENEMIES;
    bool known = (game()->seen & (1u << k)) != 0;
    header(TR("Bestiario", "Bestiary"), known ? TX(ln_enemy_name(k)) : TR("Presenza sconosciuta", "Unknown presence"));
    lv_obj_t *actor = kit_img(L.root, foe_sprite(k, 0), 2, CX, 203);
    if (!known) { lv_obj_set_style_image_recolor(actor, lv_color_hex(INK), 0); lv_obj_set_style_image_recolor_opa(actor, 235, 0); }
    static const wf_text hints[] = {
        {"Il suo colpo pesante è lento e leggibile.", "Its heavy strike is slow and readable."},
        {"Fragile, ma in gruppo non va ignorata.", "Fragile, but dangerous in a group."},
        {"Alterna colpo pesante e doppio colpo.", "Alternates heavy and double strikes."},
        {"Alterna morso e doppio assalto.", "Alternates bites and double attacks."},
        {"Il guscio riduce gli attacchi base del 20%.", "Its shell reduces basic attacks by 20%."},
        {"Pochi PV, ma attacca due volte.", "Low HP, but it can strike twice."},
        {"Ogni tre turni cura un alleato di 8 PV.", "Every three turns it heals an ally for 8 HP."}
    };
    panel(L.root, CX, 300, 304, 80, 245);
    label(L.root, known ? TX(hints[k]) : TR("Incontrala per scoprirne il comportamento.", "Encounter it to discover its behavior."), CX, 297, 280, IVORY, false);
    button(L.root, "<", 102, 208, 72, 72, C_LEFT, false); button(L.root, ">", 364, 208, 72, 72, C_RIGHT, false);
    button(L.root, TR("Torna", "Back"), CX, 383, 176, 60, C_HOME, false);
}
static void help(void) {
    scene(PAGE_HELP, 1, 165);
    static const wf_text titles[] = {{"1. Leggi l'intento", "1. Read the intent"}, {"2. Crea la tua sinergia", "2. Build your synergy"}, {"3. Scegli quando tornare", "3. Choose when to return"}};
    static const wf_text body[] = {
        {"Scegli bersaglio e azione. Tocca sull'arco dorato per colpire; sull'onda per parare. Puoi usare anche BOOT.", "Choose a target and action. Tap at the golden arc to hit; at the wave to parry. BOOT works too."},
        {"Guardia recupera 4 PM. Tre parate perfette caricano l'abilità offensiva. Le reliquie cambiano questi ritmi.", "Guard restores 4 SP. Three perfect parries empower your offensive skill. Relics change these rhythms."},
        {"Dopo ogni stanza puoi rientrare con tutto l'oro. Se perdi, ne conservi il 75%. II e PWR mettono in pausa.", "Return after any room to keep all gold. On defeat keep 75%. II and PWR pause the run."}
    };
    int p = L.pick % 3;
    header(TR("Come si gioca", "How to play"), TX(titles[p]));
    panel(L.root, CX, 230, 312, 186, 245);
    label(L.root, TX(body[p]), CX, 230, 278, IVORY, false);
    button(L.root, "<", 150, 347, 84, 60, C_LEFT, false); button(L.root, ">", 316, 347, 84, 60, C_RIGHT, false);
    button(L.root, TR("Torna", "Back"), CX, 412, 132, 44, C_HOME, false);
}
static void hunt(void) {
    scene(PAGE_HUNT, 0, 140);
    header(TR("Il viaggio in tasca", "The pocket journey"), NULL);
    rpg_haul h; rpg_hunt_preview(&R.g, r_now(), &h);
    L.hero = kit_img(L.root, hero_sprite(0), 2, CX, 193);
    char b[192];
    snprintf(b, sizeof b, TR("%lu min · %lu mostri\n%lu oro · %u oggetti\nSpazio libero: %d", "%lu min · %lu foes\n%lu gold · %u items\nBag space: %d"),
             (unsigned long)h.minutes, (unsigned long)h.kills, (unsigned long)h.gold, h.nitems, RPG_BAG - rpg_bag_count(&R.g));
    panel(L.root, CX, 285, 300, 100, 245); label(L.root, b, CX, 285, 284, IVORY, false);
    button(L.root, TR("Raccogli", "Collect"), 162, 378, 130, 64, C_COLLECT_HUNT, true);
    button(L.root, TR("Zaino", "Bag"), 304, 378, 130, 64, C_EQUIP, false);
}
static void bastion_route(const rpg_lantern *l) {
    const int routes[] = {LN_PATH_TRAIL, LN_PATH_BASTION};
    for (int i = 0; i < 2; i++) {
        ln_route_info info;
        if (!ln_preview_route(l, routes[i], &info)) continue;
        const int x = i ? 313 : 153;
        lv_obj_t *card = panel(L.root, x, 224, 150, 198, 245);
        lv_obj_set_style_border_color(card, lv_color_hex(i ? DANGER : JADE), 0);
        kit_img(L.root, foe_sprite(info.foes[0].kind, 0), 1, x, 174);
        label(L.root, enemy_short(info.foes[0].kind), x, 224, 140, IVORY, false);
        char text[48];
        snprintf(text, sizeof text, TR("%d PV", "%d HP"), info.foes[0].max_hp);
        label(L.root, text, x, 250, 140, MUTED, false);
        snprintf(text, sizeof text, TR("Costo %u PV", "Cost %u HP"), info.hp_cost);
        label(L.root, text, x, 281, 140, i ? DANGER : MUTED, false);
        snprintf(text, sizeof text, TR("Vinci +%u oro", "Win +%u gold"), info.gold);
        label(L.root, text, x, 307, 140, AMBER, false);
        lv_obj_t *enter = button(L.root, i ? TR("Bastione", "Bastion") : TR("Sentiero", "Trail"),
                                 x, 352, 150, 54, i ? C_BASTION : C_ROUTE0, !i);
        if (info.hp_cost && l->hp <= info.hp_cost) {
            lv_obj_add_state(enter, LV_STATE_DISABLED);
            snprintf(text, sizeof text, TR("Servono almeno %u PV", "Requires at least %u HP"), info.hp_cost + 1u);
            label(L.root, text, CX, 398, 274, DANGER, false);
        }
    }
}

static void route(void) {
    scene(PAGE_ROUTE, 1, 90);
    rpg_lantern *l = game();
    char b[64];
    if (l->room == LN_BASTION_ROOM) {
        snprintf(b, sizeof b, TR("%u/5 · %d PV · %u oro", "%u/5 · %d HP · %u gold"),
                 l->room + 1, l->hp, l->currency);
        header(TR("Il bivio del Bastione", "The bastion fork"), b);
        bastion_route(l);
        button(L.root, TR("Rientra", "Return"), CX, 430, 120, 42, C_RETREAT, false);
        return;
    }
    snprintf(b, sizeof b, TR("Stanza %u / 5  ·  %u oro", "Room %u / 5  ·  %u gold"), l->room + 1, l->currency);
    header(l->room == 4 ? TR("Il faro spento", "The silent beacon") : TR("Scegli il cammino", "Choose your path"), b);
    if (l->room == 1) L.hero = kit_img(L.root, hero_sprite(0), 2, CX, 205);
    else if (l->room == 4) kit_img(L.root, foe_sprite(LN_BOSS, 0), 2, CX, 205);
    else {
        for (int branch = 0; branch < 2; branch++) {
            ln_route_info preview;
            if (!ln_preview_route(l, branch, &preview)) continue;
            int x = branch ? 304 : 162;
            for (int i = 0; i < preview.count; i++)
                kit_img(L.root, foe_sprite(preview.foes[i].kind, 0), 1,
                        x + (preview.count == 1 ? 0 : i ? 24 : -24), 194 + i * 8);
            char name[64];
            snprintf(name, sizeof name, "%s", enemy_short(preview.foes[0].kind));
            panel(L.root, x, 255, 138, 34, 230);
            label(L.root, name, x, 255, 134, IVORY, false);
        }
    }
    if (l->room == 4) {
        label(L.root, TR("Il Guardiano ti attende.\nDue attacchi da imparare.", "The Guardian awaits.\nLearn its two attacks."), CX, 287, 296, IVORY, false);
        button(L.root, TR("Affrontalo", "Face the guardian"), CX, 364, 236, 72, C_ROUTE0, true);
    } else {
        const char *a = l->room == 1 ? TR("Focolare", "Hearth") : TR("Sentiero", "Trail");
        const char *c = l->room == 1 ? TR("Specchio", "Mirror") : TR("Pontile", "Pier");
        panel(L.root, CX, 290, 300, 48, 235);
        label(L.root, l->room == 1 ? TR("Riposo sicuro o patto rischioso?", "Safe rest or a risky pact?") : TR("Uno robusto o due fragili", "One sturdy or two fragile foes"), CX, 289, 292, MUTED, false);
        button(L.root, a, 162, 352, 130, 72, C_ROUTE0, true);
        button(L.root, c, 304, 352, 130, 72, C_ROUTE1, false);
    }
    button(L.root, TR("Rientra", "Return"), CX, 418, 120, 44, C_RETREAT, false);
}

static lv_obj_t *bar(int x, int y, int w, uint32_t color) {
    lv_obj_t *o = lv_bar_create(L.root);
    lv_obj_remove_style_all(o);
    lv_obj_set_size(o, w, 7);
    kit_pos(o, x, y);
    lv_obj_set_style_bg_color(o, lv_color_hex(INK), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(o, 255, LV_PART_MAIN);
    lv_obj_set_style_bg_color(o, lv_color_hex(color), LV_PART_INDICATOR);
    lv_obj_set_style_bg_opa(o, 255, LV_PART_INDICATOR);
    lv_bar_set_range(o, 0, 100);
    return o;
}
static void timing_tap(lv_event_t *e) {
    (void)e;
    if (game()->paused) return;
    int phase = game()->state;
    advance_battle_clock();
    if (L.ready_ms || game()->state != phase) return;
    rpg_lantern before = *game();
    if (ln_tap(game())) changed_from(&before);
}
static void draw_actor_frames(void) {
    rpg_lantern *l = game();
    int frame = l->reduced_motion ? 0 : (int)((lv_tick_get() / 700u) & 1u);
    if (L.hero) {
        int hf = L.page == PAGE_BATTLE && (l->state == LN_ATTACK || (l->state == LN_IMPACT && l->feedback == LN_FEEDBACK_HERO)) ? 2
               : L.page == PAGE_BATTLE && (l->state == LN_PARRY || l->guard) ? 3 : frame;
        kit_img_set(L.hero, hero_sprite(hf), 2);
    }
    if (L.page == PAGE_BATTLE) for (int i = 0; i < l->nfoes; i++) {
        int f = l->state == LN_IMPACT && l->feedback == LN_FEEDBACK_HERO && l->effect_actor == i ? 3
              : ((l->state == LN_TELEGRAPH || l->state == LN_PARRY) && l->acting == i) ||
                (l->state == LN_IMPACT && l->feedback == LN_FEEDBACK_FOE && l->effect_actor == i) ? 2 : frame;
        if (L.foes[i]) kit_img_set(L.foes[i], foe_sprite(l->foes[i].kind, f), ln_foe_scale(l));
    }
    if (L.page == PAGE_BATTLE) lantern_fx_update(&L.effects, l, L.hero, L.foes, L.ready_ms != 0);
}
static void battle(void) {
    scene(PAGE_BATTLE, 1, 0);
    rpg_lantern *l = game();
    ground_shadow(LN_HERO_X, LN_HERO_Y + 80, 70);
    L.hero = kit_img(L.root, hero_sprite(0), 2, LN_HERO_X, LN_HERO_Y);
    for (int i = 0; i < l->nfoes; i++) {
        int x = ln_foe_x(l, i), y = ln_foe_y(l, i), scale = ln_foe_scale(l);
        ground_shadow(x, y + scale*40, scale == 1 ? 46 : 90);
        L.foes[i] = kit_img(L.root, foe_sprite(l->foes[i].kind, 0), scale, x, y);
        lv_obj_add_flag(L.foes[i], LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_event_cb(L.foes[i], on_click, LV_EVENT_CLICKED, (void *)(intptr_t)(C_TARGET0 + i));
    }
    lantern_fx_create(&L.effects, L.root, l);
    panel(L.root, CX, 66, 284, 82, 235);
    L.heading = label(L.root, "", CX, 47, 270, MUTED, false);
    L.intent = label(L.root, "", CX, 82, 274, AMBER, false);
    panel(L.root, CX, 302, 336, 34, 240);
    L.hp[0] = bar(126, 289, 124, JADE);
    L.hp[1] = bar(329, 289, 124, DANGER);
    L.hp_text[0] = label(L.root, "", 129, 307, 154, IVORY, false);
    L.hp_text[1] = label(L.root, "", 333, 307, 154, IVORY, false);
    L.buttons[0] = button(L.root, TR("Attacco", "Attack"), 136, 354, 88, 76, C_ATTACK, false);
    L.buttons[1] = button(L.root, TR("Abilità", "Skills"), 233, 354, 88, 76, C_SKILLS, false);
    L.buttons[2] = button(L.root, TR("Guardia", "Guard"), 330, 354, 88, 76, C_GUARD, false);
    L.resource = label(L.root, "", CX, 410, 272, AMBER, false);
    lv_obj_t *resource_back = panel(L.root, CX, 411, 296, 34, 245);
    lv_obj_move_to_index(resource_back, lv_obj_get_index(L.resource));
    L.damage_panel = panel(L.root, CX, 258, 150, 38, 225);
    L.notice = label(L.root, "", CX, 357, 300, IVORY, false);
    L.quality_panel = panel(L.root, 222, 128, 206, 34, 230);
    L.quality = label(L.root, "", 222, 128, 200, AMBER, true);
    L.coach_text = label(L.root, "", CX, 47, 270, IVORY, false);
    L.coach_ok = button(L.root, "OK", CX, 139, 72, 52, C_ACK_TIP, false);
    L.pause = button(L.root, "II", 364, 139, 64, 64, C_PAUSE, false);
    L.arc = lv_arc_create(L.root);
    lv_obj_remove_style_all(L.arc);
    lv_obj_set_size(L.arc, 436, 436);
    lv_obj_center(L.arc);
    lv_arc_set_bg_angles(L.arc, l->assist ? 208 : 222, l->assist ? 292 : 278);
    lv_obj_set_style_arc_width(L.arc, 8, LV_PART_MAIN);
    lv_obj_set_style_arc_color(L.arc, lv_color_hex(AMBER), LV_PART_MAIN);
    lv_obj_set_style_arc_opa(L.arc, 255, LV_PART_MAIN);
    lv_obj_set_style_opa(L.arc, 0, LV_PART_INDICATOR);
    lv_obj_remove_flag(L.arc, LV_OBJ_FLAG_CLICKABLE);
    L.spark = panel(L.root, CX, 15, 12, 12, 255);
    lv_obj_set_style_radius(L.spark, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(L.spark, lv_color_hex(IVORY), 0);
    L.guard_circle = lv_obj_create(L.root);
    lv_obj_remove_style_all(L.guard_circle);
    lv_obj_set_size(L.guard_circle, 150, 150); lv_obj_center(L.guard_circle);
    lv_obj_set_style_radius(L.guard_circle, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_border_width(L.guard_circle, 2, 0);
    lv_obj_set_style_border_color(L.guard_circle, lv_color_hex(IVORY), 0);
    lv_obj_set_style_border_opa(L.guard_circle, 160, 0);
    lv_obj_remove_flag(L.guard_circle, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    L.wave = lv_obj_create(L.root);
    lv_obj_remove_style_all(L.wave);
    lv_obj_set_style_radius(L.wave, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_border_width(L.wave, 4, 0);
    lv_obj_set_style_border_color(L.wave, lv_color_hex(AMBER), 0);
    lv_obj_remove_flag(L.wave, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    L.catcher = lv_obj_create(L.root);
    lv_obj_remove_style_all(L.catcher);
    lv_obj_set_size(L.catcher, WF_RES, WF_RES);
    lv_obj_add_flag(L.catcher, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_remove_flag(L.catcher, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(L.catcher, timing_tap, LV_EVENT_PRESSED, NULL);
    battle_refresh();
}
static void pause_view(void) {
    kit_dim(L.root, 175);
    panel(L.root, CX, 231, 306, 274, 255);
    label(L.root, TR("In pausa", "Paused"), CX, 153, 270, AMBER, true);
    label(L.root, TR("Il viaggio ti aspetta.", "Your journey can wait."), CX, 196, 270, IVORY, false);
    button(L.root, TR("Riprendi", "Resume"), CX, 256, 218, 64, C_RESUME, true);
    button(L.root, TR("Abbandona", "Abandon"), CX, 324, 192, 52, C_ABORT, false);
}
static void battle_refresh(void) {
    rpg_lantern *l = game();
    char b[96];
    int f = l->state == LN_IMPACT ? l->effect_actor : (l->state == LN_TELEGRAPH || l->state == LN_PARRY) ? l->acting : l->target;
    if (f >= l->nfoes || (l->state != LN_IMPACT && l->foes[f].hp <= 0)) for (f = 0; f < l->nfoes && l->foes[f].hp <= 0; f++) {}
    if (f >= l->nfoes) f = 0;
    snprintf(b, sizeof b, "%u/5 · %s", l->room + 1, enemy_short(l->foes[f].kind));
    lv_label_set_text(L.heading, b);
    snprintf(b, sizeof b, "%s", TX(ln_intent_name(l->foes[f].intent)));
    lv_label_set_text(L.intent, b);
    lv_bar_set_value(L.hp[0], l->hp * 100 / l->max_hp, LV_ANIM_OFF);
    lv_bar_set_value(L.hp[1], l->foes[f].hp * 100 / l->foes[f].max_hp, LV_ANIM_OFF);
    snprintf(b, sizeof b, TR("PV %d/%d", "HP %d/%d"), l->hp, l->max_hp);
    lv_label_set_text(L.hp_text[0], b);
    snprintf(b, sizeof b, "%d/%d", l->foes[f].hp, l->foes[f].max_hp);
    lv_label_set_text(L.hp_text[1], b);
    snprintf(b, sizeof b, TR("PM %d · Risonanza %u/3", "SP %d · Resonance %u/3"), l->sp, l->resonance);
    lv_label_set_text(L.resource, b);
    bool timing = l->state == LN_ATTACK || l->state == LN_PARRY;
    bool choose = l->state == LN_CHOOSE && !l->paused && !L.ready_ms;
    L.coach_tip = L.ready_ms ? LNT_NONE : ln_coach_tip(&R.g.coach, l, L.skill_menu != 0);
    bool coaching = choose && L.coach_tip != LNT_NONE;
    hidden(L.heading, coaching);
    hidden(L.coach_text, !coaching);
    hidden(L.coach_ok, !coaching || L.coach_tip != LNT_RESONANCE);
    if (coaching) {
        lv_label_set_text(L.coach_text, TX(ln_coach_caption(L.coach_tip)));
    }
    for (int i = 0; i < 3; i++) hidden(L.buttons[i], !choose);
    for (int i = 0; i < 3; i++) {
        lv_obj_set_width(L.buttons[i], L.skill_menu ? 108 : 88);
        lv_obj_set_width(lv_obj_get_child(L.buttons[i], 0), L.skill_menu ? 100 : 80);
        kit_pos(L.buttons[i], (L.skill_menu ? 118 : 136) + i * (L.skill_menu ? 115 : 97), 354);
        bool suggested = (L.coach_tip == LNT_ATTACK && i == 0) || (L.coach_tip == LNT_GUARD && i == 2) || (l->resonance == 3 && i == (L.skill_menu ? 0 : 1));
        lv_obj_set_style_border_width(L.buttons[i], suggested ? 2 : 1, 0);
        lv_obj_set_style_border_color(L.buttons[i], lv_color_hex(suggested ? AMBER : 0x8b8065), 0);
    }
    if (L.skill_menu) {
        char a[64], c[64];
        snprintf(a, sizeof a, TR("%s\n6 PM", "%s\n6 SP"), l->archetype == LN_ROGUE ? TR("Doppio", "Twin") : TX(ln_skill_name(l->archetype, 0)));
        snprintf(c, sizeof c, TR("%s\n5 PM", "%s\n5 SP"), l->archetype == LN_ROGUE ? TR("Prepara", "Prepare") : TX(ln_skill_name(l->archetype, 1)));
        text_button(L.buttons[0], a); text_button(L.buttons[1], c); text_button(L.buttons[2], TR("Torna", "Back"));
        if (l->sp < 6) lv_obj_add_state(L.buttons[0], LV_STATE_DISABLED); else lv_obj_remove_state(L.buttons[0], LV_STATE_DISABLED);
        if (l->sp < 5) lv_obj_add_state(L.buttons[1], LV_STATE_DISABLED); else lv_obj_remove_state(L.buttons[1], LV_STATE_DISABLED);
    } else {
        text_button(L.buttons[0], TR("Attacco", "Attack")); text_button(L.buttons[1], TR("Abilità", "Skills")); text_button(L.buttons[2], TR("Guardia", "Guard"));
        lv_obj_remove_state(L.buttons[0], LV_STATE_DISABLED); lv_obj_remove_state(L.buttons[1], LV_STATE_DISABLED);
    }
    hidden(L.catcher, !timing || l->paused || L.ready_ms);
    hidden(L.arc, l->state != LN_ATTACK || l->paused || L.ready_ms);
    hidden(L.spark, l->state != LN_ATTACK || l->paused || L.ready_ms);
    hidden(L.wave, l->state != LN_PARRY || l->paused || L.ready_ms);
    hidden(L.guard_circle, l->state != LN_PARRY || l->paused || L.ready_ms);
    hidden(L.notice, choose || l->paused);
    hidden(L.quality, l->state != LN_IMPACT || l->paused);
    hidden(L.quality_panel, l->state != LN_IMPACT || l->paused);
    hidden(L.damage_panel, l->state != LN_IMPACT || l->paused);
    kit_pos(L.notice, CX, 357); lv_obj_set_width(L.notice, 300);
    lv_obj_set_style_text_color(L.notice, lv_color_hex(IVORY), 0);
    if (l->state == LN_IMPACT) {
        static const wf_text q[] = {{"Mancato", "Miss"}, {"Bene", "Good"}, {"Ottimo", "Great"}, {"PERFETTO", "PERFECT"}};
        lv_label_set_text(L.quality, l->feedback == LN_FEEDBACK_HEAL ? TR("CURA", "HEAL")
            : l->feedback == LN_FEEDBACK_BURN ? TR("BRUCIATURA", "BURN")
            : l->feedback == LN_FEEDBACK_HERO && (l->effect_flags & LN_EFFECT_RESONANCE) ? TR("RISONANZA", "RESONANCE") : TX(q[l->quality & 3]));
        if (l->feedback == LN_FEEDBACK_HEAL) snprintf(b, sizeof b, "%s", TR("Recupera PV", "Restores HP"));
        else if (l->feedback == LN_FEEDBACK_FOE && !l->last_damage) snprintf(b, sizeof b, "%s", TR("Assorbito", "Absorbed"));
        else snprintf(b, sizeof b, l->feedback == LN_FEEDBACK_FOE ? TR("-%d PV", "-%d HP") : TR("%d danni", "%d damage"), l->last_damage);
        int x = l->feedback == LN_FEEDBACK_FOE ? 126 : L.effects.foe_x[f];
        kit_pos(L.notice, x, 258); kit_pos(L.damage_panel, x, 258); lv_obj_set_width(L.notice, 148);
        lv_obj_set_style_text_color(L.notice, lv_color_hex(l->feedback == LN_FEEDBACK_FOE ? l->quality == WF_Q_PERFECT ? JADE : DANGER : IVORY), 0);
    } else snprintf(b, sizeof b, "%s", L.ready_ms ? TR("Preparati...", "Get ready...")
                          : L.coach_tip == LNT_ATTACK_TIMING || L.coach_tip == LNT_INTENT || L.coach_tip == LNT_PARRY_TIMING ? TX(ln_coach_text(L.coach_tip))
                          : l->state == LN_ATTACK ? TR("Tocca sull'arco dorato", "Tap at the golden arc")
                          : l->state == LN_PARRY ? TR("Para quando l'onda arriva", "Parry when the wave arrives")
                          : TR("Osserva l'intento", "Read the enemy intent"));
    lv_label_set_text(L.notice, b);
    for (int i = 0; i < l->nfoes; i++) {
        hidden(L.foes[i], l->foes[i].hp <= 0 && !(l->state == LN_IMPACT && l->effect_actor == i));
        lv_obj_set_style_image_recolor(L.foes[i], lv_color_hex(IVORY), 0);
        lv_obj_set_style_image_recolor_opa(L.foes[i], i == l->target && l->nfoes == 2 ? 35 : 0, 0);
    }
    draw_actor_frames();
    hidden(L.pause, l->paused);
    lv_obj_move_foreground(L.pause); /* Pause remains reachable over the timing catcher. */
    if (l->paused) { hidden(L.catcher, true); pause_view(); }
    /* Resizing the skill buttons changes their old/new cover regions. Refresh
     * the composition once per discrete transition so adjacent HP/SP glyphs
     * are restored; motion frames keep their normal partial invalidation. */
    lv_obj_invalidate(L.root);
}

static int chosen_relic(void) {
    if (game()->state == LN_BLUEPRINT) {
        for (int n = 0; n < LR_COUNT; n++) {
            int r = (L.pick + n) % LR_COUNT;
            if (game()->relics & (1u << r)) return r;
        }
        return 0;
    }
    return game()->offers[L.pick & 1];
}
static void reward(void) {
    scene(PAGE_RELIC, 1, 145);
    bool blueprint = game()->state == LN_BLUEPRINT;
    int r = chosen_relic();
    bool full = ln_relic_count(game()) == 3;
    header(blueprint ? TR("Progetto del Faro", "Beacon blueprint") : TR("Reliquia scoperta", "Relic discovered"),
           blueprint ? TR("Porta una reliquia al borgo", "Keep one relic in the village")
                     : full ? TR("3/3 slot · puoi sostituire", "3/3 slots · you can replace")
                            : TR("Scegli una delle due", "Choose one of two"));
    kit_img(L.root, &ln_relics[r], 2, CX, 201);
    panel(L.root, CX, 292, 316, 84, 245);
    label(L.root, TX(ln_relic_name(r)), CX, 270, 302, AMBER, false);
    label(L.root, TX(ln_relic_text(r)), CX, 309, 290, IVORY, false);
    button(L.root, "<", 102, 204, 72, 72, C_LEFT, false);
    button(L.root, ">", 364, 204, 72, 72, C_RIGHT, false);
    button(L.root, blueprint ? TR("CONSERVA", "KEEP") : full ? TR("CONFRONTA", "COMPARE") : TR("SCEGLI", "CHOOSE"), CX, 374, 214, 64, C_RELIC, true);
    if (!blueprint) button(L.root, TR("Lascia", "Skip"), CX, 433, 124, 40, C_SKIP_RELIC, false);
}
static void replace_relic(void) {
    scene(PAGE_REPLACE, 1, 165);
    int incoming = game()->offers[L.offer_pick], outgoing = L.pick;
    header(TR("Confronta", "Compare"), TR("Nuova sopra · attuale sotto", "New above · current below"));
    const int relics[] = {incoming, outgoing};
    for (int i = 0; i < 2; i++) {
        int y = 174 + i * 122;
        panel(L.root, CX, y, 324, 120, 245);
        kit_img(L.root, &ln_relics[relics[i]], 1, 116, y);
        lv_obj_t *name = label(L.root, TX(ln_relic_name(relics[i])), 280, y, 222, i ? IVORY : AMBER, false);
        lv_obj_t *text = label(L.root, TX(ln_relic_text(relics[i])), 280, y, 222, IVORY, false);
        lv_obj_update_layout(name); lv_obj_update_layout(text);
        int height = lv_obj_get_height(name);
        kit_pos(name, 280, y - 54 + height / 2);
        kit_pos(text, 280, y - 54 + height + 4 + lv_obj_get_height(text) / 2);
    }
    button(L.root, "<", 51, 296, 44, 56, C_LEFT, false);
    button(L.root, ">", 415, 296, 44, 56, C_RIGHT, false);
    button(L.root, TR("Sostituisci", "Replace"), CX, 389, 224, 56, C_REPLACE_RELIC, true);
    button(L.root, TR("Indietro", "Back"), CX, 434, 124, 40, C_CANCEL_REPLACE, false);
}
static void event(void) {
    scene(PAGE_EVENT, 1, 115);
    bool mirror = game()->branch != 0;
    header(mirror ? TR("Lo specchio del Faro", "The beacon mirror") : TR("Il santuario dimenticato", "The forgotten shrine"), NULL);
    kit_img(L.root, &ln_relics[LR_COAL], 2, CX, 199);
    panel(L.root, CX, 286, 296, 66, 240);
    label(L.root, mirror ? TR("Offri 12 PV: Risonanza piena e +8 PM. Rifiuta: +12 PM.", "Offer 12 HP: full resonance and +8 SP. Refuse: +12 SP.") : TR("Una brace ancora viva.\nCome vuoi riprendere fiato?", "One ember is still alive.\nHow will you recover?"), CX, 284, 280, IVORY, false);
    lv_obj_t *offer = button(L.root, mirror ? TR("Offri 12 PV", "Offer 12 HP") : TR("+25 PV", "+25 HP"), 162, 360, 130, 72, C_EVENT0, true);
    if (mirror && game()->hp <= 12) lv_obj_add_state(offer, LV_STATE_DISABLED);
    button(L.root, TR("+12 PM", "+12 SP"), 304, 360, 130, 72, C_EVENT1, false);
}
static void cleared(void) {
    scene(PAGE_CLEAR, 1, 110);
    char b[80];
    snprintf(b, sizeof b, TR("%u oro al sicuro se rientri", "%u gold secured if you return"), game()->currency);
    header(TR("Un passo oltre", "One step further"), b);
    L.hero = kit_img(L.root, hero_sprite(0), 2, CX, 210);
    label(L.root, TR("Continui o porti a casa\nquello che hai trovato?", "Continue or take home\nwhat you have found?"), CX, 291, 292, IVORY, false);
    button(L.root, TR("Prosegui", "Continue"), 162, 358, 130, 72, C_CONTINUE, true);
    button(L.root, TR("Rientra", "Return"), 304, 358, 130, 72, C_RETREAT, false);
}
static void result(void) {
    scene(PAGE_RESULT, game()->state == LN_WON ? 0 : 1, 110);
    bool win = game()->state == LN_WON, lost = game()->state == LN_LOST;
    header(win ? TR("Il faro ricorda", "The beacon remembers") : lost ? TR("Non è l'ultimo viaggio", "Your journey is not over") : TR("Di nuovo a casa", "Home again"), NULL);
    kit_img(L.root, &ln_relics[win ? LR_HEART : LR_COMPASS], 2, CX, 205);
    char b[144];
    unsigned banked = game()->settled ? game()->settled - 1u : 0;
    snprintf(b, sizeof b, lost ? TR("+%u oro al borgo\nPerso solo il 25%% della spedizione.", "+%u gold to the village\nOnly 25%% of run gold was lost.")
                              : TR("+%u oro al borgo\nIl prossimo viaggio ti aspetta.", "+%u gold to the village\nYour next journey awaits."), banked);
    panel(L.root, CX, 294, 316, 82, 240);
    label(L.root, b, CX, 294, 292, IVORY, false);
    button(L.root, TR("Riprova", "Retry"), 162, 370, 130, 72, C_RETRY, true);
    button(L.root, TR("Borgo", "Village"), 304, 370, 130, 72, C_HOME, false);
}
static void show(void) {
    int s = game()->state;
    if (battle_state(s)) battle();
    else if (s == LN_ROUTE) route();
    else if (s == LN_RELIC || s == LN_BLUEPRINT) reward();
    else if (s == LN_EVENT) event();
    else if (s == LN_CLEARED) cleared();
    else if (s == LN_WON || s == LN_LOST || s == LN_RETURNED) result();
    else scr_town();
}
void scr_lantern(void) { L.skill_menu = 0; show(); }

static void on_click(lv_event_t *e) {
    int c = (int)(intptr_t)lv_event_get_user_data(e);
    rpg_lantern *l = game();
    rpg_lantern before = *l;
    if (L.page == PAGE_BATTLE && (l->paused || L.ready_ms) && c != C_RESUME && c != C_ABORT && c != C_PAUSE) return;
    if (c == C_TIPS) { R.g.coach.disabled ^= 1; save(); options(); return; }
    if (c == C_REPLAY_TIPS) { R.g.coach = (ln_coach){0}; save(); options(); return; }
    if (c == C_ACK_TIP) { ln_coach_ack(&R.g.coach, L.coach_tip); save(); battle_refresh(); return; }
    if (c == C_QUICK) {
        rpg_new_hero nh = {0};
        snprintf(nh.name, sizeof nh.name, "Wayfarer"); nh.lang = wf_lang;
        rpg_game_new(&R.g, &nh, r_now(), wf_platform_random());
        R.loaded = true; save(); r_go(R_TOWN); return;
    }
    if (c == C_CUSTOM) { r_go(R_CREATE); return; }
    if (c == C_LANGUAGE) {
        wf_lang ^= 1; R.g.lang = wf_lang;
        if (R.loaded) save();
        if (L.page == PAGE_TITLE) scr_lantern_title(); else options();
        return;
    }
    if (c == C_HOME) { ln_home(l); save(); r_go(R_TOWN); return; }
    if (c == C_MORE) { menu(); return; }
    if (c == C_HERO) { R.screen = R_LANTERN; loadout(); return; }
    if (c == C_BAG || c == C_COLLECTION) {
        L.pick = 0;
        if (l->blueprints) while (!(l->blueprints & (1u << L.pick))) L.pick++;
        collection(); return;
    }
    if (c == C_OPTIONS) { options(); return; }
    if (c == C_CLASSIC) { r_go(R_TOWN_CLASSIC); return; }
    if (c == C_CLASSES) { r_go(R_CLASS_ATLAS); return; }
    if (c == C_MAP) { world_map(); return; }
    if (c == C_BESTIARY) { L.pick = 0; bestiary(); return; }
    if (c == C_HELP) { L.pick = 0; help(); return; }
    if (c == C_CANCEL_REPLACE) { L.pick = L.offer_pick; reward(); return; }
    if (c == C_RELIC && l->state == LN_RELIC && ln_relic_count(l) == 3) {
        L.offer_pick = L.pick & 1;
        L.pick = 0;
        while (!(l->relics & (1u << L.pick))) L.pick++;
        replace_relic(); return;
    }
    if (c == C_HUNT) { hunt(); return; }
    if (c == C_COLLECT_HUNT) {
        rpg_haul haul;
        int status = rpg_hunt_collect(&R.g, r_now(), &haul);
        if (status != RPG_OK) { kit_toast(TR("Fai spazio nello zaino: il bottino è conservato", "Make bag space: your haul is safe"), NULL); return; }
        save(); r_go(R_TOWN); kit_toast(TR("Bottino raccolto", "Haul collected"), NULL); return;
    }
    if (c == C_EQUIP) { r_go(R_BAG); return; }
    if (c == C_EQUIP_RELIC) {
        int r = L.pick % LR_COUNT;
        if ((l->blueprints & (1u << r)) && !ln_active(l)) { l->equipped = l->equipped == r + 1 ? 0 : (uint8_t)(r + 1); save(); collection(); }
        return;
    }
    if (c == C_ASSIST || c == C_MOTION) {
        if (c == C_ASSIST) l->assist ^= 1; else l->reduced_motion ^= 1;
        save(); options(); return;
    }
    if (c == C_LEFT || c == C_RIGHT) {
        int step = c == C_RIGHT ? 1 : -1;
        if (L.page == PAGE_LOADOUT) { l->archetype = (uint8_t)((l->archetype + 3 + step) % 3); save(); loadout(); }
        else if (L.page == PAGE_COLLECTION) { L.pick = (L.pick + LR_COUNT + step) % LR_COUNT; collection(); }
        else if (L.page == PAGE_BESTIARY) { L.pick = (L.pick + LN_ENEMIES + step) % LN_ENEMIES; bestiary(); }
        else if (L.page == PAGE_HELP) { L.pick = (L.pick + 3 + step) % 3; help(); }
        else if (L.page == PAGE_REPLACE) {
            do { L.pick = (L.pick + LR_COUNT + step) % LR_COUNT; }
            while (!(l->relics & (1u << L.pick)));
            replace_relic();
        }
        else if (L.page == PAGE_RELIC) {
            if (l->state == LN_BLUEPRINT) {
                int r = chosen_relic();
                for (int n = 0; n < LR_COUNT; n++) {
                    r = (r + LR_COUNT + step) % LR_COUNT;
                    if (l->relics & (1u << r)) break;
                }
                L.pick = r;
            } else L.pick ^= 1;
            reward();
        }
        return;
    }
    if (c == C_DEPART || c == C_BEGIN || c == C_RETRY) {
        if (R.g.run.active || R.g.hunt.active) {
            kit_toast(TR("Concludi prima il viaggio o la caccia classica", "Finish your classic run or hunt first"), NULL); return;
        }
        ln_start(l, l->archetype, wf_platform_random());
        R.screen = R_LANTERN; L.pick = L.skill_menu = 0; L.ready_ms = 0; changed(); return;
    }
    if (c == C_PAUSE) { if (!l->paused) { l->paused = 1; save(); battle_refresh(); } return; }
    if (c == C_RESUME) { l->paused = 0; L.ready_ms = 650; save(); battle(); return; }
    if (c == C_ABORT) { l->state = LN_LOST; l->paused = 0; changed(); return; }
    if (c == C_TARGET0 || c == C_TARGET1) {
        int t = c - C_TARGET0;
        if (l->state == LN_CHOOSE && t < l->nfoes && l->foes[t].hp > 0) { l->target = (uint8_t)t; save(); battle_refresh(); }
        return;
    }
    if (c == C_SKILLS && !L.skill_menu) { L.skill_menu = 1; battle_refresh(); return; }
    if (c == C_GUARD && L.skill_menu) { L.skill_menu = 0; battle_refresh(); return; }
    bool success = false;
    switch (c) {
    case C_ROUTE0: success = ln_enter(l, LN_PATH_TRAIL); break;
    case C_ROUTE1: success = ln_enter(l, LN_PATH_PIER); break;
    case C_BASTION: success = ln_enter(l, LN_PATH_BASTION); break;
    case C_ATTACK: success = ln_action(l, L.skill_menu ? LN_POWER : LN_BASIC, l->target); L.skill_menu = 0; break;
    case C_SKILLS: success = ln_action(l, LN_SUPPORT, l->target); L.skill_menu = 0; break;
    case C_GUARD: success = ln_action(l, LN_GUARD, l->target); break;
    case C_RELIC: success = l->state == LN_BLUEPRINT ? ln_keep_blueprint(l, chosen_relic()) : ln_choose_relic(l, L.pick & 1); L.pick = 0; break;
    case C_REPLACE_RELIC: success = ln_replace_relic(l, L.offer_pick, L.pick); L.pick = 0; break;
    case C_SKIP_RELIC: success = ln_skip_relic(l); L.pick = 0; break;
    case C_EVENT0: case C_EVENT1: success = ln_event(l, c == C_EVENT1); break;
    case C_CONTINUE: success = ln_continue(l); break;
    case C_RETREAT: success = ln_retreat(l); break;
    default: break;
    }
    if (success) changed_from(&before);
}

static void advance_battle_clock(void) {
    uint32_t now = lv_tick_get(), dt = now - L.clock;
    L.clock = now;
    if (L.page == PAGE_BATTLE && !game()->paused) {
        if (L.ready_ms) {
            if (dt < L.ready_ms) { L.ready_ms -= dt; return; }
            dt -= L.ready_ms; L.ready_ms = 0;
            battle_refresh();
        }
        rpg_lantern before = *game();
        if (ln_tick(game(), dt)) changed_from(&before);
    }
}
void lantern_tick(void) {
    advance_battle_clock();
    uint32_t now = lv_tick_get();
    if (L.page == PAGE_BATTLE && !game()->paused) {
        rpg_lantern *l = game();
        if (L.page == PAGE_BATTLE && !L.ready_ms) {
            if (l->state == LN_ATTACK) {
                /* At t=700 the spark is at 250 degrees, the exact target center. */
                float angle = (70.0f + (float)(l->elapsed_ms % 1400) * 360.0f / 1400.0f) * 3.14159265f / 180.0f;
                kit_pos(L.spark, CX + (int)lroundf(214 * cosf(angle)), CY + (int)lroundf(214 * sinf(angle)));
            } else if (l->state == LN_PARRY) {
                int radius = 215 - 140 * l->elapsed_ms / 1000;
                if (radius < 30) radius = 30;
                lv_obj_set_size(L.wave, radius * 2, radius * 2); lv_obj_center(L.wave);
            }
        }
    }
    if (L.page == PAGE_BATTLE && !game()->paused && now - L.visual_clock >= 20) {
        L.visual_clock = now; draw_actor_frames();
    } else if (L.page != PAGE_BATTLE && now - L.anim_clock >= 140 && !game()->paused) { L.anim_clock = now; draw_actor_frames(); }
}
bool lantern_key(int key) {
    if (key != WF_KEY_BOOT_SHORT && key != WF_KEY_BOOT_LONG && key != WF_KEY_BOOT_DOWN) return false;
    if (key == WF_KEY_BOOT_DOWN && L.page != PAGE_BATTLE) return false;
    if (L.page == PAGE_REPLACE) { L.pick = L.offer_pick; reward(); return true; }
    if (L.page == PAGE_BATTLE) {
        int phase = game()->state;
        advance_battle_clock();
        if (phase != game()->state) return true; /* never apply a stale press to a new phase */
        if (game()->paused || L.ready_ms) return true;
        rpg_lantern before = *game();
        if (!game()->paused && !L.ready_ms && ln_tap(game())) { changed_from(&before); return true; }
        if (L.skill_menu) { L.skill_menu = 0; battle_refresh(); return true; }
        if (!game()->paused) { game()->paused = 1; save(); battle_refresh(); }
        return true;
    }
    if (L.page == PAGE_LOADOUT || L.page == PAGE_MENU || L.page == PAGE_OPTIONS || L.page == PAGE_COLLECTION ||
        L.page == PAGE_MAP || L.page == PAGE_BESTIARY || L.page == PAGE_HELP || L.page == PAGE_HUNT) {
        r_go(R_TOWN); return true;
    }
    return true;
}
void lantern_suspend(void) {
    if (R.screen == R_LANTERN && battle_state(game()->state)) { game()->paused = 1; r_dirty(); }
}
void lantern_wake(void) {
    L.clock = lv_tick_get();
    if (R.screen == R_LANTERN && battle_state(game()->state)) battle();
}
