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
    for (int s = 0; s < RPG_SKILL_COUNT; s++) if (h->skill[s] > RPG_SKILL_MAX_LV) h->skill[s] = RPG_SKILL_MAX_LV;
    for (int i = 0; i < SLOT_COUNT; i++) if (h->equip[i].type >= IT_TYPE_COUNT) h->equip[i].type = 0xff;
    for (int i = 0; i < RPG_BAG; i++) if (g->bag[i].type >= IT_TYPE_COUNT) g->bag[i].type = 0xff;
    if (h->hunger < 0 || h->hunger > RPG_NEED_MAX) h->hunger = RPG_NEED_MAX / 2;
    if (h->stamina < 0 || h->stamina > 100) h->stamina = 50;
    if (h->morale < 0 || h->morale > 100) h->morale = 50;
    if (g->zone_unlocked >= RPG_ZONES) g->zone_unlocked = 0;
    if (g->run.zone >= RPG_ZONES || g->run.floor < 1 || g->run.floor > RPG_FLOORS) g->run.active = 0;
    if (g->hunt.zone >= RPG_ZONES) g->hunt.active = 0;
    for (int i = 0; i < RPG_QUESTS; i++) if (g->quests[i].kind >= Q_KIND_COUNT) g->quests[i].kind = 0;
    g->lang = g->lang ? 1 : 0;
    if (g->brightness < 10 || g->brightness > 100) g->brightness = 80;
    g->hour_offset %= 24;
}

bool rpg_save_decode(const uint8_t *in, size_t len, rpg_game *g) {
    if (!in || len < rpg_save_size()) return false;
    rpg_save_header h;
    memcpy(&h, in, sizeof h);
    if (h.magic != RPG_SAVE_MAGIC || h.version != RPG_SAVE_VERSION || h.size != sizeof *g) return false;
    if (crc32(in + sizeof h, sizeof *g) != h.crc) return false;
    memcpy(g, in + sizeof h, sizeof *g);
    sanitize(g);
    wf_lang = g->lang;
    return true;
}
