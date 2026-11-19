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
    for (int i = 0; i < 3; i++) {
        lv_obj_t *tab = r_character_button(s, TX(names[i]), CX - 105 + i * 105, 85, i == 1 ? 82 : 114, 42,
                                           i == active, on_go, screens[i]);
        if (i == active) lv_obj_remove_flag(tab, LV_OBJ_FLAG_CLICKABLE);
    }
}

static void hero_back(lv_obj_t *s, int destination) {
    r_character_button(s, TR("Indietro", "Back"), CX, 427, 132, 44, false, on_go, destination);
}

/* ---- Stats ---------------------------------------------------------------------- */
static void on_raise(lv_event_t *e) {
    int s = (int)(intptr_t)lv_event_get_user_data(e);
    if (rpg_raise_stat(&R.g, s)) {
        r_dirty();
        scr_hero();
    }
}

void scr_hero(void) {
    lv_obj_t *s = r_character_screen();
    r_character_panel(s, CX, 167, 382, 110);
    rpg_game *g = &R.g;
    rpg_stats st;
    rpg_stats_get(g, &st);
    hero_look l;
    hero_look_from_game(&l, g);
    hero_obj(s, &l, HX_IDLE, 2, 110, 160);
    char buf[160];
    snprintf(buf, sizeof buf, "%s\n%s\n%s  ·  Lv %d\n%s %d/%d", g->hero.name, TX(rpg_class(g->hero.cls)->name),
             TX(rpg_race(g->hero.race)->name), g->hero.level, TR("Classe", "Job"), g->hero.job_level, rpg_job_max(g->hero.cls));
    /* Fixed text column keeps long master-class names out of the avatar. */
    lv_obj_t *t = kit_label(s, &ln_font_body, RC_IVORY, buf, 290, 162);
    lv_obj_set_width(t, 260);
    lv_obj_set_style_text_line_space(t, -2, 0);
    lv_obj_set_style_text_align(t, LV_TEXT_ALIGN_LEFT, 0);
    kit_bar(s, 278, 215, 236, 6, RC_GOLD, (int)g->hero.xp, (int)rpg_xp_next(g->hero.level));
    tabs(s, 0);
    /* Base stats with raise buttons. */
    snprintf(buf, sizeof buf, TR("Punti: %d", "Points: %d"), g->hero.stat_points);
    kit_label(s, &ln_font_body, RC_GOLD, buf, CX, 237);
    for (int i = 0; i < ST_COUNT; i++) {
        int x = i < 3 ? 147 : 319, y = 265 + (i % 3) * 31;
        r_character_panel(s, x, y, 150, 29);
        snprintf(buf, sizeof buf, "%s %d", TX(rpg_stat_name(i)), st.stat[i]);
        kit_label(s, &ln_font_body, RC_IVORY, buf, x - 23, y);
        if (g->hero.stat[i] < 99 && g->hero.stat_points >= rpg_stat_cost(g->hero.stat[i])) {
            r_character_button(s, "+", x + 49, y, 36, 29, true, on_raise, i);
        }
    }
    r_character_panel(s, CX, 373, 320, 64);
    snprintf(buf, sizeof buf, "ATK %ld  MATK %ld  %s %ld\n%s %ld/%ld  %s %ld\nCrit %d%%  %s %d%%  x2 %d%%",
             (long)st.atk, (long)st.matk, TR("DIF", "DEF"), (long)st.def, TR("PV", "HP"), (long)g->hero.hp, (long)st.max_hp,
             TR("PM", "SP"), (long)st.max_sp, st.crit, TR("Schivata", "Dodge"), st.dodge, st.double_act);
    lv_obj_t *totals = kit_label(s, &ln_font_body, RC_MUTED, buf, CX, 373);
    lv_obj_set_style_text_line_space(totals, -8, 0);
    hero_back(s, R_TOWN_CLASSIC);
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

static lv_obj_t *skill_list;

static void on_learn(lv_event_t *e) {
    int s = (int)(intptr_t)lv_event_get_user_data(e);
    if (rpg_learn_skill(&R.g, s)) {
        r_dirty();
        int scroll_y = lv_obj_get_scroll_y(skill_list);
        r_go(R_SKILLS);
        lv_obj_update_layout(skill_list);
        lv_obj_scroll_to_y(skill_list, scroll_y, LV_ANIM_OFF);
    }
}

void scr_skills(void) {
    lv_obj_t *s = r_character_screen();
    tabs(s, 1);
    rpg_game *g = &R.g;
    char buf[320];
    snprintf(buf, sizeof buf, TR("Punti abilità: %d", "Skill points: %d"), g->hero.skill_points);
    kit_label(s, &ln_font_body, RC_GOLD, buf, CX, 119);
    lv_obj_t *list = skill_list = r_list(s, CX, 268, 322, 252);
    lv_obj_set_scrollbar_mode(list, LV_SCROLLBAR_MODE_AUTO);
    int skills[9], n = rpg_class_skills(g->hero.cls, skills);
    for (int i = n - 1; i >= 0; i--) {
        int sk = skills[i], lv = g->hero.skill[sk];
        const rpg_skill_info *k = rpg_skill(sk);
        lv_obj_t *row = r_row(list, LV_SIZE_CONTENT);
        lv_obj_set_style_bg_color(row, lv_color_hex(RC_SURFACE), 0);
        lv_obj_set_style_bg_opa(row, 245, 0);
        lv_obj_set_style_border_color(row, lv_color_hex(0x495e62), 0);
        lv_obj_set_style_border_width(row, 1, 0);
        lv_obj_set_style_radius(row, 8, 0);
        lv_obj_set_style_pad_ver(row, 8, 0);
        lv_obj_set_style_pad_left(row, 44, 0);
        lv_obj_set_style_pad_right(row, 48, 0);
        lv_obj_t *ic = lv_image_create(row);
        lv_image_set_src(ic, gfx_sprite(&rpg_skill_icons[k->kind], 2));
        lv_obj_align(ic, LV_ALIGN_LEFT_MID, -38, 0);
        if (!lv) lv_obj_set_style_opa(ic, 110, 0);
        char d[256];
        snprintf(buf, sizeof buf, "%s\n%s", TX(k->name), skill_desc(sk, lv, d, sizeof d));
        lv_obj_t *t = r_text(row, buf, lv ? RC_IVORY : RC_MUTED, &ln_font_body);
        lv_obj_set_width(t, 222);
        lv_label_set_long_mode(t, LV_LABEL_LONG_WRAP);
        lv_obj_align(t, LV_ALIGN_LEFT_MID, 0, 0);
        snprintf(buf, sizeof buf, "%d/%d", lv, RPG_SKILL_MAX_LV);
        lv_obj_t *rank = r_text(row, buf, lv ? RC_GOLD : RC_MUTED, &ln_font_body);
        lv_obj_align(rank, LV_ALIGN_RIGHT_MID, 41, g->hero.skill_points && lv < RPG_SKILL_MAX_LV ? -20 : 0);
        if (g->hero.skill_points && lv < RPG_SKILL_MAX_LV) {
            lv_obj_t *b = r_character_button(row, "+", 0, 0, 36, 36, true, on_learn, sk);
            lv_obj_align(b, LV_ALIGN_RIGHT_MID, 42, 13);
        }
    }
    hero_back(s, R_HERO);
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
    dlg = r_character_dialog(TR("Nuova classe!", "New class!"), body, ri(RI_CROWN));
    r_character_button_style(kit_dialog_button(dlg, TR("Evviva!", "Hooray!"), COL_BTN_OK, close_dlg, 0), true);
}

static void on_pick(lv_event_t *e) {
    int c = (int)(intptr_t)lv_event_get_user_data(e);
    char body[200];
    const rpg_class_info *ci = rpg_class(c);
    int n = snprintf(body, sizeof body, "%s\n", TX(ci->blurb));
    for (int k = 0; k < 3 && ci->skills[k] != 0xff && n < (int)sizeof body - 40; k++)
        n += snprintf(body + n, sizeof body - (size_t)n, "\n· %s", TX(rpg_skill(ci->skills[k])->name));
    dlg = r_character_dialog(TX(ci->name), body, ri(RI_CROWN));
    if (rpg_can_change_class(&R.g, c))
        r_character_button_style(kit_dialog_button(dlg, TR("Diventa!", "Become!"), COL_BTN_OK, on_change, c), true);
    r_character_button_style(kit_dialog_button(dlg, TR("Chiudi", "Close"), COL_PANEL_HI, close_dlg, 0), false);
}

void scr_class(void) {
    lv_obj_t *s = r_character_screen();
    tabs(s, 2);
    rpg_game *g = &R.g;
    hero_look l;
    hero_look_from_game(&l, g);
    hero_obj(s, &l, HX_HAPPY, 2, CX, 150);
    char buf[160];
    int tier = rpg_class_tier(g->hero.cls);
    r_character_panel(s, CX, 221, 336, 60);
    kit_label(s, &ln_font_title, RC_IVORY, TX(rpg_class(g->hero.cls)->name), CX, 209);
    snprintf(buf, sizeof buf, TR("Grado %d", "Tier %d"), tier);
    kit_label(s, &ln_font_body, RC_GOLD, buf, CX, 236);
    int next[2], n = rpg_class_next(g->hero.cls, next);
    if (!n) {
        r_character_panel(s, CX, 305, 320, 66);
        lv_obj_t *message = kit_label(s, &ln_font_body, RC_MUTED,
            TR("Hai raggiunto l’apice della tua via.", "You reached the peak of your path."), CX, 305);
        lv_obj_set_width(message, 300);
    } else {
        int need_base = rpg_class_tier(next[0]) == 2 ? 20 : 45;
        snprintf(buf, sizeof buf, TR("Classe %d/%d  ·  Livello %d/%d", "Job %d/%d  ·  Level %d/%d"), g->hero.job_level,
                 rpg_job_max(g->hero.cls), g->hero.level, need_base);
        kit_label(s, &ln_font_body, RC_MUTED, buf, CX, 270);
        for (int i = 0; i < n; i++) {
            int x = n == 1 ? CX : CX - 80 + i * 160;
            hero_look nl = l;
            nl.tier = (uint8_t)(rpg_class_tier(next[i]) - 1);
            nl.class_id = (uint8_t)next[i];
            int width = n == 1 ? 204 : 152;
            bool ready = rpg_can_change_class(g, next[i]);
            lv_obj_t *b = r_character_button(s, TX(rpg_class(next[i])->name), x, 344, width, 108, ready, on_pick, next[i]);
            lv_obj_t *label = lv_obj_get_child(b, 0);
            lv_obj_set_width(label, width - 6);
            lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, 0);
            lv_obj_set_style_text_line_space(label, -4, 0);
            lv_obj_align(label, LV_ALIGN_BOTTOM_MID, 0, -8);
            lv_obj_t *img = lv_image_create(b);
            lv_image_set_src(img, hero_image(&nl, HX_IDLE, 1));
            lv_obj_align(img, LV_ALIGN_TOP_MID, 0, 5);
            lv_obj_remove_flag(img, LV_OBJ_FLAG_CLICKABLE);
        }
    }
    hero_back(s, R_HERO);
}
