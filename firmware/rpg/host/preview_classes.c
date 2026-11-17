/* Exercise the actual read-only class atlas with touch, BOOT and sleep hooks. */
#define main gameplay_preview_main
#include "preview_lantern.c"
#undef main

static void assert_button_bounds(lv_obj_t *o) {
    lv_area_t a,b;lv_obj_get_coords(o,&a);lv_obj_get_coords(lv_obj_get_child(o,0),&b);
    assert(b.x1>=a.x1+3 && b.x2<=a.x2-3 && b.y1>=a.y1+3 && b.y2<=a.y2-3);
    int corners[4][2]={{a.x1,a.y1},{a.x2,a.y1},{a.x1,a.y2},{a.x2,a.y2}};
    for(int i=0;i<4;i++) { int x=corners[i][0]-233,y=corners[i][1]-233;assert(x*x+y*y<233*233); }
}
static void click_visible(lv_obj_t *o) {
    lv_obj_scroll_to_view_recursive(o,LV_ANIM_OFF);lv_obj_update_layout(lv_layer_top());
    assert_button_bounds(o);
    lv_area_t a;lv_obj_get_coords(o,&a);click((a.x1+a.x2)/2,(a.y1+a.y2)/2);
}
static void assert_card(int c) {
    assert(R.screen==R_CLASS_ATLAS && find(lv_screen_active(),TX(rpg_class(c)->name)));
    assert(find(lv_screen_active(),c<8?TR("Illustrazione di classe","Class illustration"):TR("Ritratto della famiglia","Family illustration")));
    char counter[80];snprintf(counter,sizeof counter,"%d/40  ·  %s %d",c+1,TR("Grado","Tier"),rpg_class_tier(c));
    assert(find(lv_screen_active(),counter));
    lv_obj_update_layout(lv_screen_active());
    lv_obj_t *screen=lv_screen_active();bool found=false;
    for(unsigned i=0;i<lv_obj_get_child_count(screen);i++) {
        lv_obj_t *o=lv_obj_get_child(screen,(int)i);
        if(lv_obj_get_child_count(o)==1 && lv_obj_check_type(lv_obj_get_child(o,0),&lv_label_class)) assert_button_bounds(o);
        if(!lv_obj_check_type(o,&lv_image_class) || lv_obj_get_width(o)!=176) continue;
        const lv_image_dsc_t *image=lv_image_get_src(o);assert(image->header.cf==LV_COLOR_FORMAT_RGB565A8);
        lv_area_t a;lv_obj_get_coords(o,&a);
        const uint8_t *alpha=image->data+image->header.stride*image->header.h;
        for(unsigned y=0;y<image->header.h;y++) for(unsigned x=0;x<image->header.w;x++)
            if(alpha[y*image->header.w+x]) assert(a.y1+(int)y>122 && a.y1+(int)y<279);
        found=true;
    }
    assert(found);
    check_buttons(lv_screen_active());
}
int main(int argc,char **argv) {
    assert(argc==2);dest=argv[1];setvbuf(stdout,NULL,_IOLBF,0);
    lv_init();lv_tick_set_cb(tick);
    lv_display_t *display=lv_display_create(WF_RES,WF_RES);
    lv_display_set_buffers(display,drawbuf,NULL,sizeof drawbuf,LV_DISPLAY_RENDER_MODE_PARTIAL);
    lv_display_set_flush_cb(display,flush);
    lv_indev_t *input=lv_indev_create();lv_indev_set_type(input,LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(input,read_touch);lv_timer_set_period(lv_indev_get_read_timer(input),10);
    wf_app_start(NULL,0);run(150);press("Gioca");
    R.g.hero.race=RACE_BEAST;R.g.hero.skin=4;R.g.hero.hair=5;R.g.hero.hair_color=7;R.g.hero.eyes=3;R.g.hero.body=1;
    rpg_game expected=R.g;unsigned initial_saves=saves;
    for(int language=0;language<2;language++) {
        wf_lang=language;press("...");press(TR("Classi","Classes"));
        for(int c=0;c<RPG_CLASS_COUNT;c++) {
            assert_card(c);
            if(!language && (c<8 || c==24 || c==39)) {
                char name[80];snprintf(name,sizeof name,"classe-%02d",c);shot(name);
            }
            if(language && (c==2 || c==4 || c==6 || c==7 || c==28)) {
                char name[80];snprintf(name,sizeof name,"class-%02d-english",c);shot(name);
            }
            press(TR("Abilità","Skills"));
            const rpg_class_info *ci=rpg_class(c);
            lv_obj_t *panel=lv_obj_get_child(lv_obj_get_child(lv_layer_top(),0),0);
            lv_obj_t *body=lv_obj_get_child(panel,1);
            const char *contents=lv_label_get_text(body);
            for(int k=0;k<3 && ci->skills[k]!=0xff;k++) assert(strstr(contents,TX(rpg_skill(ci->skills[k])->name)));
            assert(!find(lv_layer_top(),TR("Diventa!","Become!")));
            if(!language && c==4) shot("abilita-chierico");
            assert(wf_app_key(WF_KEY_BOOT_SHORT) && lv_obj_get_child_count(lv_layer_top())==0);
            assert_card(c);press(">");
        }
        assert_card(0);press("<");assert_card(39);press(">");assert_card(0);
        for(int family=0;family<FAM_COUNT;family++) {
            for(int i=0;i<family;i++) press(">");
            press(TR("Percorso","Path"));
            int next[2],master[2];assert(rpg_class_next(family,next)==2);
            assert(rpg_class_next(next[0],master)==1);int last=master[0];
            assert(rpg_class_next(next[1],master)==1);last=master[0];
            if(!language && family==0) shot("percorso-guerriero");
            lv_obj_t *node=find(lv_layer_top(),TX(rpg_class(last)->name));assert(node);click_visible(lv_obj_get_parent(node));
            assert_card(last);
            /* Navigate back to base0 with actual wraparound touches. */
            for(int i=last;i<RPG_CLASS_COUNT;i++) press(">");
            assert_card(0);
        }
        press(TR("Percorso","Path"));
        lv_obj_t *close=find(lv_layer_top(),TR("Chiudi","Close"));assert(close);click_visible(lv_obj_get_parent(close));
        assert_card(0);
        wf_app_flush_save();ms+=20000;seconds+=20;wf_app_wake();run(20);
        assert_card(0);assert(!wf_app_busy());
        assert(wf_app_key(WF_KEY_BOOT_SHORT) && R.screen==R_TOWN);
    }
    rpg_tick(&expected,seconds);
    assert(!memcmp(&expected,&R.g,sizeof expected));
    assert(saves==(int)initial_saves);
    wf_lang=0;
    puts("class atlas:80 bilingual cards,208 local skill references,16 family paths, wraparound, scrolled nodes, touch/BOOT close and PWR hooks: passed");
    puts("read-only: real class,cosmetics,equipment,skills,expedition and expected time simulation unchanged; no atlas save/reward: passed");
    printf("class atlas:%d native captures; image buffers peak%zu bytes; eight illustrations,32 explicit family references\n",shots,peak_image_bytes);
    return 0;
}
