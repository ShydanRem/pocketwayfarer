/* Composes the paper-doll hero from its layers and caches the result. */
#include <string.h>
#include "rui.h"

#define GRID 48
#define SLOTS 6

typedef struct {
    hero_look look;
    uint8_t expr, scale, used;
    uint32_t stamp;
    lv_image_dsc_t dsc;
    uint8_t *buf;
} hero_slot;

static hero_slot slots[SLOTS];
static uint32_t clock_;

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

const lv_image_dsc_t *hero_image(const hero_look *l, int expr, int scale) {
    clock_++;
    hero_slot *victim = &slots[0];
    for (int i = 0; i < SLOTS; i++) {
        hero_slot *s = &slots[i];
        if (s->used && s->expr == expr && s->scale == scale && !memcmp(&s->look, l, sizeof *l)) {
            s->stamp = clock_;
            return &s->dsc;
        }
        if (!s->used || s->stamp < victim->stamp) victim = s;
        if (!s->used) break;
    }
    static uint16_t rgb[GRID * GRID];
    static uint8_t a[GRID * GRID];
    compose(l, expr, rgb, a);
    int w = GRID * scale;
    size_t px = (size_t)w * (size_t)w;
    if (victim->buf && victim->scale != scale) {
        wf_platform_big_free(victim->buf);
        victim->buf = NULL;
    }
    if (!victim->buf) victim->buf = wf_platform_big_alloc(px * 3);
    uint16_t *out = (uint16_t *)victim->buf;
    uint8_t *alpha = victim->buf + px * 2;
    for (int y = 0; y < w; y++)
        for (int x = 0; x < w; x++) {
            int src = (y / scale) * GRID + x / scale;
            out[y * w + x] = rgb[src];
            alpha[y * w + x] = a[src];
        }
    memset(&victim->dsc, 0, sizeof victim->dsc);
    victim->dsc.header.magic = LV_IMAGE_HEADER_MAGIC;
    victim->dsc.header.cf = LV_COLOR_FORMAT_RGB565A8;
    victim->dsc.header.w = (uint32_t)w;
    victim->dsc.header.h = (uint32_t)w;
    victim->dsc.header.stride = (uint32_t)w * 2u;
    victim->dsc.data = victim->buf;
    victim->dsc.data_size = (uint32_t)(px * 3);
    victim->look = *l;
    victim->expr = (uint8_t)expr;
    victim->scale = (uint8_t)scale;
    victim->used = 1;
    victim->stamp = clock_;
    return &victim->dsc;
}

lv_obj_t *hero_obj(lv_obj_t *parent, const hero_look *l, int expr, int scale, int x, int y) {
    lv_obj_t *img = lv_image_create(parent);
    lv_image_set_src(img, hero_image(l, expr, scale));
    lv_obj_remove_flag(img, LV_OBJ_FLAG_CLICKABLE);
    kit_pos(img, x, y);
    return img;
}

void hero_obj_set(lv_obj_t *img, const hero_look *l, int expr, int scale) {
    lv_image_set_src(img, hero_image(l, expr, scale));
    lv_obj_invalidate(img);
}
