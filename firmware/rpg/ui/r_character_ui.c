/* Shared presentation for the Classic character screens. */
#include "rui.h"
#include "art/classic_ui_art.h"

lv_obj_t *r_character_screen(void) {
    return kit_screen(&classic_archive);
}

lv_obj_t *r_character_panel(lv_obj_t *parent, int x, int y, int w, int h) {
    lv_obj_t *panel = kit_panel(parent, x, y, w, h);
    lv_obj_set_style_bg_color(panel, lv_color_hex(RC_SURFACE), 0);
    lv_obj_set_style_bg_opa(panel, 240, 0);
    lv_obj_set_style_border_color(panel, lv_color_hex(0x495e62), 0);
    lv_obj_set_style_border_width(panel, 1, 0);
    lv_obj_set_style_radius(panel, 8, 0);
    return panel;
}

void r_character_button_style(lv_obj_t *button, bool accent) {
    uint32_t edge = accent ? 0xd4a765 : 0x577579;
    uint32_t top = accent ? 0x6d4727 : 0x24454f;
    uint32_t bottom = accent ? 0x42291c : 0x152832;
    lv_obj_set_ext_click_area(button, 0);
    lv_obj_set_style_radius(button, 8, 0);
    lv_obj_set_style_border_width(button, 1, 0);
    lv_obj_set_style_border_color(button, lv_color_hex(edge), 0);
    lv_obj_set_style_bg_color(button, lv_color_hex(top), 0);
    lv_obj_set_style_bg_grad_color(button, lv_color_hex(bottom), 0);
    lv_obj_set_style_bg_color(button, lv_color_hex(0x13252a), LV_STATE_PRESSED);
    lv_obj_set_style_bg_grad_color(button, lv_color_hex(0x0d1b20), LV_STATE_PRESSED);
    lv_obj_set_style_border_color(button, lv_color_hex(RC_GOLD), LV_STATE_PRESSED);
    lv_obj_set_style_translate_y(button, 0, LV_STATE_PRESSED);
    lv_obj_set_style_border_color(button, lv_color_hex(0x3f4d52), LV_STATE_DISABLED);

    lv_obj_t *label = lv_obj_get_child(button, 0);
    if (label && lv_obj_check_type(label, &lv_label_class)) {
        lv_obj_set_style_text_font(label, &ln_font_body, 0);
        lv_obj_set_style_text_color(label, lv_color_hex(RC_IVORY), 0);
        lv_obj_set_style_text_color(label, lv_color_hex(RC_MUTED), LV_STATE_DISABLED);
    }
}

lv_obj_t *r_character_button(lv_obj_t *parent, const char *text, int x, int y, int w, int h,
                             bool accent, lv_event_cb_t cb, intptr_t user) {
    lv_obj_t *button = kit_text_button(parent, text, x, y, w, h, COL_PANEL, cb, user);
    r_character_button_style(button, accent);
    return button;
}

lv_obj_t *r_character_list(lv_obj_t *parent, int y, int height) {
    lv_obj_t *list = r_list(parent, CX, y, 320, height);
    lv_obj_set_style_pad_right(list, 8, 0);
    lv_obj_set_style_pad_row(list, 8, 0);
    lv_obj_set_scrollbar_mode(list, LV_SCROLLBAR_MODE_AUTO);
    lv_obj_set_style_width(list, 3, LV_PART_SCROLLBAR);
    lv_obj_set_style_bg_color(list, lv_color_hex(RC_GOLD), LV_PART_SCROLLBAR);
    lv_obj_set_style_bg_opa(list, LV_OPA_70, LV_PART_SCROLLBAR);
    lv_obj_set_style_radius(list, 2, LV_PART_SCROLLBAR);
    return list;
}

lv_obj_t *r_character_row(lv_obj_t *list, int height) {
    lv_obj_t *row = r_row(list, height);
    lv_obj_set_style_bg_color(row, lv_color_hex(RC_SURFACE), 0);
    lv_obj_set_style_bg_opa(row, 246, 0);
    lv_obj_set_style_border_color(row, lv_color_hex(0x495e62), 0);
    lv_obj_set_style_border_width(row, 1, 0);
    lv_obj_set_style_radius(row, 8, 0);
    return row;
}

void r_character_row_text(lv_obj_t *row, int left, int width,
                          const char *title, uint32_t color, const char *detail) {
    const int line_space = -5;
    lv_obj_t *column = lv_obj_create(row);
    lv_obj_remove_style_all(column);
    lv_obj_remove_flag(column, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_size(column, width, LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(column, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(column, 2, 0);
    lv_obj_align(column, LV_ALIGN_LEFT_MID, left, 0);
    lv_obj_t *name = r_text(column, title, color, &ln_font_body);
    lv_obj_set_width(name, lv_pct(100));
    lv_obj_set_style_text_line_space(name, line_space, 0);
    lv_point_t size;
    lv_text_get_size(&size, title, &ln_font_body, 0, line_space, width, LV_TEXT_FLAG_NONE);
    int min_height = size.y + 12;
    if (detail) {
        lv_obj_t *caption = r_text(column, detail, RC_MUTED, &ln_font_body);
        lv_obj_set_width(caption, lv_pct(100));
        lv_obj_set_style_text_line_space(caption, line_space, 0);
        lv_text_get_size(&size, detail, &ln_font_body, 0, line_space, width, LV_TEXT_FLAG_NONE);
        min_height += size.y + 2;
    }
    lv_obj_set_style_min_height(row, min_height, 0);
}

void r_character_dialog_style(lv_obj_t *dialog) {
    lv_obj_t *panel = lv_obj_get_child(dialog, 0);
    lv_obj_set_style_bg_color(panel, lv_color_hex(RC_SURFACE), 0);
    lv_obj_set_style_radius(panel, 10, 0);
    lv_obj_set_style_border_width(panel, 1, 0);
    lv_obj_set_style_width(panel, 3, LV_PART_SCROLLBAR);
    lv_obj_set_style_bg_color(panel, lv_color_hex(RC_GOLD), LV_PART_SCROLLBAR);
    lv_obj_set_style_bg_opa(panel, LV_OPA_70, LV_PART_SCROLLBAR);
    bool title = true;
    for (unsigned i = 0; i < lv_obj_get_child_count(panel); i++) {
        lv_obj_t *child = lv_obj_get_child(panel, (int)i);
        if (!lv_obj_check_type(child, &lv_label_class)) continue;
        lv_obj_set_style_text_font(child, title ? &ln_font_title : &ln_font_body, 0);
        lv_obj_set_style_text_color(child, lv_color_hex(title ? RC_GOLD : RC_IVORY), 0);
        title = false;
    }
}

lv_obj_t *r_character_dialog(const char *title, const char *body, const wf_sprite *icon) {
    lv_obj_t *dialog = kit_dialog(title, body, icon);
    r_character_dialog_style(dialog);
    return dialog;
}
