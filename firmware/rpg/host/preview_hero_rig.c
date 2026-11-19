/* Native component studio + actual classic screens. The studio is test-only;
 * product screens below exercise the same avatar, controls and cache ownership. */
#define main gameplay_preview_main
#define wf_platform_big_alloc inherited_big_alloc
#include "preview_lantern.c"
#undef main
#undef wf_platform_big_alloc
#include "hero_rig.h"
#include "art/lantern_art.h"
#include "art/hero_rig_art.h"
extern int rpg_battle_target_now(void);
extern int rpg_battle_parry_quality_now(void);

static rpg_game before;
static bool fail_avatar_alloc;
static unsigned footer_ink_pixels;
void *wf_platform_big_alloc(size_t n) {
    if(fail_avatar_alloc){fail_avatar_alloc=false;return NULL;}
    return inherited_big_alloc(n);
}
static void profile(int cls,int race,int skin,int hair,int color) {
    rpg_new_hero h={0};h.cls=(uint8_t)rpg_class_family(cls);h.race=(uint8_t)race;h.skin=(uint8_t)skin;h.hair=(uint8_t)hair;h.hair_color=(uint8_t)color;
    rpg_game_new(&R.g,&h,seconds,177);R.loaded=true;R.g.auto_battle=0;
    if(cls>=FAM_COUNT) {
        R.g.hero.level=20;R.g.hero.job_level=30;
        assert(rpg_change_class(&R.g,cls>=24?rpg_class_parent(cls):cls));
        if(cls>=24) {
            R.g.hero.level=45;R.g.hero.job_level=40;
            assert(rpg_change_class(&R.g,cls));
        }
    }
    before=R.g;
}
static lv_obj_t *avatar(lv_obj_t *root) {
    for(unsigned i=0;i<lv_obj_get_child_count(root);i++) {
        lv_obj_t *o=lv_obj_get_child(root,(int)i);
        if(lv_obj_check_type(o,&lv_image_class) && lv_obj_get_user_data(o))return o;
    }
    return NULL;
}
static void footer_ink(lv_obj_t *label,int hero_bottom) {
    const char *text=lv_label_get_text(label);
    const lv_font_t *font=lv_obj_get_style_text_font(label,0);
    int letter_space=lv_obj_get_style_text_letter_space(label,0);
    int line_space=lv_obj_get_style_text_line_space(label,0);
    lv_area_t area;lv_obj_get_coords(label,&area);int row=0;
    while(*text) {
        const char *end=strchr(text,'\n');size_t n=end?(size_t)(end-text):strlen(text);
        char line[64];assert(n<sizeof line);memcpy(line,text,n);line[n]=0;
        lv_point_t size;lv_text_get_size(&size,line,font,letter_space,0,1000,LV_TEXT_FLAG_NONE);
        int x=area.x1+(lv_obj_get_width(label)-size.x)/2;
        int y=area.y1+row*(font->line_height+line_space);
        for(size_t i=0;i<n;i++) {
            lv_font_glyph_dsc_t g={0};assert(lv_font_get_glyph_dsc(font,&g,(unsigned char)line[i],(unsigned char)line[i+1]));
            g.req_raw_bitmap=1;const uint8_t *bits=lv_font_get_bitmap_fmt_txt(&g,NULL);
            for(int gy=0;gy<g.box_h;gy++)for(int gx=0;gx<g.box_w;gx++) {
                int bit=gy*g.box_w+gx;if(!bits || !(bits[bit/8]&(128>>(bit%8))))continue;
                int sx=x+g.ofs_x+gx,sy=y+font->line_height-font->base_line-g.box_h-g.ofs_y+gy;
                int dx=sx-CX,dy=sy-CY;
                assert(dx*dx+dy*dy<232*232 && sy>hero_bottom);footer_ink_pixels++;
            }
            x+=g.adv_w+letter_space;
        }
        if(!end)break;
        text=end+1;row++;
    }
}
static void controls_clear(void) {
    lv_obj_t *s=lv_screen_active(),*hero=avatar(s);assert(hero);
    lv_obj_update_layout(s);lv_area_t ha;lv_obj_get_coords(hero,&ha);
    const lv_image_dsc_t *d=lv_image_get_src(hero);
    const uint8_t *a=d->data+d->header.stride*d->header.h;
    int hero_bottom=ha.y1;
    for(int y=0;y<(int)d->header.h;y++)for(int x=0;x<(int)d->header.w;x++)if(a[y*d->header.w+x])hero_bottom=ha.y1+y;
    if(R.screen==R_BATTLE)for(unsigned i=0;i<lv_obj_get_child_count(s);i++) {
        lv_obj_t *label=lv_obj_get_child(s,(int)i);
        if(!lv_obj_check_type(label,&lv_label_class))continue;
        const char *text=lv_label_get_text(label);
        if(!strncmp(text,"PV ",3) || !strncmp(text,"HP ",3))footer_ink(label,hero_bottom);
    }
    unsigned checks=0;
    for(unsigned i=0;i<lv_obj_get_child_count(s);i++) {
        lv_obj_t *o=lv_obj_get_child(s,(int)i);
        if(o==hero || lv_obj_has_flag(o,LV_OBJ_FLAG_HIDDEN) || !lv_obj_has_flag(o,LV_OBJ_FLAG_CLICKABLE) ||
           lv_obj_check_type(o,&lv_image_class) || lv_obj_get_width(o)>200 || lv_obj_get_height(o)>100 ||
           lv_obj_get_style_bg_opa(o,0)<200)continue;
        lv_area_t b;lv_obj_get_coords(o,&b);
        for(int y=0;y<(int)d->header.h;y++) for(int x=0;x<(int)d->header.w;x++) {
            if(!a[y*d->header.w+x])continue;
            int sx=ha.x1+x,sy=ha.y1+y;
            if(sx>=b.x1 && sx<=b.x2 && sy>=b.y1 && sy<=b.y2) {
                fprintf(stderr,"avatar/control overlap at%d,%d control%d,%d..%d,%d screen%d\n",sx,sy,b.x1,b.y1,b.x2,b.y2,R.screen);assert(0);
            }
        }
        /* Foe levels must stay legible beside the same action buttons. */
        for(unsigned j=0;j<lv_obj_get_child_count(s);j++) {
            lv_obj_t *label=lv_obj_get_child(s,(int)j);
            if(!lv_obj_check_type(label,&lv_label_class) || !strstr(lv_label_get_text(label),"Lv"))continue;
            lv_area_t a;lv_obj_get_coords(label,&a);
            assert(a.x2<b.x1 || a.x1>b.x2 || a.y2<b.y1 || a.y1>b.y2);
        }
        checks++;
    }
    assert(checks);
}
static void studio(int cls,int race,int skin,int hair,int color,int pose) {
    profile(cls,race,skin,hair,color);R.screen=R_CLASS_ATLAS;
    lv_obj_t *s=kit_screen(&ln_scenes[0]);kit_dim(s,100);
    hero_look look;hero_look_from_game(&look,&R.g);assert(hero_uses_rig(&look));
    kit_label(s,&ln_font_title,0xfff4dd,TX(rpg_class(cls)->name),CX,64);
    kit_label(s,&ln_font_body,0xc5d7e6,TR("Studio del rig","Rig studio"),CX,100);
    lv_obj_t *image=hero_obj(s,&look,pose==HP_HIT?HX_HURT:HX_IDLE,3,CX,237);
    hero_obj_set_pose(image,&look,pose==HP_HIT?HX_HURT:HX_IDLE,3,pose);
    static const wf_text poses[]={{"Riposo","Idle"},{"Respiro","Breath"},{"Attacco","Attack"},{"Guardia","Guard"},{"Colpito","Hit"}};
    kit_label(s,&ln_font_body,0xffce78,TX(poses[pose]),CX,351);
    char label[120];snprintf(label,sizeof label,"%s / %d / %d / %d",TX(rpg_race(race)->name),skin+1,hair+1,color+1);
    kit_label(s,&ln_font_body,0xfff4dd,label,CX,388);
    char name[96];snprintf(name,sizeof name,"studio-%02d-%d-%d",cls,race,pose);shot(name);
    assert(!memcmp(&before,&R.g,sizeof before));
}
static void creation(void) {
    for(int lang=0;lang<2;lang++) {
        wf_lang=(uint8_t)lang;r_go(R_CREATE);run(80);
        for(int step=0;step<6;step++)press(TR("Avanti","Next"));
        for(int cls=0;cls<FAM_COUNT;cls++) {
            assert(find(lv_screen_active(),TX(rpg_class(cls)->name)));
            controls_clear();
            char name[80];snprintf(name,sizeof name,"creazione-%02d-%s",cls,lang?"en":"it");shot(name);
            click(374,300);
        }
    }
    wf_lang=0;
}
static void actual_views(int cls) {
    profile(cls,cls==1?RACE_ELF:RACE_HUMAN,cls?3:1,cls?2:0,cls?5:1);
    const int screens[]={R_TOWN_CLASSIC,R_HERO,R_CLASS,R_EQUIP};
    const char *names[]={"borgo","statistiche","classe","equip"};
    for(int i=0;i<4;i++) {
        r_go(screens[i]);run(80);controls_clear();
        char name[80];snprintf(name,sizeof name,"gioco-%02d-%s",cls,names[i]);shot(name);
    }
    for(int n=1;n<=3;n++) {
        rpg_foe f[3];for(int i=0;i<n;i++)rpg_foe_make(&f[i],i==0?SP_SLIME:i==1?SP_GHOST:SP_BAT,3,false);
        r_start_battle(f,n,false,false);run(1300);assert(rpg_battle_phase()==1);controls_clear();
        char name[80];snprintf(name,sizeof name,"gioco-%02d-lotta-%d",cls,n);shot(name);
        if(n>1) { /* Both side targets stay reachable by real taps. */
            click(n==2?140:112,190);assert(rpg_battle_target_now()==0);
            click(n==2?326:354,190);assert(rpg_battle_target_now()==n-1);
        }
        wf_app_flush_save();ms+=20000;seconds+=20;wf_app_wake();run(30);
        assert(R.screen==R_BATTLE && rpg_battle_phase()==1);
    }
    rpg_foe boss;rpg_foe_make(&boss,SP_BONE_KING,8,false);
    r_start_battle(&boss,1,false,true);run(1300);controls_clear();
    char name[80];snprintf(name,sizeof name,"gioco-%02d-boss",cls);shot(name);
    for(int n=2;n<=3;n++) {
        rpg_foe f[3];for(int i=0;i<n;i++)rpg_foe_make(&f[i],i==1?SP_GHOST:SP_SLIME,99,true);
        r_start_battle(f,n,false,false);run(1300);controls_clear();
        snprintf(name,sizeof name,"gioco-%02d-elite-%d",cls,n);shot(name);
    }
    assert(rpg_dungeon_enter(&R.g,0,0,seconds)==RPG_OK);
    r_go(R_DUNGEON);run(80);controls_clear();
    snprintf(name,sizeof name,"gioco-%02d-dungeon",cls);shot(name);
}
static void changing_look(void) {
    profile(0,RACE_BEAST,4,5,7);R.screen=R_CLASS_ATLAS;lv_obj_t *s=kit_screen(NULL);
    size_t persistent=image_bytes; /* existing app-lifetime FX cache, not avatar ownership */
    hero_look look;hero_look_from_game(&look,&R.g);
    lv_obj_t *o=hero_obj(s,&look,HX_IDLE,3,CX,CY);
    size_t steady=image_bytes;
    for(int i=0;i<1000;i++) {
        look.hair=(uint8_t)(i%6);look.hair_color=(uint8_t)(i%8);look.skin=(uint8_t)(i%5);look.eyes=(uint8_t)(i%4);look.body=(uint8_t)(i%2);
        hero_obj_set_pose(o,&look,HX_IDLE,3,i%HP_COUNT);assert(image_bytes==steady);
    }
    R.g.lantern.reduced_motion=1;
    hero_obj_set_pose(o,&look,HX_IDLE,3,HP_IDLE);
    const lv_image_dsc_t *idle=lv_image_get_src(o);
    for(int pose=HP_BREATH;pose<HP_COUNT;pose++) {
        hero_obj_set_pose(o,&look,HX_IDLE,3,pose);assert(lv_image_get_src(o)==idle);
    }
    R.g.lantern.reduced_motion=0;
    kit_screen(NULL);assert(image_bytes==persistent);
    printf("real avatar widget:1000 cosmetic/pose swaps retain one buffer; scene deletes all avatar bytes; persistent FX baseline%zu: passed\n",persistent);
}
static void class_mapping(void) {
    for(int cls=0;cls<RPG_CLASS_COUNT;cls++) {
        profile(rpg_class_family(cls),0,1,0,1);R.g.hero.cls=(uint8_t)cls;
        hero_look look;hero_look_from_game(&look,&R.g);
        assert(look.class_id==cls && hero_uses_rig(&look));
        for(int preset=1;preset<=3;preset++)
            assert(hero_size(&look,preset)==(preset==3?176:preset==2?88:44));
        r_save_now();rpg_game decoded;assert(rpg_save_decode(saved,saved_len,&decoded));
        assert(decoded.hero.cls==cls && decoded.hero.hair_color==R.g.hero.hair_color);
    }
    puts("40 exact class IDs, all modular costumes, size presets and save v4 roundtrips passed");
}
static void large_vitals(void) {
    const int classes[]={8,10,16,17,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39};
    for(int lang=0;lang<2;lang++)for(unsigned i=0;i<sizeof classes/sizeof classes[0];i++) {
        int cls=classes[i];profile(cls,RACE_HUMAN,1,0,1);
        R.g.hero.level=99;for(int s=0;s<ST_COUNT;s++)R.g.hero.stat[s]=99;
        wf_lang=(uint8_t)lang;R.g.lang=(uint8_t)lang;
        rpg_stats st;rpg_stats_get(&R.g,&st);R.g.hero.hp=st.max_hp;R.g.hero.sp=st.max_sp;
        rpg_foe foe;rpg_foe_make(&foe,SP_GHOST,99,true);
        r_start_battle(&foe,1,false,false);run(1300);controls_clear();
        char name[80];snprintf(name,sizeof name,"vitali-grandi-%02d-%s",cls,lang?"en":"it");shot(name);
    }
    wf_lang=0;
}
static void master_stats(void) {
    unsigned checks=0;
    for(int lang=0;lang<2;lang++)for(int cls=24;cls<RPG_CLASS_COUNT;cls++)for(int level=45;level<=99;level+=54) {
        profile(cls,RACE_HUMAN,4,5,7);R.g.hero.level=(uint8_t)level;
        wf_lang=R.g.lang=(uint8_t)lang;r_go(R_HERO);run(30);
        lv_obj_t *screen=lv_screen_active(),*hero=avatar(screen),*column=NULL;assert(hero);
        for(unsigned i=0;i<lv_obj_get_child_count(screen);i++) {
            lv_obj_t *o=lv_obj_get_child(screen,(int)i);
            if(lv_obj_check_type(o,&lv_label_class) &&
               !strncmp(lv_label_get_text(o),R.g.hero.name,strlen(R.g.hero.name)) &&
               strstr(lv_label_get_text(o),TX(rpg_class(cls)->name)))column=o;
        }
        assert(column);lv_obj_update_layout(screen);
        lv_area_t text,actor;lv_obj_get_coords(column,&text);lv_obj_get_coords(hero,&actor);
        assert(text.x1>=160 && text.x2<=419 && text.x1>actor.x2);
        assert(lv_obj_get_width(column)==260);
        /* Four complete unwrapped lines, with the entire text box in the circle. */
        int line=lv_obj_get_style_text_line_space(column,0);
        assert(lv_obj_get_height(column)==4*lv_obj_get_style_text_font(column,0)->line_height+3*line);
        int corners[][2]={{text.x1,text.y1},{text.x2,text.y1},{text.x1,text.y2},{text.x2,text.y2}};
        for(unsigned i=0;i<4;i++){int x=corners[i][0]-CX,y=corners[i][1]-CY;assert(x*x+y*y<233*233);}
        controls_clear();checks++;
        char name[96];snprintf(name,sizeof name,"statistiche-maestra-%02d-%d-%s",cls,level,lang?"en":"it");shot(name);
    }
    wf_lang=0;
    printf("master stats:%u IT/EN level45/99 text columns, four unwrapped lines inside circle and separate from avatar canvas passed\n",checks);
}
static void allocation_recovery(void) {
    profile(7,RACE_BEAST,4,5,7);R.screen=R_CLASS_ATLAS;
    lv_obj_t *s=kit_screen(NULL);hero_look look;hero_look_from_game(&look,&R.g);
    fail_avatar_alloc=true;
    lv_obj_t *o=hero_obj(s,&look,HX_IDLE,3,CX,CY);assert(!lv_image_get_src(o));
    hero_obj_pose(o,HP_GUARD);assert(lv_image_get_src(o));
    lv_obj_t *reference=hero_obj(s,&look,HX_IDLE,3,CX,CY);
    hero_obj_set_pose(reference,&look,HX_IDLE,3,HP_GUARD);
    const void *expected=lv_image_get_src(reference);assert(lv_image_get_src(o)==expected);
    hero_look other=look;other.hair=(uint8_t)((look.hair+1)%6);
    fail_avatar_alloc=true;hero_obj_set_pose(o,&other,HX_HAPPY,3,HP_ATTACK);
    assert(lv_image_get_src(o)==expected);
    hero_obj_pose(o,HP_GUARD);assert(lv_image_get_src(o)==expected);
    kit_screen(NULL);
    puts("avatar allocation failure: initial retry keeps intended hero; live failure retains matching image/key: passed");
}

static void flow_shot(const char *part) {
    char name[80];
    snprintf(name, sizeof name, "interfaccia-%s-%s", part, wf_lang ? "en" : "it");
    shot(name);
}

/* Navigate through public touch targets and verify the persisted outcomes. */
static void character_flow(void) {
    static const char *steps[] = {"razza", "pelle", "capelli", "colore", "occhi", "viso", "classe", "nome"};
    for (int lang = 0; lang < 2; lang++) {
        wf_app_start(NULL, 0);
        wf_lang = (uint8_t)lang;
        r_go(R_TITLE);
        press(TR("Personalizza", "Customize"));
        assert(R.screen == R_CREATE);
        int initial_saves = saves;
        for (int step = 0; step < 8; step++) {
            press(">");
            controls_clear();
            flow_shot(steps[step]);
            if (step == 6) {
                press(TR("Indietro", "Back"));
                press(TR("Avanti", "Next"));
            }
            assert(saves == initial_saves);
            press(step == 7 ? TR("Inizia!", "Begin!") : TR("Avanti", "Next"));
        }
        assert(R.screen == R_TOWN && R.loaded && saves > initial_saves);
        rpg_game decoded;
        assert(rpg_save_decode(saved, saved_len, &decoded));
        assert(decoded.lang == lang && !memcmp(&decoded.hero, &R.g.hero, sizeof decoded.hero));
        press(TR("Andiamo!", "Let's go!"));
        press("...");
        press(TR("Classico", "Classic"));
        assert(R.screen == R_TOWN_CLASSIC);
        click(374, 196);
        assert(R.screen == R_HERO);
        int points = R.g.hero.stat_points;
        int strength = R.g.hero.stat[ST_STR];
        press("+");
        assert(R.g.hero.stat_points < points && R.g.hero.stat[ST_STR] == strength + 1);
        flow_shot("statistiche");
        press(TR("Abilità", "Skills"));
        assert(R.screen == R_SKILLS);
        flow_shot("abilita");
        points = R.g.hero.skill_points;
        press("+");
        assert(R.g.hero.skill_points == points - 1);
        press(TR("Indietro", "Back"));
        assert(R.screen == R_HERO);
        press(TR("Classe", "Class"));
        assert(R.screen == R_CLASS);
        flow_shot("promozioni");
        assert(wf_app_key(WF_KEY_BOOT_SHORT) && R.screen == R_HERO);
        press(TR("Indietro", "Back"));
        assert(R.screen == R_TOWN_CLASSIC);
        click(92, 296);
        assert(R.screen == R_EQUIP);
        controls_clear();
        flow_shot("equipaggiamento");
        rpg_item weapon = R.g.hero.equip[SLOT_WEAPON];
        click(96, 230);
        flow_shot("dettaglio");
        press(TR("Togli", "Remove"));
        assert(R.screen == R_EQUIP && rpg_item_empty(&R.g.hero.equip[SLOT_WEAPON]));
        click(96, 230);
        assert(R.screen == R_BAG && rpg_bag_count(&R.g) == 1);
        click(CX - 2, 157);
        press(TR("Equipaggia", "Equip"));
        assert(!memcmp(&R.g.hero.equip[SLOT_WEAPON], &weapon, sizeof weapon));
        assert(wf_app_key(WF_KEY_BOOT_SHORT) && R.screen == R_EQUIP);
        assert(wf_app_key(WF_KEY_BOOT_SHORT) && R.screen == R_TOWN_CLASSIC);
        r_save_now();
        assert(rpg_save_decode(saved, saved_len, &decoded));
        assert(!memcmp(&decoded.hero, &R.g.hero, sizeof decoded.hero));
    }
    wf_lang = 0;
    puts("character flow: IT/EN creation, stat and skill point spending, match touch/BOOT back routes, remove and equip via bag, v4 saves passed");
}

static void lumen_shot(const char *part) {
    char name[80];
    snprintf(name, sizeof name, "lumen-%s-%s", part, wf_lang ? "en" : "it");
    shot(name);
}

static void lumen_flow(void) {
    enum { CHOOSE = 1, RING = 3, PARRY = 6, END = 7 };
    for (int lang = 0; lang < 2; lang++) {
        profile(FAM_WARRIOR, RACE_HUMAN, 1, 0, 1);
        R.g.lang = wf_lang = (uint8_t)lang;
        R.g.hero.stat[ST_STR] += 20;
        R.g.hero.stat[ST_VIT] += 20;
        r_go(R_TOWN);
        press("...");
        press(TR("Classico", "Classic"));
        assert(R.screen == R_TOWN_CLASSIC);
        lumen_shot("borgo");
        press(TR("Mappa", "Map"));
        assert(R.screen == R_MAP);
        click(232, 372);
        assert(R.screen == R_ZONE && R.zone == 0);
        lumen_shot("prati");

        rpg_foe foes[RPG_MAX_FOES];
        uint32_t seed;
        for (seed = 1; seed < 10000; seed++) {
            rpg_game sim = R.g;
            sim.rng_state = seed;
            int count;
            assert(rpg_field_encounter(&sim, 0, foes, &count) == RPG_OK);
            if (count == 3 && foes[0].species != foes[1].species &&
                foes[0].species != foes[2].species && foes[1].species != foes[2].species) break;
        }
        assert(seed < 10000);
        R.g.rng_state = seed;
        uint32_t gold = R.g.gold;
        int stamina = R.g.hero.stamina;
        press(TR("Combatti", "Fight"));
        assert(R.screen == R_BATTLE && R.g.hero.stamina == stamina - 1);
        run(1300);
        assert(rpg_battle_phase() == CHOOSE);
        controls_clear();
        const int points[3][2] = {{388, 190}, {233, 169}, {78, 190}};
        for (int target = 2; target >= 0; target--) {
            click(points[2 - target][0], points[2 - target][1]);
            assert(rpg_battle_target_now() == target);
            assert(find(lv_screen_active(), TX(rpg_species(foes[target].species)->name)));
        }
        lumen_shot("trio");
        press(TR("Guardia", "Guard"));
        bool attacked = false, parried = false;
        for (unsigned t = 0; t < 4000 && rpg_battle_phase() != END; t++) {
            int ph = rpg_battle_phase();
            if (ph == CHOOSE) {
                press(TR("Attacco", "Attack"));
                assert(rpg_battle_phase() == RING);
            } else if (ph == RING && rpg_battle_ring_quality_now() >= WF_Q_GREAT) {
                if (!attacked) lumen_shot("anello");
                if (lang) assert(wf_app_key(WF_KEY_BOOT_DOWN));
                else click(CX, CY);
                assert(rpg_battle_phase() != RING);
                attacked = true;
            } else if (ph == PARRY && rpg_battle_parry_quality_now() >= WF_Q_GREAT) {
                if (!parried) lumen_shot("parata");
                if (lang) assert(wf_app_key(WF_KEY_BOOT_DOWN));
                else click(CX, CY);
                assert(rpg_battle_phase() != PARRY);
                parried = true;
            }
            run(10);
        }
        assert(attacked && parried && rpg_battle_phase() == END);
        assert(R.g.gold > gold && find(lv_layer_top(), TR("Vittoria!", "Victory!")));
        lumen_shot("vittoria");
        press(TR("Continua", "Continue"));
        assert(R.screen == R_ZONE && !R.g.classic.active);
        rpg_game decoded;
        assert(rpg_save_decode(saved, saved_len, &decoded) && decoded.gold == R.g.gold);
        assert(!decoded.classic.active && decoded.lang == lang);
    }
    wf_lang = 0;

    profile(FAM_WARRIOR, RACE_HUMAN, 1, 0, 1);
    R.g.best_floor[0] = 5;
    r_go(R_ZONE);
    assert(find(lv_screen_active(), "Record piano 5"));
    press("Dal piano 6");
    assert(R.screen == R_DUNGEON && R.g.run.floor == 6);
    rpg_foe crypt[] = {{0}, {0}, {0}};
    rpg_foe_make(&crypt[0], SP_SKELETON, 8, false);
    rpg_foe_make(&crypt[1], SP_GHOST, 8, true);
    rpg_foe_make(&crypt[2], SP_BAT, 7, false);
    r_start_battle(crypt, 3, true, false);
    run(1300);
    controls_clear();
    lumen_shot("cripta");
    rpg_foe king;
    rpg_foe_make(&king, SP_SLIME_KING, 8, false);
    r_start_battle(&king, 1, true, true);
    run(1300);
    controls_clear();
    lumen_shot("re-gelatino");

    profile(FAM_WARRIOR, RACE_HUMAN, 1, 0, 1);
    rpg_foe field[3];
    rpg_foe_make(&field[0], SP_SLIME, 3, false);
    rpg_foe_make(&field[1], SP_SHROOM, 3, false);
    rpg_foe_make(&field[2], SP_WISP, 3, true);
    r_start_battle(field, 3, false, false);
    run(1300);
    controls_clear();
    lumen_shot("bestiario");
    puts("Lumen: IT/EN town-to-victory touch, all three targets, timed touch/BOOT, v4 rewards, floor 6 and new sprites passed");
}

static void crypt_shot(const char *stage) {
    char name[80];
    snprintf(name, sizeof name, "crypt-%s-%s", stage, wf_lang ? "en" : "it");
    shot(name);
}

static void stage_boss_floor(int floor) {
    R.g.run.floor = (uint8_t)floor;
    rpg_dungeon_floor(&R.g.run);
    R.g.run.row = RPG_ROWS - 1;
    for (int col = 0; col < RPG_COLS; col++) {
        R.g.run.col = (int8_t)col;
        if (R.g.run.rooms[RPG_ROWS - 1][col]) break;
    }
    assert(rpg_room_reachable(&R.g.run, RPG_ROWS, 1));
    r_go(R_DUNGEON);
}

static void auto_win(void) {
    R.g.auto_battle = 1;
    for (int t = 0; t < 1800 && rpg_battle_phase() != 7; t++) run(20);
    if (R.g.classic.outcome != B_WON)
        fprintf(stderr, "Zone %d floor %d: expected victory, phase %d outcome %d\n",
                R.zone, R.g.run.floor, rpg_battle_phase(), R.g.classic.outcome);
    assert(R.g.classic.active == 2 && R.g.classic.outcome == B_WON);
    assert(find(lv_layer_top(), TR("Vittoria!", "Victory!")));
}

static void crypt_flow(void) {
    for (int lang = 0; lang < 2; lang++) {
        profile(FAM_WARRIOR, RACE_HUMAN, 1, 0, 1);
        R.g.lang = wf_lang = (uint8_t)lang;
        R.g.hero.stat[ST_STR] += 90;
        R.g.hero.stat[ST_VIT] += 40;
        rpg_stats stats;
        rpg_stats_get(&R.g, &stats);
        R.g.hero.hp = stats.max_hp;
        R.g.cons[CO_POTION] = 2;
        R.g.cons[CO_RETURN] = 1;
        R.zone = 0;
        r_go(R_ZONE);
        run(1000 - ms % 1000);

        /* Generate a real opening with a chest, then pick an RNG state
         * that produces a mimic through the production resolver. */
        int chest = -1;
        for (uint32_t seed = 1; seed <= 10000 && chest < 0; seed++) {
            rpg_game sim = R.g;
            sim.rng_state = seed;
            assert(rpg_dungeon_enter(&sim, 0, 0, seconds) == RPG_OK);
            for (int col = 0; col < RPG_COLS; col++)
                if (sim.run.rooms[0][col] == ROOM_CHEST) chest = col;
            if (chest >= 0) R.g.rng_state = seed;
        }
        assert(chest >= 0);
        press(TR("Cripta dei Lumi", "Lantern Crypt"));
        assert(R.screen == R_DUNGEON && R.g.run.floor == 1 && R.g.run.row == -1);
        assert(R.g.run.rooms[0][chest] == ROOM_CHEST);
        assert(!find(lv_screen_active(), TR("Dal piano 6", "From floor 6")));
        crypt_shot("ingresso");
        click(CX, 108); /* visible but unreachable stairs */
        assert(R.screen == R_DUNGEON && R.g.run.row == -1);

        uint32_t mimic_seed;
        for (mimic_seed = 1; mimic_seed <= 10000; mimic_seed++) {
            rpg_game sim = R.g;
            sim.rng_state = mimic_seed;
            rpg_foe foes[RPG_MAX_FOES];
            rpg_room_result reward;
            int count;
            assert(rpg_room_enter(&sim, 0, chest, foes, &count) == ROOM_CHEST);
            rpg_room_resolve(&sim, &reward, foes, &count);
            if (reward.mimic) break;
        }
        assert(mimic_seed <= 10000);
        R.g.rng_state = mimic_seed;
        uint32_t gold = R.g.gold;
        int stamina = R.g.hero.stamina;
        click(131 + chest * 102, 340);
        assert(R.screen == R_BATTLE && R.g.classic.active == 1 && R.g.hero.stamina == stamina - 2);
        run(1300);
        assert(find(lv_screen_active(), "Mimic"));
        crypt_shot("mimic");
        wf_app_flush_save();
        wf_app_start(saved, saved_len);
        run(100);
        assert(R.screen == R_DUNGEON && R.g.run.row == -1 && !R.g.run.cleared[0]);
        assert(R.g.gold == gold && R.g.hero.stamina == stamina && R.g.rng_state == mimic_seed);
        click(131 + chest * 102, 340);
        assert(R.screen == R_BATTLE);
        run(1300);
        assert(find(lv_screen_active(), "Mimic"));
        auto_win();
        uint32_t loot = R.g.gold;
        assert(loot > gold);
        assert(R.g.run.gold == loot - gold);
        wf_app_start(saved, saved_len); /* restart before Continue */
        assert(R.screen == R_DUNGEON && R.g.run.row == 0 && R.g.run.col == chest);
        assert(R.g.gold == loot && !R.g.classic.active && (R.g.run.cleared[0] & (1u << chest)));
        assert(R.g.run.gold == loot - gold);
        rpg_game defeated = R.g;
        rpg_dungeon_leave(&defeated, true);
        assert(defeated.gold == loot - (loot - gold) / 2u);
        assert(find(lv_screen_active(), TR("Qui", "Here")));
        crypt_shot("percorso");
        int potions = R.g.cons[CO_POTION], scrolls = R.g.cons[CO_RETURN];
        assert(potions && scrolls);
        R.g.hero.hp -= 15;
        int hp = R.g.hero.hp;
        press(TR("Scorte", "Supplies"));
        press(TR("Pozione", "Potion"));
        assert(R.g.cons[CO_POTION] == potions - 1 && R.g.hero.hp > hp);
        press(TR("Esci", "Exit"));
        press(TR("Resta", "Stay"));
        assert(R.screen == R_DUNGEON && R.g.cons[CO_RETURN] == scrolls);

        int trap = 0;
        while (trap < RPG_COLS && !rpg_room_reachable(&R.g.run, 1, trap)) trap++;
        assert(trap < RPG_COLS);
        R.g.run.rooms[1][trap] = ROOM_TRAP;
        r_go(R_DUNGEON);
        run(100);
        click(131 + trap * 102, 282);
        assert(R.g.run.row == 1 && find(lv_layer_top(), TR("Trappola!", "Trap!")));
        press(TR("Avanti", "Onward"));
        assert(find(lv_screen_active(), TR("Visitata", "Visited")));
        int merchant = 0;
        while (merchant < RPG_COLS && !rpg_room_reachable(&R.g.run, 2, merchant)) merchant++;
        assert(merchant < RPG_COLS);
        R.g.run.rooms[2][merchant] = ROOM_MERCHANT;
        r_go(R_DUNGEON);
        run(100);
        click(131 + merchant * 102, 224);
        assert(R.screen == R_SHOP && R.shop_in_dungeon);
        assert(wf_app_key(WF_KEY_BOOT_SHORT) && R.screen == R_DUNGEON && R.g.run.row == 2);

        /* The route to the boss is generated. The fixture advances past
         * subsequent floors so the actual OS input can check both bosses. */
        R.g.auto_battle = 0;
        stage_boss_floor(5);
        press("Boss");
        run(1300);
        assert(find(lv_screen_active(), TR("Re Gelatino", "Slime King")));
        auto_win();
        press(TR("Continua", "Continue"));
        press(TR("Città", "Town"));
        assert(R.screen == R_TOWN && rpg_dungeon_checkpoint(&R.g, 0));
        press("Ok");
        wf_app_start(saved, saved_len);
        assert(!R.g.run.active && R.g.best_floor[0] == 5);
        R.zone = 0;
        r_go(R_ZONE);
        press(TR("Dal piano 6", "From floor 6"));
        assert(R.screen == R_DUNGEON && R.g.run.floor == 6 && R.g.run.row == -1);
        crypt_shot("checkpoint");

        R.g.auto_battle = 0;
        R.g.hero.level = 18;
        rpg_stats_get(&R.g, &stats);
        R.g.hero.hp = stats.max_hp;
        R.g.hero.sp = stats.max_sp;
        stage_boss_floor(10);
        crypt_shot("soglia");
        press("Boss");
        run(1300);
        assert(find(lv_screen_active(), TR("Re delle Ossa", "Bone King")));
        controls_clear();
        crypt_shot("re-delle-ossa");
        auto_win();
        loot = R.g.gold;
        wf_app_start(saved, saved_len);
        assert(R.screen == R_TOWN && R.g.zone_unlocked == 1 && R.g.best_floor[0] == 10);
        assert(!R.g.run.active && R.g.gold == loot && !R.g.classic.active);
        assert(find(lv_layer_top(), TR("Dungeon completato!", "Dungeon cleared!")));
        wf_app_start(saved, saved_len);
        assert(R.g.gold == loot && R.g.zone_unlocked == 1);
        R.zone = 0;
        r_go(R_ZONE);
        press(TR("Cripta dei Lumi", "Lantern Crypt"));
        scrolls = R.g.cons[CO_RETURN];
        press(TR("Esci", "Exit"));
        press(TR("Esci", "Leave"));
        assert(R.screen == R_TOWN && !R.g.run.active && R.g.cons[CO_RETURN] == scrolls - 1);
        wf_app_start(saved, saved_len);
        assert(!R.g.run.active && R.g.cons[CO_RETURN] == scrolls - 1);
    }
    wf_lang = 0;
    puts("Crypt: IT/EN touch entrance/locked rooms, mimic retry, exact-once loot, boss checkpoint after leaving, and floor 10 unlock passed");
}

static void expedition_shot(const char *region, const char *stage) {
    char name[80];
    snprintf(name, sizeof name, "%s-%s-%s", region, stage, wf_lang ? "en" : "it");
    shot(name);
}

static void map_zone(int zone) {
    r_go(R_MAP);
    run(120);
    click(rpg_world_spots[zone][0], rpg_world_spots[zone][1]);
}

/* Pick deterministic encounters through the real generator, without
 * bypassing the field button, stamina cost or monster targeting. */
static void field_fixture(unsigned mask, int total, rpg_foe *foes) {
    const rpg_zone_info *z = rpg_zone(R.zone);
    for (uint32_t seed = 1; seed <= 10000; seed++) {
        rpg_game sim = R.g;
        sim.rng_state = seed;
        int count;
        assert(rpg_field_encounter(&sim, R.zone, foes, &count) == RPG_OK);
        unsigned species = 0;
        for (int i = 0; i < count; i++)
            for (int j = 0; j < 4; j++)
                if (foes[i].species == z->field_foes[j]) species |= 1u << j;
        if (species != mask || count != total) continue;
        R.g.rng_state = seed;
        int stamina = R.g.hero.stamina;
        press(TR("Combatti", "Fight"));
        assert(R.screen == R_BATTLE && R.g.hero.stamina == stamina - 1);
        run(1300);
        assert(rpg_battle_phase() == 1);
        controls_clear();
        return;
    }
    assert(!"No matching field encounter found");
}

typedef struct {
    const char *region, *landing, *single, *dungeon, *pair, *mid, *final;
    int first_species, second_species;
    bool trap;
} expedition_example;

static void expedition_flow(int zone, const expedition_example *example) {
    const rpg_zone_info *z = rpg_zone(zone);
    assert(R.g.zone_unlocked == zone && R.g.best_floor[zone - 1] == RPG_FLOORS && !R.g.run.active);
    rpg_game unlocked = R.g;
    for (int lang = 0; lang < 2; lang++) {
        R.g = unlocked;
        R.g.lang = wf_lang = (uint8_t)lang;
        R.g.zone_unlocked = (uint8_t)(zone - 1);
        map_zone(zone);
        assert(R.screen == R_MAP);
        R.g.zone_unlocked = unlocked.zone_unlocked;
        R.g.auto_battle = 0;
        R.g.hero.level = (uint8_t)(z->max_level + 12);
        rpg_stats stats;
        rpg_stats_get(&R.g, &stats);
        R.g.hero.hp = stats.max_hp;
        R.g.hero.sp = stats.max_sp;
        map_zone(zone);
        assert(R.screen == R_ZONE && R.zone == zone);
        assert(find(lv_screen_active(), TX(z->field_name)));
        expedition_shot(example->region, example->landing);

        rpg_foe foes[RPG_MAX_FOES];
        field_fixture(0x07, 3, foes);
        const int points[3][2] = {{78, 190}, {233, 169}, {388, 190}};
        for (int i = 2; i >= 0; i--) {
            click(points[i][0], points[i][1]);
            assert(rpg_battle_target_now() == i);
            assert(find(lv_screen_active(), TX(rpg_species(foes[i].species)->name)));
        }
        expedition_shot(example->region, "trio");
        press(TR("Attacco", "Attack"));
        assert(rpg_battle_phase() == 3);
        for (int t = 0; t < 500 && rpg_battle_ring_quality_now() < 2; t++) run(5);
        assert(rpg_battle_ring_quality_now() >= 2);
        if (lang) assert(wf_app_key(WF_KEY_BOOT_DOWN));
        else click(CX, CY);
        assert(rpg_battle_phase() != 3);
        auto_win();
        press(TR("Continua", "Continue"));
        assert(R.screen == R_ZONE);
        R.g.auto_battle = 0;
        field_fixture(0x08, 1, foes);
        expedition_shot(example->region, example->single);
        auto_win();
        press(TR("Continua", "Continue"));
        assert(R.screen == R_ZONE);

        R.g.auto_battle = 0;
        press(TX(z->dungeon_name));
        assert(R.screen == R_DUNGEON && R.g.run.zone == zone && R.g.run.floor == 1);
        assert(find(lv_screen_active(), TX(z->dungeon_name)));
        expedition_shot(example->region, example->dungeon);
        int col = 0;
        while (col < RPG_COLS && !rpg_room_reachable(&R.g.run, 0, col)) col++;
        assert(col < RPG_COLS);
        R.g.run.rooms[0][col] = ROOM_FIGHT;
        uint32_t seed;
        for (seed = 1; seed <= 10000; seed++) {
            rpg_game sim = R.g;
            sim.rng_state = seed;
            int count;
            assert(rpg_room_enter(&sim, 0, col, foes, &count) == ROOM_FIGHT);
            if (count == 2 &&
                ((foes[0].species == example->first_species && foes[1].species == example->second_species) ||
                 (foes[1].species == example->first_species && foes[0].species == example->second_species))) break;
        }
        assert(seed <= 10000);
        R.g.rng_state = seed;
        r_go(R_DUNGEON);
        run(120);
        click(131 + 102 * col, 340);
        assert(R.screen == R_BATTLE && R.g.run.row == 0);
        run(1300);
        assert(find(lv_screen_active(), TX(rpg_species(foes[0].species)->name)));
        controls_clear();
        expedition_shot(example->region, example->pair);
        auto_win();
        press(TR("Continua", "Continue"));
        assert(R.screen == R_DUNGEON && (R.g.run.cleared[0] & (1u << col)));
        if (example->trap) {
            int trap = 0;
            while (trap < RPG_COLS && !rpg_room_reachable(&R.g.run, 1, trap)) trap++;
            assert(trap < RPG_COLS);
            R.g.run.rooms[1][trap] = ROOM_TRAP;
            r_go(R_DUNGEON);
            assert(find(lv_screen_active(), TR("Trappola", "Trap")));
            expedition_shot(example->region, "bivio");
            click(131 + 102 * trap, 282);
            assert(R.g.run.row == 1 && find(lv_layer_top(), TR("Trappola!", "Trap!")));
            press(TR("Avanti", "Onward"));
            /* The Wood fixture must clear the healing boss reliably.
             * Keep encounter balance to the separately leveled core runs. */
            R.g.hero.equip[SLOT_WEAPON] = (rpg_item){
                .type = IT_SWORD, .tier = RPG_TIERS - 1, .rarity = RAR_LEGENDARY, .plus = 10, .level = 40,
                .aff = {AF_ATK_PCT}, .aff_val = {50},
            };
        }

        R.g.auto_battle = 0;
        stage_boss_floor(5);
        press("Boss");
        run(1300);
        assert(find(lv_screen_active(), TX(rpg_species(z->mid_boss)->name)));
        controls_clear();
        expedition_shot(example->region, example->mid);
        auto_win();
        press(TR("Continua", "Continue"));
        press(TR("Città", "Town"));
        assert(R.screen == R_TOWN && rpg_dungeon_checkpoint(&R.g, zone));
        press("Ok");
        map_zone(zone);
        press(TR("Dal piano 6", "From floor 6"));
        assert(R.screen == R_DUNGEON && R.g.run.zone == zone && R.g.run.floor == 6);
        R.g.auto_battle = 0;
        /* Navigation/recovery fixture. Balance is checked separately using
         * leveled heroes in the core dungeon simulations. */
        R.g.hero.level = 65;
        R.g.hero.stat[ST_VIT] = 99;
        R.g.hero.stat[ST_AGI] = 80;
        rpg_stats_get(&R.g, &stats);
        R.g.hero.hp = stats.max_hp;
        R.g.hero.sp = stats.max_sp;
        stage_boss_floor(10);
        press("Boss");
        run(1300);
        assert(find(lv_screen_active(), TX(rpg_species(z->final_boss)->name)));
        controls_clear();
        expedition_shot(example->region, example->final);
        auto_win();
        uint32_t gold = R.g.gold;
        wf_app_start(saved, saved_len);
        int next = zone + 1;
        assert(R.screen == R_TOWN && R.g.zone_unlocked == next && R.g.best_floor[zone] == RPG_FLOORS);
        assert(!R.g.run.active && R.g.gold == gold && !R.g.classic.active);
        assert(find(lv_layer_top(), TR("Dungeon completato!", "Dungeon cleared!")));
        press("Ok");
        map_zone(next);
        assert(R.screen == R_ZONE && R.zone == next);
        assert(find(lv_screen_active(), TX(rpg_zone(next)->field_name)));
    }
    wf_lang = 0;
    printf("%s: IT/EN eight species, targeting, touch/BOOT attack, two bosses, checkpoint and zone %d unlock passed\n",
           example->region, zone + 1);
}

int main(int argc,char **argv) {
    assert(argc==2);dest=argv[1];setvbuf(stdout,NULL,_IOLBF,0);
    lv_init();lv_tick_set_cb(tick);lv_display_t *display=lv_display_create(WF_RES,WF_RES);
    lv_display_set_buffers(display,drawbuf,NULL,sizeof drawbuf,LV_DISPLAY_RENDER_MODE_PARTIAL);lv_display_set_flush_cb(display,flush);
    lv_indev_t *input=lv_indev_create();lv_indev_set_type(input,LV_INDEV_TYPE_POINTER);lv_indev_set_read_cb(input,read_touch);
    lv_timer_set_period(lv_indev_get_read_timer(input),10);wf_app_start(NULL,0);
    creation();
    for(int cls=0;cls<HERO_RIG_CLASS_COUNT;cls++) {
        for(int pose=0;pose<HP_COUNT;pose++)studio(cls,RACE_HUMAN,1,0,1,pose);
        actual_views(cls);
    }
    for(int race=1;race<RACE_COUNT;race++)studio(0,race,race,4,6,HP_IDLE);
    large_vitals();
    master_stats();
    changing_look();
    allocation_recovery();
    class_mapping();
    character_flow();
    lumen_flow();
    crypt_flow();
    const expedition_example shore = {
        "shore", "riva", "pirata", "tempio", "serpe-e-annegato", "granchio-corazzato", "leviatano",
        SP_SERPENT, SP_DEEP_GHOST, false,
    };
    const expedition_example wood = {
        "wood", "bosco", "ragno", "radici", "fungo-e-fata", "antico-ceppo", "regina-ragno",
        SP_TOXIC_SHROOM, SP_DARK_FAIRY, true,
    };
    expedition_flow(1, &shore);
    expedition_flow(2, &wood);
    printf("battle footer:%u actual opaque glyph pixels inside circle and below avatar; level99 all master rig stress passed\n",footer_ink_pixels);
    printf("hero rig native:%d screenshots,actual creation-adjacent classic views,formations,save/wake,full/incremental render and control separation passed;peak%zu image bytes\n",shots,peak_image_bytes);
    return 0;
}
