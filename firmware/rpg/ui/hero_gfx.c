/* Exact-class modular avatar with an explicit legacy fallback. Cache ownership
 * follows widgets so appearance editing stays bounded without live eviction. */
#include <stdlib.h>
#include <string.h>
#include "rui.h"
#include "hero_cache.h"
#include "hero_rig.h"

#define GRID 48

void hero_look_from_game(hero_look *l, const rpg_game *g) {
    const rpg_hero *h = &g->hero;
    memset(l, 0, sizeof *l);
    l->race = h->race;
    l->skin = h->skin;
    l->eyes = h->eyes;
    l->hair = h->hair;
    l->hair_color = h->hair_color;
    l->body = h->body;
    l->fam = (uint8_t)rpg_class_family(h->cls);
    l->tier = (uint8_t)(rpg_class_tier(h->cls) - 1);
    l->class_id = h->cls;
    const rpg_item *w = &h->equip[SLOT_WEAPON], *o = &h->equip[SLOT_OFFHAND], *hd = &h->equip[SLOT_HEAD];
    l->weapon = rpg_item_empty(w) ? 0xff : w->type;
    l->weapon_tier = w->tier;
    l->off = rpg_item_empty(o) ? 0xff : o->type;
    l->off_tier = o->tier;
    l->head = rpg_item_empty(hd) ? 0xff : hd->type;
    l->head_tier = hd->tier;
}

static void draw(uint16_t *rgb, uint8_t *a, const wf_layer *L) {
    const wf_sprite *s = &L->s;
    for (int y = 0; y < s->h; y++) {
        int gy = L->y + y;
        if (gy < 0 || gy >= GRID) continue;
        for (int x = 0; x < s->w; x++) {
            int gx = L->x + x;
            if (gx < 0 || gx >= GRID) continue;
            uint8_t i = s->px[y * s->w + x];
            if (s->alpha && !i) continue;
            rgb[gy * GRID + gx] = s->pal[i];
            a[gy * GRID + gx] = 255;
        }
    }
}

static void compose(const hero_look *l, int expr, uint16_t *rgb, uint8_t *a) {
    memset(rgb, 0, GRID * GRID * 2);
    memset(a, 0, GRID * GRID);
    int hc = l->hair_color % 8, hs = l->hair % 6, fam = l->fam % FAM_COUNT, tier = l->tier % 3;
    draw(rgb, a, &rpg_cape[fam][tier]);
    if (l->off == IT_QUIVER) draw(rgb, a, &rpg_offhand[1][l->off_tier % RPG_TIERS]);
    if (l->race == RACE_BEAST) draw(rgb, a, &rpg_tail[hc]);
    draw(rgb, a, &rpg_hair_back[hs][hc]);
    draw(rgb, a, &rpg_body[l->race % RACE_COUNT][l->skin % 5]);
    draw(rgb, a, &rpg_face[l->eyes % 4][expr % 4][l->body ? 1 : 0]);
    draw(rgb, a, &rpg_outfit[fam][tier]);
    if (l->race == RACE_DWARF) draw(rgb, a, &rpg_beard[hc]);
    draw(rgb, a, &rpg_hair_front[hs][hc]);
    if (l->race == RACE_BEAST) draw(rgb, a, &rpg_ears[hc]);
    if (l->head == IT_HELM || l->head == IT_HOOD || l->head == IT_CIRCLET)
        draw(rgb, a, &rpg_helm[l->head - IT_HELM][l->head_tier % RPG_TIERS]);
    else
        draw(rgb, a, &rpg_hat[fam][tier]);
    if (l->weapon <= IT_SCYTHE) draw(rgb, a, &rpg_weapon[l->weapon][l->weapon_tier % RPG_TIERS]);
    if (l->off >= IT_SHIELD && l->off <= IT_CHARM && l->off != IT_QUIVER)
        draw(rgb, a, &rpg_offhand[l->off - IT_SHIELD][l->off_tier % RPG_TIERS]);
}

typedef struct { hero_look look; uint8_t expr,preset,pose; } render_key;
typedef struct { render_key key; const lv_image_dsc_t *image; } hero_view;

static render_key key_for(const hero_look *l,int expr,int preset,int pose) {
    render_key key;memset(&key,0,sizeof key);key.look=*l;
    key.expr=(uint8_t)(expr%4);key.preset=(uint8_t)(preset<1?1:preset>3?3:preset);
    key.pose=(uint8_t)(hero_uses_rig(l) && !R.g.lantern.reduced_motion?pose%HP_COUNT:HP_IDLE);
    return key;
}
static void render(uint16_t *out,uint8_t *alpha,int w,int h,void *ctx) {
    const render_key *key=ctx;
    static uint16_t rgb[88*88];static uint8_t a[88*88];
    bool modern=hero_uses_rig(&key->look);int grid=modern?88:48;
    if(modern) hero_rig_compose(&key->look,key->expr,key->pose,rgb,a);
    else compose(&key->look,key->expr,rgb,a);
    for(int y=0;y<h;y++) for(int x=0;x<w;x++) {
        int src=(y*grid/h)*grid+x*grid/w;
        out[y*w+x]=rgb[src];alpha[y*w+x]=a[src];
    }
}
const lv_image_dsc_t *hero_image(const hero_look *l,int expr,int preset) {
    render_key key=key_for(l,expr,preset,HP_IDLE);int size=hero_size(l,preset);
    return hero_cache_image(&key,sizeof key,size,size,render,&key);
}
static void view_deleted(lv_event_t *event) {
    hero_view *view=lv_event_get_user_data(event);
    hero_cache_unref(view->image);free(view);
}

lv_obj_t *hero_obj(lv_obj_t *parent, const hero_look *l, int expr, int scale, int x, int y) {
    lv_obj_t *img = lv_image_create(parent);
    hero_view *view=calloc(1,sizeof *view);LV_ASSERT_MALLOC(view);
    lv_obj_set_user_data(img,view);
    lv_obj_add_event_cb(img,view_deleted,LV_EVENT_DELETE,view);
    hero_obj_set_pose(img,l,expr,scale,HP_IDLE);
    lv_obj_remove_flag(img, LV_OBJ_FLAG_CLICKABLE);
    kit_pos(img, x, y);
    return img;
}

void hero_obj_set(lv_obj_t *img, const hero_look *l, int expr, int scale) {
    hero_obj_set_pose(img,l,expr,scale,HP_IDLE);
}
void hero_obj_set_pose(lv_obj_t *img,const hero_look *l,int expr,int preset,int pose) {
    hero_view *view=lv_obj_get_user_data(img);LV_ASSERT_NULL(view);
    render_key key=key_for(l,expr,preset,pose);int size=hero_size(l,preset);
    const lv_image_dsc_t *image=hero_cache_acquire(&key,sizeof key,size,size,render,&key);
    if(!image)return;
    const lv_image_dsc_t *old=view->image;
    lv_image_set_src(img,image);view->image=image;view->key=key;
    hero_cache_unref(old);
    lv_obj_invalidate(img);
}
void hero_obj_pose(lv_obj_t *img,int pose) {
    hero_view *view=lv_obj_get_user_data(img);LV_ASSERT_NULL(view);
    hero_obj_set_pose(img,&view->key.look,view->key.expr,view->key.preset,pose);
}
