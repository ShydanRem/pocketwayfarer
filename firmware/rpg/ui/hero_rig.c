/* Registered layers and semantic palette channels. Cosmetic fields remain
 * independent from class costume, expression, pose and actual owned gear. */
#include <string.h>
#include "hero_rig.h"
#include "art/hero_rig_art.h"

#define GRID 88
static unsigned clipped_pixels;
static const uint32_t skin_colors[5]={0xffe0c4,0xf2c29a,0xd89a6a,0xa8673f,0x6e4128};
static const uint32_t hair_colors[8]={0x3a2a24,0x7a4a2a,0xd8a050,0xf4e2a0,0xc84a2a,0xe8e8f0,0x4a7ae8,0xe85aa8};
static const uint32_t eye_colors[4]={0x6a3a1a,0x3a7ad8,0x3a9a4a,0x8a4ad8};
static const uint32_t metal_colors[5]={0xc88a4a,0x9aa0b0,0xc8d0e0,0x9ad8e8,0xf4e0a0};
static const uint32_t wood_colors[5]={0xa8703a,0xc89a5a,0x8a4a2a,0x4a3a3a,0xd8d8ff};

bool hero_uses_rig(const hero_look *l) {
    return l->class_id<HERO_RIG_CLASS_COUNT && l->fam==rig_class_families[l->class_id] &&
           l->tier==rig_class_tiers[l->class_id];
}
int hero_size(const hero_look *l,int preset) {
    if(preset<1) preset=1;
    if(preset>3) preset=3;
    return hero_uses_rig(l)?(preset==3?176:preset==2?88:44):48*preset;
}
int hero_foot_offset(const hero_look *l,int preset) {
    return hero_uses_rig(l)?40*hero_size(l,preset)/88:22*preset;
}
static uint16_t pack(int r,int g,int b) { return (uint16_t)(((r>>3)<<11)|((g>>2)<<5)|(b>>3)); }
static uint16_t recolor(uint16_t color,uint32_t base,int whitepoint) {
    int r=((color>>11)&31)*255/31,g=((color>>5)&63)*255/63,b=(color&31)*255/31;
    int v=r>g?r:g;if(b>v)v=b;
    int shade=v*256/whitepoint;
    int channels[3]={(int)(base>>16)&255,(int)(base>>8)&255,(int)base&255};
    for(int i=0;i<3;i++) channels[i]=shade<=256?channels[i]*shade/256:channels[i]+(255-channels[i])*(shade-256)/128;
    return pack(channels[0],channels[1],channels[2]);
}
static uint16_t tint(uint16_t color,int role,const hero_look *l) {
    if(!role) return color;
    uint32_t base=role==1?skin_colors[l->skin%5]:role==2?eye_colors[l->eyes%4]:hair_colors[l->hair_color%8];
    return recolor(color,base,role==3?224:255);
}
static int body_y(int y,const hero_look *l) { return l->race==RACE_DWARF?84-(84-y)*86/100:y; }
static int body_x(int x,const hero_look *l) { (void)l;return x; }

/* Local coordinates from the registered 88px costumes. Both hands and the
 * head follow the same monotone mesh as the body. The mesh settles by y=76. */
enum { PALADIN_COSTUME = 16, DRAGON_KNIGHT_COSTUME = 24 };
typedef struct { int lean, neck_y, hands_y, main_hand, off_hand; } combat_pose;
static const combat_pose paladin_motion[HP_COUNT] = {
    [HP_WINDUP] = {-2, 0,  0, -6,  1},
    [HP_ATTACK] = { 4,-2, -4,  8,  0},
    [HP_RECOVER]= { 2, 1,  0,  3,  0},
    [HP_GUARD_ENTRY] = {-1, 1,-2, 0,-4},
    [HP_GUARD]  = {-3, 3, -5,  0, -7},
};
static const combat_pose dragon_motion[HP_COUNT] = {
    [HP_WINDUP] = {-3, 0,  0, -7,  1},
    [HP_ATTACK] = { 5,-2, -5, 10,  1},
    [HP_RECOVER]= { 1, 1,  0,  5,  0},
    [HP_GUARD_ENTRY] = {-2, 1,-2, 0,-3},
    [HP_GUARD]  = {-4, 3, -4,  0, -6},
};
bool hero_has_combat_rig(const hero_look *l) {
    return hero_uses_rig(l) && (l->class_id == PALADIN_COSTUME || l->class_id == DRAGON_KNIGHT_COSTUME);
}
static const combat_pose *combat_motion(const hero_look *l, int pose) {
    if ((unsigned)pose >= HP_COUNT || pose == HP_IDLE || pose == HP_BREATH || pose == HP_HIT || !hero_has_combat_rig(l))
        return NULL;
    return &(l->class_id == PALADIN_COSTUME ? paladin_motion : dragon_motion)[pose];
}
static const int body_rows[] = {0, 32, 56, 76, 87};
static int map_axis(int n, const int *from, const int *to, int count) {
    int i = 1;
    while (i < count - 1 && n > from[i]) i++;
    const int span = from[i] - from[i - 1], part = n - from[i - 1];
    return (to[i - 1] * (span - part) + to[i] * part + span / 2) / span;
}
static void pose_rows(const hero_look *l, const combat_pose *p, int rows[5]) {
    rows[0] = body_y(body_rows[0], l);
    rows[1] = body_y(32 + p->neck_y, l);
    rows[2] = body_y(56 + p->hands_y, l);
    rows[3] = body_y(76, l);
    rows[4] = body_y(87, l);
}
static void pose_columns(const hero_look *l, const combat_pose *p, int y, int source[5], int posed[5]) {
    const uint8_t (*hands)[2] = rig_hand_mounts[l->class_id];
    const int lean_weight = y <= 36 ? 40 : y >= 76 ? 0 : 76 - y;
    const int arm_weight = y < 38 || y >= 76 ? 0 : y <= 55 ? y - 38 : y <= 57 ? 17 : (76 - y) * 17 / 19;
    const int lean = p->lean * lean_weight / 40;
    source[0] = posed[0] = 0;
    source[1] = hands[0][0];
    source[2] = 44;
    source[3] = hands[1][0];
    source[4] = posed[4] = 87;
    posed[1] = source[1] + lean + p->main_hand * arm_weight / 17;
    posed[2] = source[2] + lean;
    posed[3] = source[3] + lean + p->off_hand * arm_weight / 17;
}
static void put(uint16_t *rgb,uint8_t *alpha,int x,int y,uint16_t c) {
    if(x>=0 && x<GRID && y>=0 && y<GRID) { rgb[y*GRID+x]=c;alpha[y*GRID+x]=255; }
    else clipped_pixels++;
}
static void combat_body(const hero_rig_layer *layer,const hero_look *l,const combat_pose *motion,uint16_t *rgb,uint8_t *alpha) {
    const wf_sprite *s = &layer->s;
    uint16_t palette[256];
    for (unsigned i = 0; i < s->colors; i++) palette[i] = tint(s->pal[i], layer->roles[i], l);
    int rows[5];
    pose_rows(l, motion, rows);
    const int first = map_axis(layer->y, body_rows, rows, 5);
    for (int y = first; y < rows[3]; y++) {
        int sy = map_axis(y, rows, body_rows, 5);
        if (sy < layer->y) sy = layer->y;
        if (sy >= layer->y + s->h) sy = layer->y + s->h - 1;
        int xs[5], posed[5];
        pose_columns(l, motion, sy, xs, posed);
        const int left = map_axis(layer->x, xs, posed, 5);
        const int right = map_axis(layer->x + s->w - 1, xs, posed, 5);
        for (int x = left; x <= right; x++) {
            int sx = map_axis(x, posed, xs, 5) - layer->x;
            if (sx < 0 || sx >= s->w) continue;
            uint8_t index = s->px[(sy - layer->y) * s->w + sx];
            if (index) put(rgb, alpha, x, y, palette[index]);
        }
    }
    /* Draw the planted lower legs with their original sampling and pixels. */
    for (int y = body_rows[3]; y < layer->y + s->h; y++) {
        const uint8_t *row = s->px + (y - layer->y) * s->w;
        for (int x = 0; x < s->w; x++) if (row[x])
            put(rgb, alpha, layer->x + x, body_y(y, l), palette[row[x]]);
    }
}
static void part(const hero_rig_layer *layer,const hero_look *l,int pose,int kind,const combat_pose *motion,uint16_t *rgb,uint8_t *alpha) {
    const wf_sprite *s=&layer->s;
    uint16_t palette[256];
    for(unsigned i=0;i<s->colors;i++) palette[i]=tint(s->pal[i],layer->roles[i],l);
    for(int yy=0;yy<s->h;yy++) for(int xx=0;xx<s->w;xx++) {
        uint8_t index=s->px[yy*s->w+xx];if(!index)continue;
        int x=layer->x+xx,y=layer->y+yy;
        if(kind==1 && y>=28) continue; /* front hair, long lengths already behind */
        int dx=0,dy=0;
        if(kind==0) {
            bool left=x<36 && y>=40 && y<66,right=x>54 && y>=40 && y<66;
            if(rig_rigid_arms[l->class_id] && pose!=HP_BREATH) {
                /* A flattened long cape cannot be cut into a moving arm.
                 * Translate the connected costume coherently; keep soles fixed. */
                if(body_y(y,l)<78)dx=pose==HP_ATTACK?2:pose==HP_GUARD?-1:pose==HP_HIT?-2:0;
            }
            else if(pose==HP_ATTACK && left){dx=6;dy=-5;}
            else if(pose==HP_GUARD && right){dx=-4;dy=-6;}
            else if(y<64) { dx=pose==HP_ATTACK?2:pose==HP_GUARD?-1:pose==HP_HIT?-2:0;dy=pose==HP_BREATH?-1:0; }
            x=body_x(x,l);y=body_y(y,l);
        } else {
            y+=body_y(32 + (motion ? motion->neck_y : 0),l)-32;
            dx=motion?motion->lean:pose==HP_ATTACK?2:pose==HP_GUARD?-1:pose==HP_HIT?-2:0;
            dy=motion?0:pose==HP_BREATH?-1:pose==HP_HIT?1:0;
        }
        put(rgb,alpha,x+dx,y+dy,palette[index]);
        /* Nearest-neighbor overlap at the waist joint: the upper half rises
         * one row on breath while the planted legs stay fixed. Forward-only
         * translation otherwise leaves an entirely empty row at the seam. */
        if(kind==0 && pose==HP_BREATH && layer->y+yy==63)
            put(rgb,alpha,x,y,palette[index]);
    }
}
static void legacy_part(const wf_layer *layer,int ax,int ay,int bx,int by,int scale,int side,uint16_t *rgb,uint8_t *alpha) {
    const wf_sprite *s=&layer->s;
    for(int y=0;y<GRID;y++) for(int x=0;x<GRID;x++) {
        int gx=ax+(x-bx)*100/scale,gy=ay+(y-by)*100/scale;
        if(side<0 && gx>=24)continue;
        if(side>0 && gx<24)continue;
        int sx=gx-layer->x,sy=gy-layer->y;
        if(sx<0 || sx>=s->w || sy<0 || sy>=s->h)continue;
        uint8_t i=s->px[sy*s->w+sx];if(s->alpha && !i)continue;
        put(rgb,alpha,x,y,s->pal[i]);
    }
}
static int rounded_q8(int v) { return v < 0 ? -((-v + 128) / 256) : (v + 128) / 256; }
static void gear_part(const hero_rig_layer *layer,int x,int y,int tier,bool head,int turn,uint16_t *rgb,uint8_t *alpha) {
    const wf_sprite *s=&layer->s;uint16_t palette[256];
    for(unsigned i=0;i<s->colors;i++) {
        int role=layer->roles[i];palette[i]=s->pal[i];
        if(role==4 || role==5)palette[i]=recolor(s->pal[i],role==4?metal_colors[tier%5]:wood_colors[tier%5],224);
    }
    int ox=x-44,oy=y-(head?32:56);
    if (turn) {
        /* Nearest-neighbor inverse rotation about the same registered grip. */
        const int cosine = 232, sine = turn * 108; /* +/-25 degrees */
        const int sx0 = layer->x - 44, sy0 = layer->y - 56;
        const int sx1 = sx0 + s->w - 1, sy1 = sy0 + s->h - 1;
        int left = GRID, right = -GRID, top = GRID, bottom = -GRID;
        for (int i = 0; i < 4; i++) {
            int sx = i & 1 ? sx1 : sx0, sy = i & 2 ? sy1 : sy0;
            int dx = rounded_q8(cosine * sx - sine * sy), dy = rounded_q8(sine * sx + cosine * sy);
            if (dx < left) left = dx;
            if (dx > right) right = dx;
            if (dy < top) top = dy;
            if (dy > bottom) bottom = dy;
        }
        for (int dy = top - 1; dy <= bottom + 1; dy++) for (int dx = left - 1; dx <= right + 1; dx++) {
            int sx = rounded_q8(cosine * dx + sine * dy) - sx0;
            int sy = rounded_q8(-sine * dx + cosine * dy) - sy0;
            if (sx < 0 || sy < 0 || sx >= s->w || sy >= s->h) continue;
            uint8_t i = s->px[sy * s->w + sx];
            if (i) put(rgb, alpha, x + dx, y + dy, palette[i]);
        }
        return;
    }
    for(int yy=0;yy<s->h;yy++) for(int xx=0;xx<s->w;xx++) {
        uint8_t index=s->px[yy*s->w+xx];if(!index)continue;
        put(rgb,alpha,layer->x+xx+ox,layer->y+yy+oy,palette[index]);
    }
}
unsigned hero_rig_compose(const hero_look *l,int expr,int pose,uint16_t *rgb,uint8_t *alpha) {
    clipped_pixels=0;
    memset(rgb,0,GRID*GRID*2);memset(alpha,0,GRID*GRID);
    const combat_pose *motion = combat_motion(l, pose);
    int fam=l->class_id,hc=l->hair_color%8,race=l->race%RACE_COUNT;
    int left_x=body_x(rig_hand_mounts[fam][0][0],l),left_y=body_y(rig_hand_mounts[fam][0][1],l);
    int right_x=body_x(rig_hand_mounts[fam][1][0],l),right_y=body_y(rig_hand_mounts[fam][1][1],l);
    if (motion) {
        int rows[5], xs[5], posed[5];
        pose_rows(l, motion, rows);
        int left_row = rig_hand_mounts[fam][0][1], right_row = rig_hand_mounts[fam][1][1];
        pose_columns(l, motion, left_row, xs, posed);
        left_x = posed[1]; left_y = map_axis(left_row, body_rows, rows, 5);
        pose_columns(l, motion, right_row, xs, posed);
        right_x = posed[3]; right_y = map_axis(right_row, body_rows, rows, 5);
    } else if(rig_rigid_arms[fam]) {
        int dx=pose==HP_ATTACK?2:pose==HP_GUARD?-1:pose==HP_HIT?-2:0;
        left_x+=dx;right_x+=dx;
    } else {
        if(pose==HP_ATTACK){left_x+=6;left_y-=5;}
        if(pose==HP_GUARD){right_x-=4;right_y-=6;}
        if(pose==HP_ATTACK)right_x+=2;
        if(pose==HP_GUARD)left_x--;
        if(pose==HP_HIT){left_x-=2;right_x-=2;}
    }
    if(pose==HP_BREATH){left_y--;right_y--;}
    if(race==RACE_BEAST) legacy_part(&rpg_tail[hc],30,40,52,72,120,0,rgb,alpha);
    if(l->off==IT_QUIVER) {
        int back_x=body_x(rig_hand_mounts[fam][1][0],l)+(motion?motion->lean:pose==HP_ATTACK?2:pose==HP_GUARD?-1:pose==HP_HIT?-2:0);
        int back_y=body_y(rig_hand_mounts[fam][1][1] + (motion ? motion->hands_y : 0),l)+(pose==HP_BREATH?-1:0);
        gear_part(&rig_gear[10],back_x,back_y,l->off_tier,false,0,rgb,alpha);
    }
    part(&rig_hair[l->hair%6],l,pose,2,motion,rgb,alpha);
    if (motion) combat_body(&rig_costumes[fam],l,motion,rgb,alpha);
    else part(&rig_costumes[fam],l,pose,0,NULL,rgb,alpha);
    /* The crescent crosses the head on attack. Keep its shaft/grip in front
     * of clothing, but let the face/hair occlude the blade instead of masking
     * the player's expression. Other held items keep the foreground order. */
    if(l->weapon==IT_SCYTHE)gear_part(&rig_gear[IT_SCYTHE],left_x,left_y,l->weapon_tier,false,0,rgb,alpha);
    part(&rig_heads[race][l->body?1:0][expr%4],l,pose,2,motion,rgb,alpha);
    part(&rig_hair[l->hair%6],l,pose,1,motion,rgb,alpha);
    int head_y=24+body_y(32 + (motion ? motion->neck_y : 0),l)-32+(pose==HP_BREATH?-1:pose==HP_HIT?1:0);
    int head_x=44+(motion?motion->lean:pose==HP_ATTACK?2:pose==HP_GUARD?-1:pose==HP_HIT?-2:0);
    if(l->head==IT_HELM || l->head==IT_HOOD || l->head==IT_CIRCLET)
        gear_part(&rig_gear[l->head],head_x,head_y+8,l->head_tier,true,0,rgb,alpha);
    else if(l->head!=0xfe)part(&rig_hats[fam],l,pose,2,motion,rgb,alpha);
    if(l->weapon<IT_SCYTHE) {
        if(l->weapon==IT_KNUCKLE) {
            gear_part(&rig_knuckles[0],left_x,left_y,l->weapon_tier,false,0,rgb,alpha);
            gear_part(&rig_knuckles[1],right_x,right_y,l->weapon_tier,false,0,rgb,alpha);
        } else {
            bool swing = motion && (l->weapon == IT_SWORD || l->weapon == IT_DAGGER || l->weapon == IT_MACE);
            int turn = swing ? pose == HP_WINDUP ? 1 : pose == HP_ATTACK ? -1 : 0 : 0;
            gear_part(&rig_gear[l->weapon],left_x,left_y,l->weapon_tier,false,turn,rgb,alpha);
        }
    }
    if(l->off>=IT_SHIELD && l->off<=IT_CHARM && l->off!=IT_QUIVER)
        gear_part(&rig_gear[l->off],right_x,right_y,l->off_tier,false,0,rgb,alpha);
    return clipped_pixels;
}
