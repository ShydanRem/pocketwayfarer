/* Equipment, bag, shop and forge. */
#include <stdio.h>
#include <string.h>
#include "rui.h"

static lv_obj_t *dlg;
static lv_obj_t *bag_list, *shop_list;

bool r_items_close_dialog(void) {
    if (!dlg) return false;
    kit_dialog_close(dlg);
    dlg = NULL;
    return true;
}

static void close_dlg(lv_event_t *e) { (void)e; r_items_close_dialog(); }
static void dialog_deleted(lv_event_t *e) {
    if (lv_event_get_target(e) == dlg) dlg = NULL;
}

static void item_dialog(const rpg_item *it, const char *extra) {
    r_item_dialog(it, extra, &dlg);
    r_character_dialog_style(dlg);
    lv_obj_t *title = lv_obj_get_child(lv_obj_get_child(dlg, 0), 1);
    lv_obj_set_style_text_color(title, lv_color_hex(rarity_color(it->rarity)), 0);
    lv_obj_add_event_cb(dlg, dialog_deleted, LV_EVENT_DELETE, NULL);
}

static void item_action(const char *label, bool accent, lv_event_cb_t cb, intptr_t value) {
    r_character_button_style(kit_dialog_button(dlg, label, COL_PANEL, cb, value), accent);
}

static void on_go(lv_event_t *e) { r_go((int)(intptr_t)lv_event_get_user_data(e)); }

static void refresh_list(int screen, lv_obj_t **list) {
    int y = lv_obj_get_scroll_y(*list);
    r_go(screen);
    lv_obj_update_layout(*list);
    lv_obj_scroll_to_y(*list, y, LV_ANIM_OFF);
}

static void row_icon(lv_obj_t *row, const wf_sprite *icon) {
    lv_obj_t *image = kit_img(row, icon, 2, 0, 0);
    lv_obj_align(image, LV_ALIGN_LEFT_MID, 9, 0);
}

static void detail_row(lv_obj_t *row, lv_event_cb_t cb, int index) {
    lv_obj_set_ext_click_area(row, 0);
    lv_obj_add_flag(row, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(row, cb, LV_EVENT_CLICKED, (void *)(intptr_t)index);
    lv_obj_set_style_border_color(row, lv_color_hex(RC_GOLD), LV_STATE_PRESSED);
    lv_obj_t *arrow = r_text(row, ">", RC_GOLD, &ln_font_body);
    lv_obj_align(arrow, LV_ALIGN_RIGHT_MID, -13, 0);
}

static lv_obj_t *item_button(lv_obj_t *parent, const rpg_item *it, int x, int y, int size, lv_event_cb_t cb, intptr_t user) {
    lv_obj_t *b = kit_round_button(parent, NULL, NULL, x, y, size, COL_PANEL_HI, cb, user);
    lv_obj_set_style_radius(b, 14, 0);
    if (!rpg_item_empty(it)) {
        lv_obj_set_style_border_color(b, lv_color_hex(rarity_color(it->rarity)), 0);
        lv_obj_t *img = lv_image_create(b);
        lv_image_set_src(img, gfx_sprite(item_icon(it), size >= 60 ? 3 : 2));
        lv_obj_center(img);
        if (it->plus) {
            char p[6];
            snprintf(p, sizeof p, "+%d", it->plus);
            lv_obj_t *l = r_text(b, p, COL_GOLD, NULL);
            lv_obj_align(l, LV_ALIGN_BOTTOM_RIGHT, 0, 2);
        }
    }
    return b;
}

/* ---- Equipment (paper doll) ---------------------------------------------------------- */
static void on_unequip(lv_event_t *e) {
    int slot = (int)(intptr_t)lv_event_get_user_data(e);
    r_items_close_dialog();
    int r = rpg_unequip(&R.g, slot);
    if (r == RPG_OK) r_dirty();
    r_go(R_EQUIP);
    if (r != RPG_OK) kit_toast(rpg_err(r), NULL);
}

static void on_slot(lv_event_t *e) {
    int slot = (int)(intptr_t)lv_event_get_user_data(e);
    const rpg_item *it = &R.g.hero.equip[slot];
    if (rpg_item_empty(it)) {
        R.bag_tab = 0;
        r_go(R_BAG);
        kit_toast(TR("Scegli dallo zaino", "Pick from the bag"), ri(RI_BAG));
        return;
    }
    item_dialog(it, NULL);
    item_action(TR("Togli", "Remove"), false, on_unequip, slot);
    item_action(TR("Chiudi", "Close"), true, close_dlg, 0);
}

void scr_equip(void) {
    static const wf_text captions[SLOT_COUNT] = {
        {"Arma", "Weapon"}, {"Secondaria", "Off-hand"}, {"Testa", "Head"},
        {"Corpo", "Body"}, {"Piedi", "Feet"}, {"Accessorio 1", "Accessory 1"}, {"Accessorio 2", "Accessory 2"},
    };
    static const int pos[SLOT_COUNT][2] = {
        {96, 230}, {370, 230}, {CX, 119}, {147, 318}, {319, 318}, {113, 138}, {353, 138},
    };
    lv_obj_t *s = r_character_screen();
    kit_label(s, &ln_font_title, RC_GOLD, TR("Equipaggiamento", "Equipment"), CX, 51);
    hero_look l;
    hero_look_from_game(&l, &R.g);
    fx_shadow(s, CX, 302, 72);
    hero_obj(s, &l, HX_IDLE, 3, CX, 221);
    for (int i = 0; i < SLOT_COUNT; i++) {
        const rpg_item *item = &R.g.hero.equip[i];
        bool empty = rpg_item_empty(item);
        int x = pos[i][0], y = pos[i][1];
        lv_obj_t *slot = item_button(s, item, x, y, 56, on_slot, i);
        r_character_button_style(slot, false);
        if (empty) {
            lv_obj_t *plus = r_text(slot, "+", RC_MUTED, &ln_font_title);
            lv_obj_center(plus);
        } else {
            lv_obj_set_style_border_color(slot, lv_color_hex(rarity_color(item->rarity)), 0);
        }
        kit_label(s, &ln_font_body, RC_IVORY, TX(captions[i]), x, i == SLOT_HEAD ? y - 40 : y + 41);
    }
    rpg_stats st;
    rpg_stats_get(&R.g, &st);
    char buf[64];
    snprintf(buf, sizeof buf, "ATK %ld · MATK %ld · %s %ld", (long)st.atk, (long)st.matk, TR("DIF", "DEF"), (long)st.def);
    r_character_panel(s, CX, 387, 288, 29);
    kit_label(s, &ln_font_body, RC_MUTED, buf, CX, 387);
    r_character_button(s, TR("Indietro", "Back"), CX - 51, 426, 96, 42, false, on_go, R_TOWN);
    r_character_button(s, TR("Zaino", "Bag"), CX + 51, 426, 96, 42, true, on_go, R_BAG);
}

/* ---- Bag -------------------------------------------------------------------------------- */
static int picked;

static void on_equip_item(lv_event_t *e) {
    (void)e;
    r_items_close_dialog();
    int r = rpg_equip(&R.g, picked);
    if (r == RPG_OK) r_dirty();
    refresh_list(R_BAG, &bag_list);
    kit_toast(r == RPG_OK ? TR("Equipaggiato!", "Equipped!") : rpg_err(r), NULL);
}

static void on_sell_item(lv_event_t *e) {
    (void)e;
    r_items_close_dialog();
    uint32_t v = rpg_sell(&R.g, picked);
    char buf[40];
    snprintf(buf, sizeof buf, TR("Venduto: +%lu oro", "Sold: +%lu gold"), (unsigned long)v);
    r_dirty();
    if (R.screen == R_SHOP) refresh_list(R_SHOP, &shop_list);
    else refresh_list(R_BAG, &bag_list);
    kit_toast(buf, ri(RI_COIN));
}

static void on_sell_row(lv_event_t *e) {
    picked = (int)(intptr_t)lv_event_get_user_data(e);
    on_sell_item(e);
}

static void on_socket(lv_event_t *e) {
    int sp = (int)(intptr_t)lv_event_get_user_data(e);
    r_items_close_dialog();
    int r = rpg_socket(&R.g, &R.g.bag[picked], sp);
    if (r == RPG_OK) r_dirty();
    refresh_list(R_BAG, &bag_list);
    kit_toast(r == RPG_OK ? TR("Gemma incastonata!", "Gem socketed!") : rpg_err(r), ri(RI_GEM));
}

static void on_gems(lv_event_t *e) {
    (void)e;
    r_items_close_dialog();
    dlg = r_character_dialog(TR("Gemme d’anima", "Soul gems"), TR("Scegli la gemma da incastonare", "Pick a gem to socket"), ri(RI_GEM));
    lv_obj_add_event_cb(dlg, dialog_deleted, LV_EVENT_DELETE, NULL);
    for (int sp = 0; sp < SP_COUNT; sp++) {
        if (!R.g.gems[sp]) continue;
        char b[64];
        snprintf(b, sizeof b, "%s x%d", TX(rpg_species(sp)->name), R.g.gems[sp]);
        item_action(b, false, on_socket, sp);
    }
    item_action(TR("Chiudi", "Close"), true, close_dlg, 0);
}

static void on_bag_item(lv_event_t *e) {
    picked = (int)(intptr_t)lv_event_get_user_data(e);
    rpg_item *it = &R.g.bag[picked];
    char extra[64] = "";
    const rpg_item *cur = &R.g.hero.equip[rpg_item_type_get(it->type)->slot];
    bool can_equip = rpg_can_equip(&R.g, it);
    if ((it->type == IT_DAGGER || cur->type == IT_DAGGER) && can_equip) {
        rpg_stats current, projected;
        rpg_stats_get(&R.g, &current);
        rpg_stats_with_weapon(&R.g, it, &projected);
        snprintf(extra, sizeof extra, TR("\nAttacco eroe %+d", "Hero ATK %+d"), projected.atk - current.atk);
    } else if (!rpg_item_empty(cur)) {
        int d = (rpg_item_atk(it) + rpg_item_matk(it) + rpg_item_def(it)) - (rpg_item_atk(cur) + rpg_item_matk(cur) + rpg_item_def(cur));
        snprintf(extra, sizeof extra, TR("\nRispetto all'attuale: %+d", "\nCompared to current: %+d"), d);
    }
    if (!can_equip)
        strncat(extra, it->level > R.g.hero.level ? TR("\nLivello troppo alto", "\nLevel too high") : TR("\nNon per la tua classe", "\nNot for your class"),
                sizeof extra - strlen(extra) - 1);
    item_dialog(it, extra);
    if (can_equip) item_action(TR("Equipaggia", "Equip"), true, on_equip_item, 0);
    bool free_socket = false;
    for (int k = 0; k < it->sockets; k++) free_socket |= !it->gems[k];
    bool have_gem = false;
    for (int sp = 0; sp < SP_COUNT; sp++) have_gem |= R.g.gems[sp] > 0;
    if (free_socket && have_gem) item_action(TR("Gemma", "Gem"), false, on_gems, 0);
    char sell[32];
    snprintf(sell, sizeof sell, TR("Vendi %lu", "Sell %lu"), (unsigned long)rpg_item_value(it));
    item_action(sell, false, on_sell_item, 0);
    item_action(TR("Chiudi", "Close"), false, close_dlg, 0);
}

static void on_use(lv_event_t *e) {
    int c = (int)(intptr_t)lv_event_get_user_data(e);
    int r = rpg_use_consumable(&R.g, c);
    if (r == RPG_OK) r_dirty();
    refresh_list(R_BAG, &bag_list);
    kit_toast(r == RPG_OK ? TX(rpg_consumable(c)->name) : rpg_err(r), &rpg_cons_icons[c]);
}

static void on_tab(lv_event_t *e) {
    R.bag_tab = (int)(intptr_t)lv_event_get_user_data(e);
    r_go(R_BAG);
}

void scr_bag(void) {
    lv_obj_t *s = r_character_screen();
    kit_label(s, &ln_font_title, RC_GOLD, TR("Zaino", "Bag"), CX, 45);
    r_character_button(s, TR("Oggetti", "Items"), CX - 66, 87, 124, 42, R.bag_tab == 0, on_tab, 0);
    r_character_button(s, TR("Scorte", "Supplies"), CX + 66, 87, 124, 42, R.bag_tab == 1, on_tab, 1);
    char buf[64];
    snprintf(buf, sizeof buf, TR("%lu oro  ·  %d/%d posti", "%lu gold  ·  %d/%d slots"),
             (unsigned long)R.g.gold, rpg_bag_count(&R.g), RPG_BAG);
    r_character_panel(s, CX, 124, 248, 27);
    kit_label(s, &ln_font_body, RC_IVORY, buf, CX, 124);
    lv_obj_t *list = bag_list = r_character_list(s, 270, 250);
    if (!R.bag_tab) {
        for (int i = 0; i < RPG_BAG; i++) {
            const rpg_item *it = &R.g.bag[i];
            if (rpg_item_empty(it)) continue;
            lv_obj_t *row = r_character_row(list, 68);
            row_icon(row, item_icon(it));
            char name[64];
            const char *status = rpg_can_equip(&R.g, it) ? TX(rpg_rarity_name(it->rarity)) : TR("Non equipaggiabile", "Cannot equip");
            snprintf(buf, sizeof buf, "Lv %d  ·  %s", it->level, status);
            r_character_row_text(row, 54, 224, rpg_item_name(it, name, sizeof name), rarity_color(it->rarity), buf);
            detail_row(row, on_bag_item, i);
        }
        if (!rpg_bag_count(&R.g))
            r_text(list, TR("Lo zaino è vuoto.", "Your bag is empty."), RC_MUTED, &ln_font_body);
    } else {
        for (int c = 0; c < CO_COUNT; c++) {
            if (!R.g.cons[c]) continue;
            const rpg_consumable_info *ci = rpg_consumable(c);
            bool usable = ci->hp_pct || ci->sp_pct || ci->hunger;
            lv_obj_t *row = r_character_row(list, 76);
            row_icon(row, &rpg_cons_icons[c]);
            snprintf(buf, sizeof buf, "%s ×%u", TX(ci->name), R.g.cons[c]);
            r_character_row_text(row, 48, usable ? 182 : 244, buf, RC_IVORY, TX(ci->blurb));
            if (usable) {
                lv_obj_t *b = r_character_button(row, TR("Usa", "Use"), 0, 0, 60, 44, false, on_use, c);
                lv_obj_align(b, LV_ALIGN_RIGHT_MID, -8, 0);
            }
        }
        int gems = 0;
        for (int sp = 0; sp < SP_COUNT; sp++) gems += R.g.gems[sp];
        if (gems) {
            snprintf(buf, sizeof buf, TR("Gemme d'anima: %d", "Soul gems: %d"), gems);
            r_text(list, buf, RC_GOLD, &ln_font_body);
        }
        if (!lv_obj_get_child_count(list))
            r_text(list, TR("Non ci sono scorte.", "No supplies."), RC_MUTED, &ln_font_body);
    }
    r_back(s, R_EQUIP);
}

/* ---- Shop --------------------------------------------------------------------------------- */
static rpg_item stock[4];

static void on_buy_cons(lv_event_t *e) {
    int c = (int)(intptr_t)lv_event_get_user_data(e);
    int r = rpg_buy(&R.g, c, 1);
    if (r == RPG_OK) {
        r_dirty();
        refresh_list(R_SHOP, &shop_list);
    }
    kit_toast(r == RPG_OK ? TX(rpg_consumable(c)->name) : rpg_err(r), &rpg_cons_icons[c]);
}

static void on_buy_item(lv_event_t *e) {
    int i = (int)(intptr_t)lv_event_get_user_data(e);
    r_items_close_dialog();
    uint32_t price = rpg_item_price(&stock[i]);
    if (R.g.gold < price) { kit_toast(rpg_err(RPG_ERR_GOLD), ri(RI_COIN)); return; }
    if (!rpg_bag_add(&R.g, &stock[i])) { kit_toast(rpg_err(RPG_ERR_BAG), NULL); return; }
    const wf_sprite *icon = item_icon(&stock[i]);
    R.g.gold -= price;
    r_dirty();
    refresh_list(R_SHOP, &shop_list);
    kit_toast(TR("Comprato!", "Bought!"), icon);
}

static void on_stock(lv_event_t *e) {
    int i = (int)(intptr_t)lv_event_get_user_data(e);
    char extra[48];
    snprintf(extra, sizeof extra, TR("\nPrezzo: %lu oro", "\nPrice: %lu gold"), (unsigned long)rpg_item_price(&stock[i]));
    item_dialog(&stock[i], extra);
    item_action(TR("Compra", "Buy"), true, on_buy_item, i);
    item_action(TR("Chiudi", "Close"), false, close_dlg, 0);
}

static void on_shop_tab(lv_event_t *e) {
    R.shop_tab = (int)(intptr_t)lv_event_get_user_data(e);
    r_go(R_SHOP);
}

void scr_shop(void) {
    lv_obj_t *s = r_character_screen();
    const char *title = R.shop_in_dungeon ? TR("Mercante", "Merchant") : TR("Bottega", "Shop");
    kit_label(s, &ln_font_title, RC_GOLD, title, CX, 45);
    static const wf_text tabs[3] = {{"Scorte", "Supplies"}, {"Equip.", "Gear"}, {"Vendi", "Sell"}};
    for (int i = 0; i < 3; i++)
        r_character_button(s, TX(tabs[i]), CX - 97 + i * 97, 87, 91, 42, R.shop_tab == i, on_shop_tab, i);
    char buf[64];
    snprintf(buf, sizeof buf, TR("%lu oro", "%lu gold"), (unsigned long)R.g.gold);
    r_character_panel(s, CX, 124, 224, 27);
    kit_label(s, &ln_font_body, RC_IVORY, buf, CX, 124);
    lv_obj_t *list = shop_list = r_character_list(s, 270, 250);
    if (R.shop_tab == 0) {
        for (int c = 0; c < CO_COUNT; c++) {
            const rpg_consumable_info *ci = rpg_consumable(c);
            lv_obj_t *row = r_character_row(list, 66);
            row_icon(row, &rpg_cons_icons[c]);
            snprintf(buf, sizeof buf, TR("%u oro  ·  ne hai %u", "%u gold  ·  own %u"), ci->price, R.g.cons[c]);
            r_character_row_text(row, 48, 174, TX(ci->name), RC_IVORY, buf);
            lv_obj_t *b = r_character_button(row, TR("Compra", "Buy"), 0, 0, 75, 44,
                                             R.g.gold >= ci->price, on_buy_cons, c);
            lv_obj_align(b, LV_ALIGN_RIGHT_MID, -8, 0);
        }
    } else if (R.shop_tab == 1) {
        rpg_shop_stock(&R.g, stock);
        for (int i = 0; i < 4; i++) {
            lv_obj_t *row = r_character_row(list, 68);
            row_icon(row, item_icon(&stock[i]));
            char name[64];
            snprintf(buf, sizeof buf, TR("%lu oro  ·  Lv %d", "%lu gold  ·  Lv %d"),
                     (unsigned long)rpg_item_price(&stock[i]), stock[i].level);
            r_character_row_text(row, 54, 224, rpg_item_name(&stock[i], name, sizeof name),
                                 rarity_color(stock[i].rarity), buf);
            detail_row(row, on_stock, i);
        }
        r_text(list, TR("Nuova merce ogni giorno", "New stock every day"), RC_MUTED, &ln_font_body);
    } else {
        for (int i = 0; i < RPG_BAG; i++) {
            const rpg_item *it = &R.g.bag[i];
            if (rpg_item_empty(it)) continue;
            lv_obj_t *row = r_character_row(list, 76);
            row_icon(row, item_icon(it));
            char name[64];
            snprintf(buf, sizeof buf, TR("%lu oro", "%lu gold"), (unsigned long)rpg_item_value(it));
            r_character_row_text(row, 48, 174, rpg_item_name(it, name, sizeof name), rarity_color(it->rarity), buf);
            lv_obj_t *b = r_character_button(row, TR("Vendi", "Sell"), 0, 0, 75, 44, false, on_sell_row, i);
            lv_obj_align(b, LV_ALIGN_RIGHT_MID, -8, 0);
        }
        if (!rpg_bag_count(&R.g)) r_text(list, TR("Niente da vendere", "Nothing to sell"), RC_MUTED, &ln_font_body);
    }
    r_back(s, R.shop_in_dungeon ? R_DUNGEON : R_TOWN);
}

/* ---- Forge ---------------------------------------------------------------------------------- */
static lv_obj_t *forge_scr;

static rpg_item *forge_item(void) {
    if (R.forge_pick >= 100 && R.forge_pick - 100 < RPG_BAG && !rpg_item_empty(&R.g.bag[R.forge_pick - 100]))
        return &R.g.bag[R.forge_pick - 100];
    if (R.forge_pick >= 0 && R.forge_pick < SLOT_COUNT && !rpg_item_empty(&R.g.hero.equip[R.forge_pick]))
        return &R.g.hero.equip[R.forge_pick];
    return NULL;
}

static void on_forge(lv_event_t *e) {
    (void)e;
    rpg_item *it = forge_item();
    if (!it) return;
    wf_rng r;
    wf_rng_seed(&r, wf_platform_random());
    int res = rpg_forge(&R.g, it, &r);
    if (res == -3) { kit_toast(rpg_err(RPG_ERR_GOLD), ri(RI_COIN)); return; }
    if (res == -4) { kit_toast(TR("Serve altro minerale", "Need more ore"), &rpg_cons_icons[CO_ORE]); return; }
    if (res == -2) { kit_toast(TR("Già al massimo!", "Already maxed!"), NULL); return; }
    r_save_now();
    r_go(R_FORGE);
    if (res == 1) {
        kit_burst(forge_scr, ri(RI_SPARKLE), CX, 190, 8);
        kit_toast(TR("Successo!", "Success!"), ri(RI_STAR));
    } else {
        kit_toast(res < 0 ? TR("Fallito... perde un livello", "Failed... lost a level") : TR("Fallito, riprova", "Failed, try again"),
                  ri(RI_SKULL));
    }
}

static void on_forge_pick(lv_event_t *e) {
    R.forge_pick = (int)(intptr_t)lv_event_get_user_data(e);
    r_go(R_FORGE);
}

void scr_forge(void) {
    forge_scr = r_character_screen();
    lv_obj_t *s = forge_scr;
    kit_label(s, &ln_font_title, RC_GOLD, TR("Forgia", "Forge"), CX, 51);
    rpg_item *it = forge_item();
    if (it) {
        lv_obj_t *preview = item_button(s, it, CX, 119, 78, NULL, 0);
        lv_obj_remove_flag(preview, LV_OBJ_FLAG_CLICKABLE);
        r_character_button_style(preview, false);
        lv_obj_set_style_border_color(preview, lv_color_hex(rarity_color(it->rarity)), 0);
        char name[64];
        rpg_item_name(it, name, sizeof name);
        lv_obj_t *label = kit_label(s, &ln_font_body, rarity_color(it->rarity), name, CX, 178);
        lv_obj_set_width(label, 290);
        if (it->plus < 10) {
            char buf[96];
            const uint32_t gold = rpg_forge_gold(it);
            const int ore = rpg_forge_ore(it);
            r_character_panel(s, CX, 238, 304, 81);
            snprintf(buf, sizeof buf, TR("Da +%d a +%d  ·  %d%% riuscita", "+%d to +%d  ·  %d%% success"),
                     it->plus, it->plus + 1, rpg_forge_chance(it->plus));
            kit_label(s, &ln_font_body, RC_GOLD, buf, CX, 216);
            snprintf(buf, sizeof buf, TR("Costo: %lu oro  ·  %d minerale", "Cost: %lu gold  ·  %d ore"), (unsigned long)gold, ore);
            kit_label(s, &ln_font_body, RC_IVORY, buf, CX, 238);
            snprintf(buf, sizeof buf, TR("Hai %lu oro  ·  %u minerale", "You have %lu gold  ·  %u ore"),
                     (unsigned long)R.g.gold, R.g.cons[CO_ORE]);
            kit_label(s, &ln_font_body, RC_MUTED, buf, CX, 260);
            r_character_button(s, TR("Potenzia", "Upgrade"), CX, 308, 154, 44,
                               R.g.gold >= gold && R.g.cons[CO_ORE] >= ore, on_forge, 0);
        } else {
            kit_label(s, &ln_font_body, RC_GOLD, TR("Potenziamento massimo", "Fully upgraded"), CX, 238);
        }
    } else {
        kit_label(s, &ln_font_body, RC_IVORY, TR("Scegli un oggetto equipaggiato", "Choose equipped gear"), CX, 174);
    }
    int slots[SLOT_COUNT], count = 0;
    for (int slot = 0; slot < SLOT_COUNT; slot++)
        if (!rpg_item_empty(&R.g.hero.equip[slot])) slots[count++] = slot;
    if (count) kit_label(s, &ln_font_body, RC_MUTED, TR("Equipaggiati", "Equipped"), CX, 343);
    for (int i = 0; i < count; i++) {
        int slot = slots[i];
        lv_obj_t *button = item_button(s, &R.g.hero.equip[slot], CX + (2 * i - count + 1) * 23, 376, 42, on_forge_pick, slot);
        r_character_button_style(button, R.forge_pick == slot);
    }
    r_back(s, R_TOWN);
}
