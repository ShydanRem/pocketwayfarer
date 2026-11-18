/* Real touch promotion gates and exact-class visuals under identical gear. */
#define main gameplay_preview_main
#include "preview_lantern.c"
#undef main
#include "art/lantern_art.h"

static const lv_image_dsc_t *avatar_source(void) {
    lv_obj_t *s=lv_screen_active();
    for(unsigned i=0;i<lv_obj_get_child_count(s);i++) {
        lv_obj_t *o=lv_obj_get_child(s,(int)i);
        if(lv_obj_check_type(o,&lv_image_class) && lv_obj_get_user_data(o))return lv_image_get_src(o);
    }
    assert(0);return NULL;
}
static uint64_t fingerprint(const lv_image_dsc_t *image) {
    assert(image);uint64_t h=UINT64_C(14695981039346656037);
    for(uint32_t i=0;i<image->data_size;i++){h^=image->data[i];h*=UINT64_C(1099511628211);}
    return h;
}
static void seed_profile(int family,int language) {
    rpg_new_hero h={0};h.cls=(uint8_t)family;h.race=(uint8_t)(family%RACE_COUNT);h.skin=4;
    h.hair=5;h.hair_color=7;h.eyes=3;h.body=1;h.lang=(uint8_t)language;
    rpg_game_new(&R.g,&h,seconds,177);R.loaded=true;wf_lang=(uint8_t)language;
    rpg_item helm={.type=IT_HELM,.tier=2,.level=1};assert(rpg_can_equip(&R.g,&helm));
    R.g.hero.equip[SLOT_HEAD]=helm;
    R.g.hero.level=19;R.g.hero.job_level=30;
}
static void select_branch(int branch) {click(CX-80+branch*160,330);}
static void blocked(int target,int branch) {
    rpg_hero before=R.g.hero;r_go(R_CLASS);run(30);select_branch(branch);
    assert(find(lv_layer_top(),TX(rpg_class(target)->name)));
    assert(!find(lv_layer_top(),TR("Diventa!","Become!")));
    press(TR("Chiudi","Close"));assert(!memcmp(&before,&R.g.hero,sizeof before));
}
static void helmet_studio(int target) {
    hero_look look;hero_look_from_game(&look,&R.g);assert(hero_uses_rig(&look));
    look.weapon=look.off=0xff; /* Diagnostic view only; owned items stay saved. */
    rpg_game before=R.g;
    lv_obj_t *s=kit_screen(&ln_scenes[0]);kit_dim(s,100);R.screen=R_CLASS_ATLAS;
    kit_label(s,&ln_font_title,0xfff4dd,TX(rpg_class(target)->name),CX,64);
    kit_label(s,&ln_font_body,0xc5d7e6,TR("Studio: stesso elmo","Studio: same helm"),CX,100);
    hero_obj(s,&look,HX_IDLE,3,CX,237);
    kit_label(s,&ln_font_body,0xffce78,TR("Senza arma e mano secondaria","No weapon or offhand"),CX,352);
    char name[80];snprintf(name,sizeof name,"elmo-%02d",target);shot(name);
    assert(!memcmp(&before,&R.g,sizeof before));
}
int main(int argc,char **argv) {
    assert(argc==2);dest=argv[1];setvbuf(stdout,NULL,_IOLBF,0);
    lv_init();lv_tick_set_cb(tick);lv_display_t *display=lv_display_create(WF_RES,WF_RES);
    lv_display_set_buffers(display,drawbuf,NULL,sizeof drawbuf,LV_DISPLAY_RENDER_MODE_PARTIAL);lv_display_set_flush_cb(display,flush);
    lv_indev_t *input=lv_indev_create();lv_indev_set_type(input,LV_INDEV_TYPE_POINTER);lv_indev_set_read_cb(input,read_touch);
    lv_timer_set_period(lv_indev_get_read_timer(input),10);wf_app_start(NULL,0);
    unsigned promotions=0,rejections=0,reloads=0;
    for(int lang=0;lang<2;lang++) for(int family=0;family<FAM_COUNT;family++) {
        uint64_t sibling[2]={0};
        for(int branch=0;branch<2;branch++) {
            int target=8+family*2+branch;seed_profile(family,lang);
            blocked(target,branch);rejections++;
            R.g.hero.level=20;R.g.hero.job_level=29;blocked(target,branch);rejections++;
            R.g.hero.job_level=30;r_go(R_CLASS);run(30);
            uint64_t previous=fingerprint(avatar_source());
            select_branch(branch);assert(find(lv_layer_top(),TR("Diventa!","Become!")));
            rpg_game expected=R.g;assert(rpg_change_class(&expected,target));int initial_saves=saves;
            press(TR("Diventa!","Become!"));assert(saves>initial_saves);
            assert(R.g.hero.cls==target && R.g.hero.job_level==1);promotions++;
            press(TR("Evviva!","Hooray!"));rpg_tick(&expected,seconds);
            assert(!memcmp(&expected,&R.g,sizeof expected));
            hero_look look;hero_look_from_game(&look,&R.g);
            assert(look.class_id==target && look.fam==family && look.tier==1 && look.head==IT_HELM);
            assert(avatar_source()==hero_image(&look,HX_HAPPY,2));
            sibling[branch]=fingerprint(avatar_source());assert(sibling[branch]!=previous);
            rpg_game decoded;assert(rpg_save_decode(saved,saved_len,&decoded));
            assert(!memcmp(&decoded.hero,&R.g.hero,sizeof decoded.hero));reloads++;
            char name[80];snprintf(name,sizeof name,"promozione-%02d-%s",target,lang?"en":"it");shot(name);
            if(!lang)helmet_studio(target);
        }
        assert(sibling[0]!=sibling[1]);
    }
    printf("promotions:%u real touch changes, %u rejected thresholds, %u save v4 hero roundtrips, IT/EN cosmetics/owned gear preserved; 16 sibling comparisons under same helm passed\n",promotions,rejections,reloads);
    printf("promotions:%d native captures; peak%zu image bytes; studio views do not change owned equipment\n",shots,peak_image_bytes);
    return 0;
}
