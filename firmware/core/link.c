#include "link.h"
#include <string.h>

#define BEACON_MS 1500u
#define PEER_TTL_MS 9000u
#define RETRY_MS 250u
#define OUTBOX_TRIES 16
#define INVITE_TIMEOUT_MS 30000u
#define DUEL_SILENCE_MS 20000u

/* ---- Packet codec ----------------------------------------------------- */
static void put16(uint8_t *p, uint16_t v) { p[0] = (uint8_t)v; p[1] = (uint8_t)(v >> 8); }
static void put32(uint8_t *p, uint32_t v) { for (int i = 0; i < 4; i++) p[i] = (uint8_t)(v >> (8 * i)); }
static uint16_t get16(const uint8_t *p) { return (uint16_t)(p[0] | (p[1] << 8)); }
static uint32_t get32(const uint8_t *p) {
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}
static uint8_t checksum(const uint8_t *p, size_t n) {
    uint8_t s = 0x5A;
    for (size_t i = 0; i < n; i++) s = (uint8_t)((s << 1 | s >> 7) ^ p[i]);
    return s;
}

size_t wf_packet_encode(const wf_packet *p, uint8_t *o, size_t cap) {
    if (cap < WF_PACKET_BYTES) return 0;
    memset(o, 0, WF_PACKET_BYTES);
    o[0] = 'W'; o[1] = 'F'; o[2] = WF_LINK_VERSION; o[3] = p->type;
    put32(o + 4, p->session);
    for (int i = 0; i < WF_NAME_LEN && p->who.name[i]; i++) o[8 + i] = (uint8_t)p->who.name[i];
    o[20] = p->who.form; o[21] = p->who.level; o[22] = p->who.element; o[23] = p->who.flags;
    put16(o + 24, p->who.wins);
    put16(o + 26, (uint16_t)p->hp); put16(o + 28, (uint16_t)p->atk);
    put16(o + 30, (uint16_t)p->def); put16(o + 32, (uint16_t)p->spd);
    put32(o + 34, p->seed);
    o[38] = p->item; o[39] = p->count;
    put16(o + 40, p->round);
    o[42] = p->action; o[43] = p->quality;
    o[WF_PACKET_BYTES - 1] = checksum(o, WF_PACKET_BYTES - 1);
    return WF_PACKET_BYTES;
}

bool wf_packet_decode(const uint8_t *in, size_t len, wf_packet *p) {
    if (len < WF_PACKET_BYTES || in[0] != 'W' || in[1] != 'F' || in[2] != WF_LINK_VERSION) return false;
    if (checksum(in, WF_PACKET_BYTES - 1) != in[WF_PACKET_BYTES - 1]) return false;
    if (in[3] == 0 || in[3] >= WF_PKT_TYPES) return false;
    memset(p, 0, sizeof *p);
    p->type = in[3];
    p->session = get32(in + 4);
    for (int i = 0; i < WF_NAME_LEN; i++) {
        char c = (char)in[8 + i];
        p->who.name[i] = c;
        if (!c) break;
    }
    p->who.name[WF_NAME_LEN] = 0;
    p->who.form = in[20] < WF_FORM_COUNT ? in[20] : WF_FORM_LUMINO;
    p->who.level = in[21];
    p->who.element = in[22] < WF_EL_COUNT ? in[22] : 0;
    p->who.flags = in[23];
    p->who.wins = get16(in + 24);
    p->hp = (int16_t)get16(in + 26); p->atk = (int16_t)get16(in + 28);
    p->def = (int16_t)get16(in + 30); p->spd = (int16_t)get16(in + 32);
    p->seed = get32(in + 34);
    p->item = in[38]; p->count = in[39];
    p->round = get16(in + 40);
    p->action = in[42]; p->quality = in[43];
    return true;
}

void wf_profile_from_game(wf_profile *out, const wf_game *g, uint32_t now) {
    memset(out, 0, sizeof *out);
    wf_copy_name(out->name, g->pet.name);
    out->form = g->pet.form;
    out->level = g->pet.level;
    out->element = (uint8_t)wf_pet_element(g);
    out->wins = g->duel_wins;
    if (g->pet.sleeping) out->flags |= WF_PROFILE_ASLEEP;
    if (wf_pet_away(g, now)) out->flags |= WF_PROFILE_AWAY;
}

/* ---- Plumbing --------------------------------------------------------- */
static void send_pkt(wf_link *l, const uint8_t *peer, const wf_packet *p) {
    uint8_t buf[WF_PACKET_BYTES];
    size_t n = wf_packet_encode(p, buf, sizeof buf);
    if (n && l->send) l->send(l->ctx, peer, buf, n);
}

static void emit(wf_link *l, int kind, const uint8_t id[6], const char *name, int item, int count, int value) {
    if (l->ev_count >= WF_LINK_EVENTS) {           /* drop the oldest */
        l->ev_head = (uint8_t)((l->ev_head + 1) % WF_LINK_EVENTS);
        l->ev_count--;
    }
    wf_link_event *e = &l->events[(l->ev_head + l->ev_count) % WF_LINK_EVENTS];
    memset(e, 0, sizeof *e);
    e->kind = (uint8_t)kind;
    if (id) memcpy(e->id, id, 6);
    if (name) wf_copy_name(e->name, name);
    e->item = (uint8_t)item;
    e->count = (uint8_t)count;
    e->value = (uint8_t)value;
    l->ev_count++;
}

bool wf_link_next_event(wf_link *l, wf_link_event *out) {
    if (!l->ev_count) return false;
    *out = l->events[l->ev_head];
    l->ev_head = (uint8_t)((l->ev_head + 1) % WF_LINK_EVENTS);
    l->ev_count--;
    return true;
}

static uint32_t seen_key(const uint8_t id[6], uint32_t session, uint8_t type) {
    uint32_t h = session * 2654435761u ^ type;
    for (int i = 0; i < 6; i++) h = (h ^ id[i]) * 16777619u;
    return h | 1u;
}

/* Returns true if this (peer, session, type) was already handled. */
static bool seen_before(wf_link *l, const uint8_t id[6], uint32_t session, uint8_t type) {
    uint32_t k = seen_key(id, session, type);
    for (int i = 0; i < 16; i++) if (l->seen[i] == k) return true;
    l->seen[l->seen_next] = k;
    l->seen_next = (uint8_t)((l->seen_next + 1) % 16);
    return false;
}

void wf_link_init(wf_link *l, wf_link_send_fn send, void *ctx, uint32_t seed) {
    memset(l, 0, sizeof *l);
    l->send = send;
    l->ctx = ctx;
    l->beacon_on = 1;
    wf_rng_seed(&l->rng, seed);
}

int wf_link_peer_count(const wf_link *l) {
    int n = 0;
    for (int i = 0; i < WF_PEERS; i++) n += l->peers[i].used;
    return n;
}

const wf_peer *wf_link_peer(const wf_link *l, int index) {
    for (int i = 0; i < WF_PEERS; i++) {
        if (!l->peers[i].used) continue;
        if (index-- == 0) return &l->peers[i];
    }
    return NULL;
}

static void note_peer(wf_link *l, const uint8_t id[6], const wf_profile *who, uint32_t now_ms) {
    wf_peer *slot = NULL, *oldest = &l->peers[0];
    for (int i = 0; i < WF_PEERS; i++) {
        wf_peer *p = &l->peers[i];
        if (p->used && !memcmp(p->id, id, 6)) { slot = p; break; }
        if (!slot && !p->used) slot = p;
        if (p->seen_ms < oldest->seen_ms) oldest = p;
    }
    if (!slot) slot = oldest;
    if (!slot->used || memcmp(slot->id, id, 6)) {
        memset(slot, 0, sizeof *slot);
        memcpy(slot->id, id, 6);
        slot->used = 1;
    }
    slot->who = *who;
    slot->seen_ms = now_ms;
}

/* ---- Duel helpers ----------------------------------------------------- */
static void fill_snapshot(wf_packet *p, const wf_game *g, uint32_t now_s) {
    wf_fighter f;
    wf_fighter_from_pet(&f, g);
    wf_profile_from_game(&p->who, g, now_s);
    p->hp = f.max_hp; p->atk = f.atk; p->def = f.def; p->spd = f.spd;
}

static void fighter_from_packet(wf_fighter *f, const wf_packet *p) {
    memset(f, 0, sizeof *f);
    wf_copy_name(f->name, p->who.name);
    f->element = p->who.element;
    f->level = p->who.level;
    f->sprite = p->who.form;
    f->is_pet = 1;
    f->max_hp = f->hp = p->hp > 0 ? p->hp : 1;
    f->atk = p->atk; f->def = p->def; f->spd = p->spd;
}

static void duel_begin(wf_duel *d, const wf_packet *a, const wf_packet *b) {
    wf_fighter fa, fb;
    fighter_from_packet(&fa, a);
    fighter_from_packet(&fb, b);
    uint32_t seed = (d->role ? d->peer_seed : d->my_seed) ^ ((d->role ? d->my_seed : d->peer_seed) * 2654435761u);
    wf_battle_start(&d->battle, &fa, &fb, seed, false, true);
    d->round = 0;
    d->my_ready = d->peer_ready = 0;
    d->state = WF_DUEL_ACTIVE;
}

static void send_turn(wf_link *l, uint16_t round, uint8_t act, uint8_t q) {
    wf_packet p = {0};
    p.type = WF_PKT_DUEL_TURN;
    p.session = l->duel.session;
    p.round = round;
    p.action = act;
    p.quality = q;
    send_pkt(l, l->duel.peer, &p);
}

static void try_resolve(wf_link *l) {
    wf_duel *d = &l->duel;
    if (d->state != WF_DUEL_ACTIVE || !d->my_ready || !d->peer_ready) return;
    int a_act = d->role ? d->peer_act : d->my_act, a_q = d->role ? d->peer_q : d->my_q;
    int b_act = d->role ? d->my_act : d->peer_act, b_q = d->role ? d->my_q : d->peer_q;
    wf_duel_round(&d->battle, a_act, a_q, b_act, b_q);
    d->prev_act = d->my_act;
    d->prev_q = d->my_q;
    d->my_ready = d->peer_ready = 0;
    d->round++;
    if (wf_battle_over(&d->battle)) {
        d->state = WF_DUEL_DONE;
        d->done_ms = d->last_send_ms > d->last_heard_ms ? d->last_send_ms : d->last_heard_ms;
    }
    emit(l, WF_LE_DUEL_ROUND, d->peer, d->peer_who.name, 0, 0, 0);
}

int wf_link_duel_me(const wf_link *l) { return l->duel.role; }

/* ---- Receive ---------------------------------------------------------- */
void wf_link_receive(wf_link *l, wf_game *g, const uint8_t from[6], const uint8_t *data, size_t len,
                     uint32_t now_ms, uint32_t now_s) {
    wf_packet p;
    if (!wf_packet_decode(data, len, &p)) return;
    wf_duel *d = &l->duel;
    bool from_duel_peer = d->state != WF_DUEL_IDLE && !memcmp(from, d->peer, 6) && p.session == d->session;
    if (from_duel_peer) d->last_heard_ms = now_ms;

    switch (p.type) {
    case WF_PKT_BEACON:
        note_peer(l, from, &p.who, now_ms);
        break;

    case WF_PKT_GREET: {
        note_peer(l, from, &p.who, now_ms);
        wf_packet ack = {0};
        ack.type = WF_PKT_GREET_ACK;
        ack.session = p.session;
        wf_profile_from_game(&ack.who, g, now_s);
        if (!seen_before(l, from, p.session, p.type)) {
            bool counted = wf_greet(g, from, p.who.name, p.who.form, p.who.level, now_s);
            emit(l, WF_LE_GREETED, from, p.who.name, 0, 0, counted);
        }
        send_pkt(l, from, &ack);
        break;
    }
    case WF_PKT_GREET_ACK:
        if (l->out.active && l->out.pkt.type == WF_PKT_GREET && l->out.pkt.session == p.session &&
            !memcmp(l->out.peer, from, 6)) {
            l->out.active = 0;
            bool counted = wf_greet(g, from, p.who.name, p.who.form, p.who.level, now_s);
            emit(l, WF_LE_GREET_DONE, from, p.who.name, 0, 0, counted);
        }
        break;

    case WF_PKT_GIFT: {
        wf_packet ack = {0};
        ack.type = WF_PKT_GIFT_ACK;
        ack.session = p.session;
        if (!seen_before(l, from, p.session, p.type) && p.item < WF_IT_COUNT && p.count) {
            wf_give_item(g, p.item, p.count);
            wf_friend_upsert(g, from, p.who.name, p.who.form, p.who.level);
            emit(l, WF_LE_GIFT_IN, from, p.who.name, p.item, p.count, 0);
        }
        send_pkt(l, from, &ack);
        break;
    }
    case WF_PKT_GIFT_ACK:
        if (l->out.active && l->out.pkt.type == WF_PKT_GIFT && l->out.pkt.session == p.session &&
            !memcmp(l->out.peer, from, 6)) {
            l->out.active = 0;
            emit(l, WF_LE_GIFT_DONE, from, NULL, l->out.pkt.item, l->out.pkt.count, 0);
        }
        break;

    case WF_PKT_DUEL_INVITE:
        note_peer(l, from, &p.who, now_ms);
        if (from_duel_peer) break;                      /* duplicate */
        if (d->state != WF_DUEL_IDLE || !wf_pet_can_act(g, now_s)) {
            wf_packet no = {0};
            no.type = WF_PKT_DUEL_DECLINE;
            no.session = p.session;
            send_pkt(l, from, &no);
            break;
        }
        memset(d, 0, sizeof *d);
        d->state = WF_DUEL_INVITED;
        d->role = 1;
        memcpy(d->peer, from, 6);
        d->peer_who = p.who;
        d->session = p.session;
        d->peer_seed = p.seed;
        d->invite = p;
        d->opened_ms = d->last_heard_ms = now_ms;
        emit(l, WF_LE_DUEL_INVITED, from, p.who.name, 0, 0, 0);
        break;

    case WF_PKT_DUEL_ACCEPT:
        if (!from_duel_peer || d->role != 0) break;
        if (d->state == WF_DUEL_INVITING) {
            d->peer_seed = p.seed;
            d->peer_who = p.who;
            duel_begin(d, &d->invite, &p);
            emit(l, WF_LE_DUEL_STARTED, from, p.who.name, 0, 0, 0);
        }
        if (d->state == WF_DUEL_ACTIVE || d->state == WF_DUEL_DONE) {
            wf_packet go = {0};
            go.type = WF_PKT_DUEL_START;
            go.session = d->session;
            send_pkt(l, from, &go);
        }
        break;

    case WF_PKT_DUEL_DECLINE:
        if (from_duel_peer && d->state == WF_DUEL_INVITING) {
            emit(l, WF_LE_DUEL_DECLINED, from, d->peer_who.name, 0, 0, 0);
            memset(d, 0, sizeof *d);
        }
        break;

    case WF_PKT_DUEL_START:
        if (from_duel_peer && d->role == 1 && !d->started) {
            d->started = 1;
            emit(l, WF_LE_DUEL_STARTED, from, d->peer_who.name, 0, 0, 0);
        }
        break;

    case WF_PKT_DUEL_TURN:
        if (!from_duel_peer || (d->state != WF_DUEL_ACTIVE && d->state != WF_DUEL_DONE)) break;
        if (d->role == 1 && !d->started) {             /* START was lost; a turn implies it */
            d->started = 1;
            emit(l, WF_LE_DUEL_STARTED, from, d->peer_who.name, 0, 0, 0);
        }
        if (p.round == d->round && d->state == WF_DUEL_ACTIVE) {
            if (!d->peer_ready) {
                d->peer_ready = 1;
                d->peer_act = p.action;
                d->peer_q = p.quality > 3 ? 3 : p.quality;
            }
            try_resolve(l);
        } else if (p.round + 1 == d->round) {
            /* The peer missed our move for the round we already resolved. */
            send_turn(l, p.round, d->prev_act, d->prev_q);
        }
        break;

    case WF_PKT_DUEL_LEAVE:
        if (from_duel_peer) {
            if (d->state != WF_DUEL_DONE) emit(l, WF_LE_DUEL_LEFT, from, d->peer_who.name, 0, 0, 0);
            memset(d, 0, sizeof *d);
        }
        break;
    default:
        break;
    }
}

/* ---- Poll ------------------------------------------------------------- */
void wf_link_poll(wf_link *l, wf_game *g, uint32_t now_ms, uint32_t now_s) {
    for (int i = 0; i < WF_PEERS; i++)
        if (l->peers[i].used && now_ms - l->peers[i].seen_ms > PEER_TTL_MS) l->peers[i].used = 0;

    if (l->beacon_on && (now_ms - l->last_beacon_ms >= BEACON_MS || !l->last_beacon_ms)) {
        l->last_beacon_ms = now_ms ? now_ms : 1;
        wf_packet b = {0};
        b.type = WF_PKT_BEACON;
        wf_profile_from_game(&b.who, g, now_s);
        if (l->duel.state != WF_DUEL_IDLE) b.who.flags |= WF_PROFILE_BUSY;
        send_pkt(l, NULL, &b);
    }

    if (l->out.active && now_ms - l->out.last_ms >= RETRY_MS) {
        if (l->out.tries >= OUTBOX_TRIES) {
            l->out.active = 0;
            if (l->out.pkt.type == WF_PKT_GIFT) wf_give_item(g, l->out.pkt.item, l->out.pkt.count);
            emit(l, WF_LE_SEND_FAILED, l->out.peer, NULL, l->out.pkt.item, l->out.pkt.count, l->out.pkt.type);
        } else {
            l->out.tries++;
            l->out.last_ms = now_ms;
            send_pkt(l, l->out.peer, &l->out.pkt);
        }
    }

    wf_duel *d = &l->duel;
    if (d->state == WF_DUEL_IDLE || now_ms - d->last_send_ms < RETRY_MS) return;
    switch (d->state) {
    case WF_DUEL_INVITING:
        if (now_ms - d->opened_ms > INVITE_TIMEOUT_MS) {
            emit(l, WF_LE_DUEL_DECLINED, d->peer, d->peer_who.name, 0, 0, 1);
            memset(d, 0, sizeof *d);
            return;
        }
        d->last_send_ms = now_ms;
        send_pkt(l, d->peer, &d->invite);
        break;
    case WF_DUEL_INVITED:
        if (now_ms - d->last_heard_ms > INVITE_TIMEOUT_MS) memset(d, 0, sizeof *d);
        break;
    case WF_DUEL_DONE:
        /* Keep repeating the final move for a while in case it was lost. */
        if (now_ms - d->done_ms < 4000u) {
            d->last_send_ms = now_ms;
            send_turn(l, (uint16_t)(d->round - 1), d->prev_act, d->prev_q);
        }
        break;
    case WF_DUEL_ACTIVE:
        if (now_ms - d->last_heard_ms > DUEL_SILENCE_MS) {
            emit(l, WF_LE_DUEL_LEFT, d->peer, d->peer_who.name, 0, 0, 1);
            memset(d, 0, sizeof *d);
            return;
        }
        d->last_send_ms = now_ms;
        if (d->role == 1 && !d->started) {
            wf_packet acc = {0};
            acc.type = WF_PKT_DUEL_ACCEPT;
            acc.session = d->session;
            fill_snapshot(&acc, g, now_s);
            acc.seed = d->my_seed;
            send_pkt(l, d->peer, &acc);
        }
        if (d->my_ready) send_turn(l, d->round, d->my_act, d->my_q);
        break;
    default:
        break;
    }
}

/* ---- Actions ---------------------------------------------------------- */
bool wf_link_greet(wf_link *l, wf_game *g, const uint8_t peer[6], uint32_t now_ms, uint32_t now_s) {
    if (l->out.active) return false;
    memset(&l->out, 0, sizeof l->out);
    l->out.active = 1;
    memcpy(l->out.peer, peer, 6);
    l->out.pkt.type = WF_PKT_GREET;
    l->out.pkt.session = wf_rng_next(&l->rng);
    wf_profile_from_game(&l->out.pkt.who, g, now_s);
    l->out.last_ms = now_ms;
    l->out.tries = 1;
    send_pkt(l, peer, &l->out.pkt);
    return true;
}

bool wf_link_gift(wf_link *l, wf_game *g, const uint8_t peer[6], int item, uint32_t now_ms) {
    if (l->out.active || item < 0 || item >= WF_IT_COUNT || !g->inv[item]) return false;
    g->inv[item]--;
    memset(&l->out, 0, sizeof l->out);
    l->out.active = 1;
    memcpy(l->out.peer, peer, 6);
    l->out.pkt.type = WF_PKT_GIFT;
    l->out.pkt.session = wf_rng_next(&l->rng);
    wf_profile_from_game(&l->out.pkt.who, g, 0);
    l->out.pkt.item = (uint8_t)item;
    l->out.pkt.count = 1;
    l->out.last_ms = now_ms;
    l->out.tries = 1;
    send_pkt(l, peer, &l->out.pkt);
    return true;
}

bool wf_link_duel_invite(wf_link *l, wf_game *g, const uint8_t peer[6], uint32_t now_ms, uint32_t now_s) {
    wf_duel *d = &l->duel;
    if (d->state != WF_DUEL_IDLE || !wf_pet_can_act(g, now_s)) return false;
    memset(d, 0, sizeof *d);
    d->state = WF_DUEL_INVITING;
    d->role = 0;
    memcpy(d->peer, peer, 6);
    for (int i = 0; i < WF_PEERS; i++)
        if (l->peers[i].used && !memcmp(l->peers[i].id, peer, 6)) d->peer_who = l->peers[i].who;
    d->session = wf_rng_next(&l->rng) | 1u;
    d->my_seed = wf_rng_next(&l->rng);
    d->invite.type = WF_PKT_DUEL_INVITE;
    d->invite.session = d->session;
    d->invite.seed = d->my_seed;
    fill_snapshot(&d->invite, g, now_s);
    d->opened_ms = d->last_heard_ms = d->last_send_ms = now_ms;
    send_pkt(l, peer, &d->invite);
    return true;
}

bool wf_link_duel_respond(wf_link *l, wf_game *g, bool accept, uint32_t now_ms, uint32_t now_s) {
    wf_duel *d = &l->duel;
    if (d->state != WF_DUEL_INVITED) return false;
    if (!accept) {
        wf_packet no = {0};
        no.type = WF_PKT_DUEL_DECLINE;
        no.session = d->session;
        send_pkt(l, d->peer, &no);
        memset(d, 0, sizeof *d);
        return true;
    }
    d->my_seed = wf_rng_next(&l->rng);
    wf_packet acc = {0};
    acc.type = WF_PKT_DUEL_ACCEPT;
    acc.session = d->session;
    fill_snapshot(&acc, g, now_s);
    acc.seed = d->my_seed;
    duel_begin(d, &d->invite, &acc);
    d->last_heard_ms = d->last_send_ms = now_ms;
    send_pkt(l, d->peer, &acc);
    return true;
}

bool wf_link_duel_submit(wf_link *l, int action, int quality, uint32_t now_ms) {
    wf_duel *d = &l->duel;
    if (d->state != WF_DUEL_ACTIVE || d->my_ready) return false;
    d->my_ready = 1;
    d->my_act = (uint8_t)action;
    d->my_q = (uint8_t)(quality < 0 ? 0 : quality > 3 ? 3 : quality);
    d->last_send_ms = now_ms;
    send_turn(l, d->round, d->my_act, d->my_q);
    try_resolve(l);
    return true;
}

void wf_link_duel_leave(wf_link *l, uint32_t now_ms) {
    (void)now_ms;
    wf_duel *d = &l->duel;
    if (d->state == WF_DUEL_IDLE) return;
    if (d->state == WF_DUEL_DONE) {     /* nothing to tell: both sides saw the end */
        memset(d, 0, sizeof *d);
        return;
    }
    wf_packet bye = {0};
    bye.type = WF_PKT_DUEL_LEAVE;
    bye.session = d->session;
    for (int i = 0; i < 3; i++) send_pkt(l, d->peer, &bye);
    memset(d, 0, sizeof *d);
}

void wf_link_duel_record(wf_link *l, wf_game *g, uint32_t now_s) {
    wf_duel *d = &l->duel;
    if (d->state != WF_DUEL_DONE || d->recorded) return;
    d->recorded = 1;
    int me = d->role;
    bool won = (d->battle.state == WF_B_WON) == (me == 0);
    if (won) g->duel_wins++; else g->duel_losses++;
    wf_friend *f = wf_friend_upsert(g, d->peer, d->peer_who.name, d->peer_who.form, d->peer_who.level);
    if (f->duels < 0xffff) f->duels++;
    /* Friendly duels: a little XP for both, a bit more for the winner. */
    wf_gain_xp(g, won ? 12u + d->peer_who.level * 2u : 6u);
    g->coins += won ? 10u : 3u;
    (void)now_s;
}
