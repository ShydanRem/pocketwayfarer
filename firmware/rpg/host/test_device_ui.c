/* Device-cadence regressions against the real LVGL UI. Reuse the deterministic
 * framebuffer/platform harness without rerunning its full gameplay script. */
#define main gameplay_preview_main
#define wf_platform_save harness_save
#include "preview_lantern.c"
#undef wf_platform_save
#undef main
#include "device_input.h"
extern int rpg_battle_parry_quality_now(void);

static bool delayed_save, reject_save;
bool wf_platform_save(const void *data, size_t n) {
    if (reject_save) return false;
    bool ok = harness_save(data, n);
    if (delayed_save) ms += 80;
    return ok;
}

static void encounter(int state) {
    down = 0;
    ln_start(&R.g.lantern, LN_SENTINEL, 177);
    assert(ln_enter(&R.g.lantern, 0));
    R.g.lantern.state = (uint8_t)state;
    R.g.coach.disabled = 1;
    r_go(R_LANTERN);
    wf_app_tick(); lv_obj_update_layout(lv_screen_active());
}
static void awake_frame(unsigned delta) {
    ms += delta;
    wf_app_tick();
    lv_timer_handler(); /* production loop advances simulation before rendering */
}
static lv_obj_t *wave_object(void) {
    lv_obj_t *screen = lv_screen_active();
    for (unsigned i = 0; i < lv_obj_get_child_count(screen); i++) {
        lv_obj_t *o = lv_obj_get_child(screen, (int)i);
        if (lv_obj_get_style_radius(o, 0) == LV_RADIUS_CIRCLE && lv_obj_get_style_border_width(o, 0) == 4)
            return o;
    }
    assert(0); return NULL;
}
static void cadence_and_judgment(void) {
    encounter(LN_PARRY);
    lv_obj_t *wave = wave_object();
    assert(lv_obj_get_width(wave) == 430);
    shot("parata-inizio");
    for (int i = 0; i < 5; i++) awake_frame(120);
    assert(R.g.lantern.elapsed_ms == 600 && lv_obj_get_width(wave) == 262);
    shot("parata-600ms");
    for (int i = 0; i < 3; i++) awake_frame(120);
    assert(R.g.lantern.elapsed_ms == 960 && lv_obj_get_width(wave) == 162);
    shot("parata-960ms");
    for (int i = 0; i < 22; i++) awake_frame(120);
    assert(R.g.lantern.state == LN_CHOOSE);
    puts("120ms frames: shrinking circle 430 -> 262 -> 162px, then normal parry/impact/choice progression: passed");

    encounter(LN_ATTACK);
    R.g.lantern.elapsed_ms = 620;
    ms += 80; tx = 233; ty = 230; down = 1;
    lv_timer_handler();
    assert(R.g.lantern.quality == WF_Q_PERFECT);
    down = 0; run(20);
    encounter(LN_ATTACK); R.g.lantern.elapsed_ms = 620; ms += 80;
    assert(wf_app_key(WF_KEY_BOOT_DOWN) && R.g.lantern.quality == WF_Q_PERFECT);
    puts("touch and BOOT press before app tick: cached620 +80ms judges actual700ms PERFECT for both: passed");

    encounter(LN_PARRY); R.g.lantern.elapsed_ms = 1100;
    ms += 300; down = 1; lv_timer_handler();
    assert(R.g.lantern.state == LN_IMPACT && R.g.lantern.quality == WF_Q_MISS);
    down = 0; run(20);
    encounter(LN_ATTACK); R.g.lantern.elapsed_ms = 2700; ms += 200;
    assert(wf_app_key(WF_KEY_BOOT_DOWN));
    assert(R.g.lantern.state == LN_IMPACT && R.g.lantern.quality == WF_Q_MISS && !R.g.lantern.paused);
    puts("expired timing press is consumed once, never applied to the next phase or converted into pause: passed");

    encounter(LN_TELEGRAPH); R.g.lantern.elapsed_ms = 649;
    delayed_save = true; ms++; wf_app_tick();
    assert(R.g.lantern.state == LN_PARRY);
    ms += 20; wf_app_tick();
    assert(R.g.lantern.elapsed_ms == 20);
    delayed_save = false;
    encounter(LN_ATTACK); R.g.lantern.elapsed_ms = 620;
    wf_app_flush_save(); ms += 20000; seconds += 20; wf_app_wake(); run(30);
    assert(R.g.lantern.paused && R.g.lantern.elapsed_ms == 620);
    press("Riprendi");
    assert(wf_app_key(WF_KEY_BOOT_DOWN) && !R.g.lantern.paused && R.g.lantern.elapsed_ms == 620);
    run(590); assert(R.g.lantern.elapsed_ms == 620);
    run(100); assert(R.g.lantern.elapsed_ms > 620);
    puts("80ms save excluded from new phase; 20s sleep preserves elapsed; early BOOT in650ms readiness ignored: passed");
    encounter(LN_CHOOSE); ms += 46000;
    assert(wf_app_key(WF_KEY_BOOT_DOWN) && R.g.lantern.paused);
    assert(lv_display_get_inactive_time(NULL) == 0 && !wf_app_busy());
    R.g.lantern.state = LN_ROUTE; r_go(R_LANTERN); assert(!wf_app_busy());
    R.g.lantern.state = LN_RELIC; r_go(R_LANTERN); assert(!wf_app_busy());
    puts("physical key registers activity; paused/route/reward surfaces allow idle-off without immediate re-sleep: passed");
    unsigned before = saves;
    R.last_save_s = r_now()-301; /* periodic fallback must not bypass retry backoff */
    reject_save = true; r_dirty(); r_save_now();
    assert(R.dirty && R.save_failed && saves == (int)before && R.save_due_s == r_now()+3);
    uint32_t retry_due = R.save_due_s;
    seconds++; wf_app_tick();
    assert(R.dirty && R.save_due_s == retry_due && saves == (int)before);
    reject_save = false; seconds += 2; wf_app_tick();
    assert(!R.dirty && !R.save_failed && saves == (int)before+1);
    puts("failed durable commit preserves dirty state and retries successfully after3s: passed");
}

static int deleted_victims;
static void victim_deleted(lv_event_t *e) { (void)e; deleted_victims++; }
static void scene_lifetimes(void) {
    kit_screen(NULL);
    const size_t persistent = image_bytes; /* shared FX entries outlive screens */
    for (int cycle = 0; cycle < 20; cycle++) {
        kit_toast("first", NULL);
        kit_screen(NULL); /* deletes toast; its old address may be reused below */
        for (int i = 0; i < 24; i++) {
            lv_obj_t *victim = lv_obj_create(lv_screen_active());
            lv_obj_add_event_cb(victim, victim_deleted, LV_EVENT_DELETE, NULL);
        }
        deleted_victims = 0;
        kit_toast("second", NULL);
        assert(!deleted_victims);
    }
    kit_screen(NULL);
    const lv_image_dsc_t *first = gfx_sprite(&rpg_item_icons[0][0], 2);
    const void *pixels = first->data;
    uint32_t bytes = first->data_size;
    for (int scale = 2; scale <= 3; scale++)
        for (int type = 0; type < IT_TYPE_COUNT; type++) for (int tier = 0; tier < RPG_TIERS; tier++)
            assert(gfx_sprite(&rpg_item_icons[type][tier], scale));
    assert(first == gfx_sprite(&rpg_item_icons[0][0], 2) && first->data == pixels && first->data_size == bytes);
    kit_screen(NULL); assert(image_bytes == persistent);
    puts("toast address reuse deletes no unrelated widgets; all220 icon/scale entries retain descriptors; scene frees all image bytes: passed");
}

static int hits, checked, rejected;
static void tapped(lv_event_t *e) { (void)e; hits++; }
static bool inside(lv_obj_t *child, lv_obj_t *parent) {
    lv_area_t a, b; lv_obj_get_coords(child, &a); lv_obj_get_coords(parent, &b);
    return a.x1 >= b.x1 && a.x2 <= b.x2 && a.y1 >= b.y1 && a.y2 <= b.y2;
}
static bool target_is(lv_obj_t *hit, lv_obj_t *wanted) {
    for (; hit; hit = lv_obj_get_parent(hit)) if (hit == wanted) return true;
    return false;
}
static void modal_targets(lv_obj_t *dialog, bool click_actions) {
    lv_obj_t *panel = lv_obj_get_child(dialog, 0), *row = lv_obj_get_user_data(dialog);
    lv_obj_update_layout(lv_layer_top());
    lv_area_t a; lv_obj_get_coords(panel, &a);
    assert(lv_obj_get_width(panel) <= 300 && lv_obj_get_height(panel) <= 350);
    int corners[4][2] = {{a.x1,a.y1},{a.x2,a.y1},{a.x1,a.y2},{a.x2,a.y2}};
    for (int i = 0; i < 4; i++) {
        int x = corners[i][0]-233, y = corners[i][1]-233;
        assert(x*x+y*y < 233*233);
    }
    for (unsigned i = 0; i < lv_obj_get_child_count(row); i++) {
        lv_obj_t *button = lv_obj_get_child(row, (int)i);
        lv_area_t b; lv_obj_get_coords(button, &b);
        lv_point_t point = {(b.x1+b.x2)/2, (b.y1+b.y2)/2};
        if (point.y < a.y1 || point.y > a.y2) {
            assert(!target_is(lv_indev_search_obj(lv_layer_top(), &point), button));
            int x = point.x-233, y = point.y-233;
            if (click_actions && x*x+y*y < 233*233) {
                int before = hits; click(point.x, point.y); assert(hits == before); rejected++;
            }
        }
        lv_obj_scroll_to_view_recursive(button, LV_ANIM_OFF); lv_obj_update_layout(lv_layer_top());
        assert(inside(button, panel));
        lv_obj_get_coords(button, &b); point = (lv_point_t){(b.x1+b.x2)/2, (b.y1+b.y2)/2};
        assert(target_is(lv_indev_search_obj(lv_layer_top(), &point), button));
        if (click_actions) { int before = hits; click(point.x, point.y); assert(hits == before+1); }
        checked++;
    }
}
static void modal_and_gems(void) {
    rpg_new_hero hero = {0}; rpg_game_new(&R.g, &hero, seconds, 71); R.loaded = true;
    for (int language = 0; language < 2; language++) for (int type = 0; type < IT_TYPE_COUNT; type++) {
        wf_lang = language; r_go(R_LANTERN_HOME);
        rpg_item item = {.type=type, .tier=4, .rarity=RAR_LEGENDARY, .plus=10, .level=70, .sockets=2};
        item.gems[0] = SP_ELDER_STUMP+1;
        item.aff[0]=AF_LIFESTEAL; item.aff[1]=AF_SP_REGEN; item.aff[2]=AF_GOLD;
        item.aff_val[0]=item.aff_val[1]=item.aff_val[2]=50;
        lv_obj_t *dialog;
        r_item_dialog(&item, language ? "\nCompared to current: +9999" : "\nRispetto all'attuale: +9999", &dialog);
        kit_dialog_button(dialog, language ? "Equip" : "Equipaggia", COL_BTN_OK, tapped, 0);
        kit_dialog_button(dialog, language ? "Gem" : "Gemma", COL_BTN_ALT, tapped, 0);
        kit_dialog_button(dialog, language ? "Sell 5000" : "Vendi 5000", COL_GOLD, tapped, 0);
        kit_dialog_button(dialog, language ? "Close" : "Chiudi", COL_PANEL_HI, tapped, 0);
        modal_targets(dialog, true);
    }
    assert(checked == IT_TYPE_COUNT*2*4);
    printf("44 item dialogs: %d action targets scrolled and clicked; %d off-panel clicks rejected: passed\n", checked, rejected);
    wf_lang = 0;
    R.g.bag[0] = R.g.hero.equip[0]; R.g.bag[0].sockets = 2; memset(R.g.bag[0].gems, 0, 2);
    for (int i = 0; i < SP_COUNT; i++) R.g.gems[i] = 1;
    R.bag_tab = 0; r_go(R_BAG); lv_obj_update_layout(lv_screen_active());
    lv_obj_t *list = NULL;
    for (unsigned i = 0; i < lv_obj_get_child_count(lv_screen_active()); i++) {
        lv_obj_t *o = lv_obj_get_child(lv_screen_active(), (int)i);
        if (lv_obj_get_width(o) == 320 && lv_obj_get_height(o) == 250) list = o;
    }
    assert(list);
    lv_area_t a; lv_obj_get_coords(lv_obj_get_child(list, 0), &a); click((a.x1+a.x2)/2, (a.y1+a.y2)/2);
    lv_obj_t *gem = find(lv_layer_top(), "Gemma"); assert(gem);
    lv_obj_scroll_to_view_recursive(lv_obj_get_parent(gem), LV_ANIM_OFF); press("Gemma"); run(50);
    lv_obj_t *dialog = lv_obj_get_child(lv_layer_top(), 0), *row = lv_obj_get_user_data(dialog);
    lv_obj_t *panel = lv_obj_get_child(dialog, 0);
    assert(lv_obj_get_child_count(row) == SP_COUNT+1);
    modal_targets(dialog, false); lv_obj_scroll_to_y(panel, 0, LV_ANIM_OFF); run(50);
    lv_obj_t *last = lv_obj_get_child(row, SP_COUNT-1);
    int swipes = 0;
    while (!inside(last, panel) && swipes++ < 12) {
        tx=233; ty=365; down=1; run(30);
        for (int y=345; y>=125; y-=20) { ty=y; run(20); }
        down=0; run(200);
    }
    assert(inside(last, panel));
    char name[64]; snprintf(name, sizeof name, "%s x1", TX(rpg_species(SP_COUNT-1)->name));
    press(name); assert(R.g.gems[SP_COUNT-1] == 0 && R.g.bag[0].gems[0] == SP_COUNT);
    puts("all42 gem species plus Close: final species reached by touch scrolling and socketed: passed");
}

static void primary_adventure(void) {
    for (int language = 0; language < 2; language++) {
        wf_app_start(NULL, 0);
        press(language ? "English" : "Italiano");
        shot(language ? "ingresso-en" : "ingresso-it");
        press(TR("Nuova avventura", "New adventure"));
        assert(find(lv_screen_active(), TR("Scegli la classe", "Choose your class")));
        for (int family = 0; family < FAM_COUNT; family++) {
            assert(find(lv_screen_active(), TX(rpg_class(family)->name)));
            press(">");
        }
        press(">"); press(">");
        shot(language ? "scelta-en" : "scelta-it");
        press(TR("Inizia", "Begin"));
        assert(R.screen == R_TOWN && R.g.hero.cls == FAM_ARCHER && R.g.hero.equip[SLOT_WEAPON].type == IT_BOW);
        assert(R.g.lantern.state == LN_NONE && !find(lv_screen_active(), TR("Classico", "Classic")));
        assert(find(lv_layer_top(), TR("Il primo viaggio", "Your first journey")));
        shot(language ? "guida-avvio-en" : "guida-avvio-it");
        press(TR("Partiamo", "Let's go"));
        check_buttons(lv_screen_active());
        const char *actions[] = {TR("Mappa", "Map"), TR("Classe", "Class"), TR("Eroe", "Hero"),
                                 TR("Equip", "Gear"), TR("Servizi", "Services")};
        for (size_t i = 0; i < sizeof actions / sizeof *actions; i++) {
            lv_obj_t *label = find(lv_screen_active(), actions[i]);
            assert(label);
            lv_area_t b;
            lv_obj_get_coords(lv_obj_get_parent(label), &b);
            for (int x = b.x1; x <= b.x2; x += b.x2 - b.x1)
            for (int y = b.y1; y <= b.y2; y += b.y2 - b.y1)
                assert((x - CX) * (x - CX) + (y - CY) * (y - CY) < CX * CY);
        }
        shot(language ? "borgo-en" : "borgo-it");
        press(TR("Mappa", "Map"));
        assert(find(lv_screen_active(), TR("Scegli i Prati di Lumen", "Choose Lumen Meadows")));
        press(TX(rpg_zone(0)->field_name));
        press(TR("Combatti", "Fight"));
        assert(R.screen == R_BATTLE);
        run(1250);
        assert(find(lv_screen_active(), TR("Inizia con Attacco", "Start with Attack")));
        shot(language ? "lotta-en" : "lotta-it");
        press(TR("Guardia", "Guard"));
        for (int t = 0; t < 500 && rpg_battle_phase() != 6; t++) run(5);
        assert(find(lv_screen_active(), TR("Onda sullo scudo\nTocca o BOOT", "Wave hits the shield\nTap or BOOT")));
        for (int t = 0; t < 500 && rpg_battle_parry_quality_now() != WF_Q_GOOD; t++) run(5);
        assert(rpg_battle_parry_quality_now() == WF_Q_GOOD && wf_app_key(WF_KEY_BOOT_DOWN));
        assert(!rpg_guide_needs(&R.g, RPG_GUIDE_PARRY));
        for (int t = 0; t < 500 && rpg_battle_phase() != 1; t++) run(5);
        press(TR("Attacco", "Attack"));
        lv_obj_t *caption = find(lv_screen_active(), TR("Scintilla nell’arco\nTocca o BOOT", "Spark in the arc\nTap or BOOT"));
        assert(caption);
        lv_obj_update_layout(lv_screen_active());
        assert(lv_obj_get_height(caption) <= lv_obj_get_height(lv_obj_get_parent(caption)) - 4);
        shot(language ? "guida-anello-en" : "guida-anello-it");
        for (int t = 0; t < 500 && rpg_battle_ring_quality_now() != WF_Q_PERFECT; t++) run(5);
        assert(rpg_battle_ring_quality_now() == WF_Q_PERFECT);
        click(CX, CY);
        assert(!rpg_guide_needs(&R.g, RPG_GUIDE_RING));
        for (int elapsed = 0; elapsed < 18000 && rpg_battle_phase() != 7; elapsed += 5) {
            if (rpg_battle_phase() == 1) press(TR("Attacco", "Attack"));
            if (rpg_battle_ring_quality_now() == WF_Q_PERFECT || rpg_battle_parry_quality_now() == WF_Q_PERFECT)
                assert(wf_app_key(WF_KEY_BOOT_DOWN));
            run(5);
        }
        assert(R.g.kills && R.g.hero.cls == FAM_ARCHER);
        assert(!rpg_guide_needs(&R.g, RPG_GUIDE_RING | RPG_GUIDE_WIN) && rpg_guide_needs(&R.g, RPG_GUIDE_ROOM));
        press(TR("Continua", "Continue"));
        assert(R.screen == R_ZONE);
        assert(find(lv_screen_active(), TR("Prossima tappa: la Cripta", "Next stop: the Crypt")));
        press(TX(rpg_zone(0)->dungeon_name));
        assert(find(lv_screen_active(), TR("Tocca una stanza dorata", "Tap a gold room")));
        shot(language ? "guida-cripta-en" : "guida-cripta-it");
        int room = 0;
        while (!rpg_room_reachable(&R.g.run, 0, room)) room++;
        /* A deterministic non-combat room isolates the retreat check. */
        R.g.run.rooms[0][room] = ROOM_EVENT;
        r_go(R_DUNGEON);
        click(131 + room * 102, 340);
        assert(!rpg_guide_needs(&R.g, RPG_GUIDE_ALL));
        press(TR("Avanti", "Onward"));
        R.g.hero.stamina = 1;
        R.g.cons[CO_RETURN] = 0;
        R.g.gold = 120;
        R.g.run.gold = 22;
        r_go(R_DUNGEON);
        assert(find(lv_screen_active(), TR("Stamina finita · Esci", "Out of stamina · Exit")));
        press(TR("Esci", "Exit"));
        press(TR("Ritirati", "Retreat"));
        assert(R.screen == R_TOWN && R.g.gold == 109 && !R.g.run.active && !R.g.cons[CO_RETURN]);
        wf_app_start(saved, saved_len);
        assert(R.screen == R_TOWN && R.g.gold == 109 && !rpg_guide_needs(&R.g, RPG_GUIDE_ALL));

        uint32_t job_xp = R.g.hero.job_xp;
        press(TR("Classe", "Class")); press(TR("Cambia classe", "Switch class"));
        shot(language ? "famiglie-en" : "famiglie-it");
        press(TX(rpg_class(FAM_MAGE)->name));
        assert(find(lv_layer_top(), TR("Conferma", "Confirm")));
        assert(wf_app_key(WF_KEY_BOOT_SHORT) && R.g.hero.cls == FAM_ARCHER);
        run(40);
        press(TX(rpg_class(FAM_MAGE)->name)); press(TR("Conferma", "Confirm"));
        assert(R.screen == R_CLASS && R.g.hero.cls == FAM_MAGE && R.g.hero.equip[SLOT_WEAPON].type == IT_STAFF);
        wf_app_start(saved, saved_len);
        assert(R.screen == R_TOWN && R.g.hero.cls == FAM_MAGE);
        press(TR("Classe", "Class")); press(TR("Cambia classe", "Switch class"));
        press(TX(rpg_class(FAM_ARCHER)->name)); press(TR("Conferma", "Confirm"));
        assert(R.g.hero.cls == FAM_ARCHER && R.g.hero.job_xp == job_xp);
        assert(wf_app_key(WF_KEY_BOOT_SHORT) && R.screen == R_TOWN);
        enter_expedition();
        assert(R.screen == R_LANTERN_HOME && !find(lv_screen_active(), TR("Eroe", "Hero")));
        press(TR("Borgo", "Village"));
        assert(R.screen == R_TOWN && R.g.hero.cls == FAM_ARCHER);

        press(TR("Servizi", "Services")); press(TR("Diario", "Journal")); press(TR("Opzioni", "Options"));
        lv_obj_t *name = find(lv_screen_active(), TR("Guida iniziale", "First journey guide"));
        assert(name);
        lv_obj_t *row = lv_obj_get_parent(lv_obj_get_parent(name));
        lv_obj_t *toggle = find(row, TR("Cambia", "Change"));
        assert(toggle);
        lv_obj_scroll_to_view_recursive(toggle, LV_ANIM_OFF);
        lv_obj_update_layout(lv_screen_active());
        lv_area_t a;
        lv_obj_get_coords(toggle, &a);
        click((a.x1 + a.x2) / 2, (a.y1 + a.y2) / 2);
        assert(rpg_guide_needs(&R.g, RPG_GUIDE_ALL));
        wf_app_flush_save(); wf_app_start(saved, saved_len);
        assert(R.screen == R_TOWN && rpg_guide_needs(&R.g, RPG_GUIDE_ALL));
        assert(!find(lv_layer_top(), TR("Il primo viaggio", "Your first journey")));
    }
    puts("primary adventure: eight classes, first-journey lessons, attack/parry, first Crypt room, no-scroll retreat, job reload and separate expedition IT/EN: passed");
}

static void pacing_ui(void) {
    for (int lang = 0; lang < 2; lang++) {
        rpg_new_hero profile = {.name = "Arcana", .cls = FAM_MAGE, .lang = (uint8_t)lang};
        rpg_game_new(&R.g, &profile, r_now(), 414);
        wf_lang = R.g.lang;
        assert(rpg_dungeon_enter(&R.g, 0, 0, r_now()) == RPG_OK);
        R.g.run.row = RPG_ROWS - 1;
        for (int col = 0; col < RPG_COLS; col++) {
            R.g.run.col = (int8_t)col;
            if (rpg_room_reachable(&R.g.run, RPG_ROWS, 1)) break;
        }
        r_go(R_DUNGEON);
        press(TR("Scale", "Stairs"));
        uint32_t xp = rpg_dungeon_first_clear_xp(&R.g);
        assert(xp == 29 && !R.g.best_floor[0]);
        const char *caption = TR("Prima conquista · +29 PE\nScendi al piano 2 o torna al borgo.",
                                 "First clear · +29 XP\nDescend to floor 2 or return to town.");
        assert(find(lv_layer_top(), caption));
        wf_app_start(saved, saved_len);
        assert(R.screen == R_DUNGEON && find(lv_layer_top(), caption) && !R.g.best_floor[0]);
        shot(lang ? "bonus-piano-en" : "bonus-piano-it");
        rpg_game expected = R.g;
        rpg_gain_xp(&expected, xp, xp * 7u / 10u, NULL);
        press(lang ? "Town" : "Scendi");
        assert(R.g.best_floor[0] == 1 && R.g.hero.xp == expected.hero.xp && R.g.hero.job_xp == expected.hero.job_xp);
        wf_app_start(saved, saved_len);
        assert(R.g.best_floor[0] == 1 && R.g.hero.xp == expected.hero.xp && R.g.hero.job_xp == expected.hero.job_xp);
        assert(lang ? R.screen == R_TOWN && !R.g.run.active : R.screen == R_DUNGEON && R.g.run.floor == 2);

        rpg_dungeon_leave(&R.g, false);
        int branches[2];
        assert(rpg_class_next(FAM_MAGE, branches) == 2);
        R.g.hero.cls = (uint8_t)branches[1];
        memset(R.g.hero.skill, 0, sizeof R.g.hero.skill);
        R.g.hero.skill[rpg_class(branches[1])->skills[0]] = 1;
        R.g.hero.sp = 20;
        rpg_foe foe;
        rpg_foe_make(&foe, SP_SKELETON, 3, false);
        r_start_battle(&foe, 1, false, false);
        run(1250);
        press(TR("Abilità", "Skills"));
        click(CX, CY - 150);
        assert(rpg_battle_phase() == 3);
        for (int t = 0; t < 500 && rpg_battle_ring_quality_now() < WF_Q_GOOD; t++) run(5);
        assert(rpg_battle_ring_quality_now() >= WF_Q_GOOD && wf_app_key(WF_KEY_BOOT_DOWN));
        run(1000);
        assert(rpg_battle_phase() != 3);
        rpg_classic_cancel(&R.g);
    }
    puts("first-clear UI: both choices and reload IT/EN; Arcane Missiles opens and resolves the timing ring");
}

static void parry_result_ui(void) {
    static const wf_text results[] = {{"Parata mancata", "Missed parry"},
        {"Bene · Danni -45%", "Good · Damage -45%"}, {"Ottimo · Danni -55%", "Great · Damage -55%"},
        {"Perfetta · Contrattacco", "Perfect · Counter"}};
    static const char *const captures[] = {"esito-mancato", "esito-bene", "esito-ottimo", "esito-perfetta"};
    for (int lang = 0; lang < 2; lang++) {
        for (int q = WF_Q_MISS; q <= WF_Q_PERFECT; q++) {
            rpg_new_hero profile = {.name = "Parry", .cls = FAM_WARRIOR, .lang = (uint8_t)lang};
            rpg_game_new(&R.g, &profile, r_now(), 120);
            R.g.hero.stat[ST_AGI] = 1; /* Wait for the wave, with no dodge chance. */
            R.g.lantern.reduced_motion = (uint8_t)lang;
            wf_lang = R.g.lang;
            R.zone = 0;
            rpg_foe foe;
            rpg_foe_make(&foe, SP_SLIME, 1, false);
            r_start_battle(&foe, 1, false, false);
            run(1250);
            press(TR("Guardia", "Guard"));
            for (int t = 0; t < 500 && rpg_battle_phase() != 6; t++) run(5);
            assert(rpg_battle_phase() == 6);
            if (q == WF_Q_MISS && !lang) {
                for (int t = 0; t < 600 && rpg_battle_phase() == 6; t++) run(5);
            } else {
                for (int t = 0; t < 500 && rpg_battle_parry_quality_now() != q; t++) run(5);
                assert(rpg_battle_parry_quality_now() == q);
                if ((q + lang) % 2) assert(wf_app_key(WF_KEY_BOOT_DOWN));
                else click(CX, CY);
            }
            run(170);
            lv_obj_t *result = find(lv_screen_active(), TX(results[q]));
            assert(result && rpg_battle_phase() == 4);
            lv_obj_update_layout(lv_screen_active());
            assert(lv_obj_get_height(result) < lv_obj_get_height(lv_obj_get_parent(result)) - 4);
            if (q == WF_Q_PERFECT) assert(!find(lv_screen_active(), "-0"));
            char name[40];
            snprintf(name, sizeof name, "%s-%s", captures[q], lang ? "en" : "it");
            shot(name);
            rpg_classic_cancel(&R.g);
        }
    }
    puts("parry result: four qualities, touch/BOOT/timeout, both languages and no zero damage on perfect guard");
}

int main(int argc, char **argv) {
    setvbuf(stdout, NULL, _IOLBF, 0);
    assert(argc == 2); dest = argv[1];
    lv_init(); lv_tick_set_cb(tick);
    lv_display_t *display = lv_display_create(WF_RES, WF_RES);
    lv_display_set_buffers(display, drawbuf, NULL, sizeof drawbuf, LV_DISPLAY_RENDER_MODE_PARTIAL);
    lv_display_set_flush_cb(display, flush);
    lv_indev_t *input = lv_indev_create(); lv_indev_set_type(input, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(input, read_touch); lv_timer_set_period(lv_indev_get_read_timer(input), 10);
    new_adventure();
    cadence_and_judgment(); scene_lifetimes(); modal_and_gems(); primary_adventure(); pacing_ui(); parry_result_ui();
    printf("device UI regressions: %d native captures; no physical latency claim\n", shots);
    return 0;
}
