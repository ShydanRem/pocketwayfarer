#include <stdlib.h>
#include <string.h>
#include "lantern.h"
#include "rng.h"
#include "timing.h"

enum {
    ROOM_GOLD = 10, GUARDIAN_GOLD = 40, BASTION_GOLD = 25,
    BASTION_COST = 12, BASTION_CRAB_HP = 88,
    MAX_RUN_GOLD = LN_BASTION_ROOM * ROOM_GOLD + BASTION_GOLD + GUARDIAN_GOLD
};

static int mini(int a, int b) { return a < b ? a : b; }
static int maxi(int a, int b) { return a > b ? a : b; }
static bool has(const rpg_lantern *l, int r) { return (l->relics & (1u << r)) != 0; }
static void stage(rpg_lantern *l, int s) { l->state = (uint8_t)s; l->elapsed_ms = 0; }
static int roll(rpg_lantern *l, int a, int b) {
    wf_rng r = {l->rng};
    int v = wf_rng_range(&r, a, b);
    l->rng = r.s;
    return v;
}
static void heal(rpg_lantern *l, int hp, int sp) {
    l->hp = (int16_t)mini(l->max_hp, l->hp + hp);
    l->sp = (int16_t)mini(l->max_sp, l->sp + sp);
}

wf_text ln_archetype_name(int a) {
    static const wf_text t[] = {{"Sentinella", "Sentinel"}, {"Piromante", "Ember mage"}, {"Predone", "Trickster"}};
    return t[(unsigned)a < LN_ARCHETYPES ? a : 0];
}
wf_text ln_skill_name(int a, int s) {
    static const wf_text t[3][2] = {
        {{"Fendente", "Cleave"}, {"Baluardo", "Bulwark"}},
        {{"Brace", "Ember"}, {"Incendio", "Ignite"}},
        {{"Doppio colpo", "Twin strike"}, {"Preparazione", "Preparation"}}
    };
    return t[(unsigned)a < 3 ? a : 0][s == 1];
}
wf_text ln_relic_name(int r) {
    static const wf_text t[] = {
        {"Cuore di Lanterna", "Lantern heart"}, {"Ago di Rovo", "Thorn needle"},
        {"Vetro del Porto", "Harbor glass"}, {"Campana del Borgo", "Village bell"},
        {"Rosa di Brace", "Ember rose"}, {"Bussola del Faro", "Beacon compass"}
    };
    return t[(unsigned)r < LR_COUNT ? r : 0];
}
wf_text ln_relic_text(int r) {
    static const wf_text t[] = {
        {"Parata perfetta: +3 PM.", "Perfect parry: +3 SP."},
        {"Contro un nemico ferito: +4 danni.", "Wounded enemies take +4 damage."},
        {"Guardia prepara +8 danni al prossimo colpo.", "Guard adds +8 damage to your next hit."},
        {"Ogni incontro inizia con 12 scudo.", "Start every encounter with 12 shield."},
        {"Ogni colpo brucia: 2 danni per 2 turni.", "Each hit burns: 2 damage for 2 turns."},
        {"Proseguire dopo una stanza cura 6 PV.", "Continue after a room to heal 6 HP."}
    };
    return t[(unsigned)r < LR_COUNT ? r : 0];
}
wf_text ln_enemy_name(int k) {
    static const wf_text t[] = {{"Custode di Rovo", "Root keeper"}, {"Fiammella", "Ember wisp"}, {"Guardiano del Faro", "Beacon guardian"},
        {"Lupo di Muschio", "Moss wolf"}, {"Granchio di Marea", "Tide crab"}, {"Pipistrello dei Lumi", "Lantern bat"}, {"Falena di Giada", "Jade moth"}};
    return t[(unsigned)k < LN_ENEMIES ? k : 0];
}
wf_text ln_intent_name(int k) {
    static const wf_text t[] = {{"COLPO", "STRIKE"}, {"COLPO PESANTE", "HEAVY STRIKE"}, {"DOPPIO COLPO", "DOUBLE STRIKE"}, {"CURA ALLEATI", "ALLY HEAL"}};
    return t[(unsigned)k <= LN_MEND ? k : 0];
}
bool ln_active(const rpg_lantern *l) { return l->state >= LN_ROUTE && l->state <= LN_BLUEPRINT; }

void ln_start(rpg_lantern *l, int archetype, uint32_t seed) {
    uint16_t wins = l->wins;
    uint8_t blueprints = l->blueprints, assist = l->assist, motion = l->reduced_motion, equipped = l->equipped, seen = l->seen;
    memset(l, 0, sizeof *l);
    l->wins = wins; l->blueprints = blueprints; l->assist = assist; l->reduced_motion = motion;
    l->equipped = equipped; l->seen = seen;
    if (equipped && equipped <= LR_COUNT && (blueprints & (1u << (equipped - 1)))) l->relics = (uint8_t)(1u << (equipped - 1));
    l->archetype = (uint8_t)((unsigned)archetype < 3 ? archetype : 0);
    static const int hp[] = {120, 96, 104}, sp[] = {24, 36, 28};
    l->hp = l->max_hp = (int16_t)hp[l->archetype];
    l->sp = l->max_sp = (int16_t)sp[l->archetype];
    wf_rng r;
    wf_rng_seed(&r, seed);
    l->rng = r.s;
    stage(l, LN_ROUTE);
}
void ln_home(rpg_lantern *l) { if (!ln_active(l)) stage(l, LN_NONE); }

static void intents(rpg_lantern *l) {
    for (int i = 0; i < l->nfoes; i++) {
        ln_foe *f = &l->foes[i];
        f->intent = (uint8_t)(f->kind == LN_BOSS ? (l->turn % 2 ? LN_DOUBLE : LN_HEAVY)
                            : f->kind == LN_MOTH && l->turn % 3 == 1 ? LN_MEND
                            : (f->kind == LN_WOLF || f->kind == LN_BAT) && l->turn % 2 ? LN_DOUBLE
                            : f->kind == LN_CRAB || l->turn % 3 == 2 ? LN_HEAVY : LN_HIT);
    }
}
static bool route_available(const rpg_lantern *l, int branch) {
    return l->state == LN_ROUTE && ((unsigned)branch <= LN_PATH_PIER ||
                                    (branch == LN_PATH_BASTION && l->room == LN_BASTION_ROOM));
}

static unsigned room_gold(int room, int branch) {
    if (room == 4) return GUARDIAN_GOLD;
    return room == LN_BASTION_ROOM && branch == LN_PATH_BASTION ? BASTION_GOLD : ROOM_GOLD;
}

static int route_cost(int branch) { return branch == LN_PATH_BASTION ? BASTION_COST : 0; }

static void prepare_foes(rpg_lantern *l, int branch) {
    memset(l->foes, 0, sizeof l->foes);
    if (branch == LN_PATH_BASTION) {
        l->nfoes = 1;
        l->foes[0] = (ln_foe){.kind = LN_CRAB, .hp = BASTION_CRAB_HP, .max_hp = BASTION_CRAB_HP};
        return;
    }
    bool pier = branch == LN_PATH_PIER;
    l->nfoes = (uint8_t)(pier && l->room != 4 ? 2 : 1);
    for (int i = 0; i < l->nfoes; i++) {
        ln_foe *f = &l->foes[i];
        f->kind = (uint8_t)(l->room == 4 ? LN_BOSS : pier ? LN_WISP : LN_ROOT);
        if (l->room >= 2 && l->room < 4) {
            static const uint8_t solo[] = {LN_ROOT, LN_WOLF, LN_CRAB};
            static const uint8_t pairs[] = {LN_WISP, LN_BAT, LN_MOTH};
            f->kind = (pier ? pairs : solo)[roll(l, 0, 2)];
        }
        f->hp = f->max_hp = (int16_t)(l->room == 4 ? 176 : pier ? 20 + l->room * 4 : 36 + l->room * 10);
        if (f->kind == LN_BAT) f->hp = f->max_hp = (int16_t)(f->hp * 4 / 5);
        if (f->kind == LN_CRAB) f->hp = f->max_hp = (int16_t)(f->hp * 11 / 10);
    }
}

bool ln_preview_route(const rpg_lantern *l, int branch, ln_route_info *out) {
    if (!route_available(l, branch) || l->room == 1) return false;
    rpg_lantern preview = *l;
    prepare_foes(&preview, branch);
    memcpy(out->foes, preview.foes, sizeof out->foes);
    out->count = preview.nfoes;
    out->hp_cost = (uint8_t)route_cost(branch);
    out->gold = (uint8_t)room_gold(l->room, branch);
    return true;
}

bool ln_enter(rpg_lantern *l, int branch) {
    if (!route_available(l, branch)) return false;
    int cost = route_cost(branch);
    if (cost && l->hp <= cost) return false;
    l->hp = (int16_t)(l->hp - cost);
    l->branch = (uint8_t)branch;
    if (l->room == 1) { stage(l, LN_EVENT); return true; }
    l->turn = l->acting = l->target = l->guard = l->focused = 0;
    prepare_foes(l, branch);
    for (int i = 0; i < l->nfoes; i++) l->seen |= (uint8_t)(1u << l->foes[i].kind);
    l->shield = (uint8_t)(has(l, LR_BELL) ? 12 : 0);
    intents(l);
    stage(l, LN_CHOOSE);
    return true;
}
static int target(const rpg_lantern *l, int t) {
    if ((unsigned)t < l->nfoes && l->foes[t].hp > 0) return t;
    for (int i = 0; i < l->nfoes; i++) if (l->foes[i].hp > 0) return i;
    return -1;
}
static void impact(rpg_lantern *l, int next) { l->next_state = (uint8_t)next; stage(l, LN_IMPACT); }
static void victory(rpg_lantern *l) {
    l->currency = (uint16_t)(l->currency + room_gold(l->room, l->branch));
    if (l->room == 4) {
        if (l->relics) impact(l, LN_BLUEPRINT);
        else {
            /* Declining every relic still permits a complete, bankable win. */
            if (l->wins < UINT16_MAX) l->wins++;
            impact(l, LN_WON);
        }
    } else {
        if (ln_relic_count(l) > 3) { impact(l, LN_CLEARED); return; }
        for (int i = 0; i < 2; i++) {
            int r;
            do { r = roll(l, 0, LR_COUNT - 1); } while (has(l, r) || (i && r == l->offers[0]));
            l->offers[i] = (uint8_t)r;
        }
        impact(l, LN_RELIC);
    }
}
static void hit(rpg_lantern *l, int t, int damage) {
    ln_foe *f = &l->foes[t];
    if (f->kind == LN_CRAB && l->action == LN_BASIC) damage = damage * 4 / 5;
    if (has(l, LR_THORN) && f->hp < f->max_hp) damage += 4;
    f->hp = (int16_t)maxi(0, f->hp - damage);
    l->last_damage = (int16_t)damage;
    if (has(l, LR_COAL) && f->hp) { f->burn = (uint8_t)maxi(f->burn, 2); f->burn_turns = 2; }
}
static void hero_hit(rpg_lantern *l, int q) {
    static const int mult[] = {40, 100, 125, 150};
    int t = target(l, l->target);
    l->quality = (uint8_t)q;
    l->feedback = LN_FEEDBACK_HERO;
    l->effect_actor = (uint8_t)(t >= 0 ? t : 0);
    l->effect_flags = (uint8_t)(l->action == LN_POWER && l->resonance == 3 ? LN_EFFECT_RESONANCE : 0);
    int hp_before = t >= 0 ? l->foes[t].hp : 0;
    int base = l->archetype == LN_SENTINEL ? 19 : 20;
    int damage = (base + roll(l, -2, 2)) * mult[q] / 100;
    if (l->action == LN_POWER) damage = damage * 3 / 2;
    damage += l->focused;
    l->focused = 0;
    if (t >= 0) hit(l, t, damage);
    if (l->action == LN_POWER && l->resonance == 3) {
        l->resonance = 0;
        if (l->archetype == LN_SENTINEL) l->shield = (uint8_t)mini(60, l->shield + l->max_hp / 5);
        else if (l->archetype == LN_MAGE && t >= 0 && l->foes[t].hp) {
            l->foes[t].burn = (uint8_t)(l->foes[t].kind == LN_BOSS ? 3 : 6);
            l->foes[t].burn_turns = 2;
        } else if (l->archetype == LN_ROGUE && t >= 0 && l->foes[t].hp) hit(l, t, base * 70 / 100);
    }
    l->last_damage = (int16_t)(t >= 0 ? hp_before - l->foes[t].hp : 0);
    l->acting = 0;
    if (target(l, -1) < 0) victory(l);
    else impact(l, LN_TELEGRAPH);
}
bool ln_action(rpg_lantern *l, int action, int t) {
    if (l->state != LN_CHOOSE || l->paused || (unsigned)action > LN_GUARD) return false;
    int cost = action == LN_POWER ? 6 : action == LN_SUPPORT ? 5 : 0;
    if (l->sp < cost) return false;
    t = target(l, t);
    if (t < 0) return false;
    l->target = (uint8_t)t;
    l->action = (uint8_t)action;
    l->sp = (int16_t)(l->sp - cost);
    l->guard = 0;
    if (action == LN_BASIC || action == LN_POWER) stage(l, LN_ATTACK);
    else {
        l->acting = 0;
        if (action == LN_GUARD) {
            l->guard = 1;
            heal(l, 0, 4);
            if (has(l, LR_GLASS)) l->focused = (uint8_t)mini(32, l->focused + 8);
        } else if (l->archetype == LN_SENTINEL) { l->shield = (uint8_t)mini(60, l->shield + 30); heal(l, 6, 0); }
        else if (l->archetype == LN_MAGE) {
            for (int i = 0; i < l->nfoes; i++) { l->foes[i].burn = 5; l->foes[i].burn_turns = 2; }
        } else { l->focused = (uint8_t)mini(32, l->focused + 12); l->shield = (uint8_t)mini(60, l->shield + 12); }
        stage(l, LN_TELEGRAPH);
    }
    return true;
}
int ln_quality(const rpg_lantern *l) {
    int d = l->state == LN_ATTACK ? abs((int)(l->elapsed_ms % 1400) - 700) : abs((int)l->elapsed_ms - 1000);
    int assist = l->assist ? 150 : 100;
    if (d <= 45 * assist / 100) return WF_Q_PERFECT;
    if (d <= 70 * assist / 100) return WF_Q_GREAT;
    if (d <= 110 * assist / 100) return WF_Q_GOOD;
    return WF_Q_MISS;
}
static void foe_hit(rpg_lantern *l, int q) {
    ln_foe *f = &l->foes[l->acting];
    l->quality = (uint8_t)q;
    l->feedback = LN_FEEDBACK_FOE;
    l->effect_actor = l->acting;
    l->effect_flags = 0;
    l->last_damage = 0;
    if (f->hp > 0 && f->burn_turns) {
        int before = f->hp;
        f->hp = (int16_t)maxi(0, f->hp - f->burn);
        f->burn_turns--;
        l->effect_flags |= LN_EFFECT_BURN;
        if (!f->hp) { l->feedback = LN_FEEDBACK_BURN; l->last_damage = (int16_t)before; }
    }
    if (target(l, -1) < 0) { victory(l); return; }
    if (f->hp > 0 && f->intent == LN_MEND) {
        int who = l->acting;
        for (int i = 0; i < l->nfoes; i++) if (l->foes[i].hp > 0 && l->foes[i].hp < l->foes[who].hp) who = i;
        l->foes[who].hp = (int16_t)mini(l->foes[who].max_hp, l->foes[who].hp + 8);
        l->feedback = LN_FEEDBACK_HEAL;
        l->effect_actor = (uint8_t)who;
    } else if (f->hp > 0) {
        int base = f->kind == LN_BOSS ? 13 : (f->kind == LN_WISP || f->kind == LN_BAT || f->kind == LN_MOTH) ? 5 + l->room : 7 + l->room;
        int hits = f->intent == LN_DOUBLE ? 2 : 1;
        int power = f->intent == LN_HEAVY ? 150 : f->intent == LN_DOUBLE ? 75 : 100;
        static const int taken[] = {100, 70, 45, 20};
        int d = maxi(1, (base + roll(l, -1, 1)) * power / 100 * taken[q] / 100);
        if (l->guard) d = maxi(1, d / 2);
        d *= hits;
        int absorbed = mini(d, l->shield);
        l->shield = (uint8_t)(l->shield - absorbed);
        d -= absorbed;
        l->hp = (int16_t)maxi(0, l->hp - d);
        l->last_damage = (int16_t)d;
        if (q == WF_Q_PERFECT) {
            l->resonance = (uint8_t)mini(3, l->resonance + 1);
            if (has(l, LR_HEART)) heal(l, 0, 3);
        }
    }
    if (l->hp <= 0) { impact(l, LN_LOST); return; }
    do { l->acting++; } while (l->acting < l->nfoes && l->foes[l->acting].hp <= 0);
    if (l->acting >= l->nfoes) {
        l->turn++;
        intents(l);
        heal(l, 0, 1);
        impact(l, LN_CHOOSE);
    } else impact(l, LN_TELEGRAPH);
}
bool ln_tap(rpg_lantern *l) {
    if (l->paused) return false;
    if (l->state == LN_ATTACK) { hero_hit(l, ln_quality(l)); return true; }
    if (l->state == LN_PARRY) { foe_hit(l, ln_quality(l)); return true; }
    return false;
}
bool ln_tick(rpg_lantern *l, unsigned ms) {
    if (l->paused) return false;
    if (l->state != LN_ATTACK && l->state != LN_TELEGRAPH && l->state != LN_PARRY && l->state != LN_IMPACT) return false;
    l->elapsed_ms = (uint16_t)mini(60000, l->elapsed_ms + (int)(ms > 60000 ? 60000 : ms));
    switch (l->state) {
    case LN_ATTACK: if (l->elapsed_ms >= 2800) { hero_hit(l, WF_Q_MISS); return true; } break;
    case LN_TELEGRAPH: if (l->elapsed_ms >= 650) {
        if (l->foes[l->acting].intent == LN_MEND) foe_hit(l, WF_Q_MISS); else stage(l, LN_PARRY);
        return true;
    } break;
    case LN_PARRY: if (l->elapsed_ms >= 1300) { foe_hit(l, WF_Q_MISS); return true; } break;
    case LN_IMPACT: if (l->elapsed_ms >= 420) { stage(l, l->next_state); return true; } break;
    default: break;
    }
    return false;
}
int ln_relic_count(const rpg_lantern *l) {
    int count = 0;
    for (int r = 0; r < LR_COUNT; r++) count += has(l, r);
    return count;
}
static bool relic_offer(const rpg_lantern *l, int choice) {
    return l->state == LN_RELIC && (unsigned)choice < 2 &&
           l->offers[choice] < LR_COUNT && !has(l, l->offers[choice]);
}
bool ln_choose_relic(rpg_lantern *l, int choice) {
    if (!relic_offer(l, choice) || ln_relic_count(l) >= 3) return false;
    l->relics |= (uint8_t)(1u << l->offers[choice]);
    stage(l, LN_CLEARED);
    return true;
}
bool ln_replace_relic(rpg_lantern *l, int choice, int outgoing) {
    if (!relic_offer(l, choice) || ln_relic_count(l) != 3 ||
        (unsigned)outgoing >= LR_COUNT || !has(l, outgoing)) return false;
    l->relics = (uint8_t)((l->relics & ~(1u << outgoing)) | (1u << l->offers[choice]));
    stage(l, LN_CLEARED);
    return true;
}
bool ln_skip_relic(rpg_lantern *l) {
    if (l->state != LN_RELIC) return false;
    stage(l, LN_CLEARED);
    return true;
}
bool ln_keep_blueprint(rpg_lantern *l, int r) {
    if (l->state != LN_BLUEPRINT || (unsigned)r >= LR_COUNT || !has(l, r)) return false;
    l->blueprints |= (uint8_t)(1u << r);
    if (l->wins < UINT16_MAX) l->wins++;
    stage(l, LN_WON);
    return true;
}
bool ln_event(rpg_lantern *l, int choice) {
    if (l->state != LN_EVENT || (unsigned)choice > 1) return false;
    if (l->branch && !choice) {
        if (l->hp <= 12) return false;
        l->hp -= 12; l->resonance = 3; heal(l, 0, 8);
    } else heal(l, choice ? 0 : 25, choice ? 12 : 0);
    l->currency += ROOM_GOLD;
    stage(l, LN_CLEARED);
    return true;
}
bool ln_continue(rpg_lantern *l) {
    if (l->state != LN_CLEARED || l->room >= 4) return false;
    if (has(l, LR_COMPASS)) heal(l, 6, 0);
    l->room++;
    stage(l, LN_ROUTE);
    return true;
}
bool ln_retreat(rpg_lantern *l) {
    if (l->state != LN_CLEARED && l->state != LN_ROUTE) return false;
    stage(l, LN_RETURNED);
    return true;
}
unsigned ln_bank(rpg_lantern *l) {
    if (l->state != LN_WON && l->state != LN_LOST && l->state != LN_RETURNED) return 0;
    if (l->settled) return 0;
    unsigned amount = l->state == LN_LOST ? l->currency - l->currency / 4u : l->currency;
    l->settled = (uint16_t)(amount + 1u); /* marker also works for a zero reward */
    return amount;
}
void ln_sanitize(rpg_lantern *l) {
    bool invalid = l->state >= LN_STATE_COUNT || l->archetype >= 3 || l->room > 4 || l->nfoes > 2;
    if (ln_relic_count(l) > 3) invalid = true;
    if (l->state == LN_BLUEPRINT && !(l->relics & ((1u << LR_COUNT) - 1u))) invalid = true;
    if (ln_active(l) && (l->max_hp < 1 || l->max_hp > 120 || l->hp < 0 || l->hp > l->max_hp || l->max_sp > 36 || l->sp < 0 || l->sp > l->max_sp)) invalid = true;
    if (l->state >= LN_CHOOSE && l->state <= LN_IMPACT && (!l->nfoes || l->target >= l->nfoes)) invalid = true;
    if ((l->state == LN_PARRY || l->state == LN_TELEGRAPH) && l->acting >= l->nfoes) invalid = true;
    if (l->state == LN_IMPACT && l->next_state != LN_CHOOSE && l->next_state != LN_TELEGRAPH && l->next_state != LN_RELIC && l->next_state != LN_CLEARED && l->next_state != LN_BLUEPRINT && l->next_state != LN_WON && l->next_state != LN_LOST) invalid = true;
    for (int i = 0; i < l->nfoes && i < 2; i++) if (l->foes[i].kind >= LN_ENEMIES || l->foes[i].intent > LN_MEND || l->foes[i].max_hp < 1 || l->foes[i].hp > l->foes[i].max_hp || l->foes[i].hp < 0) invalid = true;
    if (invalid) { ln_home(l); l->state = LN_NONE; }
    l->assist = !!l->assist; l->reduced_motion = !!l->reduced_motion;
    l->blueprints &= (1u << LR_COUNT) - 1u;
    if (l->equipped > LR_COUNT || (l->equipped && !(l->blueprints & (1u << (l->equipped - 1))))) l->equipped = 0;
    l->seen &= (1u << LN_ENEMIES) - 1u;
    if (l->effect_actor >= 2) l->effect_actor = 0;
    l->effect_flags &= LN_EFFECT_RESONANCE | LN_EFFECT_BURN;
    l->relics &= (1u << LR_COUNT) - 1u;
    l->resonance = (uint8_t)mini(l->resonance, 3);
    l->shield = (uint8_t)mini(l->shield, 60);
    l->focused = (uint8_t)mini(l->focused, 32);
    l->elapsed_ms = (uint16_t)mini(l->elapsed_ms, 2800);
    l->currency = (uint16_t)mini(l->currency, MAX_RUN_GOLD);
    for (int i = 0; i < 2; i++) l->offers[i] %= LR_COUNT;
    if (ln_active(l) && !l->rng) l->rng = 0x51eedu;
}
