/* World map, zones, dungeon floors and the quest board / options. */
#include <math.h>
#include <stdio.h>
#include <string.h>
#include "rui.h"

static lv_obj_t *dlg;
static void close_dlg(lv_event_t *e) { (void)e; kit_dialog_close(dlg); dlg = NULL; }
static void on_go(lv_event_t *e) { r_go((int)(intptr_t)lv_event_get_user_data(e)); }

/* ---- World map --------------------------------------------------------------------- */
static void on_spot(lv_event_t *e) {
    int z = (int)(intptr_t)lv_event_get_user_data(e);
    if (z > R.g.zone_unlocked) {
        char buf[96];
        snprintf(buf, sizeof buf, TR("Completa %s", "Clear %s"), TX(rpg_zone(z - 1)->dungeon_name));
        kit_toast(buf, ri(RI_LOCK));
        return;
    }
    R.zone = z;
    r_go(R_ZONE);
}

void scr_map(void) {
    lv_obj_t *s = r_scene(RS_WORLD);
    for (int z = 0; z < RPG_ZONES; z++) {
        int x = rpg_world_spots[z][0], y = rpg_world_spots[z][1];
        bool open = z <= R.g.zone_unlocked;
        lv_obj_t *hit = lv_obj_create(s);
        lv_obj_remove_style_all(hit);
        lv_obj_set_size(hit, 96, 80);
        kit_pos(hit, x, y);
        lv_obj_add_flag(hit, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_event_cb(hit, on_spot, LV_EVENT_CLICKED, (void *)(intptr_t)z);
        char buf[48];
        snprintf(buf, sizeof buf, "%s", open ? TX(rpg_zone(z)->field_name) : "???");
        lv_obj_t *p = kit_pill(s, buf, x, y + 40);
        if (!open) {
            lv_obj_set_style_opa(p, 170, 0);
            kit_img(s, ri(RI_LOCK), 2, x, y - 6);
        } else if (R.g.hunt.active && R.g.hunt.zone == z) {
            kit_img(s, ri(RI_SWORD), 2, x, y - 30);
        }
    }
    kit_label(s, &wf_font_big, COL_GOLD, TR("Isole Sospese", "Floating Isles"), CX, 42);
    lv_obj_t *town = lv_obj_create(s);
    lv_obj_remove_style_all(town);
    lv_obj_set_size(town, 90, 70);
    kit_pos(town, rpg_world_spots[5][0], rpg_world_spots[5][1]);
    lv_obj_add_flag(town, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(town, on_go, LV_EVENT_CLICKED, (void *)(intptr_t)R_TOWN);
    kit_pill(s, TR("Borgo Lanterna", "Lantern Hollow"), rpg_world_spots[5][0], rpg_world_spots[5][1] + 36);
}

/* ---- Zone ---------------------------------------------------------------------------- */
static void on_fight(lv_event_t *e) {
    (void)e;
    rpg_foe foes[RPG_MAX_FOES];
    int n;
    int r = rpg_field_encounter(&R.g, R.zone, foes, &n);
    if (r != RPG_OK) { kit_toast(rpg_err(r), ri(r == RPG_ERR_STAMINA ? RI_STAMINA : RI_HEART)); return; }
    r_dirty();
    r_start_battle(foes, n, false, false);
}

static void on_hunt(lv_event_t *e) {
    (void)e;
    int r = rpg_hunt_start(&R.g, R.zone, r_now());
    if (r != RPG_OK) { kit_toast(rpg_err(r), ri(RI_STAMINA)); return; }
    r_save_now();
    r_go(R_TOWN);
    kit_toast(TR("A caccia! Torna quando vuoi", "Off hunting! Come back any time"), ri(RI_SWORD));
}

static void on_enter(lv_event_t *e) {
    int cp = (int)(intptr_t)lv_event_get_user_data(e);
    int r = rpg_dungeon_enter(&R.g, R.zone, cp, r_now());
    if (r != RPG_OK) { kit_toast(rpg_err(-r), NULL); return; }
    r_save_now();
    r_go(R_DUNGEON);
}

void scr_zone(void) {
    const rpg_zone_info *z = rpg_zone(R.zone);
    lv_obj_t *s = r_scene(RS_FIELD_MEADOW + z->field_scene);
    kit_dim(s, 130);
    kit_label(s, &wf_font_big, COL_GOLD, TX(z->field_name), CX, 70);
    char buf[128];
    snprintf(buf, sizeof buf, "%s\n%s %d-%d", TX(z->blurb), TR("Livelli", "Levels"), z->min_level, z->max_level);
    lv_obj_t *b = kit_label(s, &wf_font_small, COL_TEXT, buf, CX, 116);
    lv_obj_set_width(b, 320);
    lv_label_set_long_mode(b, LV_LABEL_LONG_WRAP);
    kit_round_button(s, ri(RI_SWORD), TR("Combatti", "Fight"), 124, 210, 80, 0xe8503a, on_fight, 0);
    kit_round_button(s, ri(RI_AUTO), TR("Caccia auto", "Auto hunt"), 342, 210, 80, 0x3aba6a, on_hunt, 0);
    lv_obj_t *d = kit_text_button(s, TX(z->dungeon_name), CX, 300, 250, 50, 0x6a3a8a, on_enter, 0);
    lv_obj_t *ic = lv_image_create(d);
    lv_image_set_src(ic, gfx_sprite(ri(RI_STAIRS), 2));
    lv_obj_align(ic, LV_ALIGN_LEFT_MID, 4, 0);
    if (rpg_dungeon_checkpoint(&R.g, R.zone))
        kit_text_button(s, TR("Dal piano 6", "From floor 6"), CX, 354, 170, 40, 0x4a2a6a, on_enter, 1);
    snprintf(buf, sizeof buf, TR("Stamina %d · record piano %d", "Stamina %d · best floor %d"), R.g.hero.stamina,
             R.g.best_floor[R.zone]);
    kit_label(s, &wf_font_small, COL_ENERGY, buf, CX, 392);
    kit_round_button(s, ri(RI_BACK), NULL, CX, 432, 44, COL_PANEL_HI, on_go, R_MAP);
}

/* ---- Dungeon floor ------------------------------------------------------------------------ */
static lv_obj_t *d_hp, *d_sp;
static lv_point_precise_t line_pts[24][2];
static int room_row, room_col;

static int room_x(int c) { return 140 + c * 93; }
static int room_y(int r) { return 360 - r * 72; }

static void leave_to_town(const char *title, const char *body, const wf_sprite *icon) {
    r_save_now();
    r_go(R_TOWN);
    dlg = kit_dialog(title, body, icon);
    kit_dialog_button(dlg, "Ok", COL_BTN_OK, close_dlg, 0);
}

static void descend_choice(int leave) {
    kit_dialog_close(dlg);
    dlg = NULL;
    if (leave) {
        rpg_dungeon_leave(&R.g, false);
        leave_to_town(TR("Di ritorno", "Back home"), TR("Torni in città con tutto il bottino.", "You return to town with all your loot."), ri(RI_STAIRS));
        return;
    }
    int r = rpg_dungeon_descend(&R.g);
    if (r == 1) {
        char body[128];
        snprintf(body, sizeof body, TR("Hai conquistato %s!", "You conquered %s!"), TX(rpg_zone(R.g.run.zone)->dungeon_name));
        leave_to_town(TR("Dungeon completato!", "Dungeon cleared!"), body, ri(RI_TROPHY));
        return;
    }
    r_save_now();
    r_go(R_DUNGEON);
}

static void on_descend(lv_event_t *e) { descend_choice((int)(intptr_t)lv_event_get_user_data(e)); }

static void stairs_dialog(void) {
    char body[96];
    snprintf(body, sizeof body, TR("Scendi al piano %d o torni in città tenendo il bottino?", "Go down to floor %d or head back to town with your loot?"),
             R.g.run.floor + 1);
    dlg = kit_dialog(TR("Scale", "Stairs"), body, ri(RI_STAIRS));
    kit_dialog_button(dlg, TR("Scendi", "Descend"), COL_BTN_OK, on_descend, 0);
    kit_dialog_button(dlg, TR("Città", "Town"), COL_PANEL_HI, on_descend, 1);
}

void r_after_dungeon_battle(int state) {
    rpg_run *run = &R.g.run;
    if (state == B_LOST) {
        rpg_dungeon_leave(&R.g, true);
        leave_to_town(TR("Sconfitto...", "Defeated..."), TR("Ti risvegli alla locanda. Hai perso metà dell'oro della discesa.",
                                                         "You wake up at the inn. Half of this descent's gold is lost."), ri(RI_SKULL));
        return;
    }
    if (run->row >= 0) run->cleared[run->row] |= (uint8_t)(1u << run->col);
    r_save_now();
    r_go(R_DUNGEON);
    if (state == B_WON && run->row == RPG_ROWS) {
        if (run->floor >= RPG_FLOORS) {
            descend_choice(0);  /* completes the dungeon */
            return;
        }
        stairs_dialog();
    }
}

static void on_room(lv_event_t *e) {
    intptr_t v = (intptr_t)lv_event_get_user_data(e);
    room_row = (int)(v >> 4);
    room_col = (int)(v & 15);
    rpg_foe foes[RPG_MAX_FOES];
    int n = 0;
    int kind = rpg_room_enter(&R.g, room_row, room_col, foes, &n);
    if (kind < 0) { kit_toast(rpg_err(-kind), ri(RI_STAMINA)); return; }
    r_dirty();
    if (n) { r_start_battle(foes, n, true, kind == ROOM_BOSS); return; }
    if (kind == ROOM_STAIRS) { r_go(R_DUNGEON); stairs_dialog(); return; }
    if (kind == ROOM_MERCHANT) {
        rpg_room_result res;
        rpg_room_resolve(&R.g, &res, foes, &n);
        R.shop_in_dungeon = true;
        R.shop_tab = 0;
        r_go(R_SHOP);
        kit_toast(TR("Un mercante errante!", "A wandering merchant!"), ri(RI_COIN));
        return;
    }
    rpg_room_result res;
    rpg_room_resolve(&R.g, &res, foes, &n);
    if (n) {   /* mimic! */
        r_go(R_DUNGEON);
        kit_toast(TR("Era un mimic!", "It was a mimic!"), ri(RI_SKULL));
        r_start_battle(foes, n, true, false);
        return;
    }
    r_save_now();
    r_go(R_DUNGEON);
    char body[160], name[48];
    const char *title = "";
    const wf_sprite *ic = &rpg_room_icons[kind];
    switch (kind) {
    case ROOM_CHEST:
        title = TR("Forziere!", "Chest!");
        snprintf(body, sizeof body, "%s\n+%ld %s", res.got_item ? rpg_item_name(&res.item, name, sizeof name) : TR("Zaino pieno!", "Bag full!"),
                 (long)res.value, TR("oro", "gold"));
        if (res.got_item) ic = item_icon(&res.item);
        break;
    case ROOM_TRAP:
        title = TR("Trappola!", "Trap!");
        if (res.value) snprintf(body, sizeof body, TR("Ahi! -%ld PV", "Ouch! -%ld HP"), (long)res.value);
        else snprintf(body, sizeof body, "%s", TR("L'hai evitata con agilità!", "You dodged it nimbly!"));
        break;
    case ROOM_FOUNTAIN:
        title = TR("Fontana", "Fountain");
        snprintf(body, sizeof body, "%s", TR("Acqua limpida: PV e PM recuperati.", "Clear water: HP and SP restored."));
        break;
    default: {
        static const wf_text ev[4] = {{"Un santuario dimenticato ti benedice.", "A forgotten shrine blesses you."},
                                      {"Un avventuriero ferito ti regala una pozione.", "A wounded adventurer gives you a potion."},
                                      {"Una vena di minerale! +2", "An ore vein! +2"},
                                      {"Un mucchio d'oro dimenticato!", "A forgotten pile of gold!"}};
        title = TR("Evento", "Event");
        snprintf(body, sizeof body, "%s", TX(ev[res.value & 3]));
        break;
    }
    }
    dlg = kit_dialog(title, body, ic);
    kit_dialog_button(dlg, TR("Avanti", "Onward"), COL_BTN_OK, close_dlg, 0);
}

static void on_exit_run(lv_event_t *e) {
    (void)e;
    kit_dialog_close(dlg);
    dlg = NULL;
    if (!R.g.cons[CO_RETURN]) { kit_toast(TR("Ti serve una pergamena di ritorno", "You need a return scroll"), ri(RI_SCROLL)); return; }
    R.g.cons[CO_RETURN]--;
    rpg_dungeon_leave(&R.g, false);
    leave_to_town(TR("Pergamena usata", "Scroll used"), TR("Una luce ti riporta in città.", "A light carries you back to town."), &rpg_cons_icons[CO_RETURN]);
}

static void on_exit_ask(lv_event_t *e) {
    (void)e;
    char body[96];
    snprintf(body, sizeof body, TR("Usare una pergamena di ritorno? (ne hai %d)", "Use a return scroll? (you have %d)"), R.g.cons[CO_RETURN]);
    dlg = kit_dialog(TR("Uscire?", "Leave?"), body, &rpg_cons_icons[CO_RETURN]);
    kit_dialog_button(dlg, TR("Esci", "Leave"), COL_BAD, on_exit_run, 0);
    kit_dialog_button(dlg, TR("Resta", "Stay"), COL_PANEL_HI, close_dlg, 0);
}

static void on_potion(lv_event_t *e) {
    int c = (int)(intptr_t)lv_event_get_user_data(e);
    kit_dialog_close(dlg);
    dlg = NULL;
    int r = rpg_use_consumable(&R.g, c);
    kit_toast(r == RPG_OK ? TX(rpg_consumable(c)->name) : rpg_err(r), &rpg_cons_icons[c]);
    if (r == RPG_OK) r_dirty();
    rpg_stats st;
    rpg_stats_get(&R.g, &st);
    r_vitals_update(d_hp, d_sp, R.g.hero.hp, st.max_hp, R.g.hero.sp, st.max_sp);
}

static void on_supplies(lv_event_t *e) {
    (void)e;
    char body[96];
    snprintf(body, sizeof body, TR("Pozioni %d · Gran pozioni %d · Eteri %d", "Potions %d · Hi-potions %d · Ethers %d"),
             R.g.cons[CO_POTION], R.g.cons[CO_HIPOTION], R.g.cons[CO_ETHER]);
    dlg = kit_dialog(TR("Scorte", "Supplies"), body, ri(RI_BAG));
    kit_dialog_button(dlg, TR("Pozione", "Potion"), COL_BTN_ALT, on_potion, CO_POTION);
    kit_dialog_button(dlg, TR("Gran p.", "Hi-pot."), COL_BTN_ALT, on_potion, CO_HIPOTION);
    kit_dialog_button(dlg, TR("Etere", "Ether"), COL_BTN, on_potion, CO_ETHER);
    kit_dialog_button(dlg, "X", COL_PANEL_HI, close_dlg, 0);
}

void scr_dungeon(void) {
    rpg_run *run = &R.g.run;
    if (!run->active) { r_go(R_TOWN); return; }
    R.shop_in_dungeon = false;
    const rpg_zone_info *z = rpg_zone(run->zone);
    lv_obj_t *s = r_scene(RS_DUNGEON_CRYPT + z->dungeon_scene);
    kit_dim(s, 150);
    r_vitals_arcs(s, &d_hp, &d_sp);
    rpg_stats st;
    rpg_stats_get(&R.g, &st);
    r_vitals_update(d_hp, d_sp, R.g.hero.hp, st.max_hp, R.g.hero.sp, st.max_sp);
    char buf[96];
    snprintf(buf, sizeof buf, "%s · %s %d/%d", TX(z->dungeon_name), TR("Piano", "Floor"), run->floor, RPG_FLOORS);
    kit_label(s, &wf_font_small, COL_GOLD, buf, CX, 36);
    /* links */
    int nl = 0;
    for (int r = -1; r < RPG_ROWS; r++)
        for (int c = 0; c < RPG_COLS; c++) {
            if (r >= 0 && !run->rooms[r][c]) continue;
            if (r < 0 && c != 1) continue;
            for (int d = 0; d < RPG_COLS; d++) {
                bool link = r < 0 ? run->rooms[0][d] != 0 : ((run->links[r][c] >> d) & 1);
                if (!link || nl >= 24) continue;
                line_pts[nl][0].x = room_x(r < 0 ? 1 : c);
                line_pts[nl][0].y = r < 0 ? 422 : room_y(r);
                line_pts[nl][1].x = room_x(d);
                line_pts[nl][1].y = room_y(r + 1);
                lv_obj_t *ln = lv_line_create(s);
                lv_line_set_points(ln, line_pts[nl], 2);
                bool walked = (r < 0 ? run->row >= 0 && run->col == d && run->row == 0 : false) ||
                              (r >= 0 && r < run->row && ((run->cleared[r] >> c) & 1) && ((run->cleared[r + 1] >> d) & 1));
                lv_obj_set_style_line_color(ln, lv_color_hex(walked ? COL_GOLD : 0x6a6a8a), 0);
                lv_obj_set_style_line_width(ln, 4, 0);
                lv_obj_set_style_line_rounded(ln, true, 0);
                nl++;
            }
        }
    /* rooms */
    for (int r = 0; r <= RPG_ROWS; r++)
        for (int c = 0; c < RPG_COLS; c++) {
            int kind = run->rooms[r][c];
            if (!kind) continue;
            bool reach = rpg_room_reachable(run, r, c);
            bool here = run->row == r && run->col == c;
            bool cleared = (run->cleared[r] >> c) & 1;
            bool known = r <= run->row + 1 || kind == ROOM_BOSS || kind == ROOM_STAIRS;
            uint32_t color = here ? COL_GOLD : reach ? COL_BTN : cleared ? 0x3a3a4a : COL_PANEL_HI;
            const wf_sprite *ic = known ? &rpg_room_icons[kind] : &rpg_room_icons[ROOM_EVENT];
            lv_obj_t *b = kit_round_button(s, ic, NULL, room_x(c), room_y(r), kind >= ROOM_BOSS ? 64 : 56, color,
                                           reach ? on_room : NULL, (r << 4) | c);
            if (!known) lv_obj_set_style_image_opa(lv_obj_get_child(b, 0), 90, 0);
            if (cleared && !here) lv_obj_set_style_opa(b, 160, 0);
            if (reach) kit_bob(b, 4, 900);
        }
    /* the hero marker */
    hero_look l;
    hero_look_from_game(&l, &R.g);
    int hx = run->row < 0 ? CX : room_x(run->col), hy = run->row < 0 ? 418 : room_y(run->row) - 34;
    hero_obj(s, &l, HX_IDLE, 1, hx, hy);
    kit_round_button(s, &rpg_cons_icons[CO_RETURN], NULL, 52, 210, 52, COL_PANEL_HI, on_exit_ask, 0);
    kit_round_button(s, ri(RI_BAG), NULL, 414, 210, 52, COL_PANEL_HI, on_supplies, 0);
    snprintf(buf, sizeof buf, "%lu", (unsigned long)run->gold);
    kit_label(s, &wf_font_small, COL_GOLD, buf, 414, 252);
}

/* ---- Quests & options ------------------------------------------------------------------------ */
static void on_claim(lv_event_t *e) {
    int i = (int)(intptr_t)lv_event_get_user_data(e);
    int gold = rpg_quest_claim(&R.g, i);
    if (!gold) return;
    char buf[64];
    snprintf(buf, sizeof buf, TR("+%d oro e %s", "+%d gold and %s"), gold, TX(rpg_consumable(R.g.quests[i].reward)->name));
    kit_toast(buf, ri(RI_TROPHY));
    r_save_now();
    r_go(R_QUESTS);
}

static void on_option(lv_event_t *e) {
    int o = (int)(intptr_t)lv_event_get_user_data(e);
    switch (o) {
    case 0: R.g.lang ^= 1; wf_lang = R.g.lang; break;
    case 1: R.g.brightness = (uint8_t)(R.g.brightness >= 100 ? 30 : R.g.brightness + 10); wf_platform_brightness(R.g.brightness); break;
    case 2: R.g.hour_offset = (uint8_t)((R.g.hour_offset + 1) % 24); break;
    case 3: R.g.auto_battle ^= 1; break;
    }
    r_dirty();
    r_go(R_QUESTS);
}

void scr_quests(void) {
    lv_obj_t *s = kit_screen(NULL);
    lv_obj_set_style_bg_color(s, lv_color_hex(0x1e1a14), 0);
    lv_obj_t *list = r_list(s, CX, 226, 330, 380);
    lv_obj_set_style_pad_top(list, 30, 0);
    lv_obj_set_style_pad_bottom(list, 70, 0);
    r_text(list, TR("Missioni di oggi", "Today's quests"), COL_GOLD, &wf_font_big);
    char buf[128], q[64];
    for (int i = 0; i < RPG_QUESTS; i++) {
        const rpg_quest *qt = &R.g.quests[i];
        lv_obj_t *row = r_row(list, 64);
        snprintf(buf, sizeof buf, "%s\n%d/%d · %d %s", rpg_quest_text(qt, q, sizeof q), qt->kind == Q_GOLD ? qt->progress * 50 : qt->progress,
                 qt->kind == Q_GOLD ? qt->goal * 50 : qt->goal, qt->gold, TR("oro", "gold"));
        lv_obj_t *t = r_text(row, buf, qt->claimed ? COL_DIM : COL_TEXT, NULL);
        lv_obj_align(t, LV_ALIGN_LEFT_MID, 10, 0);
        if (!qt->claimed && qt->progress >= qt->goal) {
            lv_obj_t *b = kit_text_button(row, TR("Ritira", "Claim"), 0, 0, 76, 40, COL_GOLD, on_claim, i);
            lv_obj_align(b, LV_ALIGN_RIGHT_MID, -6, 0);
        }
    }
    snprintf(buf, sizeof buf, TR("Mostri %lu · Boss %lu · Sconfitte %lu\nGiorni di fila: %d", "Monsters %lu · Bosses %lu · Defeats %lu\nDaily streak: %d"),
             (unsigned long)R.g.kills, (unsigned long)R.g.bosses, (unsigned long)R.g.deaths, R.g.streak);
    r_text(list, buf, COL_MOOD, NULL);
    r_text(list, TR("Opzioni", "Options"), COL_GOLD, &wf_font_big);
    static const wf_text opts[4] = {{"Lingua: Italiano", "Language: English"}, {"Luminosità", "Brightness"},
                                    {"Ora del giorno", "Time of day"}, {"AUTO nelle lotte", "AUTO in battles"}};
    for (int o = 0; o < 4; o++) {
        lv_obj_t *row = r_row(list, 50);
        if (o == 1) snprintf(buf, sizeof buf, "%s %d%%", TX(opts[o]), R.g.brightness);
        else if (o == 2) snprintf(buf, sizeof buf, "%s %02d:00", TX(opts[o]), r_hour());
        else if (o == 3) snprintf(buf, sizeof buf, "%s: %s", TX(opts[o]), R.g.auto_battle ? TR("sì", "on") : TR("no", "off"));
        else snprintf(buf, sizeof buf, "%s", TX(opts[o]));
        lv_obj_t *t = r_text(row, buf, COL_TEXT, NULL);
        lv_obj_align(t, LV_ALIGN_LEFT_MID, 10, 0);
        lv_obj_t *b = kit_text_button(row, TR("Cambia", "Change"), 0, 0, 84, 38, COL_PANEL, on_option, o);
        lv_obj_align(b, LV_ALIGN_RIGHT_MID, -6, 0);
    }
    r_text(list, "Pocket Wayfarer v0.2", COL_DIM, NULL);
    r_back(s, R_TOWN);
}
