/* Read-only class reference. Representative illustrations never replace the
 * player's saved appearance or grant a job/skill. All rules come from core. */
#include <stdio.h>
#include "rui.h"
#include "art/class_art.h"
#include "art/lantern_art.h"

LV_FONT_DECLARE(ln_font_body)
LV_FONT_DECLARE(ln_font_title)
static int selected;
static lv_obj_t *dialog;
enum { PREV, NEXT, FAMILY, SKILLS, HOME };

static void style_button(lv_obj_t *b,int width) {
    lv_obj_set_ext_click_area(b,0);
    lv_obj_set_style_radius(b,6,0);
    lv_obj_set_style_border_width(b,1,0);lv_obj_set_style_border_color(b,lv_color_hex(0x63739b),0);
    lv_obj_set_style_bg_color(b,lv_color_hex(0x263658),0);
    lv_obj_set_style_bg_grad_color(b,lv_color_hex(0x171c3c),0);lv_obj_set_style_bg_grad_dir(b,LV_GRAD_DIR_VER,0);
    lv_obj_t *label=lv_obj_get_child(b,0);
    lv_obj_set_style_text_font(label,&ln_font_body,0);lv_obj_set_style_text_color(label,lv_color_hex(0xfff4dd),0);
    lv_obj_set_style_text_align(label,LV_TEXT_ALIGN_CENTER,0);
    lv_obj_set_width(label,width?width-8:lv_pct(96));lv_label_set_long_mode(label,LV_LABEL_LONG_WRAP);lv_obj_center(label);
}
static lv_obj_t *text(lv_obj_t *p, const char *value, int y, int width, bool title, uint32_t color) {
    lv_obj_t *o = kit_label(p, title ? &ln_font_title : &ln_font_body, color, value, CX, y);
    lv_obj_set_width(o,width); lv_label_set_long_mode(o,LV_LABEL_LONG_WRAP); kit_pos(o,CX,y);
    return o;
}
static void deleted(lv_event_t *e) { if(lv_event_get_target(e)==dialog) dialog=NULL; }
static void close_dialog(lv_event_t *e) { (void)e; if(dialog) { lv_obj_delete(dialog); dialog=NULL; } }
static lv_obj_t *info_dialog(const char *title, const char *body) {
    dialog=kit_dialog(title,body,NULL);
    lv_obj_add_event_cb(dialog,deleted,LV_EVENT_DELETE,NULL);
    lv_obj_t *panel=lv_obj_get_child(dialog,0);
    lv_obj_set_style_bg_color(panel,lv_color_hex(0x171c3c),0);
    lv_obj_set_style_max_height(panel,322,0);
    lv_obj_set_style_text_font(lv_obj_get_child(panel,0),&ln_font_title,0);
    lv_obj_set_style_text_font(lv_obj_get_child(panel,1),&ln_font_body,0);
    lv_obj_set_style_text_color(lv_obj_get_child(panel,0),lv_color_hex(0xffce78),0);
    lv_obj_t *close=kit_text_button(dialog,TR("Chiudi","Close"),CX,420,144,52,0x263658,close_dialog,0);
    style_button(close,144); /* Fixed outside the scrolling pane, always reachable. */
    return dialog;
}
static void choose(lv_event_t *e) {
    selected=(int)(intptr_t)lv_event_get_user_data(e);
    close_dialog(NULL); scr_class_atlas();
}
static void family_dialog(void) {
    int family=rpg_class_family(selected);
    int ids[5]={family,8+family*2,9+family*2,24+family*2,25+family*2};
    info_dialog(TR("La tua via","Your path"),TR("Base, due rami avanzati e le rispettive maestre. Tocca per consultare.",
        "Base, two advanced branches and their master paths. Tap to inspect."));
    for(int i=0;i<5;i++) {
        lv_obj_t *b=kit_dialog_button(dialog,TX(rpg_class(ids[i])->name),ids[i]==selected?COL_BTN_OK:COL_PANEL_HI,choose,ids[i]);
        lv_obj_set_width(b,lv_pct(100));
        style_button(b,0);
    }
}
static void skills_dialog(void) {
    const rpg_class_info *ci=rpg_class(selected);
    char body[768];
    int n=snprintf(body,sizeof body,"%s\n\n",TX(ci->blurb));
    int tier=rpg_class_tier(selected), parent=rpg_class_parent(selected);
    if(tier>1) n+=snprintf(body+n,sizeof body-(size_t)n,TR("Richiede %s: livello %d e classe %d.\n\n","Requires %s: level %d and job %d.\n\n"),
                          TX(rpg_class(parent)->name),tier==2?20:45,rpg_job_max(parent));
    for(int i=0;i<3 && ci->skills[i]!=0xff;i++) {
        const rpg_skill_info *skill=rpg_skill(ci->skills[i]);
        n+=snprintf(body+n,sizeof body-(size_t)n,"%s%s\n",skill->kind==SK_PASSIVE?TR("Passiva: ","Passive: "):"",TX(skill->name));
    }
    snprintf(body+n,sizeof body-(size_t)n,"\n%s",TR("Cambio: Classico / Eroe / Classe.",
        "Change: Classic / Hero / Class."));
    info_dialog(TX(ci->name),body);
}
static void action(lv_event_t *e) {
    int a=(int)(intptr_t)lv_event_get_user_data(e);
    if(a==HOME) { r_go(R_TOWN); return; }
    if(a==FAMILY) { family_dialog(); return; }
    if(a==SKILLS) { skills_dialog(); return; }
    selected=(selected+RPG_CLASS_COUNT+(a==NEXT?1:-1))%RPG_CLASS_COUNT;
    scr_class_atlas();
}
static void button(lv_obj_t *s,const char *value,int x,int y,int width,int code) {
    lv_obj_t *b=kit_text_button(s,value,x,y,width,52,0x263658,action,code);
    style_button(b,width);
}
void scr_class_atlas(void) {
    if(selected<0 || selected>=RPG_CLASS_COUNT) selected=0;
    lv_obj_t *s=kit_screen(&ln_scenes[0]); kit_dim(s,130);
    lv_obj_t *header=lv_obj_create(s);lv_obj_remove_style_all(header);
    lv_obj_set_size(header,330,88);kit_pos(header,CX,78);
    lv_obj_set_style_bg_color(header,lv_color_hex(0x171c3c),0);lv_obj_set_style_bg_opa(header,235,0);
    lv_obj_set_style_radius(header,6,0);lv_obj_remove_flag(header,LV_OBJ_FLAG_CLICKABLE);
    text(s,TX(rpg_class(selected)->name),61,310,true,0xfff4dd);
    char subtitle[80];
    snprintf(subtitle,sizeof subtitle,"%d/40  ·  %s %d",selected+1,TR("Grado","Tier"),rpg_class_tier(selected));
    text(s,subtitle,101,280,false,0xc5d7e6);
    /* Representative illustration, separate from the customizable avatar. */
    kit_img(s,&class_portraits[rpg_class_family(selected)],2,CX,197);
    button(s,"<",80,210,56,PREV);button(s,">",386,210,56,NEXT);
    lv_obj_t *badge=lv_obj_create(s);lv_obj_remove_style_all(badge);lv_obj_set_size(badge,294,28);kit_pos(badge,CX,293);
    lv_obj_set_style_bg_color(badge,lv_color_hex(0x171c3c),0);lv_obj_set_style_bg_opa(badge,235,0);lv_obj_set_style_radius(badge,6,0);
    text(s,selected<8?TR("Illustrazione di classe","Class illustration"):TR("Ritratto della famiglia","Family illustration"),293,286,false,0xffce78);
    const rpg_class_info *ci=rpg_class(selected);
    snprintf(subtitle,sizeof subtitle,"%s  ·  %s",TX(rpg_stat_name(ci->main_stat)),TX(rpg_stat_name(ci->second_stat)));
    text(s,subtitle,319,260,false,0xfff4dd);
    button(s,TR("Percorso","Path"),155,365,144,FAMILY);
    button(s,TR("Abilità","Skills"),311,365,144,SKILLS);
    button(s,TR("Borgo","Village"),CX,420,144,HOME);
}
bool class_atlas_key(int key) {
    if(key==WF_KEY_BOOT_DOWN) return false;
    if(key!=WF_KEY_BOOT_SHORT && key!=WF_KEY_BOOT_LONG) return false;
    if(dialog) close_dialog(NULL); else r_go(R_TOWN);
    return true;
}
