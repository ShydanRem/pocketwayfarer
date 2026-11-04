/* RPG battle screen. */
#include <math.h>
#include <stdio.h>
#include <string.h>
#include "rui.h"

enum { PH_INTRO, PH_CHOOSE, PH_SKILLS, PH_RING, PH_ANIM, PH_TELEGRAPH, PH_PARRY, PH_END };
enum { AFTER_HERO, AFTER_NEXT_FOE, AFTER_TELEGRAPH, AFTER_FOE, AFTER_ROUND };

static struct {
    rpg_battle b;
    rpg_foe start_foes[RPG_MAX_FOES];
    bool dungeon, boss;
    int phase, after;
    uint32_t phase_at, next_ev_at;
    int ev_index;
    int target;
    rpg_action pending;
    bool skill_ring;
    lv_obj_t *scr, *foe_img[RPG_MAX_FOES], *foe_bar[RPG_MAX_FOES], *foe_lbl[RPG_MAX_FOES], *marker;
    lv_obj_t *hero, *summon, *hp_arc, *sp_arc, *vitals, *info, *buttons[6], *skill_layer, *end_dlg, *item_dlg;
    int foe_x[RPG_MAX_FOES], foe_y[RPG_MAX_FOES], scale[RPG_MAX_FOES];
    ring_widget ring;
    parry_widget parry;
    hero_look look;
} B;

#define HERO_X CX
#define HERO_Y 336

void r_start_battle(const rpg_foe *foes, int n, bool dungeon, bool boss) {
    memset(&B.b, 0, sizeof B.b);
    memcpy(B.start_foes, foes, sizeof(rpg_foe) * (size_t)n);
    int flags = (dungeon ? B_FLAG_DUNGEON : 0) | (boss ? B_FLAG_BOSS : 0);
    rpg_battle_start(&B.b, &R.g, foes, n, wf_platform_random(), flags);
    B.dungeon = dungeon;
    B.boss = boss;
    r_go(R_BATTLE);
}

static void phase(int p) { B.phase = p; B.phase_at = lv_tick_get(); }

static void set_info(const char *t) {
    if (!t) { lv_obj_add_flag(B.info, LV_OBJ_FLAG_HIDDEN); return; }
    lv_label_set_text(lv_obj_get_child(B.info, 0), t);
    lv_obj_remove_flag(B.info, LV_OBJ_FLAG_HIDDEN);
}

static void update_vitals(void) {
    r_vitals_update(B.hp_arc, B.sp_arc, B.b.h.hp, B.b.h.st.max_hp, B.b.h.sp, B.b.h.st.max_sp);
    char buf[48];
    snprintf(buf, sizeof buf, "%s %ld/%ld · %s %ld", TR("PV", "HP"), (long)B.b.h.hp, (long)B.b.h.st.max_hp, TR("PM", "SP"), (long)B.b.h.sp);
    lv_label_set_text(B.vitals, buf);
}

static void update_foe(int i) {
    const rpg_foe *f = &B.b.foe[i];
    kit_bar_set(B.foe_bar[i], f->hp, f->max_hp, true);
    if (f->hp <= 0) {
        lv_obj_add_flag(B.foe_bar[i], LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(B.foe_lbl[i], LV_OBJ_FLAG_HIDDEN);
    }
}

static void place_marker(void) {
    B.target = rpg_battle_target(&B.b, B.target);
    if (B.target < 0) { lv_obj_add_flag(B.marker, LV_OBJ_FLAG_HIDDEN); return; }
    lv_obj_remove_flag(B.marker, LV_OBJ_FLAG_HIDDEN);
    int h = (int)rpg_monsters[B.b.foe[B.target].species][0].h * B.scale[B.target];
    kit_pos(B.marker, B.foe_x[B.target], B.foe_y[B.target] - h / 2 - 14);
}

static void show_buttons(bool on) {
    for (int i = 0; i < 6; i++) {
        if (!B.buttons[i]) continue;
        if (on) lv_obj_remove_flag(B.buttons[i], LV_OBJ_FLAG_HIDDEN);
        else lv_obj_add_flag(B.buttons[i], LV_OBJ_FLAG_HIDDEN);
    }
    lv_obj_remove_flag(B.buttons[5], LV_OBJ_FLAG_HIDDEN);    /* AUTO always visible */
}

/* ---- Animations --------------------------------------------------------------------- */
static void anim_tx(void *o, int32_t v) { lv_obj_set_style_translate_x(o, v, 0); }
static void anim_ty(void *o, int32_t v) { lv_obj_set_style_translate_y(o, v, 0); }
static void anim_opa(void *o, int32_t v) { lv_obj_set_style_opa(o, (lv_opa_t)v, 0); }

static void lunge(lv_obj_t *o, int dx, int dy) {
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, o);
    lv_anim_set_duration(&a, 120);
    lv_anim_set_playback_duration(&a, 180);
    lv_anim_set_exec_cb(&a, anim_tx);
    lv_anim_set_values(&a, 0, dx);
    lv_anim_start(&a);
    lv_anim_set_exec_cb(&a, anim_ty);
    lv_anim_set_values(&a, 0, dy);
    lv_anim_start(&a);
}

static void fade_out(lv_obj_t *o) {
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, o);
    lv_anim_set_duration(&a, 500);
    lv_anim_set_exec_cb(&a, anim_opa);
    lv_anim_set_values(&a, 255, 0);
    lv_anim_start(&a);
}

static int tx(int who) { return who < 0 ? HERO_X : who == 3 ? HERO_X - 92 : B.foe_x[who]; }
static int ty(int who) { return who < 0 ? HERO_Y - 40 : who == 3 ? HERO_Y : B.foe_y[who] - 20; }

static uint32_t status_color(int s) {
    static const uint32_t c[STS_COUNT] = {COL_TEXT, 0xff7a2a, 0x9aff5a, 0x8ae8ff, 0xffe04a, 0xff4a5a, 0xc8a0ff, 0x8a8aa8};
    return c[s % STS_COUNT];
}

static void hero_face(int expr, int ms) {
    hero_obj_set(B.hero, &B.look, expr, 3);
    (void)ms;
}

static int play(const rpg_event *e) {
    char buf[48];
    switch (e->kind) {
    case EV_HIT:
    case EV_CRIT_HIT:
        if (e->actor < 0 || e->actor == 3) {
            lv_obj_t *t = B.foe_img[e->target];
            lunge(e->actor == 3 && B.summon ? B.summon : B.hero, (B.foe_x[e->target] - HERO_X) / 4, -40);
            kit_flash(t, 0xffffff, 240);
            kit_shake(t, e->kind == EV_CRIT_HIT ? 10 : 5);
            snprintf(buf, sizeof buf, "-%ld", (long)e->value);
            kit_float_text(B.scr, buf, e->kind == EV_CRIT_HIT ? COL_GOLD : COL_TEXT,
                           e->kind == EV_CRIT_HIT ? &wf_font_huge : &wf_font_big, tx(e->target), ty(e->target));
            if (e->kind == EV_CRIT_HIT) kit_float_text(B.scr, TR("CRITICO!", "CRITICAL!"), COL_GOLD, &wf_font_small, tx(e->target), ty(e->target) + 36);
            else if (e->effect == 1) kit_float_text(B.scr, TR("Superefficace!", "Super effective!"), COL_HUNGER, &wf_font_small, tx(e->target), ty(e->target) + 36);
            else if (e->effect == 2) kit_float_text(B.scr, TR("Resiste", "Resisted"), COL_DIM, &wf_font_small, tx(e->target), ty(e->target) + 36);
            if (e->skill != 0xff) kit_burst(B.scr, &rpg_skill_icons[rpg_skill(e->skill)->kind], tx(e->target), ty(e->target), 2);
            update_foe(e->target);
            return 420;
        }
        lunge(B.foe_img[e->actor], (HERO_X - B.foe_x[e->actor]) / 4, 40);
        lv_obj_set_style_image_recolor(B.hero, lv_color_hex(0xff4a4a), 0);
        kit_flash(B.hero, 0xff4a4a, 260);
        kit_shake(B.hero, 6);
        hero_face(HX_HURT, 0);
        snprintf(buf, sizeof buf, "-%ld", (long)e->value);
        kit_float_text(B.scr, buf, e->value ? COL_BAD : COL_ENERGY, &wf_font_big, HERO_X + 40, HERO_Y - 70);
        if (e->quality == 3) kit_float_text(B.scr, TR("Parata perfetta!", "Perfect parry!"), COL_GOLD, &wf_font_small, HERO_X, HERO_Y - 110);
        update_vitals();
        return 420;
    case EV_MISS:
        kit_float_text(B.scr, TR("Mancato", "Miss"), COL_DIM, &wf_font_small, tx(e->target), ty(e->target));
        return 300;
    case EV_HEAL:
        if (e->actor < 0) {
            if (e->value) snprintf(buf, sizeof buf, "+%ld", (long)e->value);
            else snprintf(buf, sizeof buf, "%s", TR("Curato", "Cured"));
            kit_float_text(B.scr, buf, COL_HP, &wf_font_big, HERO_X - 40, HERO_Y - 70);
            kit_burst(B.scr, ri(RI_HEART), HERO_X, HERO_Y - 30, 3);
            hero_face(HX_HAPPY, 0);
            update_vitals();
        } else {
            snprintf(buf, sizeof buf, "+%ld", (long)e->value);
            kit_float_text(B.scr, buf, COL_HP, &wf_font_big, tx(e->target), ty(e->target));
            update_foe(e->target);
        }
        return 380;
    case EV_SP:
        snprintf(buf, sizeof buf, "+%ld %s", (long)e->value, TR("PM", "SP"));
        kit_float_text(B.scr, buf, 0x5aa0ff, &wf_font_small, HERO_X + 40, HERO_Y - 70);
        update_vitals();
        return 300;
    case EV_BUFF:
        kit_burst(B.scr, ri(RI_SPARKLE), HERO_X, HERO_Y - 30, 4);
        hero_face(HX_HAPPY, 0);
        return 380;
    case EV_STATUS:
        kit_float_text(B.scr, TX(rpg_status_name(e->effect)), status_color(e->effect), &wf_font_small,
                       e->target < 0 ? HERO_X : tx(e->target), (e->target < 0 ? HERO_Y - 100 : ty(e->target) + 30));
        return 300;
    case EV_DOT:
        snprintf(buf, sizeof buf, "-%ld", (long)e->value);
        kit_float_text(B.scr, buf, status_color(e->effect), &wf_font_small, e->target < 0 ? HERO_X : tx(e->target),
                       e->target < 0 ? HERO_Y - 80 : ty(e->target));
        if (e->target < 0) update_vitals(); else update_foe(e->target);
        return 320;
    case EV_DODGE:
        lunge(B.hero, -40, 0);
        kit_float_text(B.scr, TR("Schivato!", "Dodged!"), COL_ENERGY, &wf_font_big, HERO_X, HERO_Y - 90);
        return 380;
    case EV_COUNTER:
        lunge(B.hero, (B.foe_x[e->target] - HERO_X) / 4, -40);
        snprintf(buf, sizeof buf, TR("Contrattacco -%ld", "Counter -%ld"), (long)e->value);
        kit_float_text(B.scr, buf, COL_GOLD, &wf_font_small, tx(e->target), ty(e->target));
        kit_flash(B.foe_img[e->target], 0xffffff, 240);
        update_foe(e->target);
        return 420;
    case EV_KO:
        if (e->actor >= 0 && e->actor < RPG_MAX_FOES) {
            fade_out(B.foe_img[e->actor]);
            update_foe(e->actor);
            place_marker();
        } else if (e->actor < 0) {
            hero_face(HX_HURT, 0);
        }
        return 450;
    case EV_SUMMON:
        if (!B.summon) {
            const wf_sprite *sp = rpg_skill(e->skill)->element == EL_DARK ? &rpg_monsters[SP_SKELETON][0] : &rpg_monsters[SP_WOLF][0];
            B.summon = kit_img(B.scr, sp, 2, HERO_X - 92, HERO_Y + 6);
        }
        lv_obj_set_style_opa(B.summon, 255, 0);
        kit_flash(B.summon, 0xffffff, 400);
        return 450;
    case EV_SUMMON_HIT:
        if (B.summon) lunge(B.summon, (B.foe_x[e->target] - HERO_X + 92) / 4, -40);
        kit_flash(B.foe_img[e->target], 0xffffff, 240);
        snprintf(buf, sizeof buf, "-%ld", (long)e->value);
        kit_float_text(B.scr, buf, COL_TEXT, &wf_font_big, tx(e->target), ty(e->target));
        update_foe(e->target);
        if (!B.b.h.summon_turns && B.summon) fade_out(B.summon);
        return 380;
    case EV_GUARD:
        kit_burst(B.scr, ri(RI_SHIELD), HERO_X, HERO_Y - 30, 2);
        update_vitals();
        return 300;
    case EV_FLEE_OK:
        kit_float_text(B.scr, TR("Fuga riuscita!", "Got away!"), COL_TEXT, &wf_font_big, CX, 200);
        return 500;
    case EV_FLEE_FAIL:
        kit_float_text(B.scr, TR("Non riesci a fuggire!", "Can't escape!"), COL_BAD, &wf_font_small, HERO_X, HERO_Y - 90);
        return 400;
    case EV_EXTRA:
        kit_float_text(B.scr, TR("Azione extra!", "Extra action!"), COL_ENERGY, &wf_font_big, HERO_X, HERO_Y - 110);
        return 400;
    case EV_SKILL:
        snprintf(buf, sizeof buf, "%s", TX(rpg_skill(e->skill)->name));
        set_info(buf);
        update_vitals();
        return 250;
    case EV_NO_SP:
        kit_float_text(B.scr, TR("Non disponibile", "Not available"), COL_DIM, &wf_font_small, HERO_X, HERO_Y - 90);
        return 250;
    case EV_FOE_SKILL:
        return 0;
    case EV_FROZEN:
        kit_float_text(B.scr, TX(rpg_status_name(e->effect)), status_color(e->effect), &wf_font_small,
                       e->actor < 0 ? HERO_X : tx(e->actor), e->actor < 0 ? HERO_Y - 90 : ty(e->actor));
        return 380;
    default:
        return 0;
    }
}

static void play_events(int after) {
    B.after = after;
    B.ev_index = 0;
    B.next_ev_at = lv_tick_get() + 80;
    phase(PH_ANIM);
}

/* ---- End -------------------------------------------------------------------------------- */
static void on_end(lv_event_t *e) {
    (void)e;
    kit_dialog_close(B.end_dlg);
    B.end_dlg = NULL;
    int st = B.b.state;
    if (B.dungeon) { r_after_dungeon_battle(st); return; }
    r_save_now();
    r_go(st == B_LOST ? R_TOWN : R_ZONE);
}

static void finish(void) {
    phase(PH_END);
    show_buttons(false);
    set_info(NULL);
    rpg_loot loot;
    rpg_battle_finish(&R.g, &B.b, &loot);
    char body[300], name[48];
    const char *title;
    const wf_sprite *ic = NULL;
    if (B.b.state == B_WON) {
        hero_face(HX_HAPPY, 0);
        title = TR("Vittoria!", "Victory!");
        ic = ri(RI_TROPHY);
        int n = snprintf(body, sizeof body, TR("+%lu PE  +%lu PE classe\n+%lu oro", "+%lu XP  +%lu job XP\n+%lu gold"),
                         (unsigned long)loot.xp, (unsigned long)loot.job_xp, (unsigned long)loot.gold);
        for (int i = 0; i < loot.nitems && n < (int)sizeof body - 60; i++)
            n += snprintf(body + n, sizeof body - (size_t)n, "\n%s (%s)", rpg_item_name(&loot.items[i], name, sizeof name),
                          TX(rpg_rarity_name(loot.items[i].rarity)));
        if (loot.gem) n += snprintf(body + n, sizeof body - (size_t)n, TR("\nGemma d'anima: %s!", "\nSoul gem: %s!"), TX(rpg_species(loot.gem - 1)->name));
        if (loot.cons[CO_ORE]) n += snprintf(body + n, sizeof body - (size_t)n, TR("\nMinerale x%d", "\nOre x%d"), loot.cons[CO_ORE]);
        if (loot.levels) n += snprintf(body + n, sizeof body - (size_t)n, TR("\nLIVELLO %d!", "\nLEVEL %d!"), R.g.hero.level);
        if (loot.job_levels) snprintf(body + n, sizeof body - (size_t)n, TR("\nLivello classe %d!", "\nJob level %d!"), R.g.hero.job_level);
    } else if (B.b.state == B_LOST) {
        title = TR("Sconfitta", "Defeat");
        ic = ri(RI_SKULL);
        snprintf(body, sizeof body, "%s", TR("Sei crollato. Riposa alla locanda e riprova!", "You collapsed. Rest at the inn and try again!"));
    } else {
        title = TR("Fuga", "Escaped");
        snprintf(body, sizeof body, "%s", TR("Meglio tornare più forti.", "Better come back stronger."));
    }
    B.end_dlg = kit_dialog(title, body, ic);
    kit_dialog_button(B.end_dlg, TR("Continua", "Continue"), COL_BTN_OK, on_end, 0);
}

/* ---- Turn flow ------------------------------------------------------------------------------- */
static void hero_turn(void) {
    if (rpg_battle_over(&B.b)) { finish(); return; }
    hero_face(HX_IDLE, 0);
    if (B.b.h.status == STS_STUN || B.b.h.status == STS_FREEZE) {
        rpg_action a = {0};
        rpg_battle_hero(&B.b, &R.g, &a, 0);
        play_events(AFTER_HERO);
        return;
    }
    place_marker();
    set_info(B.b.round == 0 && B.phase == PH_INTRO ? NULL : NULL);
    show_buttons(true);
    phase(PH_CHOOSE);
}

static void next_foe(void) {
    bool acting = rpg_battle_next_foe(&B.b);
    play_events(acting ? AFTER_NEXT_FOE : AFTER_ROUND);
}

static void after_events(void) {
    set_info(NULL);
    switch (B.after) {
    case AFTER_HERO:
        if (rpg_battle_over(&B.b)) finish();
        else if (B.b.state == B_HERO) hero_turn();
        else next_foe();
        break;
    case AFTER_NEXT_FOE: {
        int i = B.b.acting;
        kit_img_set(B.foe_img[i], &rpg_monsters[B.b.foe[i].species][1], B.scale[i]);
        static const wf_text moves[] = {{"attacca", "attacks"}, {"colpo pesante!", "heavy blow!"}, {"raffica!", "flurry!"},
                                        {"soffio!", "breath!"}, {"si cura", "heals"}, {"maledizione!", "curse!"}};
        char buf[64];
        snprintf(buf, sizeof buf, "%s: %s", TX(rpg_species(B.b.foe[i].species)->name), TX(moves[B.b.foe_move % 6]));
        set_info(buf);
        phase(PH_TELEGRAPH);
        break;
    }
    case AFTER_FOE:
        kit_img_set(B.foe_img[B.b.acting], &rpg_monsters[B.b.foe[B.b.acting].species][0], B.scale[B.b.acting]);
        if (rpg_battle_over(&B.b)) finish();
        else next_foe();
        break;
    default:   /* AFTER_ROUND */
        if (rpg_battle_over(&B.b)) finish();
        else hero_turn();
        break;
    }
}

static void do_hero(const rpg_action *a, int q) {
    show_buttons(false);
    rpg_battle_hero(&B.b, &R.g, a, q);
    play_events(AFTER_HERO);
}

static void on_ring_tap(lv_event_t *e) {
    (void)e;
    if (B.phase != PH_RING) return;
    int q = ring_judge(&B.ring);
    ring_close(&B.ring);
    quality_popup(B.scr, q, CX, 110);
    do_hero(&B.pending, q);
}

static bool needs_ring(const rpg_action *a) {
    if (a->kind == ACT_ATTACK) return true;
    if (a->kind != ACT_SKILL) return false;
    const rpg_skill_info *k = rpg_skill(a->skill);
    return k->kind <= SK_NOVA || k->kind == SK_DRAIN || (k->kind == SK_DEBUFF && k->power);
}

static void act(const rpg_action *a) {
    B.pending = *a;
    B.pending.target = (uint8_t)(B.target < 0 ? 0 : B.target);
    if (R.g.auto_battle || !needs_ring(&B.pending)) {
        do_hero(&B.pending, WF_Q_GOOD);
        return;
    }
    wf_rng r;
    wf_rng_seed(&r, wf_platform_random());
    wf_ring ring;
    int lvl = 0;
    for (int i = 0; i < B.b.nfoes; i++) if (B.b.foe[i].level > lvl) lvl = B.b.foe[i].level;
    wf_ring_make(&ring, &r, B.b.h.st.timing * 3 + 10, lvl / 2, R.g.hero.morale);
    show_buttons(false);
    set_info(TR("Tocca sull'arco dorato!", "Tap on the golden arc!"));
    ring_open(&B.ring, B.scr, &ring, on_ring_tap);
    phase(PH_RING);
}

static void close_skills(void) {
    if (B.skill_layer) { lv_obj_delete(B.skill_layer); B.skill_layer = NULL; }
}

static void on_skill(lv_event_t *e) {
    int s = (int)(intptr_t)lv_event_get_user_data(e);
    close_skills();
    if (s < 0) { show_buttons(true); phase(PH_CHOOSE); return; }
    rpg_action a = {.kind = ACT_SKILL, .skill = (uint8_t)s};
    act(&a);
}

static void open_skills(void) {
    show_buttons(false);
    B.skill_layer = kit_dim(B.scr, 150);
    lv_obj_add_event_cb(B.skill_layer, on_skill, LV_EVENT_CLICKED, (void *)(intptr_t)-1);
    const rpg_hero_b *h = &B.b.h;
    int list[9], n = 0;
    for (int i = 0; i < h->nskills; i++) {
        int s = h->skills[i];
        if (rpg_skill(s)->kind != SK_PASSIVE && R.g.hero.skill[s]) list[n++] = i;
    }
    if (!n) {
        kit_label(B.skill_layer, &wf_font_small, COL_TEXT, TR("Nessuna abilità attiva:\nimparale dalla scheda Eroe", "No active skills:\nlearn them from the Hero sheet"), CX, CY);
    }
    for (int k = 0; k < n; k++) {
        int i = list[k], s = h->skills[i], lv = R.g.hero.skill[s];
        float ang = (-90.0f + 360.0f / (float)(n < 4 ? 4 : n) * (float)k) * 3.14159265f / 180.0f;
        int x = CX + (int)lroundf(140 * cosf(ang)), y = CY - 10 + (int)lroundf(140 * sinf(ang));
        bool ready = !h->cooldown[i] && h->sp >= rpg_skill_cost(s, lv);
        const rpg_skill_info *ki = rpg_skill(s);
        lv_obj_t *b = kit_round_button(B.skill_layer, &rpg_skill_icons[ki->kind], NULL, x, y, 66,
                                       ready ? (ki->element ? 0x8a5ae8 : COL_BTN) : 0x3a3a4a, ready ? on_skill : NULL, s);
        char buf[48];
        if (h->cooldown[i]) snprintf(buf, sizeof buf, "%s\n(%d)", TX(ki->name), h->cooldown[i]);
        else snprintf(buf, sizeof buf, "%s\n%d %s", TX(ki->name), rpg_skill_cost(s, lv), TR("PM", "SP"));
        lv_obj_t *l = kit_label(B.skill_layer, &wf_font_small, ready ? COL_TEXT : COL_DIM, buf, x, y + 50);
        lv_obj_set_style_bg_color(l, lv_color_hex(COL_PANEL), 0);
        lv_obj_set_style_bg_opa(l, 210, 0);
        lv_obj_set_style_radius(l, 6, 0);
        (void)b;
    }
    phase(PH_SKILLS);
}

static void on_item_pick(lv_event_t *e) {
    int c = (int)(intptr_t)lv_event_get_user_data(e);
    kit_dialog_close(B.item_dlg);
    B.item_dlg = NULL;
    if (c < 0 || !R.g.cons[c]) { if (c >= 0) kit_toast(rpg_err(RPG_ERR_NONE), NULL); return; }
    rpg_action a = {.kind = ACT_ITEM, .item = (uint8_t)c};
    act(&a);
}

static void on_button(lv_event_t *e) {
    int which = (int)(intptr_t)lv_event_get_user_data(e);
    if (B.phase != PH_CHOOSE && which != 5) return;
    rpg_action a = {0};
    switch (which) {
    case 0: a.kind = ACT_ATTACK; act(&a); break;
    case 1: open_skills(); break;
    case 2: {
        char body[96];
        snprintf(body, sizeof body, TR("Pozioni %d · Gran pozioni %d\nEteri %d · Antidoti %d", "Potions %d · Hi-potions %d\nEthers %d · Antidotes %d"),
                 R.g.cons[CO_POTION], R.g.cons[CO_HIPOTION], R.g.cons[CO_ETHER], R.g.cons[CO_ANTIDOTE]);
        B.item_dlg = kit_dialog(TR("Zaino", "Bag"), body, ri(RI_BAG));
        kit_dialog_button(B.item_dlg, TR("Pozione", "Potion"), COL_BTN_ALT, on_item_pick, CO_POTION);
        kit_dialog_button(B.item_dlg, TR("Gran p.", "Hi-pot."), COL_BTN_ALT, on_item_pick, CO_HIPOTION);
        kit_dialog_button(B.item_dlg, TR("Etere", "Ether"), COL_BTN, on_item_pick, CO_ETHER);
        kit_dialog_button(B.item_dlg, TR("Antidoto", "Antidote"), COL_BTN_OK, on_item_pick, CO_ANTIDOTE);
        kit_dialog_button(B.item_dlg, "X", COL_PANEL_HI, on_item_pick, -1);
        break;
    }
    case 3: a.kind = ACT_GUARD; act(&a); break;
    case 4: a.kind = ACT_FLEE; act(&a); break;
    case 5:
        R.g.auto_battle ^= 1;
        lv_obj_set_style_bg_grad_color(B.buttons[5], lv_color_hex(R.g.auto_battle ? COL_BTN_OK : COL_PANEL_HI), 0);
        lv_obj_set_style_bg_color(B.buttons[5], lv_color_hex(R.g.auto_battle ? 0x8ae8a0 : 0x4a5288), 0);
        kit_toast(R.g.auto_battle ? TR("AUTO attivo", "AUTO on") : TR("AUTO spento", "AUTO off"), ri(RI_AUTO));
        r_dirty();
        break;
    }
}

static void on_foe(lv_event_t *e) {
    int i = (int)(intptr_t)lv_event_get_user_data(e);
    if (B.b.foe[i].hp <= 0) return;
    B.target = i;
    place_marker();
}

static void on_parry_tap(lv_event_t *e) {
    (void)e;
    if (B.phase != PH_PARRY) return;
    int q = parry_judge(&B.parry);
    parry_close(&B.parry);
    rpg_battle_foe(&B.b, q);
    play_events(AFTER_FOE);
}

/* ---- Screen --------------------------------------------------------------------------------- */
void scr_battle(void) {
    const rpg_zone_info *z = rpg_zone(R.g.run.active && B.dungeon ? R.g.run.zone : R.zone);
    int scene = B.dungeon ? RS_DUNGEON_CRYPT + z->dungeon_scene : RS_FIELD_MEADOW + z->field_scene;
    B.scr = kit_screen(&rpg_scenes[scene]);
    memset(&B.ring, 0, sizeof B.ring);
    memset(&B.parry, 0, sizeof B.parry);
    B.summon = NULL;
    B.skill_layer = NULL;
    B.end_dlg = B.item_dlg = NULL;
    int n = B.b.nfoes;
    static const int xs[3][3] = {{CX, 0, 0}, {150, 316, 0}, {112, CX, 354}};
    static const int ys[3][3] = {{176, 0, 0}, {180, 180, 0}, {190, 164, 190}};
    for (int i = 0; i < n; i++) {
        const rpg_foe *f = &B.b.foe[i];
        B.scale[i] = n == 3 && !f->boss ? 2 : 3;
        B.foe_x[i] = xs[n - 1][i];
        B.foe_y[i] = f->boss ? 158 : ys[n - 1][i];
        B.foe_img[i] = kit_img(B.scr, &rpg_monsters[f->species][0], B.scale[i], B.foe_x[i], B.foe_y[i]);
        lv_obj_add_flag(B.foe_img[i], LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_event_cb(B.foe_img[i], on_foe, LV_EVENT_CLICKED, (void *)(intptr_t)i);
        kit_bob(B.foe_img[i], 3, 1100 + i * 170);
        kit_fade_in(B.foe_img[i], 350, i * 120);
        int h = rpg_monsters[f->species][0].h * B.scale[i];
        int by = B.foe_y[i] + h / 2 + 10;
        B.foe_bar[i] = kit_bar(B.scr, B.foe_x[i], by, n == 3 ? 84 : 110, 12, COL_BAD, f->hp, f->max_hp);
        char buf[24];
        snprintf(buf, sizeof buf, "%sLv%d", f->elite ? "\xe2\x98\x85" : "", f->level);
        B.foe_lbl[i] = kit_label(B.scr, &wf_font_small, f->elite ? COL_GOLD : COL_TEXT, buf, B.foe_x[i], by + 18);
    }
    hero_look_from_game(&B.look, &R.g);
    B.hero = hero_obj(B.scr, &B.look, HX_IDLE, 3, HERO_X, HERO_Y);
    kit_bob(B.hero, 3, 1500);
    r_vitals_arcs(B.scr, &B.hp_arc, &B.sp_arc);
    B.vitals = kit_label(B.scr, &wf_font_small, COL_TEXT, "", CX, 436);
    B.marker = kit_img(B.scr, ri(RI_STAR), 2, 0, 0);
    kit_bob(B.marker, 5, 600);
    static const struct { int icon, x, y, size; uint32_t color; } btn[6] = {
        {RI_SWORD, 92, 280, 66, 0xe8503a}, {RI_SPARKLE, 126, 366, 66, 0x8a5ae8}, {RI_BAG, 340, 366, 66, 0xa8783a},
        {RI_SHIELD, 374, 280, 66, 0x3a7ae8}, {RI_RUN, 66, 196, 48, COL_PANEL_HI}, {RI_AUTO, 400, 196, 48, COL_PANEL_HI},
    };
    for (int i = 0; i < 6; i++) {
        B.buttons[i] = kit_round_button(B.scr, ri(btn[i].icon), NULL, btn[i].x, btn[i].y, btn[i].size, btn[i].color, on_button, i);
        lv_obj_set_style_bg_color(B.buttons[i], lv_color_lighten(lv_color_hex(btn[i].color), 40), 0);
    }
    if (B.boss || B.dungeon) {
        lv_obj_delete(B.buttons[4]);
        B.buttons[4] = NULL;
    }
    if (R.g.auto_battle) {
        lv_obj_set_style_bg_grad_color(B.buttons[5], lv_color_hex(COL_BTN_OK), 0);
        lv_obj_set_style_bg_color(B.buttons[5], lv_color_hex(0x8ae8a0), 0);
    }
    B.info = kit_pill(B.scr, "", CX, 52);
    B.target = 0;
    place_marker();
    update_vitals();
    show_buttons(false);
    char buf[64];
    if (n == 1) snprintf(buf, sizeof buf, "%s!", TX(rpg_species(B.b.foe[0].species)->name));
    else snprintf(buf, sizeof buf, TR("%d nemici!", "%d foes!"), n);
    set_info(buf);
    phase(PH_INTRO);
}

void battle_tick(void) {
    uint32_t t = lv_tick_get();
    switch (B.phase) {
    case PH_INTRO:
        if (t - B.phase_at > 900) { set_info(NULL); hero_turn(); }
        break;
    case PH_CHOOSE:
        if (R.g.auto_battle && t - B.phase_at > 350) {
            rpg_action a;
            rpg_battle_auto(&B.b, &R.g, &a);
            a.target = (uint8_t)(a.kind == ACT_SKILL && B.target >= 0 ? B.target : a.target);
            B.target = a.target;
            place_marker();
            B.pending = a;
            do_hero(&a, WF_Q_GOOD);
        }
        break;
    case PH_RING:
        ring_update(&B.ring);
        if (ring_expired(&B.ring)) on_ring_tap(NULL);
        break;
    case PH_ANIM:
        if ((int32_t)(t - B.next_ev_at) < 0) break;
        if (B.ev_index < B.b.ev_count) {
            int d = play(&B.b.ev[B.ev_index++]);
            if (R.g.auto_battle) d = d * 2 / 3;
            B.next_ev_at = t + (uint32_t)d;
        } else {
            after_events();
        }
        break;
    case PH_TELEGRAPH:
        if (t - B.phase_at > (R.g.auto_battle ? 350u : 650u)) {
            if (B.b.foe_dodge || R.g.auto_battle || B.b.foe_move == FOE_HEAL) {
                rpg_battle_foe(&B.b, WF_Q_GOOD);
                play_events(AFTER_FOE);
                break;
            }
            set_info(TR("Para: tocca quando l'onda arriva!", "Parry: tap when the wave lands!"));
            wf_rng r;
            wf_rng_seed(&r, wf_platform_random());
            wf_parry p;
            const rpg_foe *f = &B.b.foe[B.b.acting];
            wf_parry_make(&p, &r, B.b.h.st.stat[ST_AGI], f->agi, f->boss);
            uint32_t color = B.b.foe_move == FOE_HIT ? 0xff4f7a : 0xff8a3a;
            parry_open(&B.parry, B.scr, &p, color, ri(RI_SHIELD), on_parry_tap);
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

bool battle_key(int key) {
    if (key != WF_KEY_BOOT_SHORT) return B.phase != PH_END;
    if (B.phase == PH_RING) { on_ring_tap(NULL); return true; }
    if (B.phase == PH_PARRY) { on_parry_tap(NULL); return true; }
    if (B.phase == PH_SKILLS) { close_skills(); show_buttons(true); phase(PH_CHOOSE); return true; }
    return true;
}

/* Test hooks for the host preview. */
int rpg_battle_ring_quality_now(void) { return B.phase == PH_RING ? ring_judge(&B.ring) : -1; }
int rpg_battle_parry_quality_now(void) { return B.phase == PH_PARRY ? parry_judge(&B.parry) : -1; }
int rpg_battle_phase(void) { return B.phase; }
