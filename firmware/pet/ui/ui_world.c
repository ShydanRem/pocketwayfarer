/* World map, zone sheet (explore / guardian / expeditions) and the Legami radar. */
#include <math.h>
#include <stdio.h>
#include <string.h>
#include "ui_internal.h"

static void on_home(lv_event_t *e) { (void)e; go(WF_SCR_HOME); }
static void on_map(lv_event_t *e) { (void)e; go(WF_SCR_MAP); }

/* ---- Map --------------------------------------------------------------------------- */
static void on_island(lv_event_t *e) {
    int z = (int)(intptr_t)lv_event_get_user_data(e);
    if (z > ui.g.zone_unlocked) {
        char buf[64];
        snprintf(buf, sizeof buf, "Sconfiggi il guardiano di %s", wf_zone_get(z - 1)->name);
        kit_toast(buf, icon(WF_ICON_TROPHY));
        return;
    }
    ui.zone = z;
    go(WF_SCR_ZONE);
}

void scr_map(void) {
    lv_obj_t *s = pet_screen(WF_SCENE_MAP);
    for (int z = 0; z < WF_ZONE_COUNT; z++) {
        int x = wf_map_islands[z][0] * PIX, y = wf_map_islands[z][1] * PIX;
        bool open = z <= ui.g.zone_unlocked;
        lv_obj_t *hit = lv_obj_create(s);
        lv_obj_remove_style_all(hit);
        lv_obj_set_size(hit, 96, 80);
        kit_pos(hit, x, y);
        lv_obj_add_flag(hit, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_event_cb(hit, on_island, LV_EVENT_CLICKED, (void *)(intptr_t)z);
        lv_obj_t *p = kit_pill(s, open ? wf_zone_get(z)->name : "???", x, y + 40);
        if (!open) lv_obj_set_style_opa(p, 170, 0);
        if (wf_pet_away(&ui.g, now_s()) && ui.g.pet.away_zone == z) {
            lv_obj_t *m = kit_img(s, &wf_pet_sprites[ui.g.pet.form][WF_EXPR_HAPPY], 1, x, y - 30);
            kit_bob(m, 4, 800);
        } else if (wf_boss_ready(&ui.g, z)) {
            lv_obj_t *m = kit_img(s, icon(WF_ICON_TROPHY), 2, x, y - 28);
            kit_bob(m, 5, 700);
        } else if (!open) {
            kit_label(s, &wf_font_big, COL_DIM, "?", x, y - 6);
        }
    }
    kit_label(s, &wf_font_big, COL_GOLD, "Isole Sospese", CX, 40);
    kit_round_button(s, icon(WF_ICON_BACK), NULL, CX, 270, 50, COL_PANEL_HI, on_home, 0);
}

/* ---- Zone ---------------------------------------------------------------------------- */
static lv_obj_t *zone_dlg;

static void close_zone_dlg(lv_event_t *e) {
    (void)e;
    kit_dialog_close(zone_dlg);
    zone_dlg = NULL;
}

static void on_explore(lv_event_t *e) {
    (void)e;
    wf_step st;
    int r = wf_explore(&ui.g, ui.zone, now_s(), &st);
    if (r != WF_OK) {
        kit_toast(wf_result_text(r), r == WF_ERR_TIRED ? icon(WF_ICON_BOLT) : NULL);
        return;
    }
    mark_dirty();
    char body[96];
    switch (st.kind) {
    case WF_STEP_BATTLE:
        start_battle(ui.zone, &st);
        return;
    case WF_STEP_ITEM:
        snprintf(body, sizeof body, "Hai trovato: %s!", wf_item_get(st.item)->name);
        zone_dlg = kit_dialog("Trovato!", body, &wf_item_icons[st.item]);
        break;
    case WF_STEP_COINS:
        snprintf(body, sizeof body, "Un mucchietto di monete: +%u", st.coins);
        zone_dlg = kit_dialog("Che fortuna!", body, icon(WF_ICON_COIN));
        break;
    case WF_STEP_SPRING:
        zone_dlg = kit_dialog("Sorgente", "Un sorso d'acqua fresca.\nEnergia e Umore su!", icon(WF_ICON_HEART));
        break;
    default:
        snprintf(body, sizeof body, "Un forziere! +%u monete e %s", st.coins, wf_item_get(st.item)->name);
        zone_dlg = kit_dialog("Forziere!", body, icon(WF_ICON_GIFT));
        break;
    }
    kit_dialog_button(zone_dlg, "Avanti", COL_BTN_OK, close_zone_dlg, 0);
    scr_zone();
}

static void on_boss(lv_event_t *e) {
    (void)e;
    wf_step st;
    int r = wf_boss_step(&ui.g, ui.zone, now_s(), &st);
    if (r != WF_OK) {
        kit_toast(wf_result_text(r), NULL);
        return;
    }
    start_battle(ui.zone, &st);
}

static void on_expedition_pick(lv_event_t *e) {
    int opt = (int)(intptr_t)lv_event_get_user_data(e);
    kit_dialog_close(zone_dlg);
    zone_dlg = NULL;
    if (opt < 0) return;
    int r = wf_expedition_start(&ui.g, ui.zone, opt, now_s());
    if (r != WF_OK) {
        kit_toast(wf_result_text(r), NULL);
        return;
    }
    mark_dirty();
    wf_ui_flush_save();
    go(WF_SCR_HOME);
    kit_toast("Buon viaggio!", icon(WF_ICON_MAP));
}

static void on_expedition(lv_event_t *e) {
    (void)e;
    zone_dlg = kit_dialog("Spedizione", "Parte da solo e torna con PE, monete e oggetti. Quanto a lungo?", icon(WF_ICON_MAP));
    kit_dialog_button(zone_dlg, "30m", COL_BTN, on_expedition_pick, 0);
    kit_dialog_button(zone_dlg, "2h", COL_BTN, on_expedition_pick, 1);
    kit_dialog_button(zone_dlg, "8h", COL_BTN, on_expedition_pick, 2);
}

void scr_zone(void) {
    const wf_zone_info *z = wf_zone_get(ui.zone);
    lv_obj_t *s = pet_screen(WF_SCENE_ARENA_MEADOW + z->scene);
    kit_dim(s, 140);
    kit_label(s, &wf_font_big, COL_GOLD, z->name, CX, 76);
    char buf[96];
    snprintf(buf, sizeof buf, "%s\nLivelli %d-%d", z->blurb, z->min_level, z->max_level);
    lv_obj_t *b = kit_label(s, &wf_font_small, COL_TEXT, buf, CX, 124);
    lv_obj_set_width(b, 330);
    lv_label_set_long_mode(b, LV_LABEL_LONG_WRAP);
    /* Progress toward the guardian. */
    int prog = ui.g.zone_progress[ui.zone];
    for (int i = 0; i < WF_BOSS_AFTER; i++) {
        lv_obj_t *d = lv_obj_create(s);
        lv_obj_remove_style_all(d);
        lv_obj_set_size(d, 18, 18);
        lv_obj_set_style_radius(d, LV_RADIUS_CIRCLE, 0);
        lv_obj_set_style_bg_opa(d, LV_OPA_COVER, 0);
        lv_obj_set_style_bg_color(d, lv_color_hex(i < prog ? COL_GOLD : 0x2a3266), 0);
        lv_obj_set_style_border_color(d, lv_color_hex(COL_INK), 0);
        lv_obj_set_style_border_width(d, 2, 0);
        kit_pos(d, CX - (WF_BOSS_AFTER - 1) * 13 + i * 26, 172);
    }
    const wf_species_info *boss = wf_species_get(z->boss);
    if (wf_boss_ready(&ui.g, ui.zone)) snprintf(buf, sizeof buf, "%s ti aspetta!", boss->name);
    else snprintf(buf, sizeof buf, "Ancora %d vittorie per il guardiano", WF_BOSS_AFTER - prog);
    kit_label(s, &wf_font_small, COL_DIM, buf, CX, 198);

    kit_round_button(s, icon(WF_ICON_MAP), "Esplora", CX - 100, 270, 84, COL_BTN_OK, on_explore, 0);
    lv_obj_t *bb = kit_round_button(s, icon(WF_ICON_TROPHY), "Guardiano", CX, 270, 84, COL_BAD, on_boss, 0);
    if (!wf_boss_ready(&ui.g, ui.zone)) lv_obj_add_state(bb, LV_STATE_DISABLED);
    kit_round_button(s, icon(WF_ICON_RUN), "Spedizione", CX + 100, 270, 84, COL_BTN, on_expedition, 0);
    char st[64];
    snprintf(st, sizeof st, "Energia %d%%  Fame %d%%", wf_need_percent(ui.g.pet.energy), wf_need_percent(ui.g.pet.hunger));
    kit_label(s, &wf_font_small, COL_ENERGY, st, CX, 352);
    kit_round_button(s, icon(WF_ICON_BACK), NULL, CX, 410, 50, COL_PANEL_HI, on_map, 0);
}

/* ---- Legami radar ------------------------------------------------------------------- */
static lv_obj_t *link_scr, *sweep, *peer_btn[WF_PEERS], *peer_lbl[WF_PEERS], *hint_l, *link_dlg;
static uint8_t shown_ids[WF_PEERS][6];
static int shown_count = -1;
static bool invite_open;
static uint32_t last_layout;

static void close_link_dlg(lv_event_t *e) {
    (void)e;
    kit_dialog_close(link_dlg);
    link_dlg = NULL;
}

static void on_cancel_invite(lv_event_t *e) {
    close_link_dlg(e);
    wf_link_duel_leave(&ui.link, lv_tick_get());
}

static void on_gift_item(lv_event_t *e) {
    int item = (int)(intptr_t)lv_event_get_user_data(e);
    kit_dialog_close(link_dlg);
    link_dlg = NULL;
    if (item < 0) return;
    if (!wf_link_gift(&ui.link, &ui.g, ui.link_peer_id, item, lv_tick_get()))
        kit_toast("Un momento, sto ancora inviando", NULL);
    else
        kit_toast("Dono in viaggio...", &wf_item_icons[item]);
}

static void on_peer_action(lv_event_t *e) {
    int a = (int)(intptr_t)lv_event_get_user_data(e);
    kit_dialog_close(link_dlg);
    link_dlg = NULL;
    uint32_t t = lv_tick_get();
    if (a == 0) {
        if (wf_link_greet(&ui.link, &ui.g, ui.link_peer_id, t, now_s())) kit_toast("Saluto inviato!", icon(WF_ICON_HEART));
    } else if (a == 1) {
        /* Offer up to four of the most plentiful items. */
        int picks[4], n = 0;
        uint8_t used[WF_IT_COUNT] = {0};
        while (n < 4) {
            int best = -1;
            for (int i = 0; i < WF_IT_COUNT; i++)
                if (!used[i] && ui.g.inv[i] && (best < 0 || ui.g.inv[i] > ui.g.inv[best])) best = i;
            if (best < 0) break;
            used[best] = 1;
            picks[n++] = best;
        }
        if (!n) {
            kit_toast("Lo zaino è vuoto", icon(WF_ICON_BAG));
            return;
        }
        link_dlg = kit_dialog("Cosa regali?", NULL, icon(WF_ICON_GIFT));
        for (int i = 0; i < n; i++) {
            lv_obj_t *btn = kit_dialog_button(link_dlg, "", COL_PANEL_HI, on_gift_item, picks[i]);
            lv_obj_set_size(btn, 62, 62);
            lv_obj_t *lbl = lv_obj_get_child(btn, 0);
            lv_obj_delete(lbl);
            lv_obj_t *img = lv_image_create(btn);
            lv_image_set_src(img, gfx_sprite(&wf_item_icons[picks[i]], 3));
            lv_obj_center(img);
        }
    } else if (a == 2) {
        if (wf_link_duel_invite(&ui.link, &ui.g, ui.link_peer_id, t, now_s())) {
            link_dlg = kit_dialog("Sfida inviata", "In attesa di risposta...", icon(WF_ICON_SWORD));
            kit_dialog_button(link_dlg, "Annulla", COL_PANEL_HI, on_cancel_invite, 0);
            invite_open = true;
        } else {
            kit_toast(wf_result_text(ui.g.pet.sleeping ? WF_ERR_ASLEEP : WF_ERR_AWAY), NULL);
        }
    }
}

static void on_peer(lv_event_t *e) {
    int i = (int)(intptr_t)lv_event_get_user_data(e);
    const wf_peer *p = wf_link_peer(&ui.link, i);
    if (!p) return;
    memcpy(ui.link_peer_id, p->id, 6);
    char body[96];
    const wf_friend *f = wf_friend_find(&ui.g, p->id);
    snprintf(body, sizeof body, "%s, Lv %d\n%s%s", wf_form_get(p->who.form)->name, p->who.level,
             f ? "Amico" : "Nuovo incontro",
             p->who.flags & WF_PROFILE_BUSY ? "  -  occupato" : p->who.flags & WF_PROFILE_ASLEEP ? "  -  dorme" : "");
    link_dlg = kit_dialog(p->who.name, body, &wf_pet_sprites[p->who.form][WF_EXPR_HAPPY]);
    kit_dialog_button(link_dlg, "Saluta", COL_BTN_ALT, on_peer_action, 0);
    kit_dialog_button(link_dlg, "Dono", COL_GOLD, on_peer_action, 1);
    kit_dialog_button(link_dlg, "Sfida", COL_BAD, on_peer_action, 2);
}

static void layout_peers(void) {
    int n = wf_link_peer_count(&ui.link);
    bool same = n == shown_count;
    for (int i = 0; same && i < n; i++)
        if (memcmp(wf_link_peer(&ui.link, i)->id, shown_ids[i], 6)) same = false;
    if (same) return;
    shown_count = n;
    for (int i = 0; i < WF_PEERS; i++) {
        if (peer_btn[i]) { lv_obj_delete(peer_btn[i]); peer_btn[i] = NULL; }
        if (peer_lbl[i]) { lv_obj_delete(peer_lbl[i]); peer_lbl[i] = NULL; }
    }
    for (int i = 0; i < n; i++) {
        const wf_peer *p = wf_link_peer(&ui.link, i);
        memcpy(shown_ids[i], p->id, 6);
        float a = (30.0f + 360.0f / (float)(n < 3 ? 3 : n) * (float)i) * 3.14159265f / 180.0f;
        int x = CX + (int)lroundf(150 * cosf(a)), y = CY + (int)lroundf(150 * sinf(a));
        peer_btn[i] = kit_round_button(link_scr, NULL, NULL, x, y, 78, wf_friend_find(&ui.g, p->id) ? COL_BTN_ALT : COL_BTN,
                                       on_peer, i);
        lv_obj_t *img = lv_image_create(peer_btn[i]);
        lv_image_set_src(img, gfx_sprite(&wf_pet_sprites[p->who.form][WF_EXPR_IDLE], 1));
        lv_obj_center(img);
        peer_lbl[i] = kit_label(link_scr, &wf_font_small, COL_TEXT, p->who.name, x, y + 52);
        kit_fade_in(peer_btn[i], 300, 0);
    }
    if (n) lv_obj_add_flag(hint_l, LV_OBJ_FLAG_HIDDEN);
    else lv_obj_remove_flag(hint_l, LV_OBJ_FLAG_HIDDEN);
}

void scr_link(void) {
    link_scr = pet_screen(-1);
    lv_obj_set_style_bg_color(link_scr, lv_color_hex(0x0a1a24), 0);
    memset(peer_btn, 0, sizeof peer_btn);
    memset(peer_lbl, 0, sizeof peer_lbl);
    link_dlg = NULL;
    invite_open = false;
    shown_count = -1;
    for (int r = 70; r <= 230; r += 52) {
        lv_obj_t *c = lv_obj_create(link_scr);
        lv_obj_remove_style_all(c);
        lv_obj_set_size(c, r * 2, r * 2);
        kit_pos(c, CX, CY);
        lv_obj_set_style_radius(c, LV_RADIUS_CIRCLE, 0);
        lv_obj_set_style_border_color(c, lv_color_hex(0x1e5a5a), 0);
        lv_obj_set_style_border_width(c, 2, 0);
        lv_obj_remove_flag(c, LV_OBJ_FLAG_CLICKABLE);
    }
    sweep = lv_arc_create(link_scr);
    lv_obj_remove_style_all(sweep);
    lv_obj_set_size(sweep, 440, 440);
    kit_pos(sweep, CX, CY);
    lv_arc_set_bg_angles(sweep, 0, 40);
    lv_obj_set_style_arc_width(sweep, 150, LV_PART_MAIN);
    lv_obj_set_style_arc_color(sweep, lv_color_hex(0x3ae8c8), LV_PART_MAIN);
    lv_obj_set_style_arc_opa(sweep, 40, LV_PART_MAIN);
    lv_obj_set_style_opa(sweep, 0, LV_PART_INDICATOR);
    lv_obj_remove_flag(sweep, LV_OBJ_FLAG_CLICKABLE);
    kit_img(link_scr, &wf_pet_sprites[ui.g.pet.form][WF_EXPR_HAPPY], 2, CX, CY - 6);
    kit_label(link_scr, &wf_font_big, 0x7af8e0, "Legami", CX, 62);
    hint_l = kit_label(link_scr, &wf_font_small, COL_DIM, "Cerco altri Wayfarer vicini...\nAprite Legami su entrambi.", CX, 330);
    int friends = 0;
    for (int i = 0; i < WF_FRIENDS; i++) friends += ui.g.friends[i].used;
    char buf[32];
    snprintf(buf, sizeof buf, "Amici: %d", friends);
    kit_label(link_scr, &wf_font_small, COL_MOOD, buf, CX, 96);
    kit_round_button(link_scr, icon(WF_ICON_BACK), NULL, CX, 420, 50, COL_PANEL_HI, on_home, 0);
    last_layout = 0;
    layout_peers();
}

void link_tick(void) {
    uint32_t t = lv_tick_get();
    lv_arc_set_rotation(sweep, (int32_t)((t / 8) % 360));
    if (t - last_layout > 500) {
        last_layout = t;
        layout_peers();
        /* A pending invite dialog is closed once the invitation ends either way. */
        if (link_dlg && lv_obj_is_valid(link_dlg) && ui.link.duel.state == WF_DUEL_IDLE &&
            !ui.link.out.active && invite_open) {
            close_link_dlg(NULL);
            invite_open = false;
        }
    }
}
