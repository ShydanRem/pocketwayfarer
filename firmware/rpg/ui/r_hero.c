/* Hero sheet: stats, skills and class advancement. */
#include <stdio.h>
#include <string.h>
#include "rui.h"

static lv_obj_t *dlg;
static void close_dlg(lv_event_t *e) { (void)e; kit_dialog_close(dlg); dlg = NULL; }
static void on_go(lv_event_t *e) { r_go((int)(intptr_t)lv_event_get_user_data(e)); }

static void tabs(lv_obj_t *s, int active) {
    static const wf_text names[3] = {{"Statistiche", "Stats"}, {"Abilità", "Skills"}, {"Classe", "Class"}};
    static const int screens[3] = {R_HERO, R_SKILLS, R_CLASS};
    for (int i = 0; i < 3; i++)
        kit_text_button(s, TX(names[i]), CX - 112 + i * 112, 70, 106, 40, i == active ? COL_BTN : COL_PANEL_HI, on_go, screens[i]);
}

/* ---- Stats ---------------------------------------------------------------------- */
static void on_raise(lv_event_t *e) {
    int s = (int)(intptr_t)lv_event_get_user_data(e);
    if (rpg_raise_stat(&R.g, s)) {
        r_dirty();
        int32_t y = lv_obj_get_scroll_y(lv_obj_get_child(lv_screen_active(), 0));
        (void)y;
        scr_hero();
    }
}

void scr_hero(void) {
    lv_obj_t *s = kit_screen(NULL);
    lv_obj_set_style_bg_color(s, lv_color_hex(0x161430), 0);
    rpg_game *g = &R.g;
    rpg_stats st;
    rpg_stats_get(g, &st);
    hero_look l;
    hero_look_from_game(&l, g);
    hero_obj(s, &l, HX_IDLE, 2, 110, 160);
    char buf[160];
    snprintf(buf, sizeof buf, "%s\n%s\n%s  Lv %d\n%s %d/%d", g->hero.name, TX(rpg_race(g->hero.race)->name),
             TX(rpg_class(g->hero.cls)->name), g->hero.level, TR("Classe", "Job"), g->hero.job_level, rpg_job_max(g->hero.cls));
    lv_obj_t *t = kit_label(s, &wf_font_small, COL_TEXT, buf, 258, 160);
    lv_obj_set_style_text_align(t, LV_TEXT_ALIGN_LEFT, 0);
    snprintf(buf, sizeof buf, "PE %lu/%lu", (unsigned long)g->hero.xp, (unsigned long)rpg_xp_next(g->hero.level));
    kit_bar(s, 290, 214, 150, 12, COL_XP, (int)(g->hero.xp * 100 / rpg_xp_next(g->hero.level)), 100);
    tabs(s, 0);
    /* Base stats with raise buttons. */
    snprintf(buf, sizeof buf, TR("Punti: %d", "Points: %d"), g->hero.stat_points);
    kit_label(s, &wf_font_small, COL_GOLD, buf, CX, 236);
    for (int i = 0; i < ST_COUNT; i++) {
        int x = i < 3 ? 130 : 290, y = 252 + (i % 3) * 30;
        snprintf(buf, sizeof buf, "%s %d", TX(rpg_stat_name(i)), st.stat[i]);
        lv_obj_t *lbl = kit_label(s, &wf_font_small, COL_TEXT, buf, x - 16, y);
        (void)lbl;
        if (g->hero.stat_points >= rpg_stat_cost(g->hero.stat[i])) {
            lv_obj_t *b = kit_round_button(s, ri(RI_PLUS), NULL, x + 48, y, 30, COL_BTN_OK, on_raise, i);
            (void)b;
        }
    }
    snprintf(buf, sizeof buf, "ATK %ld  MATK %ld  %s %ld\n%s %ld/%ld  %s %ld\nCrit %d%%  %s %d%%  x2 %d%%",
             (long)st.atk, (long)st.matk, TR("DIF", "DEF"), (long)st.def, TR("PV", "HP"), (long)g->hero.hp, (long)st.max_hp,
             TR("PM", "SP"), (long)st.max_sp, st.crit, TR("Schivata", "Dodge"), st.dodge, st.double_act);
    kit_label(s, &wf_font_small, COL_ENERGY, buf, CX, 370);
    r_back(s, R_TOWN);
}

/* ---- Skills -------------------------------------------------------------------------- */
static const char *skill_desc(int s, int lv, char *buf, int cap) {
    const rpg_skill_info *k = rpg_skill(s);
    int p = rpg_skill_power(s, lv > 0 ? lv : 1), cost = rpg_skill_cost(s, lv > 0 ? lv : 1);
    static const wf_text kinds[] = {
        {"Colpo %d%%", "Strike %d%%"}, {"%d colpi da %d%%", "%d hits of %d%%"}, {"Fisico a tutti %d%%", "Physical, all foes %d%%"},
        {"Magia %d%%", "Spell %d%%"}, {"Magia a tutti %d%%", "Spell, all foes %d%%"}, {"Cura %d%%", "Heal %d%%"},
        {"Potenzia %d%% per %d turni", "Boost %d%% for %d turns"}, {"Indebolisce (%d%%)", "Hex (%d%%)"},
        {"Risucchio %d%%", "Drain %d%%"}, {"Evoca un alleato (%d%%) per %d turni", "Summons an ally (%d%%) for %d turns"},
        {"Passiva: +%d", "Passive: +%d"},
    };
    int n;
    if (k->kind == SK_MULTI) n = snprintf(buf, (size_t)cap, TX(kinds[1]), k->hits, p);
    else if (k->kind == SK_BUFF || k->kind == SK_SUMMON) n = snprintf(buf, (size_t)cap, TX(kinds[k->kind]), p, k->turns);
    else n = snprintf(buf, (size_t)cap, TX(kinds[k->kind]), p);
    if (k->element && k->kind != SK_PASSIVE) n += snprintf(buf + n, (size_t)(cap - n), " · %s", TX(rpg_element_name(k->element)));
    if (k->status) n += snprintf(buf + n, (size_t)(cap - n), " · %s %d%%", TX(rpg_status_name(k->status)), k->chance);
    if (k->kind != SK_PASSIVE)
        snprintf(buf + n, (size_t)(cap - n), TR("\n%d PM · ricarica %d", "\n%d SP · cooldown %d"), cost, k->cooldown);
    return buf;
}

static void on_learn(lv_event_t *e) {
    int s = (int)(intptr_t)lv_event_get_user_data(e);
    if (rpg_learn_skill(&R.g, s)) {
        r_dirty();
        r_go(R_SKILLS);
    }
}

void scr_skills(void) {
    lv_obj_t *s = kit_screen(NULL);
    lv_obj_set_style_bg_color(s, lv_color_hex(0x161430), 0);
    tabs(s, 1);
    rpg_game *g = &R.g;
    char buf[160];
    snprintf(buf, sizeof buf, TR("Punti abilità: %d", "Skill points: %d"), g->hero.skill_points);
    kit_label(s, &wf_font_small, COL_GOLD, buf, CX, 108);
    lv_obj_t *list = r_list(s, CX, 262, 330, 270);
    int skills[9], n = rpg_class_skills(g->hero.cls, skills);
    for (int i = n - 1; i >= 0; i--) {
        int sk = skills[i], lv = g->hero.skill[sk];
        const rpg_skill_info *k = rpg_skill(sk);
        lv_obj_t *row = r_row(list, LV_SIZE_CONTENT);
        lv_obj_set_style_pad_ver(row, 8, 0);
        lv_obj_set_style_pad_left(row, 44, 0);
        lv_obj_set_style_pad_right(row, 48, 0);
        lv_obj_t *ic = lv_image_create(row);
        lv_image_set_src(ic, gfx_sprite(&rpg_skill_icons[k->kind], 2));
        lv_obj_align(ic, LV_ALIGN_LEFT_MID, -38, 0);
        if (!lv) lv_obj_set_style_opa(ic, 110, 0);
        char d[96];
        snprintf(buf, sizeof buf, "%s  %d/%d\n%s", TX(k->name), lv, RPG_SKILL_MAX_LV, skill_desc(sk, lv, d, sizeof d));
        lv_obj_t *t = r_text(row, buf, lv ? COL_TEXT : COL_DIM, NULL);
        lv_obj_set_width(t, 220);
        lv_label_set_long_mode(t, LV_LABEL_LONG_WRAP);
        lv_obj_align(t, LV_ALIGN_LEFT_MID, 0, 0);
        if (g->hero.skill_points && lv < RPG_SKILL_MAX_LV) {
            lv_obj_t *b = kit_round_button(row, ri(RI_PLUS), NULL, 0, 0, 36, COL_BTN_OK, on_learn, sk);
            lv_obj_align(b, LV_ALIGN_RIGHT_MID, 42, 0);
        }
    }
    r_back(s, R_TOWN);
}

/* ---- Class --------------------------------------------------------------------------- */
static void on_change(lv_event_t *e) {
    int c = (int)(intptr_t)lv_event_get_user_data(e);
    kit_dialog_close(dlg);
    dlg = NULL;
    if (!rpg_change_class(&R.g, c)) return;
    r_save_now();
    r_go(R_CLASS);
    char body[160];
    snprintf(body, sizeof body, TR("%s è ora %s!\n%s", "%s is now a %s!\n%s"), R.g.hero.name, TX(rpg_class(c)->name),
             TX(rpg_class(c)->blurb));
    dlg = kit_dialog(TR("Nuova classe!", "New class!"), body, ri(RI_CROWN));
    kit_dialog_button(dlg, TR("Evviva!", "Hooray!"), COL_BTN_OK, close_dlg, 0);
}

static void on_pick(lv_event_t *e) {
    int c = (int)(intptr_t)lv_event_get_user_data(e);
    char body[200], d[96];
    const rpg_class_info *ci = rpg_class(c);
    int n = snprintf(body, sizeof body, "%s\n", TX(ci->blurb));
    for (int k = 0; k < 3 && ci->skills[k] != 0xff && n < (int)sizeof body - 40; k++)
        n += snprintf(body + n, sizeof body - (size_t)n, "\n· %s", TX(rpg_skill(ci->skills[k])->name));
    (void)d;
    dlg = kit_dialog(TX(ci->name), body, ri(RI_CROWN));
    if (rpg_can_change_class(&R.g, c)) kit_dialog_button(dlg, TR("Diventa!", "Become!"), COL_BTN_OK, on_change, c);
    kit_dialog_button(dlg, TR("Chiudi", "Close"), COL_PANEL_HI, close_dlg, 0);
}

void scr_class(void) {
    lv_obj_t *s = kit_screen(NULL);
    lv_obj_set_style_bg_color(s, lv_color_hex(0x161430), 0);
    tabs(s, 2);
    rpg_game *g = &R.g;
    hero_look l;
    hero_look_from_game(&l, g);
    hero_obj(s, &l, HX_HAPPY, 2, CX, 156);
    char buf[160];
    int tier = rpg_class_tier(g->hero.cls);
    snprintf(buf, sizeof buf, "%s · %s %d", TX(rpg_class(g->hero.cls)->name), TR("grado", "tier"), tier);
    kit_label(s, &wf_font_big, COL_GOLD, buf, CX, 222);
    int next[2], n = rpg_class_next(g->hero.cls, next);
    if (!n) {
        kit_label(s, &wf_font_small, COL_TEXT, TR("Hai raggiunto l'apice della tua via.", "You reached the peak of your path."), CX, 300);
    } else {
        int need_base = rpg_class_tier(next[0]) == 2 ? 20 : 45;
        snprintf(buf, sizeof buf, TR("Serve: classe %d/%d, livello %d/%d", "Needs: job %d/%d, level %d/%d"), g->hero.job_level,
                 rpg_job_max(g->hero.cls), g->hero.level, need_base);
        kit_label(s, &wf_font_small, COL_DIM, buf, CX, 256);
        for (int i = 0; i < n; i++) {
            int x = n == 1 ? CX : CX - 80 + i * 160;
            hero_look nl = l;
            nl.tier = (uint8_t)(rpg_class_tier(next[i]) - 1);
            lv_obj_t *b = kit_round_button(s, NULL, NULL, x, 330, 96, rpg_can_change_class(g, next[i]) ? COL_BTN_OK : COL_PANEL_HI,
                                           on_pick, next[i]);
            lv_obj_t *img = lv_image_create(b);
            lv_image_set_src(img, hero_image(&nl, HX_IDLE, 1));
            lv_obj_center(img);
            kit_label(s, &wf_font_small, COL_TEXT, TX(rpg_class(next[i])->name), x, 392);
        }
    }
    r_back(s, R_TOWN);
}
