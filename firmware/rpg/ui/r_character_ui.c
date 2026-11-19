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

void r_character_dialog_style(lv_obj_t *dialog) {
    lv_obj_t *panel = lv_obj_get_child(dialog, 0);
    lv_obj_set_style_bg_color(panel, lv_color_hex(RC_SURFACE), 0);
    lv_obj_set_style_radius(panel, 10, 0);
    lv_obj_set_style_border_width(panel, 1, 0);
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
