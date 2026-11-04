/* Equipment, bag, shop and forge. */
#include <math.h>
#include <stdio.h>
#include <string.h>
#include "rui.h"

static lv_obj_t *dlg;
static void close_dlg(lv_event_t *e) { (void)e; kit_dialog_close(dlg); dlg = NULL; }
static void on_go(lv_event_t *e) { r_go((int)(intptr_t)lv_event_get_user_data(e)); }

static const wf_text slot_names[SLOT_COUNT] = {{"Arma", "Weapon"}, {"Mano sinistra", "Off-hand"}, {"Testa", "Head"},
                                               {"Corpo", "Body"}, {"Piedi", "Feet"}, {"Accessorio", "Accessory"},
                                               {"Accessorio", "Accessory"}};

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
    kit_dialog_close(dlg);
    dlg = NULL;
    int r = rpg_unequip(&R.g, slot);
    if (r != RPG_OK) kit_toast(rpg_err(r), NULL);
    else r_dirty();
    r_go(R_EQUIP);
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
    r_item_dialog(it, NULL, &dlg);
    kit_dialog_button(dlg, TR("Togli", "Remove"), COL_BAD, on_unequip, slot);
    kit_dialog_button(dlg, "Ok", COL_PANEL_HI, close_dlg, 0);
}

void scr_equip(void) {
    lv_obj_t *s = kit_screen(NULL);
    lv_obj_set_style_bg_color(s, lv_color_hex(0x1a1830), 0);
    hero_look l;
    hero_look_from_game(&l, &R.g);
    hero_obj(s, &l, HX_IDLE, 3, CX, 222);
    static const int pos[SLOT_COUNT][2] = {{96, 230}, {370, 230}, {CX, 96}, {150, 336}, {316, 336}, {130, 130}, {336, 130}};
    for (int i = 0; i < SLOT_COUNT; i++) {
        item_button(s, &R.g.hero.equip[i], pos[i][0], pos[i][1], 66, on_slot, i);
        if (rpg_item_empty(&R.g.hero.equip[i]))
            kit_label(s, &wf_font_small, COL_DIM, TX(slot_names[i]), pos[i][0], pos[i][1]);
    }
    rpg_stats st;
    rpg_stats_get(&R.g, &st);
    char buf[64];
    snprintf(buf, sizeof buf, "ATK %ld · MATK %ld · %s %ld", (long)st.atk, (long)st.matk, TR("DIF", "DEF"), (long)st.def);
    kit_label(s, &wf_font_small, COL_ENERGY, buf, CX, 380);
    kit_text_button(s, TR("Zaino", "Bag"), CX + 62, 428, 110, 44, COL_BTN, on_go, R_BAG);
    kit_round_button(s, ri(RI_BACK), NULL, CX - 50, 428, 48, COL_PANEL_HI, on_go, R_TOWN);
}

/* ---- Bag -------------------------------------------------------------------------------- */
static int picked;

static void on_equip_item(lv_event_t *e) {
    (void)e;
    kit_dialog_close(dlg);
    dlg = NULL;
    int r = rpg_equip(&R.g, picked);
    kit_toast(r == RPG_OK ? TR("Equipaggiato!", "Equipped!") : rpg_err(r), NULL);
    if (r == RPG_OK) r_dirty();
    r_go(R_BAG);
}

static void on_sell_item(lv_event_t *e) {
    (void)e;
    kit_dialog_close(dlg);
    dlg = NULL;
    uint32_t v = rpg_sell(&R.g, picked);
    char buf[40];
    snprintf(buf, sizeof buf, TR("Venduto: +%lu oro", "Sold: +%lu gold"), (unsigned long)v);
    kit_toast(buf, ri(RI_COIN));
    r_dirty();
    r_go(R.screen);
}

static void on_sell_row(lv_event_t *e) {
    picked = (int)(intptr_t)lv_event_get_user_data(e);
    on_sell_item(e);
}

static void on_socket(lv_event_t *e) {
    int sp = (int)(intptr_t)lv_event_get_user_data(e);
    kit_dialog_close(dlg);
    dlg = NULL;
    int r = rpg_socket(&R.g, &R.g.bag[picked], sp);
    kit_toast(r == RPG_OK ? TR("Gemma incastonata!", "Gem socketed!") : rpg_err(r), ri(RI_GEM));
    if (r == RPG_OK) r_dirty();
    r_go(R_BAG);
}

static void on_gems(lv_event_t *e) {
    (void)e;
    kit_dialog_close(dlg);
    dlg = kit_dialog(TR("Gemme d'anima", "Soul gems"), TR("Scegli la gemma da incastonare", "Pick a gem to socket"), ri(RI_GEM));
    int shown = 0;
    for (int sp = 0; sp < SP_COUNT && shown < 6; sp++) {
        if (!R.g.gems[sp]) continue;
        char b[40];
        snprintf(b, sizeof b, "%.10s x%d", TX(rpg_species(sp)->name), R.g.gems[sp]);
        kit_dialog_button(dlg, b, COL_BTN_ALT, on_socket, sp);
        shown++;
    }
    kit_dialog_button(dlg, "X", COL_PANEL_HI, close_dlg, 0);
}

static void on_bag_item(lv_event_t *e) {
    picked = (int)(intptr_t)lv_event_get_user_data(e);
    rpg_item *it = &R.g.bag[picked];
    char extra[64] = "";
    const rpg_item *cur = &R.g.hero.equip[rpg_item_type_get(it->type)->slot];
    if (!rpg_item_empty(cur)) {
        int d = (rpg_item_atk(it) + rpg_item_matk(it) + rpg_item_def(it)) - (rpg_item_atk(cur) + rpg_item_matk(cur) + rpg_item_def(cur));
        snprintf(extra, sizeof extra, TR("\nRispetto all'attuale: %+d", "\nCompared to current: %+d"), d);
    }
    if (!rpg_can_equip(&R.g, it))
        strncat(extra, it->level > R.g.hero.level ? TR("\nLivello troppo alto", "\nLevel too high") : TR("\nNon per la tua classe", "\nNot for your class"),
                sizeof extra - strlen(extra) - 1);
    r_item_dialog(it, extra, &dlg);
    if (rpg_can_equip(&R.g, it)) kit_dialog_button(dlg, TR("Equipaggia", "Equip"), COL_BTN_OK, on_equip_item, 0);
    bool free_socket = false;
    for (int k = 0; k < it->sockets; k++) free_socket |= !it->gems[k];
    bool have_gem = false;
    for (int sp = 0; sp < SP_COUNT; sp++) have_gem |= R.g.gems[sp] > 0;
    if (free_socket && have_gem) kit_dialog_button(dlg, TR("Gemma", "Gem"), COL_BTN_ALT, on_gems, 0);
    char sell[32];
    snprintf(sell, sizeof sell, TR("Vendi %lu", "Sell %lu"), (unsigned long)rpg_item_value(it));
    kit_dialog_button(dlg, sell, COL_GOLD, on_sell_item, 0);
}

static void on_use(lv_event_t *e) {
    int c = (int)(intptr_t)lv_event_get_user_data(e);
    int r = rpg_use_consumable(&R.g, c);
    kit_toast(r == RPG_OK ? TX(rpg_consumable(c)->name) : rpg_err(r), &rpg_cons_icons[c]);
    if (r == RPG_OK) r_dirty();
    r_go(R_BAG);
}

static void on_tab(lv_event_t *e) {
    R.bag_tab = (int)(intptr_t)lv_event_get_user_data(e);
    r_go(R_BAG);
}

void scr_bag(void) {
    lv_obj_t *s = kit_screen(NULL);
    lv_obj_set_style_bg_color(s, lv_color_hex(0x1a1830), 0);
    kit_text_button(s, TR("Oggetti", "Items"), CX - 64, 66, 120, 40, R.bag_tab ? COL_PANEL_HI : COL_BTN, on_tab, 0);
    kit_text_button(s, TR("Scorte", "Supplies"), CX + 64, 66, 120, 40, R.bag_tab ? COL_BTN : COL_PANEL_HI, on_tab, 1);
    char buf[64];
    snprintf(buf, sizeof buf, "%lu  ·  %d/%d", (unsigned long)R.g.gold, rpg_bag_count(&R.g), RPG_BAG);
    kit_img(s, ri(RI_COIN), 2, CX - 70, 104);
    kit_label(s, &wf_font_small, COL_GOLD, buf, CX + 6, 104);
    lv_obj_t *list = r_list(s, CX, 262, 320, 280);
    if (!R.bag_tab) {
        /* grid of item tiles, four per row */
        lv_obj_set_flex_flow(list, LV_FLEX_FLOW_ROW_WRAP);
        lv_obj_set_style_pad_column(list, 8, 0);
        lv_obj_set_flex_align(list, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
        int n = 0;
        for (int i = 0; i < RPG_BAG; i++) {
            if (rpg_item_empty(&R.g.bag[i])) continue;
            lv_obj_t *b = item_button(list, &R.g.bag[i], 0, 0, 68, on_bag_item, i);
            lv_obj_set_align(b, LV_ALIGN_DEFAULT);
            if (!rpg_can_equip(&R.g, &R.g.bag[i])) lv_obj_set_style_opa(b, 150, 0);
            n++;
        }
        if (!n) r_text(list, TR("Nessun oggetto. Vai a caccia!", "No items. Go hunting!"), COL_DIM, NULL);
    } else {
        for (int c = 0; c < CO_COUNT; c++) {
            if (!R.g.cons[c]) continue;
            lv_obj_t *row = r_row(list, 56);
            lv_obj_t *img = lv_image_create(row);
            lv_image_set_src(img, gfx_sprite(&rpg_cons_icons[c], 2));
            lv_obj_align(img, LV_ALIGN_LEFT_MID, 8, 0);
            snprintf(buf, sizeof buf, "%s x%d\n%s", TX(rpg_consumable(c)->name), R.g.cons[c], TX(rpg_consumable(c)->blurb));
            lv_obj_t *t = r_text(row, buf, COL_TEXT, NULL);
            lv_obj_set_width(t, 190);
            lv_label_set_long_mode(t, LV_LABEL_LONG_DOT);
            lv_obj_align(t, LV_ALIGN_LEFT_MID, 46, 0);
            const rpg_consumable_info *ci = rpg_consumable(c);
            if (ci->hp_pct || ci->sp_pct || ci->hunger) {
                lv_obj_t *b = kit_text_button(row, TR("Usa", "Use"), 0, 0, 60, 40, COL_BTN_OK, on_use, c);
                lv_obj_align(b, LV_ALIGN_RIGHT_MID, -6, 0);
            }
        }
        int gems = 0;
        for (int sp = 0; sp < SP_COUNT; sp++) gems += R.g.gems[sp];
        if (gems) {
            snprintf(buf, sizeof buf, TR("Gemme d'anima: %d", "Soul gems: %d"), gems);
            r_text(list, buf, 0xff8ae8, NULL);
        }
    }
    kit_round_button(s, ri(RI_BACK), NULL, CX, 428, 48, COL_PANEL_HI, on_go, R_EQUIP);
}

/* ---- Shop --------------------------------------------------------------------------------- */
static rpg_item stock[4];

static void on_buy_cons(lv_event_t *e) {
    int c = (int)(intptr_t)lv_event_get_user_data(e);
    int r = rpg_buy(&R.g, c, 1);
    kit_toast(r == RPG_OK ? TX(rpg_consumable(c)->name) : rpg_err(r), &rpg_cons_icons[c]);
    if (r == RPG_OK) r_dirty();
    r_go(R_SHOP);
}

static void on_buy_item(lv_event_t *e) {
    int i = (int)(intptr_t)lv_event_get_user_data(e);
    kit_dialog_close(dlg);
    dlg = NULL;
    uint32_t price = rpg_item_price(&stock[i]);
    if (R.g.gold < price) { kit_toast(rpg_err(RPG_ERR_GOLD), ri(RI_COIN)); return; }
    if (!rpg_bag_add(&R.g, &stock[i])) { kit_toast(rpg_err(RPG_ERR_BAG), NULL); return; }
    R.g.gold -= price;
    r_dirty();
    kit_toast(TR("Comprato!", "Bought!"), item_icon(&stock[i]));
    r_go(R_SHOP);
}

static void on_stock(lv_event_t *e) {
    int i = (int)(intptr_t)lv_event_get_user_data(e);
    char extra[40];
    snprintf(extra, sizeof extra, TR("\nPrezzo: %lu oro", "\nPrice: %lu gold"), (unsigned long)rpg_item_price(&stock[i]));
    r_item_dialog(&stock[i], extra, &dlg);
    kit_dialog_button(dlg, TR("Compra", "Buy"), COL_BTN_OK, on_buy_item, i);
    kit_dialog_button(dlg, "X", COL_PANEL_HI, close_dlg, 0);
}

static void on_shop_tab(lv_event_t *e) {
    R.shop_tab = (int)(intptr_t)lv_event_get_user_data(e);
    r_go(R_SHOP);
}

void scr_shop(void) {
    lv_obj_t *s = kit_screen(NULL);
    lv_obj_set_style_bg_color(s, lv_color_hex(0x221a14), 0);
    static const wf_text tabs_[3] = {{"Scorte", "Supplies"}, {"Armi", "Gear"}, {"Vendi", "Sell"}};
    for (int i = 0; i < 3; i++)
        kit_text_button(s, TX(tabs_[i]), CX - 104 + i * 104, 66, 98, 40, R.shop_tab == i ? COL_BTN : COL_PANEL_HI, on_shop_tab, i);
    char buf[96];
    snprintf(buf, sizeof buf, "%lu", (unsigned long)R.g.gold);
    kit_img(s, ri(RI_COIN), 2, CX - 30, 104);
    kit_label(s, &wf_font_small, COL_GOLD, buf, CX + 10, 104);
    lv_obj_t *list = r_list(s, CX, 262, 320, 280);
    if (R.shop_tab == 0) {
        static const uint8_t sold[] = {CO_POTION, CO_HIPOTION, CO_ETHER, CO_ANTIDOTE, CO_BREAD, CO_STEW, CO_PIE, CO_ORE, CO_RETURN};
        for (unsigned k = 0; k < sizeof sold; k++) {
            int c = sold[k];
            const rpg_consumable_info *ci = rpg_consumable(c);
            lv_obj_t *row = r_row(list, 54);
            lv_obj_t *img = lv_image_create(row);
            lv_image_set_src(img, gfx_sprite(&rpg_cons_icons[c], 2));
            lv_obj_align(img, LV_ALIGN_LEFT_MID, 8, 0);
            snprintf(buf, sizeof buf, "%s (%d)\n%u %s", TX(ci->name), R.g.cons[c], ci->price, TR("oro", "gold"));
            lv_obj_t *t = r_text(row, buf, COL_TEXT, NULL);
            lv_obj_align(t, LV_ALIGN_LEFT_MID, 46, 0);
            lv_obj_t *b = kit_round_button(row, ri(RI_PLUS), NULL, 0, 0, 40, R.g.gold >= ci->price ? COL_BTN_OK : 0x4a4a5a,
                                           on_buy_cons, c);
            lv_obj_align(b, LV_ALIGN_RIGHT_MID, -6, 0);
        }
    } else if (R.shop_tab == 1) {
        rpg_shop_stock(&R.g, stock);
        for (int i = 0; i < 4; i++) {
            lv_obj_t *row = r_row(list, 58);
            lv_obj_t *b = item_button(row, &stock[i], 0, 0, 48, on_stock, i);
            lv_obj_align(b, LV_ALIGN_LEFT_MID, 4, 0);
            char name[48];
            snprintf(buf, sizeof buf, "%s\n%lu %s", rpg_item_name(&stock[i], name, sizeof name),
                     (unsigned long)rpg_item_price(&stock[i]), TR("oro", "gold"));
            lv_obj_t *t = r_text(row, buf, rarity_color(stock[i].rarity), NULL);
            lv_obj_set_width(t, 230);
            lv_label_set_long_mode(t, LV_LABEL_LONG_DOT);
            lv_obj_align(t, LV_ALIGN_LEFT_MID, 60, 0);
            lv_obj_add_flag(row, LV_OBJ_FLAG_CLICKABLE);
            lv_obj_add_event_cb(row, on_stock, LV_EVENT_CLICKED, (void *)(intptr_t)i);
        }
        r_text(list, TR("Nuova merce ogni giorno", "New stock every day"), COL_DIM, NULL);
    } else {
        int n = 0;
        for (int i = 0; i < RPG_BAG; i++) {
            const rpg_item *it = &R.g.bag[i];
            if (rpg_item_empty(it)) continue;
            lv_obj_t *row = r_row(list, 54);
            lv_obj_t *img = lv_image_create(row);
            lv_image_set_src(img, gfx_sprite(item_icon(it), 2));
            lv_obj_align(img, LV_ALIGN_LEFT_MID, 8, 0);
            char name[48];
            snprintf(buf, sizeof buf, "%s", rpg_item_name(it, name, sizeof name));
            lv_obj_t *t = r_text(row, buf, rarity_color(it->rarity), NULL);
            lv_obj_set_width(t, 170);
            lv_label_set_long_mode(t, LV_LABEL_LONG_DOT);
            lv_obj_align(t, LV_ALIGN_LEFT_MID, 46, 0);
            snprintf(buf, sizeof buf, "%lu", (unsigned long)rpg_item_value(it));
            lv_obj_t *b = kit_text_button(row, buf, 0, 0, 70, 40, COL_GOLD, on_sell_row, i);
            lv_obj_align(b, LV_ALIGN_RIGHT_MID, -6, 0);
            n++;
        }
        if (!n) r_text(list, TR("Niente da vendere", "Nothing to sell"), COL_DIM, NULL);
    }
    if (R.shop_in_dungeon) kit_round_button(s, ri(RI_BACK), NULL, CX, 428, 48, COL_PANEL_HI, on_go, R_DUNGEON);
    else r_back(s, R_TOWN);
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
    forge_scr = kit_screen(NULL);
    lv_obj_t *s = forge_scr;
    lv_obj_set_style_bg_color(s, lv_color_hex(0x2a1612), 0);
    kit_label(s, &wf_font_big, COL_HUNGER, TR("Forgia", "Forge"), CX, 62);
    rpg_item *it = forge_item();
    char buf[160];
    if (it) {
        item_button(s, it, CX, 150, 96, NULL, 0);
        char name[48];
        rpg_item_name(it, name, sizeof name);
        kit_label(s, &wf_font_small, rarity_color(it->rarity), name, CX, 214);
        if (it->plus < 10) {
            snprintf(buf, sizeof buf, TR("+%d → +%d  ·  %d%%\n%lu oro, %d minerale (hai %d)", "+%d → +%d  ·  %d%%\n%lu gold, %d ore (have %d)"),
                     it->plus, it->plus + 1, rpg_forge_chance(it->plus), (unsigned long)rpg_forge_gold(it), rpg_forge_ore(it),
                     R.g.cons[CO_ORE]);
            kit_label(s, &wf_font_small, COL_TEXT, buf, CX, 250);
            kit_round_button(s, ri(RI_ANVIL), TR("Forgia!", "Forge!"), CX, 310, 70, COL_HUNGER, on_forge, 0);
        } else {
            kit_label(s, &wf_font_small, COL_GOLD, TR("Potenziamento massimo!", "Fully upgraded!"), CX, 250);
        }
    } else {
        kit_label(s, &wf_font_small, COL_DIM, TR("Scegli un oggetto qui sotto", "Pick an item below"), CX, 150);
    }
    /* strip of equipped items */
    int x = CX - 3 * 46;
    for (int i = 0; i < SLOT_COUNT; i++) {
        if (rpg_item_empty(&R.g.hero.equip[i])) continue;
        item_button(s, &R.g.hero.equip[i], x, 382, 42, on_forge_pick, i);
        x += 46;
    }
    r_back(s, R_TOWN);
}
