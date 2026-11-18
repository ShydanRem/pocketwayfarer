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
static void select_branch(int branch) {click(branch<0?CX:CX-80+branch*160,330);}
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
static unsigned promotions,rejections,reloads,master_skills;
static uint64_t promote(int target,int branch,int language,bool capture) {
    r_go(R_CLASS);run(30);uint64_t previous=fingerprint(avatar_source());
    select_branch(branch);assert(find(lv_layer_top(),TR("Diventa!","Become!")));
    if(target>=24) {
        char name[80];snprintf(name,sizeof name,"scelta-%02d-%s",target,language?"en":"it");shot(name);
    }
    rpg_game expected=R.g;assert(rpg_change_class(&expected,target));int initial_saves=saves;
    press(TR("Diventa!","Become!"));assert(saves>initial_saves);
    assert(R.g.hero.cls==target && R.g.hero.job_level==1 && R.g.hero.job_xp==0);promotions++;
    press(TR("Evviva!","Hooray!"));rpg_tick(&expected,seconds);
    assert(!memcmp(&expected,&R.g,sizeof expected));
    hero_look look;hero_look_from_game(&look,&R.g);
    assert(look.class_id==target && look.fam==rpg_class_family(target));
    assert(look.tier==rpg_class_tier(target)-1 && look.head==IT_HELM);
    assert(avatar_source()==hero_image(&look,HX_HAPPY,2));
    uint64_t current=fingerprint(avatar_source());assert(current!=previous);
    rpg_game decoded;assert(rpg_save_decode(saved,saved_len,&decoded));
    assert(!memcmp(&decoded.hero,&R.g.hero,sizeof decoded.hero));reloads++;
    if(capture) {
        char name[80];snprintf(name,sizeof name,"promozione-%02d-%s",target,language?"en":"it");shot(name);
        if(!language)helmet_studio(target);
    }
    return current;
}
static void inherited_skills(int target,const rpg_hero *previous) {
    int chain[]={rpg_class_family(target),rpg_class_parent(target),target},skills[9],count=0;
    assert(rpg_class_skills(target,skills)==8);
    for(unsigned i=0;i<3;i++)for(unsigned j=0;j<3;j++) {
        int sk=rpg_class(chain[i])->skills[j];if(sk==0xff)continue;
        assert(skills[count++]==sk);
    }
    assert(count==8);
    for(int sk=0;sk<RPG_SKILL_COUNT;sk++) {
        int expected=previous->skill[sk];
        if(sk==rpg_class(target)->skills[0] && !expected)expected=1;
        assert(R.g.hero.skill[sk]==expected);
    }
    assert(R.g.hero.skill_points==previous->skill_points+1);
    int next[2];assert(rpg_class_next(target,next)==0 && rpg_job_max(target)==50);
    rpg_foe foe;rpg_foe_make(&foe,SP_GHOST,45,false);
    rpg_battle battle;rpg_battle_start(&battle,&R.g,&foe,1,177,0);
    assert(battle.h.nskills==8);
    for(int i=0;i<8;i++)assert(battle.h.skills[i]==skills[i]);
    master_skills++;
}
int main(int argc,char **argv) {
    assert(argc==2);dest=argv[1];setvbuf(stdout,NULL,_IOLBF,0);
    lv_init();lv_tick_set_cb(tick);lv_display_t *display=lv_display_create(WF_RES,WF_RES);
    lv_display_set_buffers(display,drawbuf,NULL,sizeof drawbuf,LV_DISPLAY_RENDER_MODE_PARTIAL);lv_display_set_flush_cb(display,flush);
    lv_indev_t *input=lv_indev_create();lv_indev_set_type(input,LV_INDEV_TYPE_POINTER);lv_indev_set_read_cb(input,read_touch);
    lv_timer_set_period(lv_indev_get_read_timer(input),10);wf_app_start(NULL,0);
    for(int lang=0;lang<2;lang++) for(int family=0;family<FAM_COUNT;family++) {
        uint64_t sibling[2]={0};
        for(int branch=0;branch<2;branch++) {
            int target=8+family*2+branch;seed_profile(family,lang);
            blocked(target,branch);rejections++;
            R.g.hero.level=20;R.g.hero.job_level=29;blocked(target,branch);rejections++;
            R.g.hero.job_level=30;sibling[branch]=promote(target,branch,lang,true);
        }
        assert(sibling[0]!=sibling[1]);
    }
    for(int lang=0;lang<2;lang++)for(int family=0;family<4;family++) {
        uint64_t sibling[2]={0};
        for(int branch=0;branch<2;branch++) {
            int parent=8+family*2+branch,target=parent+16;seed_profile(family,lang);
            R.g.hero.level=45;R.g.hero.job_level=40;
            rpg_game unchanged=R.g;
            assert(!rpg_can_change_class(&R.g,target) && !rpg_change_class(&R.g,target));
            assert(!memcmp(&unchanged,&R.g,sizeof unchanged));
            R.g.hero.level=20;R.g.hero.job_level=30; /* Reachable base progression. */
            promote(parent,branch,lang,false); /* Actual base -> advanced touch path. */
            int learned[9],n=rpg_class_skills(parent,learned);assert(n==6);
            R.g.hero.skill_points=20;
            for(int i=0;i<n;i++)assert(rpg_learn_skill(&R.g,learned[i]));
            R.g.hero.level=45;R.g.hero.job_level=40;
            unchanged=R.g;
            assert(!rpg_can_change_class(&R.g,target^1) && !rpg_change_class(&R.g,target^1));
            assert(!memcmp(&unchanged,&R.g,sizeof unchanged));
            R.g.hero.level=44;blocked(target,-1);rejections++;
            R.g.hero.level=45;R.g.hero.job_level=39;blocked(target,-1);rejections++;
            R.g.hero.job_level=40;rpg_hero previous=R.g.hero;
            sibling[branch]=promote(target,-1,lang,true);inherited_skills(target,&previous);
            r_go(R_HERO);run(30);
            char name[80];snprintf(name,sizeof name,"statistiche-%02d-%s",target,lang?"en":"it");shot(name);
        }
        assert(sibling[0]!=sibling[1]);
    }
    printf("promotions:%u real touch changes, %u rejected thresholds, %u save v4 hero roundtrips, IT/EN cosmetics/owned gear preserved; 24 sibling comparisons under same helm passed\n",promotions,rejections,reloads);
    printf("masters:%u complete touch paths, direct/wrong-branch rejection, exact8 inherited/battle skills and previous ranks preserved; terminal class/job50 verified\n",master_skills);
    printf("promotions:%d native captures; peak%zu image bytes; studio views do not change owned equipment\n",shots,peak_image_bytes);
    return 0;
}
