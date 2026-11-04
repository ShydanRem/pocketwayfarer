#include "save.h"
#include <string.h>

uint32_t wf_crc32(const void *data, size_t len) {
    const uint8_t *p = data;
    uint32_t crc = 0xFFFFFFFFu;
    while (len--) {
        crc ^= *p++;
        for (int k = 0; k < 8; k++) crc = (crc >> 1) ^ (0xEDB88320u & (uint32_t)-(int32_t)(crc & 1u));
    }
    return ~crc;
}

size_t wf_save_encode(const wf_game *g, uint32_t sequence, uint8_t *out, size_t cap) {
    if (cap < WF_SAVE_BYTES) return 0;
    wf_save_header h = {WF_SAVE_MAGIC, WF_SAVE_VERSION, (uint16_t)sizeof(wf_game), wf_crc32(g, sizeof *g), sequence};
    memcpy(out, &h, sizeof h);
    memcpy(out + sizeof h, g, sizeof *g);
    return WF_SAVE_BYTES;
}

bool wf_save_decode(const uint8_t *in, size_t len, wf_game *g, uint32_t *sequence) {
    if (len < WF_SAVE_BYTES) return false;
    wf_save_header h;
    memcpy(&h, in, sizeof h);
    if (h.magic != WF_SAVE_MAGIC || h.version != WF_SAVE_VERSION || h.size != sizeof(wf_game)) return false;
    if (wf_crc32(in + sizeof h, sizeof(wf_game)) != h.crc) return false;
    memcpy(g, in + sizeof h, sizeof *g);
    if (sequence) *sequence = h.sequence;
    wf_save_sanitize(g);
    return true;
}

static void clamp32(int32_t *v) { if (*v < 0) *v = 0; if (*v > WF_NEED_MAX) *v = WF_NEED_MAX; }

void wf_save_sanitize(wf_game *g) {
    wf_pet *p = &g->pet;
    p->name[WF_NAME_LEN] = 0;
    if (p->form >= WF_FORM_COUNT) p->form = WF_FORM_LUMINO;
    if (p->pending_form >= WF_FORM_COUNT) p->pending_form = 0;
    if (p->level < 1) p->level = 1;
    if (p->level > WF_MAX_LEVEL) p->level = WF_MAX_LEVEL;
    clamp32(&p->hunger); clamp32(&p->energy); clamp32(&p->mood); clamp32(&p->bond);
    if (p->mess > WF_MESS_MAX) p->mess = WF_MESS_MAX;
    p->sleeping = p->sleeping ? 1 : 0;
    if (p->away_zone >= 5) p->away_zone = 0;
    for (int i = 0; i < WF_IT_COUNT; i++) if (g->inv[i] > WF_ITEM_STACK) g->inv[i] = WF_ITEM_STACK;
    if (g->zone_unlocked >= 5) g->zone_unlocked = 4;
    for (int i = 0; i < WF_QUESTS; i++) {
        if (g->quests[i].kind >= WF_Q_COUNT) g->quests[i].kind = 0;
        if (g->quests[i].item >= WF_IT_COUNT) g->quests[i].item = WF_IT_BERRY;
        if (g->quests[i].progress > g->quests[i].goal) g->quests[i].progress = g->quests[i].goal;
    }
    for (int i = 0; i < WF_FRIENDS; i++) g->friends[i].name[WF_NAME_LEN] = 0;
    if (g->brightness < 10 || g->brightness > 100) g->brightness = 80;
    g->hour_offset %= 24;
}
