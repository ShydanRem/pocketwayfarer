/* Battles: wild encounters (with parry) and linked duels (lock-step rounds). */
#include <math.h>
#include <stdio.h>
#include <string.h>
#include "ui_internal.h"
#include "ring.h"

enum { PH_INTRO, PH_CHOOSE, PH_RING, PH_ANIM, PH_TELEGRAPH, PH_PARRY, PH_WAIT, PH_END };

static struct {
    bool duel;
    int me;                 /* side of the battle shown bottom-left */
    wf_battle solo;
    wf_battle *b;
    int phase;
    int action;
    uint32_t phase_at;
    int ev_index;
    uint32_t next_ev_at;
    bool after_player;      /* solo: animating the player's half of the round */
    lv_obj_t *scr, *spr[2], *hp_bar[2], *hp_l[2], *name_l[2], *focus[3], *buttons[5], *info, *items_dlg, *end_dlg;
    int shown_hp[2];
    ring_widget ring;
    parry_widget parry;
    int last_quality;
} B;

#define ME_X 150
#define ME_Y 318
#define FOE_X 284

static int foe_y(void) {
    const wf_fighter *f = &B.b->f[!B.me];
    return f->is_pet ? 174 : B.b->boss ? 156 : 182;
}
static int side_x(int side) { return side == B.me ? ME_X : FOE_X; }
static int side_y(int side) { return side == B.me ? ME_Y : foe_y(); }

static const wf_sprite *side_sprite(int side, bool attack) {
    const wf_fighter *f = &B.b->f[side];
    if (f->is_pet) {
        int expr = f->hp <= 0 ? WF_EXPR_SAD : attack ? WF_EXPR_HAPPY : WF_EXPR_IDLE;
        return &wf_pet_sprites[f->sprite % WF_FORM_COUNT][expr];
    }
    return &wf_foe_sprites[f->sprite % WF_SP_COUNT][attack ? 1 : 0];
}

static void update_side(int side, bool anim) {
    const wf_fighter *f = &B.b->f[side];
    kit_bar_set(B.hp_bar[side], f->hp, f->max_hp, anim);
    char buf[40];
    snprintf(buf, sizeof buf, "PV %d/%d", f->hp, f->max_hp);
    lv_label_set_text(B.hp_l[side], buf);
    int pct = f->max_hp ? f->hp * 100 / f->max_hp : 0;
    lv_obj_set_style_bg_color(B.hp_bar[side], lv_color_hex(pct > 50 ? COL_HP : pct > 20 ? COL_GOLD : COL_BAD),
                              LV_PART_INDICATOR);
    if (side == B.me)
        for (int i = 0; i < WF_FOCUS_MAX; i++)
            lv_obj_set_style_bg_color(B.focus[i], lv_color_hex(i < f->focus ? 0x5ad8ff : 0x2a3266), 0);
}

static void show_buttons(bool on) {
    for (int i = 0; i < 5; i++) {
        if (!B.buttons[i]) continue;
        if (on) lv_obj_remove_flag(B.buttons[i], LV_OBJ_FLAG_HIDDEN);
        else lv_obj_add_flag(B.buttons[i], LV_OBJ_FLAG_HIDDEN);
    }
    if (on && B.buttons[1]) {
        if (B.b->f[B.me].focus >= WF_SKILL_COST) lv_obj_remove_state(B.buttons[1], LV_STATE_DISABLED);
        else lv_obj_add_state(B.buttons[1], LV_STATE_DISABLED);
    }
}

static void set_info(const char *text) {
    if (!text) {
        lv_obj_add_flag(B.info, LV_OBJ_FLAG_HIDDEN);
        return;
    }
    lv_label_set_text(lv_obj_get_child(B.info, 0), text);
    lv_obj_remove_flag(B.info, LV_OBJ_FLAG_HIDDEN);
}

static void phase(int p) {
    B.phase = p;
    B.phase_at = lv_tick_get();
}

/* ---- Lunge animation ---------------------------------------------------------- */
static void anim_tx(void *o, int32_t v) { lv_obj_set_style_translate_x(o, v, 0); }
static void anim_ty(void *o, int32_t v) { lv_obj_set_style_translate_y(o, v, 0); }

static void lunge(int side) {
    lv_obj_t *o = B.spr[side];
    int dx = side == B.me ? 60 : -60, dy = side == B.me ? -50 : 50;
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, o);
    lv_anim_set_duration(&a, 130);
    lv_anim_set_playback_duration(&a, 200);
    lv_anim_set_path_cb(&a, lv_anim_path_ease_out);
    lv_anim_set_exec_cb(&a, anim_tx);
    lv_anim_set_values(&a, 0, dx);
    lv_anim_start(&a);
    lv_anim_set_exec_cb(&a, anim_ty);
    lv_anim_set_values(&a, 0, dy);
    lv_anim_start(&a);
}

/* ---- Event playback --------------------------------------------------------------- */
static int play_event(const wf_event *e) {
    int actor = e->actor, target = !actor;
    char buf[40];
    switch (e->kind) {
    case WF_EV_HIT:
        lunge(actor);
        kit_img_set(B.spr[actor], side_sprite(actor, true), PIX);
        kit_flash(B.spr[target], 0xffffff, 260);
        kit_shake(B.spr[target], e->quality >= 3 ? 10 : 6);
        if (e->value) snprintf(buf, sizeof buf, "-%d", e->value);
        else snprintf(buf, sizeof buf, "Parato!");
        kit_float_text(B.scr, buf, e->quality >= 3 ? COL_GOLD : COL_TEXT, e->quality >= 3 ? &wf_font_huge : &wf_font_big,
                       side_x(target), side_y(target) - 50);
        if (e->effect == 1) kit_float_text(B.scr, "Superefficace!", COL_HUNGER, &wf_font_small, side_x(target), side_y(target) + 10);
        if (e->effect == 2) kit_float_text(B.scr, "Poco efficace", COL_DIM, &wf_font_small, side_x(target), side_y(target) + 10);
        if (e->skill) kit_burst(B.scr, icon(WF_ICON_BURST), side_x(target), side_y(target), 4);
        update_side(target, true);
        return 520;
    case WF_EV_GUARD:
        kit_burst(B.scr, icon(WF_ICON_SHIELD), side_x(actor), side_y(actor) - 30, 2);
        kit_float_text(B.scr, "In guardia", 0x9ac8ff, &wf_font_small, side_x(actor), side_y(actor) - 60);
        return 450;
    case WF_EV_HEAL:
        snprintf(buf, sizeof buf, "+%d", e->value);
        kit_float_text(B.scr, buf, COL_HP, &wf_font_big, side_x(actor), side_y(actor) - 50);
        kit_burst(B.scr, icon(WF_ICON_HEART), side_x(actor), side_y(actor), 3);
        update_side(actor, true);
        return 450;
    case WF_EV_BURN:
        snprintf(buf, sizeof buf, "-%d scotta!", e->value);
        kit_float_text(B.scr, buf, COL_HUNGER, &wf_font_small, side_x(actor), side_y(actor) - 40);
        kit_flash(B.spr[actor], 0xff6a1a, 300);
        update_side(actor, true);
        return 450;
    case WF_EV_IGNITE:
        kit_float_text(B.scr, "In fiamme!", COL_HUNGER, &wf_font_small, side_x(target), side_y(target) + 30);
        return 300;
    case WF_EV_ROOT:
        kit_float_text(B.scr, "Radicato!", COL_XP, &wf_font_small, side_x(target), side_y(target) + 30);
        return 300;
    case WF_EV_COUNTER:
        lunge(actor);
        snprintf(buf, sizeof buf, "Contrattacco -%d", e->value);
        kit_float_text(B.scr, buf, COL_GOLD, &wf_font_small, side_x(target), side_y(target) - 40);
        kit_flash(B.spr[target], 0xffffff, 240);
        update_side(target, true);
        return 520;
    case WF_EV_FOCUS:
        update_side(actor, false);
        return 0;
    case WF_EV_NO_FOCUS:
        kit_float_text(B.scr, "Focus insufficiente", COL_DIM, &wf_font_small, side_x(actor), side_y(actor) - 60);
        return 300;
    case WF_EV_KO:
        kit_img_set(B.spr[actor], side_sprite(actor, false), PIX);
        lv_obj_set_style_opa(B.spr[actor], 110, 0);
        return 600;
    case WF_EV_FLEE_OK:
        kit_float_text(B.scr, "Fuga riuscita!", COL_TEXT, &wf_font_big, CX, 200);
        return 600;
    case WF_EV_FLEE_FAIL:
        kit_float_text(B.scr, "Non riesce a fuggire!", COL_BAD, &wf_font_small, ME_X, ME_Y - 60);
        return 500;
    default:
        return 0;
    }
}

static void start_events(void) {
    B.ev_index = 0;
    B.next_ev_at = lv_tick_get() + 120;
    phase(PH_ANIM);
}

/* ---- Results ------------------------------------------------------------------------ */
static void on_end(lv_event_t *e) {
    (void)e;
    kit_dialog_close(B.end_dlg);
    if (B.duel) {
        wf_link_duel_leave(&ui.link, lv_tick_get());
        go(WF_SCR_LINK);
        return;
    }
    if (B.b->state == WF_B_WON) go_home_anim(ANIM_HAPPY);
    else if (B.b->state == WF_B_LOST) go_home_anim(ANIM_SAD);
    else go(WF_SCR_MAP);
}

static void finish(void) {
    phase(PH_END);
    show_buttons(false);
    set_info(NULL);
    char body[160];
    const char *title;
    if (B.duel) {
        bool won = (B.b->state == WF_B_WON) == (B.me == 0);
        wf_link_duel_record(&ui.link, &ui.g, now_s());
        title = won ? "Vittoria!" : "Sconfitta";
        snprintf(body, sizeof body, won ? "Hai battuto %s!\n+PE e +10 monete" : "%s ha vinto.\nLa prossima volta!",
                 B.b->f[!B.me].name);
    } else {
        bool unlocked = false;
        uint32_t xp = 0, coins = 0;
        int levels = wf_battle_finish(&ui.g, ui.zone, B.b, &unlocked, &xp, &coins);
        if (B.b->state == WF_B_WON) {
            title = "Vittoria!";
            int n = snprintf(body, sizeof body, "+%lu PE   +%lu monete", (unsigned long)xp, (unsigned long)coins);
            if (levels) n += snprintf(body + n, sizeof body - (size_t)n, "\nSale al livello %d!", ui.g.pet.level);
            if (ui.g.pet.pending_form) n += snprintf(body + n, sizeof body - (size_t)n, "\nSta per cambiare forma...");
            if (unlocked) snprintf(body + n, sizeof body - (size_t)n, "\nNuova zona: %s!", wf_zone_get(ui.g.zone_unlocked)->name);
        } else if (B.b->state == WF_B_LOST) {
            title = "Sconfitta";
            snprintf(body, sizeof body, "%s è esausto.\nRiposa, mangia e riprova!", ui.g.pet.name);
        } else {
            title = "Fuga";
            snprintf(body, sizeof body, "Meglio tornare più forti.");
        }
    }
    mark_dirty();
    wf_ui_flush_save();
    B.end_dlg = kit_dialog(title, body, B.b->state == WF_B_WON ? icon(WF_ICON_TROPHY) : NULL);
    kit_dialog_button(B.end_dlg, "Continua", COL_BTN_OK, on_end, 0);
}

/* ---- Player input --------------------------------------------------------------------- */
static void on_ring_tap(lv_event_t *e) {
    (void)e;
    if (B.phase != PH_RING) return;
    int q = ring_judge(&B.ring);
    ring_close(&B.ring);
    set_info(NULL);
    B.last_quality = q;
    quality_popup(B.scr, q, CX, 120);
    if (B.duel) {
        wf_link_duel_submit(&ui.link, B.action, q, lv_tick_get());
        if (B.phase == PH_RING) {           /* not already resolved by the submit */
            char buf[48];
            snprintf(buf, sizeof buf, "In attesa di %s...", B.b->f[!B.me].name);
            set_info(buf);
            phase(PH_WAIT);
        }
        return;
    }
    wf_battle_player(B.b, B.action, q);
    B.after_player = true;
    start_events();
}

static void begin_ring(void) {
    wf_rng r;
    wf_rng_seed(&r, wf_platform_random());
    wf_ring ring;
    const wf_fighter *me = &B.b->f[B.me], *foe = &B.b->f[!B.me];
    wf_ring_make(&ring, &r, me->spd, foe->level, B.duel ? 60 : wf_need_percent(ui.g.pet.mood));
    show_buttons(false);
    set_info(B.action == WF_ACT_SKILL ? "Abilità! Tocca sull'arco" : "Tocca sull'arco dorato!");
    ring_open(&B.ring, B.scr, &ring, on_ring_tap);
    phase(PH_RING);
}

static void act(int action) {
    if (B.phase != PH_CHOOSE) return;
    B.action = action;
    if (action == WF_ACT_STRIKE || action == WF_ACT_SKILL) {
        begin_ring();
        return;
    }
    show_buttons(false);
    if (B.duel) {
        wf_link_duel_submit(&ui.link, WF_ACT_GUARD, 0, lv_tick_get());
        if (B.phase == PH_CHOOSE) {
            char buf[48];
            snprintf(buf, sizeof buf, "In attesa di %s...", B.b->f[!B.me].name);
            set_info(buf);
            phase(PH_WAIT);
        }
        return;
    }
    if (action == WF_ACT_POTION || action == WF_ACT_TONIC) {
        int item = action == WF_ACT_POTION ? WF_IT_POTION : WF_IT_TONIC;
        ui.g.inv[item]--;
        mark_dirty();
    }
    wf_battle_player(B.b, action, 0);
    B.after_player = true;
    start_events();
}

static void on_action(lv_event_t *e) {
    int a = (int)(intptr_t)lv_event_get_user_data(e);
    if (a == WF_ACT_SKILL && B.b->f[B.me].focus < WF_SKILL_COST) {
        kit_toast("Serve 2 Focus: attacca o difenditi", NULL);
        return;
    }
    act(a);
}

static void on_item_pick(lv_event_t *e) {
    int a = (int)(intptr_t)lv_event_get_user_data(e);
    kit_dialog_close(B.items_dlg);
    B.items_dlg = NULL;
    if (a < 0) return;
    int item = a == WF_ACT_POTION ? WF_IT_POTION : WF_IT_TONIC;
    if (!ui.g.inv[item]) {
        kit_toast("Non ne hai", &wf_item_icons[item]);
        return;
    }
    act(a);
}

static void on_items(lv_event_t *e) {
    (void)e;
    if (B.phase != PH_CHOOSE) return;
    char body[64];
    snprintf(body, sizeof body, "Pozioni: %d   Tonici: %d", ui.g.inv[WF_IT_POTION], ui.g.inv[WF_IT_TONIC]);
    B.items_dlg = kit_dialog("Zaino", body, icon(WF_ICON_BAG));
    kit_dialog_button(B.items_dlg, "Pozione", COL_BTN_ALT, on_item_pick, WF_ACT_POTION);
    kit_dialog_button(B.items_dlg, "Tonico", COL_BTN, on_item_pick, WF_ACT_TONIC);
    kit_dialog_button(B.items_dlg, "X", COL_PANEL_HI, on_item_pick, -1);
}

static void on_parry_tap(lv_event_t *e) {
    (void)e;
    if (B.phase != PH_PARRY) return;
    int q = parry_judge(&B.parry);
    parry_close(&B.parry);
    set_info(NULL);
    static const char *const words[] = {"Colpito!", "Parata", "Gran parata!", "PARATA PERFETTA!"};
    kit_float_text(B.scr, words[q], q == 3 ? COL_GOLD : q ? COL_ENERGY : COL_BAD, q == 3 ? &wf_font_big : &wf_font_small,
                   CX, 150);
    wf_battle_foe(B.b, q);
    B.after_player = false;
    start_events();
}

/* ---- Screen ---------------------------------------------------------------------------- */
static void side_panel(int side, int x, int y, int bar_w) {
    const wf_fighter *f = &B.b->f[side];
    char buf[40];
    snprintf(buf, sizeof buf, "%s  Lv%d", f->name, f->level);
    B.name_l[side] = kit_label(B.scr, &wf_font_small, COL_TEXT, buf, x, y - 21);
    lv_obj_set_style_bg_color(B.name_l[side], lv_color_hex(COL_PANEL), 0);
    lv_obj_set_style_bg_opa(B.name_l[side], 210, 0);
    lv_obj_set_style_pad_hor(B.name_l[side], 8, 0);
    lv_obj_set_style_pad_ver(B.name_l[side], 2, 0);
    lv_obj_set_style_radius(B.name_l[side], 6, 0);
    B.hp_bar[side] = kit_bar(B.scr, x, y, bar_w, 18, COL_HP, f->hp, f->max_hp);
    B.hp_l[side] = kit_label(B.scr, &wf_font_small, COL_INK, "", x, y);
}

static void build(int scene) {
    B.scr = pet_screen(scene);
    memset(&B.ring, 0, sizeof B.ring);
    memset(&B.parry, 0, sizeof B.parry);
    B.items_dlg = B.end_dlg = NULL;
    int foe = !B.me;
    B.spr[foe] = kit_img(B.scr, side_sprite(foe, false), PIX, FOE_X, foe_y());
    B.spr[B.me] = kit_img(B.scr, side_sprite(B.me, false), PIX, ME_X, ME_Y);
    kit_bob(B.spr[foe], 4, 1200);
    kit_bob(B.spr[B.me], 3, 1500);
    kit_fade_in(B.spr[foe], 400, 0);
    side_panel(foe, CX, 76, 160);
    side_panel(B.me, 220, 424, 120);
    for (int i = 0; i < WF_FOCUS_MAX; i++) {
        B.focus[i] = lv_obj_create(B.scr);
        lv_obj_remove_style_all(B.focus[i]);
        lv_obj_set_size(B.focus[i], 14, 14);
        lv_obj_set_style_radius(B.focus[i], 3, 0);
        lv_obj_set_style_bg_opa(B.focus[i], LV_OPA_COVER, 0);
        lv_obj_set_style_border_color(B.focus[i], lv_color_hex(COL_INK), 0);
        lv_obj_set_style_border_width(B.focus[i], 2, 0);
        kit_pos(B.focus[i], 298 + i * 18, 424);
    }
    static const float ang[4] = {-32, -4, 24, 52};
    static const int icons_[4] = {WF_ICON_SWORD, WF_ICON_BURST, WF_ICON_SHIELD, WF_ICON_BAG};
    static const char *const names[4] = {"Attacca", "Abilità", "Difendi", "Zaino"};
    static const uint32_t colors[4] = {0xe8503a, 0xe8a03a, 0x3a7ae8, 0x8a6a4a};
    for (int i = 0; i < 4; i++) {
        float a = ang[i] * 3.14159265f / 180.0f;
        int x = CX + (int)lroundf(170 * cosf(a)), y = CY + (int)lroundf(170 * sinf(a));
        bool duel_item = B.duel && i == 3;
        B.buttons[i] = kit_round_button(B.scr, icon(icons_[i]), NULL, x, y, 68, colors[i],
                                        i == 3 ? on_items : on_action,
                                        i == 0 ? WF_ACT_STRIKE : i == 1 ? WF_ACT_SKILL : WF_ACT_GUARD);
        (void)names;
        if (duel_item) {
            lv_obj_delete(B.buttons[i]);
            B.buttons[i] = NULL;
        }
    }
    if (!B.duel && !B.b->boss)
        B.buttons[4] = kit_round_button(B.scr, icon(WF_ICON_RUN), NULL, 64, 236, 54, COL_PANEL_HI, on_action, WF_ACT_FLEE);
    else
        B.buttons[4] = NULL;
    B.info = kit_pill(B.scr, "", CX, 128);
    update_side(0, false);
    update_side(1, false);
    show_buttons(false);
}

void start_battle(int zone, const wf_step *step) {
    ui.zone = zone;
    ui.step = *step;
    wf_fighter me, foe;
    wf_fighter_from_pet(&me, &ui.g);
    wf_foe_make(&foe, step->species, step->level);
    wf_battle_start(&B.solo, &me, &foe, wf_platform_random(), step->kind == WF_STEP_BOSS, false);
    go(WF_SCR_BATTLE);
}

void scr_battle(void) {
    B.duel = false;
    B.me = 0;
    B.b = &B.solo;
    build(WF_SCENE_ARENA_MEADOW + wf_zone_get(ui.zone)->scene);
    char buf[64];
    snprintf(buf, sizeof buf, B.b->boss ? "Il guardiano %s!" : "Un %s selvatico!", B.b->f[1].name);
    set_info(buf);
    phase(PH_INTRO);
}

void scr_duel(void) {
    B.duel = true;
    B.me = wf_link_duel_me(&ui.link);
    B.b = &ui.link.duel.battle;
    build(WF_SCENE_ARENA_SKY);
    char buf[64];
    snprintf(buf, sizeof buf, "Duello contro %s!", B.b->f[!B.me].name);
    set_info(buf);
    phase(PH_INTRO);
}

void duel_on_event(const wf_link_event *e) {
    (void)e;
    if (!B.duel) return;
    if (B.phase == PH_RING) ring_close(&B.ring);
    show_buttons(false);
    set_info(NULL);
    start_events();
}

static void next_turn(void) {
    if (wf_battle_over(B.b)) {
        finish();
        return;
    }
    for (int s = 0; s < 2; s++) kit_img_set(B.spr[s], side_sprite(s, false), PIX);
    if (!B.duel && B.b->state == WF_B_FOE) {
        kit_img_set(B.spr[1], side_sprite(1, true), PIX);
        char buf[64];
        snprintf(buf, sizeof buf, B.b->foe_intent == WF_ACT_SKILL ? "%s carica un'abilità!" : "%s attacca!",
                 B.b->f[1].name);
        set_info(buf);
        phase(PH_TELEGRAPH);
        return;
    }
    set_info(B.duel ? "Scegli la mossa" : NULL);
    show_buttons(true);
    phase(PH_CHOOSE);
}

void battle_tick(void) {
    uint32_t t = lv_tick_get();
    switch (B.phase) {
    case PH_INTRO:
        if (t - B.phase_at > 1100) {
            set_info(NULL);
            next_turn();
        }
        break;
    case PH_RING:
        ring_update(&B.ring);
        if (ring_expired(&B.ring)) on_ring_tap(NULL);
        break;
    case PH_ANIM:
        if ((int32_t)(t - B.next_ev_at) < 0) break;
        if (B.ev_index < B.b->ev_count) {
            int d = play_event(&B.b->ev[B.ev_index++]);
            B.next_ev_at = t + (uint32_t)d;
        } else if (t - B.next_ev_at > 200) {
            next_turn();
        }
        break;
    case PH_TELEGRAPH:
        if (t - B.phase_at > 800) {
            set_info("Tocca quando l'onda raggiunge lo scudo!");
            wf_rng r;
            wf_rng_seed(&r, wf_platform_random());
            wf_parry p;
            wf_parry_make(&p, &r, B.b->f[0].spd, B.b->f[1].spd, B.b->boss);
            uint32_t color = B.b->foe_intent == WF_ACT_SKILL ? 0xff6a3a : 0xff4f7a;
            parry_open(&B.parry, B.scr, &p, color, icon(WF_ICON_SHIELD), on_parry_tap);
            phase(PH_PARRY);
        }
        break;
    case PH_PARRY:
        parry_update(&B.parry);
        if (parry_expired(&B.parry)) on_parry_tap(NULL);
        break;
    default:
        break;
    }
}

void duel_tick(void) { battle_tick(); }

bool battle_key(int key) {
    /* BOOT acts as "tap" during timing phases: handy with gloves or wet fingers. */
    if (key != WF_KEY_BOOT_SHORT) return B.phase != PH_END;
    if (B.phase == PH_RING) { on_ring_tap(NULL); return true; }
    if (B.phase == PH_PARRY) { on_parry_tap(NULL); return true; }
    return true;
}

/* Test hooks used by the host preview to tap at a chosen moment. */
int battle_ring_quality_now(void) { return B.phase == PH_RING ? ring_judge(&B.ring) : -1; }
int battle_parry_quality_now(void) { return B.phase == PH_PARRY ? parry_judge(&B.parry) : -1; }
