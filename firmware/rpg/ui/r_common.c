/* Small helpers shared by the RPG screens. */
#include <stdio.h>
#include <string.h>
#include "rui.h"

uint32_t rarity_color(int r) {
    static const uint32_t c[RAR_COUNT] = {0xe8e8f0, 0x6ce06a, 0x5aa8ff, 0xc87aff, 0xffa83a};
    return c[(unsigned)r < RAR_COUNT ? r : 0];
}

const wf_sprite *item_icon(const rpg_item *it) {
    return &rpg_item_icons[it->type % IT_TYPE_COUNT][it->tier % RPG_TIERS];
}

const char *r_fmt_time(uint32_t s, char *buf, int cap) {
    if (s >= 3600) snprintf(buf, (size_t)cap, "%luh %02lum", (unsigned long)(s / 3600), (unsigned long)(s / 60 % 60));
    else snprintf(buf, (size_t)cap, "%lum", (unsigned long)(s / 60));
    return buf;
}

lv_obj_t *r_list(lv_obj_t *parent, int x, int y, int w, int h) {
    lv_obj_t *l = lv_obj_create(parent);
    lv_obj_remove_style_all(l);
    lv_obj_set_size(l, w, h);
    kit_pos(l, x, y);
    lv_obj_set_flex_flow(l, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(l, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_row(l, 6, 0);
    lv_obj_set_scroll_dir(l, LV_DIR_VER);
    lv_obj_set_scrollbar_mode(l, LV_SCROLLBAR_MODE_OFF);
    return l;
}

lv_obj_t *r_row(lv_obj_t *list, int h) {
    lv_obj_t *row = lv_obj_create(list);
    lv_obj_remove_style_all(row);
    lv_obj_set_size(row, lv_pct(100), h);
    lv_obj_set_style_bg_color(row, lv_color_hex(COL_PANEL_HI), 0);
    lv_obj_set_style_bg_opa(row, 210, 0);
    lv_obj_set_style_radius(row, 12, 0);
    lv_obj_remove_flag(row, LV_OBJ_FLAG_SCROLLABLE);
    return row;
}

lv_obj_t *r_text(lv_obj_t *parent, const char *t, uint32_t color, const lv_font_t *font) {
    lv_obj_t *l = lv_label_create(parent);
    lv_obj_set_style_text_font(l, font ? font : &wf_font_small, 0);
    lv_obj_set_style_text_color(l, lv_color_hex(color), 0);
    lv_label_set_text(l, t);
    return l;
}

const char *r_item_lines(const rpg_item *it, char *buf, int cap) {
    int n = 0;
    int atk = rpg_item_atk(it), matk = rpg_item_matk(it), def = rpg_item_def(it), mdef = rpg_item_mdef(it), hp = rpg_item_hp(it);
    n += snprintf(buf + n, (size_t)(cap - n), "%s · Lv %d\n", TX(rpg_rarity_name(it->rarity)), it->level);
    if (atk) n += snprintf(buf + n, (size_t)(cap - n), "ATK %d  ", atk);
    if (matk) n += snprintf(buf + n, (size_t)(cap - n), "MATK %d  ", matk);
    if (def) n += snprintf(buf + n, (size_t)(cap - n), "%s %d  ", TR("DIF", "DEF"), def);
    if (mdef) n += snprintf(buf + n, (size_t)(cap - n), "%s %d  ", TR("DIFM", "MDEF"), mdef);
    if (hp) n += snprintf(buf + n, (size_t)(cap - n), "%s %d", TR("PV", "HP"), hp);
    for (int a = 0; a < 3 && n < cap - 24; a++)
        if (it->aff[a]) n += snprintf(buf + n, (size_t)(cap - n), "\n+%d %s", it->aff_val[a], TX(rpg_affix_name(it->aff[a])));
    for (int k = 0; k < it->sockets && n < cap - 32; k++) {
        if (it->gems[k]) {
            const rpg_species_info *sp = rpg_species(it->gems[k] - 1);
            n += snprintf(buf + n, (size_t)(cap - n), TR("\nGemma %s: +%d %s", "\n%s gem: +%d %s"), TX(sp->name),
                          sp->gem_value, TX(rpg_affix_name(sp->gem_affix)));
        } else {
            n += snprintf(buf + n, (size_t)(cap - n), "%s", TR("\nAlveolo vuoto", "\nEmpty socket"));
        }
    }
    return buf;
}

void r_item_dialog(const rpg_item *it, const char *extra, lv_obj_t **dlg) {
    char name[48], body[260];
    rpg_item_name(it, name, sizeof name);
    r_item_lines(it, body, sizeof body);
    if (extra) strncat(body, extra, sizeof body - strlen(body) - 1);
    *dlg = kit_dialog(name, body, item_icon(it));
    lv_obj_t *panel = lv_obj_get_child(*dlg, 0);
    lv_obj_set_style_border_color(panel, lv_color_hex(rarity_color(it->rarity)), 0);
    lv_obj_t *title = lv_obj_get_child(panel, 1);
    if (title) lv_obj_set_style_text_color(title, lv_color_hex(rarity_color(it->rarity)), 0);
}

void r_vitals_arcs(lv_obj_t *s, lv_obj_t **hp, lv_obj_t **sp) {
    *hp = kit_arc(s, 226, 16, 98, 172, COL_HP, 0, 100);
    lv_arc_set_mode(*hp, LV_ARC_MODE_REVERSE);
    *sp = kit_arc(s, 226, 16, 8, 82, 0x5aa0ff, 0, 100);
    lv_obj_t *a = kit_img(s, ri(RI_HEART), 2, 48, 290);
    lv_obj_t *b = kit_img(s, ri(RI_SP), 2, 418, 290);
    (void)a; (void)b;
}

void r_vitals_update(lv_obj_t *hp, lv_obj_t *sp, int32_t hpv, int32_t hpmax, int32_t spv, int32_t spmax) {
    lv_arc_set_range(hp, 0, hpmax > 0 ? hpmax : 1);
    lv_arc_set_value(hp, hpv);
    lv_arc_set_range(sp, 0, spmax > 0 ? spmax : 1);
    lv_arc_set_value(sp, spv);
    int pct = hpmax ? (int)(hpv * 100 / hpmax) : 0;
    lv_obj_set_style_arc_color(hp, lv_color_hex(pct > 50 ? COL_HP : pct > 20 ? COL_GOLD : COL_BAD), LV_PART_INDICATOR);
}
