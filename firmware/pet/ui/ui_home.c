/* Home: the companion on its floating island, needs around the rim. */
#include <math.h>
#include <stdio.h>
#include <string.h>
#include "ui_internal.h"

#define PET_X CX
#define PET_Y 268

static lv_obj_t *scr, *pet, *name_l, *status_l, *arcs[3], *xp_arc, *lvl_l, *gift_btn, *mess[WF_MESS_MAX];
static lv_obj_t *seed_bar, *away_panel, *away_l, *away_btn, *night_dim, *evo_spark;
static uint32_t expr_until, next_blink, next_zzz, last_refresh;
static int expr_override = -1, shown_form = -1;
static bool hatch_running;

static int base_expr(void) {
    const wf_game *g = &ui.g;
    if (g->pet.sleeping) return WF_EXPR_SLEEP;
    switch (wf_pet_face(g)) {
    case WF_MOOD_HUNGRY: case WF_MOOD_TIRED: case WF_MOOD_SAD: return WF_EXPR_SAD;
    default: return WF_EXPR_IDLE;
    }
}

static void set_expr(int expr, uint32_t ms) {
    expr_override = expr;
    expr_until = lv_tick_get() + ms;
}

static void apply_pet_frame(void) {
    if (!pet) return;
    int form = ui.g.pet.form;
    int expr = expr_override >= 0 ? expr_override : base_expr();
    if (form == WF_FORM_SEED) {
        int heat = ui.g.pet.hatch_heat;
        expr = heat >= 16 ? 2 : heat >= 8 ? 1 : 0;
    }
    kit_img_set(pet, &wf_pet_sprites[form][expr], PIX);
}

static void pos_on_ring(lv_obj_t *o, int radius, float deg) {
    float a = deg * 3.14159265f / 180.0f;
    kit_pos(o, CX + (int)lroundf(radius * cosf(a)), CY + (int)lroundf(radius * sinf(a)));
}

static void refresh(void) {
    const wf_game *g = &ui.g;
    uint32_t now = now_s();
    bool seed = wf_pet_stage(g) == WF_STAGE_SEED;
    bool away = wf_pet_away(g, now);
    lv_arc_set_value(arcs[0], wf_need_percent(g->pet.hunger));
    lv_arc_set_value(arcs[1], wf_need_percent(g->pet.energy));
    lv_arc_set_value(arcs[2], wf_need_percent(g->pet.mood));
    lv_arc_set_range(xp_arc, 0, (int32_t)wf_xp_to_next(g->pet.level));
    lv_arc_set_value(xp_arc, (int32_t)g->pet.xp);
    char buf[48];
    snprintf(buf, sizeof buf, "Lv %d", g->pet.level);
    lv_label_set_text(lvl_l, buf);
    lv_label_set_text(status_l, wf_pet_status_text(g, now));
    for (int i = 0; i < WF_MESS_MAX; i++) {
        if (i < g->pet.mess && !away && !seed) lv_obj_remove_flag(mess[i], LV_OBJ_FLAG_HIDDEN);
        else lv_obj_add_flag(mess[i], LV_OBJ_FLAG_HIDDEN);
    }
    if (g->daily_claimed || seed) lv_obj_add_flag(gift_btn, LV_OBJ_FLAG_HIDDEN);
    else lv_obj_remove_flag(gift_btn, LV_OBJ_FLAG_HIDDEN);
    if (seed) {
        lv_obj_remove_flag(seed_bar, LV_OBJ_FLAG_HIDDEN);
        kit_bar_set(seed_bar, g->pet.hatch_heat, 24, true);
    } else {
        lv_obj_add_flag(seed_bar, LV_OBJ_FLAG_HIDDEN);
    }
    if (away) {
        lv_obj_add_flag(pet, LV_OBJ_FLAG_HIDDEN);
        lv_obj_remove_flag(away_panel, LV_OBJ_FLAG_HIDDEN);
        char d[24];
        bool ready = wf_expedition_ready(g, now);
        snprintf(buf, sizeof buf, "%s\n%s", wf_zone_get(g->pet.away_zone)->name,
                 ready ? "È tornato!" : fmt_duration(g->pet.away_until - now, d, sizeof d));
        lv_label_set_text(away_l, buf);
        if (ready) lv_obj_remove_flag(away_btn, LV_OBJ_FLAG_HIDDEN);
        else lv_obj_add_flag(away_btn, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_remove_flag(pet, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(away_panel, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(away_btn, LV_OBJ_FLAG_HIDDEN);
    }
    if (g->pet.sleeping) lv_obj_remove_flag(night_dim, LV_OBJ_FLAG_HIDDEN);
    else lv_obj_add_flag(night_dim, LV_OBJ_FLAG_HIDDEN);
    if (g->pet.pending_form && !away) lv_obj_remove_flag(evo_spark, LV_OBJ_FLAG_HIDDEN);
    else lv_obj_add_flag(evo_spark, LV_OBJ_FLAG_HIDDEN);
    if (shown_form != g->pet.form) {
        shown_form = g->pet.form;
        lv_label_set_text(name_l, g->pet.name);
    }
    apply_pet_frame();
}

/* ---- Evolution & hatching cutscenes --------------------------------------- */
static lv_obj_t *cut_dialog;
static void close_cut(lv_event_t *e) {
    (void)e;
    kit_dialog_close(cut_dialog);
    cut_dialog = NULL;
}

static void evolve_done(lv_timer_t *t) {
    lv_timer_delete(t);
    hatch_running = false;
    if (!pet || ui.screen != WF_SCR_HOME) return;
    kit_flash(pet, 0xffffff, 600);
    kit_burst(scr, icon(WF_ICON_SPARKLE), PET_X, PET_Y - 30, 10);
    set_expr(WF_EXPR_HAPPY, 2500);
    refresh();
    const wf_form_info *f = wf_form_get(ui.g.pet.form);
    char body[128];
    snprintf(body, sizeof body, "%s si è trasformato in %s!\n%s", ui.g.pet.name, f->name, f->blurb);
    cut_dialog = kit_dialog(f->name, body, NULL);
    kit_dialog_button(cut_dialog, "Evviva!", COL_BTN_OK, close_cut, 0);
}

static void flicker(lv_timer_t *t) {
    /* Alternate old/new silhouettes faster and faster. */
    uintptr_t step = (uintptr_t)lv_timer_get_user_data(t);
    if (!pet) return;
    int from = (int)(step >> 16), to = (int)((step >> 8) & 0xff), n = (int)(step & 0xff);
    kit_img_set(pet, &wf_pet_sprites[n % 2 ? to : from][WF_EXPR_IDLE], PIX);
    lv_obj_set_style_image_recolor(pet, lv_color_white(), 0);
    lv_obj_set_style_image_recolor_opa(pet, 255, 0);
    n++;
    lv_timer_set_user_data(t, (void *)(uintptr_t)((from << 16) | (to << 8) | n));
    lv_timer_set_period(t, (uint32_t)(n < 6 ? 260 : n < 12 ? 140 : 70));
    if (n >= 18) {
        lv_obj_set_style_image_recolor_opa(pet, 0, 0);
        lv_timer_delete(t);
        lv_timer_t *done = lv_timer_create(evolve_done, 50, NULL);
        lv_timer_set_repeat_count(done, 1);
    }
}

static void start_evolution(void) {
    if (hatch_running) return;
    int from = ui.g.pet.form;
    int to = wf_evolve(&ui.g);
    if (to < 0) return;
    hatch_running = true;
    mark_dirty();
    wf_ui_flush_save();
    kit_toast("Cosa sta succedendo?!", icon(WF_ICON_SPARKLE));
    lv_timer_create(flicker, 300, (void *)(uintptr_t)((from << 16) | (to << 8)));
}

static void hatch_done(lv_timer_t *t) {
    lv_timer_delete(t);
    hatch_running = false;
    if (!pet || ui.screen != WF_SCR_HOME) return;
    shown_form = -1;
    refresh();
    kit_flash(pet, 0xffffff, 700);
    kit_burst(scr, icon(WF_ICON_HEART), PET_X, PET_Y - 20, 8);
    set_expr(WF_EXPR_HAPPY, 3000);
    char body[128];
    snprintf(body, sizeof body, "%s è nato! Dagli da mangiare, giocaci e fallo riposare.", ui.g.pet.name);
    cut_dialog = kit_dialog("Benvenuto!", body, NULL);
    kit_dialog_button(cut_dialog, "Ciao!", COL_BTN_OK, close_cut, 0);
}

/* ---- Interaction ------------------------------------------------------------ */
static void on_pet(lv_event_t *e) {
    (void)e;
    uint32_t now = now_s();
    if (hatch_running) return;
    if (wf_pet_stage(&ui.g) == WF_STAGE_SEED) {
        kit_shake(pet, 6);
        kit_burst(scr, icon(WF_ICON_SPARKLE), PET_X, PET_Y, 2);
        if (wf_warm_seed(&ui.g, now)) {
            hatch_running = true;
            mark_dirty();
            wf_ui_flush_save();
            kit_flash(pet, 0xffffff, 900);
            lv_timer_t *t = lv_timer_create(hatch_done, 900, NULL);
            lv_timer_set_repeat_count(t, 1);
        }
        refresh();
        return;
    }
    if (ui.g.pet.pending_form) {
        start_evolution();
        return;
    }
    if (ui.g.pet.sleeping) {
        kit_toast("Shh... sta dormendo", icon(WF_ICON_ZZZ));
        return;
    }
    int r = wf_pat(&ui.g, now);
    set_expr(WF_EXPR_HAPPY, 1400);
    apply_pet_frame();
    kit_shake(pet, 3);
    if (r == WF_OK) {
        kit_burst(scr, icon(WF_ICON_HEART), PET_X, PET_Y - 40, 5);
        mark_dirty();
        refresh();
    }
}

static void on_mess(lv_event_t *e) {
    intptr_t i = (intptr_t)lv_event_get_user_data(e);
    if (wf_clean(&ui.g, now_s()) == WF_OK) {
        kit_burst(scr, icon(WF_ICON_SPARKLE), (int)lv_obj_get_x(mess[i]) + 24, (int)lv_obj_get_y(mess[i]) + 20, 3);
        ui.g.coins += 1;
        mark_dirty();
        refresh();
    }
}

static void on_gift(lv_event_t *e) {
    (void)e;
    int item;
    int coins = wf_daily_claim(&ui.g, &item);
    if (!coins) return;
    char body[96];
    snprintf(body, sizeof body, "Giorno %d di fila!\n+%d monete e %s", ui.g.streak, coins, wf_item_get(item)->name);
    cut_dialog = kit_dialog("Regalo del giorno", body, icon(WF_ICON_GIFT));
    kit_dialog_button(cut_dialog, "Grazie!", COL_BTN_OK, close_cut, 0);
    mark_dirty();
    refresh();
}

static void on_collect(lv_event_t *e) {
    (void)e;
    wf_haul h;
    if (wf_expedition_collect(&ui.g, now_s(), &h) != WF_OK) return;
    char body[160];
    int n = snprintf(body, sizeof body, "+%lu PE  +%lu monete", (unsigned long)h.xp, (unsigned long)h.coins);
    for (int i = 0; i < WF_IT_COUNT && n < (int)sizeof body - 24; i++)
        if (h.items[i]) n += snprintf(body + n, sizeof body - (size_t)n, "\n%s x%d", wf_item_get(i)->name, h.items[i]);
    if (h.levels) snprintf(body + n, sizeof body - (size_t)n, "\nSale al livello %d!", ui.g.pet.level);
    cut_dialog = kit_dialog("Bentornato!", body, icon(WF_ICON_MAP));
    kit_dialog_button(cut_dialog, "Ottimo", COL_BTN_OK, close_cut, 0);
    set_expr(WF_EXPR_HAPPY, 2500);
    mark_dirty();
    wf_ui_flush_save();
    refresh();
}

static void on_menu(lv_event_t *e) {
    (void)e;
    if (wf_pet_stage(&ui.g) == WF_STAGE_SEED) {
        kit_toast("Prima fallo schiudere!", icon(WF_ICON_EGG));
        return;
    }
    go(WF_SCR_MENU);
}

void scr_home(void) {
    int hour = ui_hour();
    int scene = hour >= 7 && hour < 18 ? WF_SCENE_HOME_DAY : hour >= 18 && hour < 21 ? WF_SCENE_HOME_DUSK
              : WF_SCENE_HOME_NIGHT;
    scr = pet_screen(scene);
    shown_form = -1;
    expr_override = -1;
    hatch_running = false;
    night_dim = kit_dim(scr, 110);
    lv_obj_remove_flag(night_dim, LV_OBJ_FLAG_CLICKABLE);

    static const uint32_t colors[3] = {COL_HUNGER, COL_ENERGY, COL_MOOD};
    static const int icons_[3] = {WF_ICON_FOOD, WF_ICON_BOLT, WF_ICON_SMILE};
    static const float mids[3] = {222.5f, 270.0f, 317.5f};
    for (int i = 0; i < 3; i++) {
        int start = (int)mids[i] - 21, end = (int)mids[i] + 21;
        arcs[i] = kit_arc(scr, 226, 18, start, end, colors[i], 0, 100);
        lv_obj_t *ic = kit_img(scr, icon(icons_[i]), 2, 0, 0);
        pos_on_ring(ic, 190, mids[i]);
    }
    xp_arc = kit_arc(scr, 226, 14, 52, 128, COL_XP, 0, 1);
    lvl_l = kit_label(scr, &wf_font_small, COL_TEXT, "", CX, 436);
    lv_obj_set_style_bg_color(lvl_l, lv_color_hex(COL_PANEL), 0);
    lv_obj_set_style_bg_opa(lvl_l, 200, 0);
    lv_obj_set_style_pad_hor(lvl_l, 8, 0);
    lv_obj_set_style_radius(lvl_l, 6, 0);

    name_l = kit_label(scr, &wf_font_big, COL_TEXT, ui.g.pet.name, CX, 104);
    lv_obj_set_style_bg_color(name_l, lv_color_hex(COL_PANEL), 0);
    lv_obj_set_style_bg_opa(name_l, 210, 0);
    lv_obj_set_style_pad_hor(name_l, 12, 0);
    lv_obj_set_style_pad_ver(name_l, 4, 0);
    lv_obj_set_style_radius(name_l, 10, 0);
    status_l = kit_label(scr, &wf_font_small, COL_GOLD, "", CX, 142);
    lv_obj_set_style_bg_color(status_l, lv_color_hex(COL_PANEL), 0);
    lv_obj_set_style_bg_opa(status_l, 190, 0);
    lv_obj_set_style_pad_hor(status_l, 8, 0);
    lv_obj_set_style_pad_ver(status_l, 2, 0);
    lv_obj_set_style_radius(status_l, 6, 0);

    static const int mess_xy[WF_MESS_MAX][2] = {{142, 338}, {326, 342}, {196, 352}};
    for (int i = 0; i < WF_MESS_MAX; i++) {
        mess[i] = lv_image_create(scr);
        lv_image_set_src(mess[i], gfx_sprite(icon(WF_ICON_MESS), 3));
        kit_pos(mess[i], mess_xy[i][0], mess_xy[i][1]);
        lv_obj_add_flag(mess[i], LV_OBJ_FLAG_CLICKABLE);
        lv_obj_set_ext_click_area(mess[i], 12);
        lv_obj_add_event_cb(mess[i], on_mess, LV_EVENT_CLICKED, (void *)(intptr_t)i);
    }

    pet = lv_image_create(scr);
    kit_pos(pet, PET_X, PET_Y);
    lv_obj_add_flag(pet, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(pet, on_pet, LV_EVENT_CLICKED, NULL);
    apply_pet_frame();
    kit_bob(pet, 4, 1400);
    evo_spark = kit_img(scr, icon(WF_ICON_SPARKLE), 3, PET_X + 60, PET_Y - 70);
    kit_bob(evo_spark, 8, 700);

    seed_bar = kit_bar(scr, CX, 352, 150, 16, COL_HUNGER, 0, 24);

    gift_btn = kit_round_button(scr, icon(WF_ICON_GIFT), NULL, 98, 250, 62, COL_BTN_ALT, on_gift, 0);
    kit_bob(gift_btn, 6, 900);

    away_panel = kit_panel(scr, CX, 262, 250, 120);
    lv_obj_t *sign = lv_image_create(away_panel);
    lv_image_set_src(sign, gfx_sprite(icon(WF_ICON_MAP), 2));
    lv_obj_align(sign, LV_ALIGN_TOP_MID, 0, 0);
    away_l = lv_label_create(away_panel);
    lv_obj_set_style_text_font(away_l, &wf_font_small, 0);
    lv_obj_set_style_text_color(away_l, lv_color_hex(COL_TEXT), 0);
    lv_obj_set_style_text_align(away_l, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(away_l, LV_ALIGN_CENTER, 0, 14);
    away_btn = kit_text_button(scr, "Accogli", CX, 346, 140, 46, COL_BTN_OK, on_collect, 0);

    kit_round_button(scr, icon(WF_ICON_SPARKLE), NULL, CX, 392, 56, COL_GOLD, on_menu, 0);
    lv_obj_move_foreground(lvl_l);

    last_refresh = 0;
    next_blink = lv_tick_get() + 2500;
    next_zzz = 0;
    refresh();

    switch (ui.pending_anim) {
    case ANIM_EAT:
        set_expr(WF_EXPR_EAT, 1800);
        kit_burst(scr, icon(WF_ICON_HEART), PET_X, PET_Y - 30, 3);
        break;
    case ANIM_HAPPY:
        set_expr(WF_EXPR_HAPPY, 2000);
        kit_burst(scr, icon(WF_ICON_SPARKLE), PET_X, PET_Y - 30, 5);
        break;
    case ANIM_SAD:
        set_expr(WF_EXPR_SAD, 2000);
        break;
    default:
        break;
    }
    ui.pending_anim = ANIM_NONE;
    apply_pet_frame();
}

void home_tick(void) {
    uint32_t t = lv_tick_get();
    if (expr_override >= 0 && (int32_t)(t - expr_until) >= 0 && !hatch_running) {
        expr_override = -1;
        apply_pet_frame();
    }
    if (!hatch_running && expr_override < 0 && (int32_t)(t - next_blink) >= 0 && !ui.g.pet.sleeping) {
        set_expr(WF_EXPR_BLINK, 140);
        apply_pet_frame();
        next_blink = t + 2600 + wf_platform_random() % 2500;
    }
    if (ui.g.pet.sleeping && (int32_t)(t - next_zzz) >= 0 && !wf_pet_away(&ui.g, now_s())) {
        kit_float_text(scr, "z", COL_DIM, &wf_font_big, PET_X + 50, PET_Y - 60);
        next_zzz = t + 1400;
    }
    if (t - last_refresh >= 1000 && !hatch_running) {
        last_refresh = t;
        refresh();
    }
}
