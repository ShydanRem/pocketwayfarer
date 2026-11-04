/* Minigames: "Pioggia di stelle" (play) and ring training. */
#include <math.h>
#include <stdio.h>
#include <string.h>
#include "ui_internal.h"
#include "ring.h"

static void on_home(lv_event_t *e) { (void)e; go(WF_SCR_HOME); }

/* ---- Pioggia di stelle -------------------------------------------------------------- */
#define STARS 10
#define PLAY_MS 20000u

typedef struct {
    lv_obj_t *obj;
    int32_t x, y;      /* x100 */
    int32_t vy, vx;
    uint8_t kind;      /* 0 star, 1 golden, 2 storm cloud */
    bool active;
} star;

static star stars[STARS];
static lv_obj_t *play_scr, *play_pet, *score_l, *timer_arc;
static uint32_t play_start, next_spawn, last_step, pet_happy_until;
static int score, combo;
static bool play_over;
static lv_obj_t *play_dialog;

static void finish_play(void);

static void on_star(lv_event_t *e) {
    star *s = lv_event_get_user_data(e);
    if (!s->active || play_over) return;
    s->active = false;
    int x = (int)(s->x / 100), y = (int)(s->y / 100);
    lv_obj_add_flag(s->obj, LV_OBJ_FLAG_HIDDEN);
    if (s->kind == 2) {
        score = score > 2 ? score - 2 : 0;
        combo = 0;
        kit_float_text(play_scr, "-2", COL_BAD, &wf_font_big, x, y);
        kit_shake(play_pet, 6);
    } else {
        combo++;
        int pts = (s->kind == 1 ? 3 : 1) + (combo >= 5 ? 1 : 0);
        score += pts;
        char buf[16];
        snprintf(buf, sizeof buf, combo >= 5 ? "+%d combo!" : "+%d", pts);
        kit_float_text(play_scr, buf, s->kind == 1 ? COL_GOLD : COL_TEXT, &wf_font_big, x, y);
        kit_burst(play_scr, icon(WF_ICON_SPARKLE), x, y, s->kind == 1 ? 4 : 2);
        pet_happy_until = lv_tick_get() + 500;
    }
    char buf[16];
    snprintf(buf, sizeof buf, "%d", score);
    lv_label_set_text(score_l, buf);
}

static void spawn(void) {
    for (int i = 0; i < STARS; i++) {
        star *s = &stars[i];
        if (s->active) continue;
        uint32_t r = wf_platform_random();
        int kind = r % 100 < 12 ? 1 : r % 100 < 24 ? 2 : 0;
        s->kind = (uint8_t)kind;
        s->x = (90 + (int32_t)((r >> 8) % 286)) * 100;
        s->y = 70 * 100;
        uint32_t elapsed = lv_tick_get() - play_start;
        s->vy = 150 + (int32_t)(elapsed / 120) + (int32_t)((r >> 16) % 80);   /* px*100 per 16 ms */
        s->vx = (int32_t)((r >> 20) % 61) - 30;
        lv_image_set_src(s->obj, gfx_sprite(icon(kind == 2 ? WF_ICON_STORM : WF_ICON_STAR), 3));
        lv_obj_remove_flag(s->obj, LV_OBJ_FLAG_HIDDEN);
        lv_obj_set_pos(s->obj, s->x / 100 - 24, s->y / 100 - 24);
        s->active = true;
        return;
    }
}

void scr_play(void) {
    play_scr = pet_screen(WF_SCENE_HOME_NIGHT);
    memset(stars, 0, sizeof stars);
    score = combo = 0;
    play_over = false;
    play_dialog = NULL;
    play_pet = kit_img(play_scr, &wf_pet_sprites[ui.g.pet.form][WF_EXPR_IDLE], PIX, CX, 300);
    for (int i = 0; i < STARS; i++) {
        stars[i].obj = lv_image_create(play_scr);
        lv_image_set_src(stars[i].obj, gfx_sprite(icon(WF_ICON_STAR), 3));
        lv_obj_add_flag(stars[i].obj, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_HIDDEN);
        lv_obj_set_ext_click_area(stars[i].obj, 18);
        lv_obj_add_event_cb(stars[i].obj, on_star, LV_EVENT_PRESSED, &stars[i]);
    }
    timer_arc = kit_arc(play_scr, 226, 14, 0, 360, COL_GOLD, 1000, 1000);
    lv_arc_set_rotation(timer_arc, 270);
    lv_arc_set_bg_angles(timer_arc, 0, 360);
    score_l = kit_label(play_scr, &wf_font_huge, COL_GOLD, "0", CX, 70);
    kit_label(play_scr, &wf_font_small, COL_TEXT, "Tocca le stelle, evita le nuvole!", CX, 112);
    play_start = lv_tick_get();
    next_spawn = play_start + 400;
    last_step = play_start;
}

static lv_obj_t *done_dialog;
static void on_play_done(lv_event_t *e) {
    (void)e;
    kit_dialog_close(done_dialog);
    go_home_anim(ANIM_HAPPY);
}

static void finish_play(void) {
    play_over = true;
    int coins = 0;
    int r = wf_play_finish(&ui.g, score, now_s(), &coins);
    mark_dirty();
    char body[96];
    if (r == WF_OK) snprintf(body, sizeof body, "%d stelle prese!\nUmore alle stelle, +%d monete.", score, coins);
    else snprintf(body, sizeof body, "%s", wf_result_text(r));
    done_dialog = kit_dialog(score >= 25 ? "Fantastico!" : score >= 12 ? "Bravo!" : "Fine!", body, icon(WF_ICON_STAR));
    kit_dialog_button(done_dialog, "Continua", COL_BTN_OK, on_play_done, 0);
}

void play_tick(void) {
    if (play_over) return;
    uint32_t t = lv_tick_get();
    uint32_t elapsed = t - play_start;
    if (elapsed >= PLAY_MS) {
        for (int i = 0; i < STARS; i++) lv_obj_add_flag(stars[i].obj, LV_OBJ_FLAG_HIDDEN);
        finish_play();
        return;
    }
    lv_arc_set_value(timer_arc, (int32_t)(1000 - elapsed * 1000 / PLAY_MS));
    if ((int32_t)(t - next_spawn) >= 0) {
        spawn();
        uint32_t gap = 700 - (elapsed / 50);
        next_spawn = t + (gap < 320 ? 320 : gap);
    }
    uint32_t dt = t - last_step;
    if (dt < 16) return;
    last_step = t;
    for (int i = 0; i < STARS; i++) {
        star *s = &stars[i];
        if (!s->active) continue;
        s->y += s->vy * (int32_t)dt / 16;
        s->x += s->vx * (int32_t)dt / 16;
        int x = (int)(s->x / 100), y = (int)(s->y / 100);
        int dx = x - CX, dy = y - CY;
        if (y > 380 || dx * dx + dy * dy > 215 * 215) {
            s->active = false;
            lv_obj_add_flag(s->obj, LV_OBJ_FLAG_HIDDEN);
            if (s->kind != 2) combo = 0;
            continue;
        }
        lv_obj_set_pos(s->obj, x - 24, y - 24);
    }
    kit_img_set(play_pet, &wf_pet_sprites[ui.g.pet.form][(int32_t)(t - pet_happy_until) < 0 ? WF_EXPR_HAPPY : WF_EXPR_IDLE], PIX);
}

/* ---- Training ------------------------------------------------------------------------- */
static ring_widget tring;
static lv_obj_t *train_scr, *train_pet, *swing_l, *train_dialog;
static int train_stat = -1, swings, quality_sum;
static uint32_t next_swing_at;

static void train_next(void);

static void on_train_tap(lv_event_t *e) {
    (void)e;
    if (!tring.active) return;
    int q = ring_judge(&tring);
    ring_close(&tring);
    quality_sum += q;
    swings++;
    quality_popup(train_scr, q, CX, 180);
    if (q) {
        kit_shake(train_pet, 4 + q * 2);
        kit_burst(train_scr, icon(q == 3 ? WF_ICON_STAR : WF_ICON_SPARKLE), CX, 260, q + 1);
    }
    char buf[32];
    snprintf(buf, sizeof buf, "Colpo %d/5", swings < 5 ? swings + 1 : 5);
    lv_label_set_text(swing_l, buf);
    next_swing_at = lv_tick_get() + 650;
}

static void on_train_done(lv_event_t *e) {
    (void)e;
    kit_dialog_close(train_dialog);
    go_home_anim(ANIM_HAPPY);
}

static void train_next(void) {
    if (swings >= 5) {
        int gain = 0;
        int r = wf_train_finish(&ui.g, train_stat, quality_sum, now_s(), &gain);
        mark_dirty();
        static const char *const names[] = {"Attacco", "Difesa", "Velocità", "Vigore"};
        char body[96];
        if (r != WF_OK) snprintf(body, sizeof body, "%s", wf_result_text(r));
        else if (gain) snprintf(body, sizeof body, "%s +%d!\nPrecisione %d/15", names[train_stat], gain, quality_sum);
        else snprintf(body, sizeof body, "Nessun progresso stavolta.\n(o hai raggiunto il limite del livello)");
        train_dialog = kit_dialog("Allenamento", body, icon(WF_ICON_DUMBBELL));
        kit_dialog_button(train_dialog, "Ok", COL_BTN_OK, on_train_done, 0);
        next_swing_at = 0;
        return;
    }
    wf_rng r;
    wf_rng_seed(&r, wf_platform_random());
    wf_ring ring;
    wf_stats st = wf_pet_stats(&ui.g);
    wf_ring_make(&ring, &r, st.spd, ui.g.pet.level / 2 + swings * 3, wf_need_percent(ui.g.pet.mood));
    ring_open(&tring, train_scr, &ring, on_train_tap);
}

static void on_pick(lv_event_t *e) {
    train_stat = (int)(intptr_t)lv_event_get_user_data(e);
    scr_train();
}

void scr_train(void) {
    train_scr = pet_screen(WF_SCENE_ARENA_MEADOW);
    memset(&tring, 0, sizeof tring);
    if (train_stat < 0) {
        kit_dim(train_scr, 120);
        kit_label(train_scr, &wf_font_big, COL_GOLD, "Cosa alleniamo?", CX, 90);
        static const char *const names[] = {"Attacco", "Difesa", "Velocità", "Vigore"};
        static const int icons_[] = {WF_ICON_SWORD, WF_ICON_SHIELD, WF_ICON_BOLT, WF_ICON_HEART};
        static const int xy[4][2] = {{150, 190}, {316, 190}, {150, 310}, {316, 310}};
        for (int i = 0; i < 4; i++)
            kit_round_button(train_scr, icon(icons_[i]), names[i], xy[i][0], xy[i][1], 84, 0x8a5ae8, on_pick, i);
        kit_label(train_scr, &wf_font_small, COL_DIM, "Tocca quando la scintilla\nattraversa l'arco dorato", CX, 396);
        kit_round_button(train_scr, icon(WF_ICON_BACK), NULL, CX, 440, 44, COL_PANEL_HI, on_home, 0);
        return;
    }
    train_pet = kit_img(train_scr, &wf_pet_sprites[ui.g.pet.form][WF_EXPR_IDLE], PIX, CX, 270);
    kit_img(train_scr, icon(WF_ICON_DUMBBELL), 3, CX, 110);
    swing_l = kit_label(train_scr, &wf_font_big, COL_TEXT, "Colpo 1/5", CX, 150);
    swings = quality_sum = 0;
    next_swing_at = lv_tick_get() + 500;
}

void train_tick(void) {
    if (train_stat < 0) return;
    if (tring.active) {
        ring_update(&tring);
        if (ring_expired(&tring)) on_train_tap(NULL);
        return;
    }
    if (next_swing_at && (int32_t)(lv_tick_get() - next_swing_at) >= 0) {
        next_swing_at = 0;
        train_next();
        if (swings >= 5) train_stat = -2;   /* finished: keep dialog, no new swings */
    }
}

/* Reset the stat picker whenever training is entered from the menu. */
void train_reset(void) { train_stat = -1; }
