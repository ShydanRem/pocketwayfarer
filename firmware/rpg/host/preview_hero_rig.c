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
        assert(look.class_id==cls && hero_uses_rig(&look)==(cls<HERO_RIG_CLASS_COUNT));
        for(int preset=1;preset<=3;preset++)
            assert(hero_size(&look,preset)==(cls<HERO_RIG_CLASS_COUNT?(preset==3?176:preset==2?88:44):48*preset));
        r_save_now();rpg_game decoded;assert(rpg_save_decode(saved,saved_len,&decoded));
        assert(decoded.hero.cls==cls && decoded.hero.hair_color==R.g.hero.hair_color);
    }
    puts("40 exact class IDs, explicit advanced/master fallback, size presets and save v4 roundtrips passed");
}
static void large_vitals(void) {
    const int classes[]={8,10,16,17,23,24,25,26,27,28,29,30,31,39};
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
    for(int lang=0;lang<2;lang++)for(int cls=24;cls<32;cls++)for(int level=45;level<=99;level+=54) {
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
        assert(lv_obj_get_height(column)==4*wf_font_small.line_height+3*line);
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
    printf("battle footer:%u actual opaque glyph pixels inside circle and below avatar; level99 rig and legacy stress passed\n",footer_ink_pixels);
    printf("hero rig native:%d screenshots,actual creation-adjacent classic views,formations,save/wake,full/incremental render and control separation passed;peak%zu image bytes\n",shots,peak_image_bytes);
    return 0;
}
