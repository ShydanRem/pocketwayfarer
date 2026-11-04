#pragma once
/* Device-to-device play ("Legami") over a connectionless radio.
 *
 * The firmware moves packets with ESP-NOW (no router, ~30 m range); the host
 * tests move them through a lossy in-memory pipe. Everything here is
 * transport-agnostic: packets may be lost, duplicated or reordered, so every
 * exchange is retried and idempotent.
 *
 *  - Beacon:  who is nearby (name, form, level) every 1.5 s while the radio is on
 *  - Greet:   both companions get a mood/bond boost once per friend per day
 *  - Gift:    send one item from the bag
 *  - Duel:    lock-step battle; both devices resolve each round with the same
 *             seed and the same pair of actions, so they always agree. */
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "game.h"
#include "battle.h"

#define WF_LINK_VERSION 1
#define WF_PACKET_BYTES 52
#define WF_PEERS 8
#define WF_LINK_EVENTS 8

typedef enum {
    WF_PKT_BEACON = 1,
    WF_PKT_GREET, WF_PKT_GREET_ACK,
    WF_PKT_GIFT, WF_PKT_GIFT_ACK,
    WF_PKT_DUEL_INVITE, WF_PKT_DUEL_ACCEPT, WF_PKT_DUEL_DECLINE, WF_PKT_DUEL_START,
    WF_PKT_DUEL_TURN, WF_PKT_DUEL_LEAVE,
    WF_PKT_TYPES
} wf_packet_type;

typedef struct {
    char name[WF_NAME_LEN + 1];
    uint8_t form, level, element, flags;
    uint16_t wins;
} wf_profile;

enum { WF_PROFILE_BUSY = 1, WF_PROFILE_ASLEEP = 2, WF_PROFILE_AWAY = 4 };

typedef struct {
    uint8_t type;
    uint32_t session;
    wf_profile who;
    int16_t hp, atk, def, spd;   /* duel snapshot */
    uint32_t seed;
    uint8_t item, count;
    uint16_t round;
    uint8_t action, quality;
} wf_packet;

size_t wf_packet_encode(const wf_packet *p, uint8_t *out, size_t cap);
bool wf_packet_decode(const uint8_t *in, size_t len, wf_packet *p);
void wf_profile_from_game(wf_profile *out, const wf_game *g, uint32_t now);

typedef struct {
    uint8_t used;
    uint8_t id[6];
    wf_profile who;
    uint32_t seen_ms;
} wf_peer;

typedef enum {
    WF_LE_GREETED = 1,      /* a friend waved at us (value = 1 if it counted today) */
    WF_LE_GREET_DONE,       /* our wave arrived (value = 1 if it counted today) */
    WF_LE_GIFT_IN,          /* item received: item, count */
    WF_LE_GIFT_DONE,
    WF_LE_SEND_FAILED,      /* greet/gift never acknowledged (gift refunded) */
    WF_LE_DUEL_INVITED,
    WF_LE_DUEL_DECLINED,
    WF_LE_DUEL_STARTED,
    WF_LE_DUEL_ROUND,       /* a round resolved: read link->duel.battle.ev */
    WF_LE_DUEL_LEFT         /* opponent left or connection lost */
} wf_link_event_kind;

typedef struct {
    uint8_t kind;
    uint8_t id[6];
    char name[WF_NAME_LEN + 1];
    uint8_t item, count, value;
} wf_link_event;

typedef enum {
    WF_DUEL_IDLE = 0,
    WF_DUEL_INVITING,
    WF_DUEL_INVITED,
    WF_DUEL_ACTIVE,
    WF_DUEL_DONE
} wf_duel_state;

typedef struct {
    uint8_t state;
    uint8_t role;               /* 0 = inviter (side A), 1 = invited (side B) */
    uint8_t started;            /* B: START received */
    uint8_t peer[6];
    wf_profile peer_who;
    uint32_t session;
    uint32_t my_seed, peer_seed;
    wf_packet invite;           /* A's snapshot as received / sent */
    wf_battle battle;
    uint16_t round;
    uint8_t my_ready, my_act, my_q;
    uint8_t peer_ready, peer_act, peer_q;
    uint8_t prev_act, prev_q;   /* our move for round-1, for late resends */
    uint8_t recorded;
    uint32_t last_send_ms, last_heard_ms, opened_ms, done_ms;
} wf_duel;

typedef struct {
    uint8_t active;
    uint8_t peer[6];
    wf_packet pkt;
    uint32_t last_ms;
    uint8_t tries;
} wf_outbox;

typedef void (*wf_link_send_fn)(void *ctx, const uint8_t *peer /* NULL = broadcast */,
                                const uint8_t *data, size_t len);

typedef struct {
    wf_link_send_fn send;
    void *ctx;
    uint8_t beacon_on;
    uint32_t last_beacon_ms;
    wf_peer peers[WF_PEERS];
    wf_outbox out;
    wf_duel duel;
    uint32_t seen[16];          /* recent (peer, session) hashes for dedupe */
    uint8_t seen_next;
    wf_link_event events[WF_LINK_EVENTS];
    uint8_t ev_head, ev_count;
    wf_rng rng;
} wf_link;

void wf_link_init(wf_link *l, wf_link_send_fn send, void *ctx, uint32_t seed);
void wf_link_receive(wf_link *l, wf_game *g, const uint8_t from[6], const uint8_t *data, size_t len,
                     uint32_t now_ms, uint32_t now_s);
void wf_link_poll(wf_link *l, wf_game *g, uint32_t now_ms, uint32_t now_s);
bool wf_link_next_event(wf_link *l, wf_link_event *out);
int wf_link_peer_count(const wf_link *l);
const wf_peer *wf_link_peer(const wf_link *l, int index);

bool wf_link_greet(wf_link *l, wf_game *g, const uint8_t peer[6], uint32_t now_ms, uint32_t now_s);
bool wf_link_gift(wf_link *l, wf_game *g, const uint8_t peer[6], int item, uint32_t now_ms);

bool wf_link_duel_invite(wf_link *l, wf_game *g, const uint8_t peer[6], uint32_t now_ms, uint32_t now_s);
bool wf_link_duel_respond(wf_link *l, wf_game *g, bool accept, uint32_t now_ms, uint32_t now_s);
bool wf_link_duel_submit(wf_link *l, int action, int quality, uint32_t now_ms);
void wf_link_duel_leave(wf_link *l, uint32_t now_ms);
/* Which side of duel.battle belongs to this device. */
int wf_link_duel_me(const wf_link *l);
/* Records the outcome in the save (wins, friend stats, small rewards). */
void wf_link_duel_record(wf_link *l, wf_game *g, uint32_t now_s);
