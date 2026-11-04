/* Radial menu, feeding, bag & shop, diary. */
#include <math.h>
#include <stdio.h>
#include <string.h>
#include "ui_internal.h"

static int home_scene(void) {
    int hour = ui_hour();
    return hour >= 7 && hour < 18 ? WF_SCENE_HOME_DAY : hour >= 18 && hour < 21 ? WF_SCENE_HOME_DUSK
         : WF_SCENE_HOME_NIGHT;
}

static void on_home(lv_event_t *e) { (void)e; go(WF_SCR_HOME); }

/* ---- Radial menu -------------------------------------------------------------- */
enum { M_FEED, M_PLAY, M_TRAIN, M_MAP, M_LINK, M_BAG, M_DIARY, M_SLEEP, M_COUNT };

static void on_menu_item(lv_event_t *e) {
    int item = (int)(intptr_t)lv_event_get_user_data(e);
    uint32_t now = now_s();
    int gate = wf_pet_away(&ui.g, now) ? WF_ERR_AWAY : ui.g.pet.sleeping ? WF_ERR_ASLEEP : WF_OK;
    switch (item) {
    case M_FEED: if (gate) { kit_toast(wf_result_text(gate), NULL); return; } go(WF_SCR_FEED); break;
    case M_PLAY: if (gate) { kit_toast(wf_result_text(gate), NULL); return; } go(WF_SCR_PLAY); break;
    case M_TRAIN:
        if (gate) { kit_toast(wf_result_text(gate), NULL); return; }
        if (ui.g.pet.energy < 15000) { kit_toast(wf_result_text(WF_ERR_TIRED), icon(WF_ICON_BOLT)); return; }
        train_reset();
        go(WF_SCR_TRAIN);
        break;
    case M_MAP: go(WF_SCR_MAP); break;
    case M_LINK: go(WF_SCR_LINK); break;
    case M_BAG: go(WF_SCR_BAG); break;
    case M_DIARY: go(WF_SCR_DIARY); break;
    case M_SLEEP: {
        int r = wf_sleep_toggle(&ui.g, now);
        if (r != WF_OK) { kit_toast(wf_result_text(r), NULL); return; }
        mark_dirty();
        go(WF_SCR_HOME);
        kit_toast(ui.g.pet.sleeping ? "Buonanotte..." : "Sveglia!", icon(ui.g.pet.sleeping ? WF_ICON_MOON : WF_ICON_SUN));
        break;
    }
    }
}

void scr_menu(void) {
    lv_obj_t *s = pet_screen(home_scene());
    lv_obj_t *dim = kit_dim(s, 165);
    lv_obj_add_event_cb(dim, on_home, LV_EVENT_CLICKED, NULL);
    static const char *const names[M_COUNT] = {"Nutri", "Gioca", "Allena", "Esplora", "Legami", "Zaino", "Diario", NULL};
    static const int icons_[M_COUNT] = {WF_ICON_FOOD, WF_ICON_BALL, WF_ICON_DUMBBELL, WF_ICON_MAP, WF_ICON_LINK,
                                        WF_ICON_BAG, WF_ICON_SCROLL, WF_ICON_MOON};
    static const uint32_t colors[M_COUNT] = {0xe8803a, 0xe8508a, 0x8a5ae8, 0x3aba6a, 0x3aa8e8, 0xc8803a, 0x8a7a5a, 0x3a4ab8};
    for (int i = 0; i < M_COUNT; i++) {
        float a = (-90.0f + 45.0f * (float)i) * 3.14159265f / 180.0f;
        int x = CX + (int)lroundf(150.0f * cosf(a)), y = CY + (int)lroundf(150.0f * sinf(a)) - 8;
        const char *name = names[i];
        int ic = icons_[i];
        if (i == M_SLEEP) {
            name = ui.g.pet.sleeping ? "Sveglia" : "Dormi";
            ic = ui.g.pet.sleeping ? WF_ICON_SUN : WF_ICON_MOON;
        }
        lv_obj_t *b = kit_round_button(s, icon(ic), name, x, y, 70, colors[i], on_menu_item, i);
        kit_fade_in(b, 180, i * 25);
    }
    lv_obj_t *p = kit_img(s, &wf_pet_sprites[ui.g.pet.form][ui.g.pet.sleeping ? WF_EXPR_SLEEP : WF_EXPR_HAPPY], 2, CX, CY - 8);
    lv_obj_add_flag(p, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(p, on_home, LV_EVENT_CLICKED, NULL);
    char buf[32];
    snprintf(buf, sizeof buf, "%lu", (unsigned long)ui.g.coins);
    lv_obj_t *coin = kit_img(s, icon(WF_ICON_COIN), 2, CX - 26, CY + 58);
    (void)coin;
    kit_label(s, &wf_font_small, COL_GOLD, buf, CX + 14, CY + 58);
}

/* ---- Feeding ------------------------------------------------------------------ */
static int foods[WF_IT_COUNT], food_count;
static lv_obj_t *feed_icon, *feed_name, *feed_info, *feed_btn;

static void feed_show(void) {
    int item = foods[ui.feed_index];
    const wf_item_info *it = wf_item_get(item);
    kit_img_set(feed_icon, &wf_item_icons[item], 6);
    lv_label_set_text(feed_name, it->name);
    char buf[96];
    int n = snprintf(buf, sizeof buf, "Ne hai: %d\n", ui.g.inv[item]);
    if (it->hunger) n += snprintf(buf + n, sizeof buf - (size_t)n, "Fame +%ld  ", (long)(it->hunger / 1000));
    if (it->energy) n += snprintf(buf + n, sizeof buf - (size_t)n, "Energia +%ld  ", (long)(it->energy / 1000));
    if (it->mood) n += snprintf(buf + n, sizeof buf - (size_t)n, "Umore +%ld", (long)(it->mood / 1000));
    if (it->element) snprintf(buf + n, sizeof buf - (size_t)n, "\nAffinità %s", wf_element_name(it->element));
    lv_label_set_text(feed_info, buf);
    if (ui.g.inv[item]) lv_obj_remove_state(feed_btn, LV_STATE_DISABLED);
    else lv_obj_add_state(feed_btn, LV_STATE_DISABLED);
}

static void on_feed_step(lv_event_t *e) {
    int d = (int)(intptr_t)lv_event_get_user_data(e);
    ui.feed_index = (ui.feed_index + d + food_count) % food_count;
    feed_show();
}

static void on_feed(lv_event_t *e) {
    (void)e;
    int item = foods[ui.feed_index];
    int r = wf_feed(&ui.g, item, now_s());
    if (r != WF_OK) {
        kit_toast(wf_result_text(r), &wf_item_icons[item]);
        return;
    }
    mark_dirty();
    go_home_anim(ANIM_EAT);
    char buf[48];
    snprintf(buf, sizeof buf, "Gnam! %s", wf_item_get(item)->name);
    kit_toast(buf, &wf_item_icons[item]);
}

static void on_shop(lv_event_t *e) { (void)e; go(WF_SCR_SHOP); }

void scr_feed(void) {
    lv_obj_t *s = pet_screen(home_scene());
    kit_dim(s, 150);
    food_count = 0;
    for (int i = 0; i < WF_IT_COUNT; i++)
        if (wf_item_get(i)->kind == WF_KIND_FOOD) foods[food_count++] = i;
    /* Start from something we actually own. */
    if (ui.feed_index >= food_count || !ui.g.inv[foods[ui.feed_index]]) {
        ui.feed_index = 0;
        for (int i = 0; i < food_count; i++) if (ui.g.inv[foods[i]]) { ui.feed_index = i; break; }
    }
    kit_label(s, &wf_font_big, COL_GOLD, "Cosa mangiamo?", CX, 70);
    lv_obj_t *halo = kit_panel(s, CX, 172, 132, 132);
    lv_obj_set_style_radius(halo, LV_RADIUS_CIRCLE, 0);
    feed_icon = kit_img(s, &wf_item_icons[0], 6, CX, 172);
    feed_name = kit_label(s, &wf_font_big, COL_TEXT, "", CX, 262);
    feed_info = kit_label(s, &wf_font_small, COL_DIM, "", CX, 304);
    kit_round_button(s, icon(WF_ICON_BACK), NULL, 70, 172, 60, COL_PANEL_HI, on_feed_step, -1);
    lv_obj_t *next = kit_round_button(s, icon(WF_ICON_BACK), NULL, 396, 172, 60, COL_PANEL_HI, on_feed_step, 1);
    lv_obj_t *arrow = lv_obj_get_child(next, 0);
    lv_image_set_rotation(arrow, 1800);
    feed_btn = kit_text_button(s, "Dai da mangiare", CX - 36, 362, 196, 52, COL_BTN_OK, on_feed, 0);
    kit_round_button(s, icon(WF_ICON_COIN), NULL, CX + 102, 362, 52, COL_GOLD, on_shop, 0);
    kit_round_button(s, icon(WF_ICON_BACK), NULL, CX, 428, 50, COL_PANEL_HI, on_home, 0);
    feed_show();
}

/* ---- Bag & shop ------------------------------------------------------------------ */
static lv_obj_t *bag_list, *coin_label;

static void bag_fill(void);

static void on_tab(lv_event_t *e) {
    ui.bag_tab = (int)(intptr_t)lv_event_get_user_data(e);
    ui.screen = ui.bag_tab ? WF_SCR_SHOP : WF_SCR_BAG;
    scr_bag();
}

static void on_buy(lv_event_t *e) {
    int item = (int)(intptr_t)lv_event_get_user_data(e);
    int r = wf_buy(&ui.g, item);
    if (r != WF_OK) {
        kit_toast(wf_result_text(r), icon(WF_ICON_COIN));
        return;
    }
    mark_dirty();
    char buf[48];
    snprintf(buf, sizeof buf, "Comprato: %s", wf_item_get(item)->name);
    kit_toast(buf, &wf_item_icons[item]);
    bag_fill();
}

static void on_sell(lv_event_t *e) {
    (void)e;
    if (!ui.g.inv[WF_IT_SHARD]) return;
    ui.g.inv[WF_IT_SHARD]--;
    ui.g.coins += 40;
    mark_dirty();
    kit_toast("Venduto per 40 monete", icon(WF_ICON_COIN));
    bag_fill();
}

static void bag_row(int item, bool shop) {
    const wf_item_info *it = wf_item_get(item);
    lv_obj_t *row = lv_obj_create(bag_list);
    lv_obj_remove_style_all(row);
    lv_obj_set_size(row, 290, 58);
    lv_obj_set_style_bg_color(row, lv_color_hex(COL_PANEL_HI), 0);
    lv_obj_set_style_bg_opa(row, 200, 0);
    lv_obj_set_style_radius(row, 12, 0);
    lv_obj_remove_flag(row, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_t *img = lv_image_create(row);
    lv_image_set_src(img, gfx_sprite(&wf_item_icons[item], 3));
    lv_obj_align(img, LV_ALIGN_LEFT_MID, 6, 0);
    lv_obj_t *l = lv_label_create(row);
    lv_obj_set_style_text_font(l, &wf_font_small, 0);
    lv_obj_set_style_text_color(l, lv_color_hex(COL_TEXT), 0);
    char buf[64];
    if (shop) snprintf(buf, sizeof buf, "%s\n%d monete  (hai %d)", it->name, it->price, ui.g.inv[item]);
    else snprintf(buf, sizeof buf, "%s  x%d\n%s", it->name, ui.g.inv[item], it->blurb);
    lv_label_set_text(l, buf);
    lv_obj_set_width(l, shop ? 160 : 220);
    lv_label_set_long_mode(l, LV_LABEL_LONG_DOT);
    lv_obj_align(l, LV_ALIGN_LEFT_MID, 60, 0);
    if (shop) {
        lv_obj_t *b = kit_text_button(row, "Compra", 0, 0, 72, 40, ui.g.coins >= it->price ? COL_BTN_OK : 0x4a4a5a,
                                      on_buy, item);
        lv_obj_align(b, LV_ALIGN_RIGHT_MID, -6, 0);
    } else if (item == WF_IT_SHARD) {
        lv_obj_set_width(l, 150);
        lv_obj_t *b = kit_text_button(row, "Vendi", 0, 0, 72, 40, COL_GOLD, on_sell, item);
        lv_obj_align(b, LV_ALIGN_RIGHT_MID, -6, 0);
    }
}

static void bag_fill(void) {
    lv_obj_clean(bag_list);
    char buf[24];
    snprintf(buf, sizeof buf, "%lu", (unsigned long)ui.g.coins);
    lv_label_set_text(coin_label, buf);
    int shown = 0;
    for (int i = 0; i < WF_IT_COUNT; i++) {
        if (ui.bag_tab) {
            if (wf_item_get(i)->price) { bag_row(i, true); shown++; }
        } else if (ui.g.inv[i]) {
            bag_row(i, false);
            shown++;
        }
    }
    if (!shown) {
        lv_obj_t *l = lv_label_create(bag_list);
        lv_obj_set_style_text_font(l, &wf_font_small, 0);
        lv_obj_set_style_text_color(l, lv_color_hex(COL_DIM), 0);
        lv_label_set_text(l, "Lo zaino è vuoto.\nPassa dalla Bottega!");
    }
}

void scr_bag(void) {
    lv_obj_t *s = pet_screen(-1);
    lv_obj_set_style_bg_color(s, lv_color_hex(0x141830), 0);
    kit_text_button(s, "Zaino", CX - 66, 62, 120, 44, ui.bag_tab ? COL_PANEL_HI : COL_BTN, on_tab, 0);
    kit_text_button(s, "Bottega", CX + 66, 62, 120, 44, ui.bag_tab ? COL_BTN : COL_PANEL_HI, on_tab, 1);
    kit_img(s, icon(WF_ICON_COIN), 2, CX - 22, 106);
    coin_label = kit_label(s, &wf_font_small, COL_GOLD, "", CX + 14, 106);
    bag_list = lv_obj_create(s);
    lv_obj_remove_style_all(bag_list);
    lv_obj_set_size(bag_list, 310, 270);
    kit_pos(bag_list, CX, 262);
    lv_obj_set_flex_flow(bag_list, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(bag_list, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_row(bag_list, 8, 0);
    lv_obj_set_scroll_dir(bag_list, LV_DIR_VER);
    lv_obj_set_scrollbar_mode(bag_list, LV_SCROLLBAR_MODE_OFF);
    bag_fill();
    kit_round_button(s, icon(WF_ICON_BACK), NULL, CX, 428, 50, COL_PANEL_HI, on_home, 0);
}

/* ---- Diary ------------------------------------------------------------------------ */
static void on_claim(lv_event_t *e) {
    int i = (int)(intptr_t)lv_event_get_user_data(e);
    int coins = wf_quest_claim(&ui.g, i);
    if (!coins) return;
    char buf[48];
    snprintf(buf, sizeof buf, "+%d monete e %s", coins, wf_item_get(ui.g.quests[i].item)->name);
    kit_toast(buf, icon(WF_ICON_TROPHY));
    mark_dirty();
    scr_diary();
}

static void on_bright(lv_event_t *e) {
    int d = (int)(intptr_t)lv_event_get_user_data(e);
    int b = ui.g.brightness + d;
    ui.g.brightness = (uint8_t)(b < 20 ? 20 : b > 100 ? 100 : b);
    wf_platform_brightness(ui.g.brightness);
    mark_dirty();
    scr_diary();
}

static void on_hour(lv_event_t *e) {
    int d = (int)(intptr_t)lv_event_get_user_data(e);
    ui.g.hour_offset = (uint8_t)((ui.g.hour_offset + d) % 24);
    mark_dirty();
    scr_diary();
}

static lv_obj_t *section(lv_obj_t *list, const char *title) {
    lv_obj_t *l = lv_label_create(list);
    lv_obj_set_style_text_font(l, &wf_font_big, 0);
    lv_obj_set_style_text_color(l, lv_color_hex(COL_GOLD), 0);
    lv_label_set_text(l, title);
    return l;
}

static lv_obj_t *text(lv_obj_t *list, const char *t, uint32_t color) {
    lv_obj_t *l = lv_label_create(list);
    lv_obj_set_style_text_font(l, &wf_font_small, 0);
    lv_obj_set_style_text_color(l, lv_color_hex(color), 0);
    lv_obj_set_style_text_align(l, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_width(l, 290);
    lv_label_set_long_mode(l, LV_LABEL_LONG_WRAP);
    lv_label_set_text(l, t);
    return l;
}

static int32_t diary_scroll;

void scr_diary(void) {
    lv_obj_t *old_list = NULL;
    if (ui.screen == WF_SCR_DIARY && lv_screen_active()) {
        old_list = lv_obj_get_child(lv_screen_active(), 0);
        if (old_list) diary_scroll = lv_obj_get_scroll_y(old_list);
    }
    lv_obj_t *s = pet_screen(-1);
    lv_obj_set_style_bg_color(s, lv_color_hex(0x1a1630), 0);
    lv_obj_t *list = lv_obj_create(s);
    lv_obj_remove_style_all(list);
    lv_obj_set_size(list, 330, 400);
    kit_pos(list, CX, 222);
    lv_obj_set_flex_flow(list, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(list, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_row(list, 8, 0);
    lv_obj_set_style_pad_top(list, 40, 0);
    lv_obj_set_style_pad_bottom(list, 60, 0);
    lv_obj_set_scrollbar_mode(list, LV_SCROLLBAR_MODE_OFF);

    const wf_game *g = &ui.g;
    const wf_form_info *f = wf_form_get(g->pet.form);
    lv_obj_t *img = lv_image_create(list);
    lv_image_set_src(img, gfx_sprite(&wf_pet_sprites[g->pet.form][WF_EXPR_HAPPY], 2));
    char buf[160];
    snprintf(buf, sizeof buf, "%s", g->pet.name);
    section(list, buf);
    snprintf(buf, sizeof buf, "%s  -  %s  -  Lv %d\n%s", f->name, wf_element_name(f->element), g->pet.level, f->blurb);
    text(list, buf, COL_TEXT);
    wf_stats st = wf_pet_stats(g);
    snprintf(buf, sizeof buf, "PV %d   ATT %d   DIF %d   VEL %d", st.hp, st.atk, st.def, st.spd);
    text(list, buf, COL_ENERGY);
    int hearts = (int)(g->pet.bond / 20000);
    char h[16] = "";
    for (int i = 0; i < 5; i++) strcat(h, i < hearts ? "\xe2\x99\xa5" : "\xc2\xb7");
    snprintf(buf, sizeof buf, "Legame  %s\nVittorie %u  Sconfitte %u  Duelli %u/%u\nGiorni di fila: %u",
             h, g->wins, g->losses, g->duel_wins, g->duel_wins + g->duel_losses, g->streak);
    text(list, buf, COL_MOOD);
    int target = wf_evolution_target(g);
    if (wf_pet_stage(g) == WF_STAGE_BABY) text(list, "Al livello 5 cambierà forma: il cibo che mangia ne decide l'elemento.", COL_DIM);
    else if (wf_pet_stage(g) == WF_STAGE_TEEN) text(list, "Al livello 12 diventerà adulto: un legame forte e poche trascuratezze fanno la differenza.", COL_DIM);
    (void)target;

    section(list, "Missioni di oggi");
    for (int i = 0; i < WF_QUESTS; i++) {
        const wf_quest *q = &g->quests[i];
        char qt[48];
        wf_quest_text(q, qt, sizeof qt);
        lv_obj_t *row = lv_obj_create(list);
        lv_obj_remove_style_all(row);
        lv_obj_set_size(row, 300, 64);
        lv_obj_set_style_bg_color(row, lv_color_hex(COL_PANEL_HI), 0);
        lv_obj_set_style_bg_opa(row, 200, 0);
        lv_obj_set_style_radius(row, 12, 0);
        lv_obj_remove_flag(row, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_t *l = lv_label_create(row);
        lv_obj_set_style_text_font(l, &wf_font_small, 0);
        lv_obj_set_style_text_color(l, lv_color_hex(q->claimed ? COL_DIM : COL_TEXT), 0);
        snprintf(buf, sizeof buf, "%s\n%d/%d  -  %d monete", qt, q->progress, q->goal, q->coins);
        lv_label_set_text(l, buf);
        lv_obj_align(l, LV_ALIGN_LEFT_MID, 10, 0);
        if (q->claimed) continue;
        if (q->progress >= q->goal) {
            lv_obj_t *b = kit_text_button(row, "Ritira", 0, 0, 74, 40, COL_GOLD, on_claim, i);
            lv_obj_align(b, LV_ALIGN_RIGHT_MID, -6, 0);
        }
    }
    section(list, "Opzioni");
    snprintf(buf, sizeof buf, "Luminosità %d%%", g->brightness);
    text(list, buf, COL_TEXT);
    lv_obj_t *row = lv_obj_create(list);
    lv_obj_remove_style_all(row);
    lv_obj_set_size(row, 200, 56);
    lv_obj_t *minus = kit_text_button(row, "-", 0, 0, 80, 48, COL_PANEL_HI, on_bright, -10);
    lv_obj_align(minus, LV_ALIGN_LEFT_MID, 0, 0);
    lv_obj_t *plus = kit_text_button(row, "+", 0, 0, 80, 48, COL_PANEL_HI, on_bright, 10);
    lv_obj_align(plus, LV_ALIGN_RIGHT_MID, 0, 0);
    snprintf(buf, sizeof buf, "Ora del giorno: %02d:00", ui_hour());
    text(list, buf, COL_TEXT);
    lv_obj_t *row2 = lv_obj_create(list);
    lv_obj_remove_style_all(row2);
    lv_obj_set_size(row2, 200, 56);
    lv_obj_t *hm = kit_text_button(row2, "-1h", 0, 0, 80, 48, COL_PANEL_HI, on_hour, 23);
    lv_obj_align(hm, LV_ALIGN_LEFT_MID, 0, 0);
    lv_obj_t *hp = kit_text_button(row2, "+1h", 0, 0, 80, 48, COL_PANEL_HI, on_hour, 1);
    lv_obj_align(hp, LV_ALIGN_RIGHT_MID, 0, 0);
    text(list, "Pocket Wayfarer v0.1", COL_DIM);
    if (old_list) lv_obj_scroll_to_y(list, diary_scroll, LV_ANIM_OFF);
    else diary_scroll = 0;
    kit_round_button(s, icon(WF_ICON_BACK), NULL, CX, 428, 50, COL_PANEL_HI, on_home, 0);
}
