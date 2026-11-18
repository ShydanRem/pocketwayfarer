/* Pure 88x88 rig properties; no platform, save, LVGL runtime or screenshot stubs.
 * --full additionally visits all 7680 cosmetics for each candidate class;
 * raw renderer coverage does not itself admit a class to the UI. */
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "hero_rig.h"
#include "art/hero_rig_art.h"

enum { GRID = 88, PIXELS = GRID * GRID, NO_HAT = 0xfe, EMPTY = 0xff };
typedef struct { uint16_t rgb[PIXELS]; uint8_t alpha[PIXELS]; } image;
static unsigned failures, frames, comparisons, layers_checked, full_cosmetics, joint_rows;
static unsigned sibling_checks, minimum_sibling_alpha = PIXELS;
static unsigned minimum_foot = GRID, maximum_foot;
static unsigned decorative_checks;
static unsigned cape_checks, minimum_cape_opaque = PIXELS;

#define CHECK(condition, ...) do { if (!(condition)) { \
    if (failures < 24) { fprintf(stderr, "FAIL: "); fprintf(stderr, __VA_ARGS__); fputc('\n', stderr); } \
    failures++; } } while (0)

static hero_look bare_look(unsigned class_id, unsigned race) {
    hero_look l = {0};
    l.class_id = (uint8_t)class_id; l.fam = rig_class_families[class_id];
    l.tier = rig_class_tiers[class_id]; l.race = (uint8_t)race;
    l.weapon = l.off = EMPTY; l.head = NO_HAT;
    return l;
}

static int bottom(const image *im) {
    for (int y = GRID-1; y >= 0; y--)
        for (int x = 0; x < GRID; x++) if (im->alpha[y*GRID+x]) return y;
    return -1;
}

static void compose(const hero_look *l, int expression, int pose, image *im) {
    hero_look before = *l;
    /* Poison output: the renderer must fully initialize transparent pixels too. */
    memset(im, 0xa5, sizeof *im);
    unsigned clipped = hero_rig_compose(l, expression, pose, im->rgb, im->alpha);
    frames++;
    CHECK(!clipped, "clipped=%u class=%u race=%u expr=%d pose=%d gear=%u/%u/%u tiers=%u/%u/%u",
          clipped,l->class_id,l->race,expression,pose,l->weapon,l->off,l->head,
          l->weapon_tier,l->off_tier,l->head_tier);
    CHECK(!memcmp(&before,l,sizeof before), "render mutated look class=%u",l->class_id);
    unsigned opaque = 0;
    for (unsigned i = 0; i < PIXELS; i++) {
        if (im->alpha[i]) opaque++;
        CHECK(im->alpha[i] == 0 || im->alpha[i] == 255, "nonbinary alpha at %u",i);
        CHECK(im->alpha[i] || !im->rgb[i], "transparent RGB not cleared at %u",i);
    }
    CHECK(opaque >= 250, "empty/degenerate figure (%u pixels), class=%u race=%u",opaque,l->class_id,l->race);
    int foot = bottom(im);
    CHECK(foot >= 81 && foot <= 84, "feet outside registered plane: row=%d class=%u race=%u",foot,l->class_id,l->race);
    if (foot >= 0 && (unsigned)foot < minimum_foot) minimum_foot = (unsigned)foot;
    if (foot >= 0 && (unsigned)foot > maximum_foot) maximum_foot = (unsigned)foot;
}

static bool differs(const image *a,const image *b,unsigned first_row,unsigned end_row) {
    comparisons++;
    for (unsigned i = first_row*GRID; i < end_row*GRID; i++)
        if (a->alpha[i] != b->alpha[i] || (a->alpha[i] && a->rgb[i] != b->rgb[i])) return true;
    return false;
}

static void check_layer(const hero_rig_layer *layer,unsigned max_role) {
    const wf_sprite *s = &layer->s;
    CHECK(s->px && s->pal && layer->roles && s->w && s->h && s->colors && s->colors <= 256,
          "invalid layer descriptor %u",layers_checked);
    if (!s->px || !s->pal || !layer->roles) return;
    CHECK(s->alpha && layer->roles[0] == 0, "layer %u has invalid transparent slot",layers_checked);
    unsigned count = 0;
    for (unsigned i = 0; i < (unsigned)s->w*s->h; i++) {
        CHECK(s->px[i] < s->colors, "layer %u pixel palette index out of range",layers_checked);
        if (s->px[i]) count++;
    }
    for (unsigned i = 0; i < s->colors; i++) CHECK(layer->roles[i] <= max_role,"layer %u invalid palette role",layers_checked);
    CHECK(count, "layer %u empty",layers_checked);
    layers_checked++;
}

static void metadata(void) {
    /* The core's actual IT_* table uses these exact positions: weapon 0..8,
     * off-hand 9..12, head 13..15. This is NOT the legacy slot-relative look. */
    _Static_assert(IT_SWORD == 0 && IT_KNUCKLE == 6 && IT_SCYTHE == 8,"weapon indices changed");
    _Static_assert(IT_SHIELD == 9 && IT_QUIVER == 10 && IT_TOME == 11 && IT_CHARM == 12,"off-hand indices changed");
    _Static_assert(IT_HELM == 13 && IT_HOOD == 14 && IT_CIRCLET == 15,"head indices changed");
    _Static_assert(HERO_RIG_CLASS_COUNT == 32,"update the explicit class metadata contract for the next batch");
    for (unsigned f = 0; f < HERO_RIG_CLASS_COUNT; f++) {
        /* Freeze core IDs: bases0..7, advanced8..23, masters24..31.
         * Master IDs restart at family0; their UI tier is2, not1. */
        unsigned expected_family = f < FAM_COUNT ? f : f < 24 ? (f-8)/2 : (f-24)/2;
        unsigned expected_tier = f < FAM_COUNT ? 0 : f < 24 ? 1 : 2;
        CHECK(rig_class_families[f] == expected_family && rig_class_tiers[f] == expected_tier,
              "metadata disagrees with core IDs class=%u family=%u tier=%u",f,rig_class_families[f],rig_class_tiers[f]);
        hero_look admitted = bare_look(f,RACE_HUMAN);
        CHECK(hero_uses_rig(&admitted),"admitted exact class rejected class=%u",f);
        admitted.fam = (uint8_t)((expected_family+1)%FAM_COUNT);
        CHECK(!hero_uses_rig(&admitted),"wrong family admitted class=%u",f);
        admitted.fam = (uint8_t)expected_family;
        for (unsigned tier = 0; tier < 3; tier++) if (tier != expected_tier) {
            admitted.tier = (uint8_t)tier;
            CHECK(!hero_uses_rig(&admitted),"wrong UI tier admitted class=%u tier=%u",f,tier);
        }
        check_layer(&rig_costumes[f],3); check_layer(&rig_hats[f],3);
        for (unsigned hand = 0; hand < 2; hand++)
            CHECK(rig_hand_mounts[f][hand][0] > 0 && rig_hand_mounts[f][hand][0] < GRID &&
                  rig_hand_mounts[f][hand][1] > 0 && rig_hand_mounts[f][hand][1] < GRID,"invalid hand mount");
    }
    for (unsigned r = 0; r < RACE_COUNT; r++) for (unsigned body = 0; body < 2; body++)
        for (unsigned e = 0; e < 4; e++) check_layer(&rig_heads[r][body][e],3);
    for (unsigned h = 0; h < 6; h++) check_layer(&rig_hair[h],3);
    for (unsigned g = 0; g <= IT_CIRCLET; g++) check_layer(&rig_gear[g],5);
    for (unsigned hand = 0; hand < 2; hand++) check_layer(&rig_knuckles[hand],5);
}

static void geometry(void) {
    image idle, frame, repeat;
    for (unsigned f = 0; f < HERO_RIG_CLASS_COUNT; f++) for (unsigned r = 0; r < RACE_COUNT; r++)
    for (unsigned skin = 0; skin < 5; skin++) for (unsigned body = 0; body < 2; body++)
    for (unsigned expression = 0; expression < 4; expression++) {
        hero_look l = bare_look(f,r);
        l.skin = (uint8_t)skin; l.body = (uint8_t)body;
        l.hair = (uint8_t)((f+skin+body)%6); l.hair_color = (uint8_t)((f+r+skin)%8);
        l.eyes = (uint8_t)((skin+body)%4); l.head = EMPTY; /* class default hat */
        compose(&l,expression,HP_IDLE,&idle);
        for (int pose = HP_IDLE; pose < HP_COUNT; pose++) {
            compose(&l,expression,pose,&frame);
            CHECK(!differs(&idle,&frame,78,GRID),"feet moved class=%u race=%u expr=%u pose=%d",f,r,expression,pose);
            /* Same input must reproduce every byte, independent of previous frame. */
            compose(&l,expression,pose,&repeat);
            CHECK(!memcmp(&frame,&repeat,sizeof frame),"nondeterministic renderer class=%u race=%u pose=%d",f,r,pose);
        }
    }
}

static bool occupied_body_row(const image *im,unsigned y) {
    /* The central trunk excludes hands, weapons, back-mounted gear and most
     * cape edges. A split at the waist must not be hidden by those layers. */
    for (unsigned x = 36; x <= 54; x++) if (im->alpha[y*GRID+x]) return true;
    return false;
}

static void joint_continuity(void) {
    image idle, frame;
    for (unsigned f = 0; f < HERO_RIG_CLASS_COUNT; f++) for (unsigned r = 0; r < RACE_COUNT; r++) {
        hero_look l = bare_look(f,r); /* No weapon/offhand/hat to hide the seam. */
        compose(&l,HX_IDLE,HP_IDLE,&idle);
        for (int pose = HP_IDLE; pose < HP_COUNT; pose++) {
            compose(&l,HX_IDLE,pose,&frame);
            /* Planted lower legs and moving trunk overlap at their joint.
             * This catches the former breath gap at row63 (row66 for dwarf),
             * without demanding that intentionally empty art becomes filled. */
            for (unsigned y = 40; y <= 77; y++) if (occupied_body_row(&idle,y)) {
                joint_rows++;
                CHECK(occupied_body_row(&frame,y),
                      "transparent torso/leg seam class=%u race=%u pose=%d row=%u",f,r,pose,y);
            }
        }
    }
}

static void visible_channels(void) {
    image variants[HERO_RIG_CLASS_COUNT], baseline, changed;
    static const char *names[] = {"skin","hair style","hair color","eyes","lashes"};
    static const unsigned counts[] = {5,6,8,4,2};
    for (unsigned f = 0; f < HERO_RIG_CLASS_COUNT; f++) for (unsigned r = 0; r < RACE_COUNT; r++) {
        for (unsigned channel = 0; channel < 5; channel++) {
            for (unsigned value = 0; value < counts[channel]; value++) {
                hero_look l = bare_look(f,r); /* idle/open face, no class hat/gear */
                switch (channel) {
                    case 0: l.skin = (uint8_t)value; break;
                    case 1: l.hair = (uint8_t)value; break;
                    case 2: l.hair_color = (uint8_t)value; break;
                    case 3: l.eyes = (uint8_t)value; break;
                    default: l.body = (uint8_t)value; break;
                }
                compose(&l,HX_IDLE,HP_IDLE,&variants[value]);
                for (unsigned previous = 0; previous < value; previous++)
                    CHECK(differs(&variants[previous],&variants[value],0,40),
                          "hidden visible %s values=%u/%u class=%u race=%u (idle face, no hat)",
                          names[channel],previous,value,f,r);
            }
        }
        hero_look l = bare_look(f,r);
        compose(&l,HX_IDLE,HP_IDLE,&baseline);
        for (int expression = HX_BLINK; expression <= HX_HAPPY; expression++) {
            compose(&l,expression,HP_IDLE,&changed);
            CHECK(differs(&baseline,&changed,0,40),"expression %d invisible on exposed face class=%u race=%u",expression,f,r);
        }
        for (int pose = HP_BREATH; pose < HP_COUNT; pose++) {
            compose(&l,HX_IDLE,pose,&changed);
            CHECK(differs(&baseline,&changed,0,GRID),"pose %d invisible class=%u race=%u",pose,f,r);
        }
    }
    /* Different class IDs must select different costumes even without a hat. */
    for (unsigned f = 0; f < HERO_RIG_CLASS_COUNT; f++) {
        hero_look l = bare_look(f,RACE_HUMAN); compose(&l,HX_IDLE,HP_IDLE,&variants[f]);
        for (unsigned prior = 0; prior < f; prior++)
            CHECK(differs(&variants[prior],&variants[f],35,78),"costumes indistinguishable class=%u/%u",prior,f);
    }
}

static void sibling_identity(void) {
    image first, second;
    static const uint8_t siblings[][2] = {
        {8,9},{10,11},{12,13},{14,15},{16,17},{18,19},{20,21},{22,23},
        {24,25},{26,27},{28,29},{30,31}
    };
    for (unsigned pair = 0; pair < sizeof siblings/sizeof siblings[0]; pair++)
    for (unsigned race = 0; race < RACE_COUNT; race++)
    for (unsigned tier = 0; tier < RPG_TIERS; tier++)
    for (int pose = HP_IDLE; pose < HP_COUNT; pose++) {
        hero_look a = bare_look(siblings[pair][0],race);
        hero_look b = bare_look(siblings[pair][1],race);
        a.head = b.head = IT_HELM; a.head_tier = b.head_tier = (uint8_t)tier;
        a.skin = b.skin = (uint8_t)(race%5); a.hair_color = b.hair_color = 2;
        compose(&a,HX_IDLE,pose,&first); compose(&b,HX_IDLE,pose,&second);
        CHECK(a.fam == b.fam && a.tier == b.tier,"siblings assigned different family/tier");
        CHECK(differs(&first,&second,35,78),"siblings share costume below helmet class=%u/%u race=%u pose=%d",
              a.class_id,b.class_id,race,pose);
        unsigned alpha_pixels = 0;
        for (unsigned y = 35; y < 78; y++) for (unsigned x = 0; x < GRID; x++)
            if (first.alpha[y*GRID+x] != second.alpha[y*GRID+x]) alpha_pixels++;
        /* Require a body-outline difference beyond a handful of edge pixels.
         * Shared owned helmet and absent hand gear cannot supply branch identity. */
        CHECK(alpha_pixels >= 16,"siblings lack distinct silhouette class=%u/%u race=%u pose=%d alpha_pixels=%u",
              a.class_id,b.class_id,race,pose,alpha_pixels);
        if (alpha_pixels < minimum_sibling_alpha) minimum_sibling_alpha = alpha_pixels;
        sibling_checks++;
    }
}

static void equipment(void) {
    image bare, frame, tiers[RPG_TIERS];
    for (unsigned f = 0; f < HERO_RIG_CLASS_COUNT; f++) for (unsigned r = 0; r < RACE_COUNT; r++) {
        hero_look l = bare_look(f,r); compose(&l,HX_IDLE,HP_IDLE,&bare);
        for (unsigned item = IT_SWORD; item <= IT_CIRCLET; item++) {
            for (unsigned tier = 0; tier < RPG_TIERS; tier++) {
                hero_look geared = l;
                if (item <= IT_SCYTHE) { geared.weapon = (uint8_t)item; geared.weapon_tier = (uint8_t)tier; }
                else if (item <= IT_CHARM) { geared.off = (uint8_t)item; geared.off_tier = (uint8_t)tier; }
                else { geared.head = (uint8_t)item; geared.head_tier = (uint8_t)tier; }
                for (int pose = HP_IDLE; pose < HP_COUNT; pose++) {
                    compose(&geared,HX_IDLE,pose,&frame);
                    CHECK(!differs(&bare,&frame,78,GRID),"gear moved feet class=%u race=%u item=%u pose=%d",f,r,item,pose);
                    if (pose == HP_IDLE) tiers[tier] = frame;
                }
                CHECK(differs(&bare,&tiers[tier],0,78),"owned item invisible class=%u race=%u item=%u tier=%u",f,r,item,tier);
                for (unsigned prior = 0; prior < tier; prior++)
                    CHECK(differs(&tiers[prior],&tiers[tier],0,78),"gear tiers identical class=%u race=%u item=%u tiers=%u/%u",f,r,item,prior,tier);
            }
        }
        /* Mixed real channels, large weapon plus offhand and helmet together. */
        l.weapon = IT_SCYTHE; l.off = IT_SHIELD; l.head = IT_HELM;
        l.weapon_tier = 4; l.off_tier = 3; l.head_tier = 2;
        for (int pose = HP_IDLE; pose < HP_COUNT; pose++) compose(&l,HX_HURT,pose,&frame);
    }
}

static void fixed_decorations(void) {
    /* The Alchemist's flask belt is clothing, not exposed skin. Purple glass
     * can share RGB565 with hand shadows without inheriting their skin tint. */
    image baseline, changed;hero_look l=bare_look(21,RACE_HUMAN);
    compose(&l,HX_IDLE,HP_IDLE,&baseline);
    for(unsigned skin=1;skin<5;skin++) {
        l.skin=(uint8_t)skin;compose(&l,HX_IDLE,HP_IDLE,&changed);
        for(unsigned y=45;y<64;y++)for(unsigned x=36;x<55;x++) {
            unsigned p=y*GRID+x;decorative_checks++;
            CHECK(baseline.alpha[p]==changed.alpha[p] && baseline.rgb[p]==changed.rgb[p],
                  "decorative flask belt follows skin=%u at %u,%u",skin,x,y);
        }
    }
}

static void cape_continuity(void) {
    /* Long left capes must remain intact through the provisional torso lean.
     * Observe the composed pixels below the arm/neck, without held equipment,
     * class hat, tail or long hair supplying pixels that could hide a tear. */
    static const unsigned classes[] = {24,30};
    static const struct { int pose, dx; } leans[] = {
        {HP_ATTACK,2},{HP_GUARD,-1},{HP_HIT,-2}
    };
    image idle, leaned;
    CHECK(rig_hair[0].y+rig_hair[0].s.h < 55,
          "short hair intrudes into cape observation region");
    CHECK(rig_heads[RACE_HUMAN][0][HX_IDLE].y+rig_heads[RACE_HUMAN][0][HX_IDLE].s.h+1 < 55,
          "head intrudes into cape observation region");
    for (unsigned c = 0; c < sizeof classes/sizeof classes[0]; c++) {
        hero_look l = bare_look(classes[c],RACE_HUMAN);
        l.hair = 0;
        compose(&l,HX_IDLE,HP_IDLE,&idle);
        unsigned opaque = 0;
        for (unsigned y = 55; y < 77; y++) for (unsigned x = 10; x < 33; x++)
            if (idle.alpha[y*GRID+x]) opaque++;
        CHECK(opaque >= 64,"cape observation lacks coverage class=%u opaque=%u",l.class_id,opaque);
        if (opaque < minimum_cape_opaque) minimum_cape_opaque = opaque;
        for (unsigned p = 0; p < sizeof leans/sizeof leans[0]; p++) {
            compose(&l,HX_IDLE,leans[p].pose,&leaned);
            for (unsigned y = 55; y < 77; y++) for (unsigned x = 10; x < 33; x++) {
                unsigned from = y*GRID+x, to = (unsigned)((int)from+leans[p].dx);
                cape_checks++;
                CHECK(idle.alpha[from] == leaned.alpha[to] && idle.rgb[from] == leaned.rgb[to],
                      "cape torn/recolored class=%u pose=%d at %u,%u",l.class_id,leans[p].pose,x,y);
            }
        }
    }
}

static void full(void) {
    image frame;
    for (unsigned f = 0; f < HERO_RIG_CLASS_COUNT; f++) for (unsigned r = 0; r < RACE_COUNT; r++)
    for (unsigned skin = 0; skin < 5; skin++) for (unsigned hair = 0; hair < 6; hair++)
    for (unsigned color = 0; color < 8; color++) for (unsigned eyes = 0; eyes < 4; eyes++)
    for (unsigned body = 0; body < 2; body++) {
        hero_look l = bare_look(f,r);
        l.skin = (uint8_t)skin; l.hair = (uint8_t)hair; l.hair_color = (uint8_t)color;
        l.eyes = (uint8_t)eyes; l.body = (uint8_t)body;
        compose(&l,HX_IDLE,HP_IDLE,&frame); full_cosmetics++;
    }
    CHECK(full_cosmetics == HERO_RIG_CLASS_COUNT*7680,"incomplete cosmetics enumeration: %u",full_cosmetics);
}

int main(int argc,char **argv) {
    bool exhaustive = argc == 2 && !strcmp(argv[1],"--full");
    if (argc > 1 && !exhaustive) { fprintf(stderr,"usage: %s [--full]\n",argv[0]); return 2; }
    metadata(); geometry(); joint_continuity(); visible_channels(); sibling_identity(); equipment(); fixed_decorations(); cape_continuity();
    if (exhaustive) full();
    printf("hero rig: layers=%u frames=%u comparisons=%u joint_rows=%u full_cosmetics=%u sibling_checks=%u min_sibling_alpha=%u decorative_checks=%u cape_checks=%u min_cape_opaque=%u foot_rows=%u..%u failures=%u\n",
           layers_checked,frames,comparisons,joint_rows,full_cosmetics,sibling_checks,minimum_sibling_alpha,
           decorative_checks,cape_checks,minimum_cape_opaque,minimum_foot,maximum_foot,failures);
    return failures ? 1 : 0;
}
