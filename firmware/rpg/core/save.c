/* Versioned, checksummed snapshot of rpg_game. */
#include <string.h>
#include "rpg.h"

typedef struct {
    uint32_t magic;
    uint16_t version;
    uint16_t reserved;
    uint32_t size;
    uint32_t crc;
} rpg_save_header;

_Static_assert(sizeof(rpg_save_header) == 16, "Save header ABI changed");
_Static_assert(offsetof(rpg_game, lantern) == 1356, "Keep the v1 profile prefix byte-compatible");
_Static_assert(offsetof(rpg_lantern, action) == 12, "Keep the combat action and victory receipt at their saved offset");
_Static_assert(offsetof(rpg_lantern, equipped) == 64, "Keep the v2 run prefix byte-compatible");
_Static_assert(sizeof(rpg_lantern) == 68, "Keep the v3 run size byte-compatible");
_Static_assert(offsetof(rpg_game, coach) == RPG_V3_PROFILE_BYTES, "Keep the v3 profile byte-compatible");
_Static_assert(offsetof(rpg_game, families) == RPG_V4_PROFILE_BYTES, "Keep the v4 profile byte-compatible");
_Static_assert(sizeof(rpg_game) + sizeof(rpg_save_header) <= 4096, "UI save buffer too small");

void rpg_classic_begin(rpg_game *g) {
    if (g->classic.active) return;
    memcpy(g->classic.profile, g, RPG_V1_PROFILE_BYTES);
    g->classic.gold_remainder = g->gold_remainder;
    g->classic.active = 1;
}
void rpg_classic_cancel(rpg_game *g) { memset(&g->classic, 0, sizeof g->classic); }
void rpg_classic_recover(rpg_game *g) {
    if (g->classic.active != 1) return;
    memcpy(g, g->classic.profile, RPG_V1_PROFILE_BYTES);
    g->gold_remainder = g->classic.gold_remainder;
    rpg_classic_cancel(g);
}

static uint32_t crc32(const uint8_t *p, size_t len) {
    uint32_t crc = 0xFFFFFFFFu;
    while (len--) {
        crc ^= *p++;
        for (int k = 0; k < 8; k++) crc = (crc >> 1) ^ (0xEDB88320u & (uint32_t)-(int32_t)(crc & 1u));
    }
    return ~crc;
}

size_t rpg_save_size(void) { return sizeof(rpg_save_header) + sizeof(rpg_game); }

size_t rpg_save_encode(const rpg_game *g, uint8_t *out, size_t cap) {
    if (cap < rpg_save_size()) return 0;
    rpg_save_header h = {RPG_SAVE_MAGIC, RPG_SAVE_VERSION, 0, (uint32_t)sizeof *g, crc32((const uint8_t *)g, sizeof *g)};
    memcpy(out, &h, sizeof h);
    memcpy(out + sizeof h, g, sizeof *g);
    return rpg_save_size();
}

static void sanitize(rpg_game *g) {
    rpg_hero *h = &g->hero;
    h->name[RPG_NAME_LEN] = 0;
    if (h->race >= RACE_COUNT) h->race = 0;
    if (h->cls >= RPG_CLASS_COUNT) h->cls = 0;
    if (h->level < 1 || h->level > RPG_MAX_LEVEL) h->level = 1;
    if (h->job_level < 1 || h->job_level > rpg_job_max(h->cls)) h->job_level = 1;
    for (int family = 0; family < FAM_COUNT; family++) {
        rpg_family_progress *job = &g->families[family];
        if (!job->job_level || job->cls >= RPG_CLASS_COUNT || rpg_class_family(job->cls) != family ||
            rpg_class_family(h->cls) == family || job->job_level > rpg_job_max(job->cls))
            *job = (rpg_family_progress){0};
    }
    for (int s = 0; s < RPG_SKILL_COUNT; s++) if (h->skill[s] > RPG_SKILL_MAX_LV) h->skill[s] = RPG_SKILL_MAX_LV;
    for (int i = 0; i < SLOT_COUNT; i++) if (h->equip[i].type >= IT_TYPE_COUNT) h->equip[i].type = 0xff;
    for (int i = 0; i < RPG_BAG; i++) if (g->bag[i].type >= IT_TYPE_COUNT) g->bag[i].type = 0xff;
    if (h->hunger < 0 || h->hunger > RPG_NEED_MAX) h->hunger = RPG_NEED_MAX / 2;
    if (h->stamina < 0 || h->stamina > 100) h->stamina = 50;
    if (h->morale < 0 || h->morale > 100) h->morale = 50;
    if (g->zone_unlocked >= RPG_ZONES) g->zone_unlocked = 0;
    if (g->run.zone >= RPG_ZONES || g->run.floor < 1 || g->run.floor > RPG_FLOORS) g->run.active = 0;
    if (g->run.row < -1 || g->run.row > RPG_ROWS || g->run.col < 0 || g->run.col >= RPG_COLS) g->run.active = 0;
    if (g->hunt.zone >= RPG_ZONES) g->hunt.active = 0;
    for (int i = 0; i < RPG_QUESTS; i++) if (g->quests[i].kind >= Q_KIND_COUNT) g->quests[i].kind = 0;
    for (int i = 0; i < RPG_QUESTS; i++) if (g->quests[i].kind == Q_GREET && !g->quests[i].claimed) {
        unsigned used = 0;
        for (int j = 0; j < RPG_QUESTS; j++) if (j != i) used |= 1u << g->quests[j].kind;
        int kind = 0;
        while (kind < Q_GREET && (used & (1u << kind))) kind++;
        g->quests[i].kind = (uint8_t)kind;
        g->quests[i].goal = (uint8_t)(kind == Q_KILL ? 10 : kind == Q_ROOMS ? 4 : 1);
        g->quests[i].progress = 0;
    }
    g->lang = g->lang ? 1 : 0;
    if (g->brightness < 10 || g->brightness > 100) g->brightness = 80;
    g->hour_offset %= 24;
    ln_sanitize(&g->lantern);
    g->coach.learned &= LNC_ALL;
    g->coach.disabled = !!g->coach.disabled;
    g->gold_remainder %= 50;
    if (g->classic.active > 2 || g->classic.zone >= RPG_ZONES ||
        (g->classic.active == 2 && (g->classic.outcome < B_WON || g->classic.outcome > B_FLED))) rpg_classic_cancel(g);
}

bool rpg_save_decode(const uint8_t *in, size_t len, rpg_game *g) {
    if (!in || len < sizeof(rpg_save_header)) return false;
    rpg_save_header h;
    memcpy(&h, in, sizeof h);
    if (h.magic != RPG_SAVE_MAGIC) return false;
    bool old = (h.version == 1 && h.size == RPG_V1_PROFILE_BYTES) ||
               (h.version == 2 && h.size == RPG_V2_PROFILE_BYTES);
    bool v3 = h.version == 3 && h.size == RPG_V3_PROFILE_BYTES;
    bool v4 = h.version == 4 && h.size == RPG_V4_PROFILE_BYTES;
    if (!old && !v3 && !v4 && (h.version != RPG_SAVE_VERSION || h.size != sizeof *g)) return false;
    if (len != sizeof h + h.size) return false;
    if (crc32(in + sizeof h, h.size) != h.crc) return false;
    memset(g, 0, sizeof *g);
    memcpy(g, in + sizeof h, h.size);
    if (old || v3) {
        g->lantern.effect_actor = g->lantern.target < 2 ? g->lantern.target : 0;
        g->lantern.effect_flags = 0;
        /* Existing winners can opt into replaying tips rather than being
         * forced back through first-fight instruction after an upgrade. */
        if (g->lantern.wins) g->coach.learned = LNC_ALL;
    }
    rpg_classic_recover(g);
    sanitize(g);
    /* Old saves may point at a fight which was never resolved. Return to its
     * actual parent node; do not permit skipping the fight or stranding a boss. */
    if (old && g->run.active && g->run.row >= 0 &&
        !(g->run.cleared[g->run.row] & (1u << g->run.col))) {
        int row = g->run.row, col = g->run.col;
        int kind = g->run.rooms[row][col];
        if (kind == ROOM_FIGHT || kind == ROOM_ELITE || kind == ROOM_BOSS) {
            int parent = 1;
            if (row > 0) for (int c = 0; c < RPG_COLS; c++)
                if (g->run.rooms[row - 1][c] && (g->run.links[row - 1][c] & (1u << col))) { parent = c; break; }
            g->run.row = (int8_t)(row - 1); g->run.col = (int8_t)parent;
            if (kind != ROOM_BOSS && g->hero.stamina <= 98) g->hero.stamina += 2;
        }
    }
    wf_lang = g->lang;
    return true;
}
