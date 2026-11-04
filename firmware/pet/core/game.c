#include "game.h"
#include <stdio.h>
#include <string.h>

/* ---- Tables ----------------------------------------------------------- */
static const wf_form_info forms[WF_FORM_COUNT] = {
    [WF_FORM_SEED]      = {"Seme d'Astro", "Tiepido e luminoso. Scaldalo con le dita.",
                           WF_STAGE_SEED, WF_EL_NONE, 1, 1, 1, 1, 0, 0, 0, 0},
    [WF_FORM_LUMINO]    = {"Lumino", "Curioso di tutto, specialmente del cibo.",
                           WF_STAGE_BABY, WF_EL_NONE, 40, 12, 8, 10, 70, 25, 22, 20},
    [WF_FORM_FAVILLO]   = {"Favillo", "Scoppietta quando è felice.",
                           WF_STAGE_TEEN, WF_EL_EMBER, 46, 16, 9, 12, 75, 30, 21, 22},
    [WF_FORM_SPRUZZO]   = {"Spruzzo", "Lascia piccole pozzanghere ovunque.",
                           WF_STAGE_TEEN, WF_EL_TIDE, 50, 13, 11, 11, 82, 25, 25, 20},
    [WF_FORM_FOGLIOLO]  = {"Fogliolo", "Fa la fotosintesi durante i pisolini.",
                           WF_STAGE_TEEN, WF_EL_GROVE, 54, 13, 12, 9, 88, 24, 26, 18},
    [WF_FORM_PIROLUME]  = {"Pirolume", "Una fiamma gentile che scalda gli amici.",
                           WF_STAGE_ADULT, WF_EL_EMBER, 60, 22, 13, 16, 82, 34, 24, 25},
    [WF_FORM_CENEROTTO] = {"Cenerotto", "Brontola fumo, ma colpisce fortissimo.",
                           WF_STAGE_ADULT, WF_EL_EMBER, 56, 25, 11, 15, 76, 38, 20, 24},
    [WF_FORM_CORALBA]   = {"Coralba", "Canta con la voce delle onde calme.",
                           WF_STAGE_ADULT, WF_EL_TIDE, 66, 19, 16, 14, 90, 29, 28, 22},
    [WF_FORM_ABISSINO]  = {"Abissino", "Viene dal blu profondo. Non ama il sole.",
                           WF_STAGE_ADULT, WF_EL_TIDE, 62, 22, 14, 16, 84, 33, 25, 25},
    [WF_FORM_SILVANTE]  = {"Silvante", "Dove cammina sbocciano fiori.",
                           WF_STAGE_ADULT, WF_EL_GROVE, 72, 18, 18, 12, 98, 27, 31, 19},
    [WF_FORM_ROVACCIO]  = {"Rovaccio", "Spinoso fuori, morbido dentro. Forse.",
                           WF_STAGE_ADULT, WF_EL_GROVE, 68, 21, 17, 12, 92, 31, 29, 20},
};

static const wf_item_info items[WF_IT_COUNT] = {
    [WF_IT_BERRY]  = {"Bacca lumina", "Spuntino che brilla al buio.", WF_KIND_FOOD, 5, 22000, 0, 2000, WF_EL_NONE, 0},
    [WF_IT_BREAD]  = {"Pane al miele", "Soffice e dorato.", WF_KIND_FOOD, 12, 40000, 0, 6000, WF_EL_NONE, 0},
    [WF_IT_STEW]   = {"Stufato", "Il piatto del viandante. Ricarica tutto.", WF_KIND_FOOD, 28, 70000, 15000, 5000, WF_EL_NONE, 0},
    [WF_IT_CANDY]  = {"Caramella astrale", "Dolcissima: fa felici, sazia poco.", WF_KIND_FOOD, 15, 8000, 0, 30000, WF_EL_NONE, 0},
    [WF_IT_CHILI]  = {"Peperoncino", "Accende lo spirito di Brace.", WF_KIND_FOOD, 10, 18000, 4000, 1000, WF_EL_EMBER, 12},
    [WF_IT_KELP]   = {"Alga dolce", "Sa di mare e di Marea.", WF_KIND_FOOD, 10, 20000, 0, 3000, WF_EL_TIDE, 12},
    [WF_IT_SPROUT] = {"Germoglio", "Croccante, profuma di Fronda.", WF_KIND_FOOD, 10, 20000, 2000, 2000, WF_EL_GROVE, 12},
    [WF_IT_POTION] = {"Pozione", "In lotta: cura il 40% dei PV.", WF_KIND_BATTLE, 20, 0, 0, 0, WF_EL_NONE, 0},
    [WF_IT_TONIC]  = {"Tonico", "In lotta: Focus al massimo.", WF_KIND_BATTLE, 18, 0, 0, 0, WF_EL_NONE, 0},
    [WF_IT_SHARD]  = {"Frammento d'astro", "Raro. Si vende bene alla bottega.", WF_KIND_TREASURE, 0, 0, 0, 0, WF_EL_NONE, 0},
};

const wf_form_info *wf_form_get(int form) {
    if (form < 0 || form >= WF_FORM_COUNT) form = WF_FORM_LUMINO;
    return &forms[form];
}

const wf_item_info *wf_item_get(int item) {
    if (item < 0 || item >= WF_IT_COUNT) item = WF_IT_BERRY;
    return &items[item];
}

/* ---- Helpers ---------------------------------------------------------- */
static int32_t clamp_need(int32_t v) { return v < 0 ? 0 : v > WF_NEED_MAX ? WF_NEED_MAX : v; }
static void add_need(int32_t *v, int32_t d) { *v = clamp_need(*v + d); }

uint32_t wf_day_of(uint32_t now) { return now / 86400u; }

void wf_copy_name(char *dst, const char *src) {
    size_t n = 0;
    if (src) while (src[n] && n < WF_NAME_LEN) { dst[n] = src[n]; n++; }
    dst[n] = 0;
}

int wf_need_percent(int32_t v) { return (int)((clamp_need(v) + 999) / 1000); }

uint32_t wf_xp_to_next(int level) { return 15u + (uint32_t)(level * level) * 6u; }

int wf_pet_stage(const wf_game *g) { return wf_form_get(g->pet.form)->stage; }
int wf_pet_element(const wf_game *g) { return wf_form_get(g->pet.form)->element; }

bool wf_pet_away(const wf_game *g, uint32_t now) {
    (void)now;
    return g->pet.away_until != 0;
}

bool wf_pet_can_act(const wf_game *g, uint32_t now) {
    return wf_pet_stage(g) != WF_STAGE_SEED && !g->pet.sleeping && !wf_pet_away(g, now);
}

wf_stats wf_pet_stats(const wf_game *g) {
    const wf_form_info *f = wf_form_get(g->pet.form);
    int l = g->pet.level > 0 ? g->pet.level - 1 : 0;
    wf_stats s = {
        f->hp + f->g_hp * l / 10 + g->pet.train_hp,
        f->atk + f->g_atk * l / 10 + g->pet.train_atk,
        f->def + f->g_def * l / 10 + g->pet.train_def,
        f->spd + f->g_spd * l / 10 + g->pet.train_spd,
    };
    /* Care matters in battle: a hungry, sad or tired companion fights worse. */
    if (g->pet.mood < 25000) s.atk = s.atk * 85 / 100;
    if (g->pet.hunger < 20000) s.def = s.def * 85 / 100;
    if (g->pet.energy < 20000) s.spd = s.spd * 85 / 100;
    if (g->pet.mood > 80000 && g->pet.bond > 60000) s.atk = s.atk * 110 / 100;
    return s;
}

int wf_pet_face(const wf_game *g) {
    const wf_pet *p = &g->pet;
    if (p->sleeping) return WF_MOOD_SLEEP;
    if (p->hunger < 20000) return WF_MOOD_HUNGRY;
    if (p->energy < 20000) return WF_MOOD_TIRED;
    if (p->mood < 30000 || p->mess >= 2) return WF_MOOD_SAD;
    if (p->mood > 75000 && p->hunger > 50000) return WF_MOOD_GREAT;
    return WF_MOOD_OK;
}

const char *wf_pet_status_text(const wf_game *g, uint32_t now) {
    if (wf_pet_stage(g) == WF_STAGE_SEED) return "Scaldami!";
    if (wf_pet_away(g, now)) return now >= g->pet.away_until ? "È tornato!" : "In spedizione";
    switch (wf_pet_face(g)) {
    case WF_MOOD_SLEEP: return "Zzz...";
    case WF_MOOD_HUNGRY: return "Ho fame!";
    case WF_MOOD_TIRED: return "Che sonno...";
    case WF_MOOD_SAD: return g->pet.mess >= 2 ? "Che disordine..." : "Giochiamo?";
    case WF_MOOD_GREAT: return "Felicissimo!";
    default: return "Tutto bene";
    }
}

const char *wf_result_text(int r) {
    switch (r) {
    case WF_OK: return "Fatto!";
    case WF_ERR_ASLEEP: return "Sta dormendo";
    case WF_ERR_AWAY: return "È in spedizione";
    case WF_ERR_FULL: return "È già sazio!";
    case WF_ERR_NONE_LEFT: return "Non ne hai";
    case WF_ERR_NOT_TIRED: return "Non ha sonno";
    case WF_ERR_TIRED: return "Troppo stanco";
    case WF_ERR_NO_MESS: return "Tutto pulito";
    case WF_ERR_COINS: return "Monete insufficienti";
    case WF_ERR_SEED: return "Prima deve schiudersi";
    case WF_ERR_LOCKED: return "Zona bloccata";
    case WF_ERR_COOLDOWN: return "Riprova tra poco";
    case WF_ERR_STACK: return "Zaino pieno";
    default: return "?";
    }
}

/* ---- Lifecycle -------------------------------------------------------- */
void wf_game_new(wf_game *g, const char *name, uint32_t now, uint32_t seed) {
    memset(g, 0, sizeof *g);
    wf_copy_name(g->pet.name, name && *name ? name : "Lumi");
    g->pet.form = WF_FORM_SEED;
    g->pet.level = 1;
    g->pet.hunger = 80000;
    g->pet.energy = 90000;
    g->pet.mood = 70000;
    g->pet.bond = 10000;
    g->pet.born_at = now;
    g->pet.last_update = now;
    g->coins = 30;
    g->inv[WF_IT_BERRY] = 5;
    g->inv[WF_IT_BREAD] = 2;
    g->inv[WF_IT_POTION] = 2;
    g->brightness = 80;
    g->sound = 1;
    wf_rng r;
    wf_rng_seed(&r, seed);
    g->rng_state = r.s;
    g->day = wf_day_of(now);
    wf_game_roll_day(g, g->day);
    g->daily_claimed = 0;
    g->streak = 0;
}

static wf_rng game_rng(wf_game *g) {
    wf_rng r = {g->rng_state ? g->rng_state : 0x1234567u};
    return r;
}
static void game_rng_store(wf_game *g, const wf_rng *r) { g->rng_state = r->s; }

static void sim_minute(wf_game *g) {
    wf_pet *p = &g->pet;
    bool away = p->away_until != 0;
    if (p->sleeping) {
        add_need(&p->hunger, -104);
        add_need(&p->energy, +417);
        if (p->energy >= WF_NEED_MAX) {
            p->sleeping = 0;
            add_need(&p->mood, 4000);
        }
    } else {
        add_need(&p->hunger, away ? -104 : -208);
        add_need(&p->energy, away ? -70 : -139);
        if (!away) {
            int32_t m = 167;
            if (p->hunger < 20000 || p->energy < 15000) m *= 2;
            m += m * p->mess / 2;
            add_need(&p->mood, -m);
            if (++p->mess_timer >= 150) {
                p->mess_timer = 0;
                if (p->mess < WF_MESS_MAX) p->mess++;
            }
        }
        if (p->energy <= 0) {          /* collapsed: sleeps on its own */
            p->sleeping = 1;
            p->care_mistakes++;
            add_need(&p->bond, -2000);
        }
    }
    if (!away && (p->hunger <= 0 || p->mood <= 0)) {
        if (++p->neglect_minutes >= 60) {
            p->neglect_minutes = 0;
            p->care_mistakes++;
            add_need(&p->bond, -3000);
        }
    } else {
        p->neglect_minutes = 0;
    }
}

bool wf_game_tick(wf_game *g, uint32_t now) {
    bool changed = false;
    uint32_t today = wf_day_of(now);
    if (today != g->day && now > g->pet.last_update) {
        wf_game_roll_day(g, today);
        changed = true;
    }
    if (now <= g->pet.last_update) return changed;
    if (wf_pet_stage(g) == WF_STAGE_SEED) {   /* time stands still for the seed */
        g->pet.last_update = now;
        return changed;
    }
    uint32_t minutes = (now - g->pet.last_update) / 60u;
    if (!minutes) return changed;
    uint32_t simulated = minutes > 14u * 24u * 60u ? 14u * 24u * 60u : minutes;
    for (uint32_t i = 0; i < simulated; i++) sim_minute(g);
    g->pet.last_update += minutes * 60u;
    return true;
}

void wf_game_roll_day(wf_game *g, uint32_t day) {
    if (day != g->day) {
        /* The streak survives only if yesterday's gift was collected. */
        if (day != g->day + 1 || !g->daily_claimed) g->streak = 0;
        g->daily_claimed = 0;
    }
    g->day = day;
    wf_rng r;
    wf_rng_seed(&r, day * 2654435761u ^ g->rng_state);
    static const uint8_t goals[WF_Q_COUNT][2] = {
        [WF_Q_FEED] = {3, 5}, [WF_Q_PLAY] = {2, 3}, [WF_Q_WIN] = {2, 4}, [WF_Q_EXPLORE] = {3, 6},
        [WF_Q_GREET] = {1, 2}, [WF_Q_CLEAN] = {2, 3}, [WF_Q_TRAIN] = {1, 3},
    };
    uint8_t used = 0;
    for (int i = 0; i < WF_QUESTS; i++) {
        int k;
        do { k = wf_rng_range(&r, 0, WF_Q_COUNT - 1); } while (used & (1u << k));
        used |= (uint8_t)(1u << k);
        wf_quest *q = &g->quests[i];
        q->kind = (uint8_t)k;
        q->goal = (uint8_t)wf_rng_range(&r, goals[k][0], goals[k][1]);
        q->progress = 0;
        q->claimed = 0;
        q->coins = (uint16_t)(15 + q->goal * 6);
        static const uint8_t prizes[] = {WF_IT_BREAD, WF_IT_STEW, WF_IT_CANDY, WF_IT_POTION, WF_IT_TONIC};
        q->item = prizes[wf_rng_range(&r, 0, (int)sizeof prizes - 1)];
    }
}

/* ---- Care ------------------------------------------------------------- */
static int care_gate(const wf_game *g, uint32_t now) {
    if (wf_pet_stage(g) == WF_STAGE_SEED) return WF_ERR_SEED;
    if (wf_pet_away(g, now)) return WF_ERR_AWAY;
    if (g->pet.sleeping) return WF_ERR_ASLEEP;
    return WF_OK;
}

int wf_feed(wf_game *g, int item, uint32_t now) {
    int gate = care_gate(g, now);
    if (gate) return gate;
    if (item < 0 || item >= WF_IT_COUNT || wf_item_get(item)->kind != WF_KIND_FOOD) return WF_ERR_NONE_LEFT;
    if (!g->inv[item]) return WF_ERR_NONE_LEFT;
    const wf_item_info *it = wf_item_get(item);
    if (g->pet.hunger >= 95000 && it->hunger > 10000) return WF_ERR_FULL;
    bool needed = g->pet.hunger < 50000;
    g->inv[item]--;
    add_need(&g->pet.hunger, it->hunger);
    add_need(&g->pet.energy, it->energy);
    add_need(&g->pet.mood, it->mood);
    add_need(&g->pet.bond, needed ? 1500 : 400);
    if (it->element) g->pet.affinity[it->element] += it->affinity;
    g->feeds++;
    wf_quest_bump(g, WF_Q_FEED, 1);
    return WF_OK;
}

int wf_sleep_toggle(wf_game *g, uint32_t now) {
    if (wf_pet_stage(g) == WF_STAGE_SEED) return WF_ERR_SEED;
    if (wf_pet_away(g, now)) return WF_ERR_AWAY;
    if (g->pet.sleeping) {
        g->pet.sleeping = 0;
        if (g->pet.energy < 50000) add_need(&g->pet.mood, -5000);
        return WF_OK;
    }
    if (g->pet.energy > 85000) return WF_ERR_NOT_TIRED;
    g->pet.sleeping = 1;
    add_need(&g->pet.bond, 500);
    return WF_OK;
}

int wf_clean(wf_game *g, uint32_t now) {
    int gate = care_gate(g, now);
    if (gate) return gate;
    if (!g->pet.mess) return WF_ERR_NO_MESS;
    g->pet.mess--;
    add_need(&g->pet.mood, 4000);
    add_need(&g->pet.bond, 300);
    wf_quest_bump(g, WF_Q_CLEAN, 1);
    return WF_OK;
}

int wf_pat(wf_game *g, uint32_t now) {
    int gate = care_gate(g, now);
    if (gate) return gate;
    if (g->pet.last_pat && now - g->pet.last_pat < 300) return WF_ERR_COOLDOWN;
    g->pet.last_pat = now;
    add_need(&g->pet.mood, 2500);
    add_need(&g->pet.bond, 400);
    return WF_OK;
}

bool wf_warm_seed(wf_game *g, uint32_t now) {
    if (g->pet.form != WF_FORM_SEED) return false;
    if (++g->pet.hatch_heat < 24) return false;
    g->pet.form = WF_FORM_LUMINO;
    g->pet.born_at = now;
    g->pet.last_update = now;
    g->pet.hatch_heat = 0;
    add_need(&g->pet.bond, 10000);
    return true;
}

int wf_play_finish(wf_game *g, int score, uint32_t now, int *coins_out) {
    int gate = care_gate(g, now);
    if (gate) return gate;
    if (score < 0) score = 0;
    if (score > 40) score = 40;
    add_need(&g->pet.mood, 12000 + score * 1200);
    add_need(&g->pet.energy, -7000);
    add_need(&g->pet.hunger, -4000);
    add_need(&g->pet.bond, 1200);
    int coins = score / 3;
    g->coins += (uint32_t)coins;
    if (coins_out) *coins_out = coins;
    g->plays++;
    wf_quest_bump(g, WF_Q_PLAY, 1);
    wf_gain_xp(g, (uint32_t)(score / 4 + 1));
    return WF_OK;
}

int wf_train_finish(wf_game *g, int stat, int quality_sum, uint32_t now, int *gain_out) {
    int gate = care_gate(g, now);
    if (gate) return gate;
    if (g->pet.energy < 15000) return WF_ERR_TIRED;
    if (quality_sum < 0) quality_sum = 0;
    if (quality_sum > 15) quality_sum = 15;
    int gain = quality_sum >= 13 ? 3 : quality_sum >= 8 ? 2 : quality_sum >= 3 ? 1 : 0;
    int16_t *target = stat == 1 ? &g->pet.train_def : stat == 2 ? &g->pet.train_spd
                    : stat == 3 ? &g->pet.train_hp : &g->pet.train_atk;
    int cap = g->pet.level * (stat == 3 ? 6 : 3);
    if (stat == 3) gain *= 2;
    if (*target + gain > cap) gain = cap - *target > 0 ? cap - *target : 0;
    *target = (int16_t)(*target + gain);
    add_need(&g->pet.energy, -12000);
    add_need(&g->pet.hunger, -6000);
    add_need(&g->pet.bond, 600);
    g->pet.affinity[WF_EL_NONE] += 1;
    if (gain_out) *gain_out = gain;
    wf_quest_bump(g, WF_Q_TRAIN, 1);
    wf_gain_xp(g, (uint32_t)(2 + quality_sum / 3));
    return WF_OK;
}

int wf_buy(wf_game *g, int item) {
    if (item < 0 || item >= WF_IT_COUNT) return WF_ERR_NONE_LEFT;
    const wf_item_info *it = wf_item_get(item);
    if (!it->price) return WF_ERR_LOCKED;
    if (g->coins < it->price) return WF_ERR_COINS;
    if (g->inv[item] >= WF_ITEM_STACK) return WF_ERR_STACK;
    g->coins -= it->price;
    g->inv[item]++;
    return WF_OK;
}

bool wf_give_item(wf_game *g, int item, int count) {
    if (item < 0 || item >= WF_IT_COUNT || count <= 0) return false;
    int v = g->inv[item] + count;
    g->inv[item] = (uint8_t)(v > WF_ITEM_STACK ? WF_ITEM_STACK : v);
    return true;
}

int wf_evolution_target(const wf_game *g) {
    const wf_pet *p = &g->pet;
    int stage = wf_pet_stage(g);
    if (stage == WF_STAGE_BABY && p->level >= 5) {
        int best = WF_EL_EMBER;
        for (int e = WF_EL_TIDE; e < WF_EL_COUNT; e++)
            if (p->affinity[e] > p->affinity[best]) best = e;
        if (!p->affinity[best]) {
            /* No food preference yet: let the companion's own nature decide. */
            best = WF_EL_EMBER + (int)(p->born_at % 3u);
        }
        return best == WF_EL_EMBER ? WF_FORM_FAVILLO : best == WF_EL_TIDE ? WF_FORM_SPRUZZO : WF_FORM_FOGLIOLO;
    }
    if (stage == WF_STAGE_TEEN && p->level >= 12) {
        bool good = p->bond >= 50000 && p->care_mistakes <= 6;
        switch (p->form) {
        case WF_FORM_FAVILLO: return good ? WF_FORM_PIROLUME : WF_FORM_CENEROTTO;
        case WF_FORM_SPRUZZO: return good ? WF_FORM_CORALBA : WF_FORM_ABISSINO;
        default: return good ? WF_FORM_SILVANTE : WF_FORM_ROVACCIO;
        }
    }
    return -1;
}

int wf_gain_xp(wf_game *g, uint32_t xp) {
    wf_pet *p = &g->pet;
    if (wf_pet_stage(g) == WF_STAGE_SEED) return 0;
    int levels = 0;
    p->xp += xp;
    while (p->level < WF_MAX_LEVEL && p->xp >= wf_xp_to_next(p->level)) {
        p->xp -= wf_xp_to_next(p->level);
        p->level++;
        levels++;
    }
    if (p->level >= WF_MAX_LEVEL) p->xp = 0;
    int target = wf_evolution_target(g);
    if (target >= 0) p->pending_form = (uint8_t)target;
    return levels;
}

int wf_evolve(wf_game *g) {
    wf_pet *p = &g->pet;
    if (!p->pending_form) return -1;
    int target = p->pending_form;
    p->pending_form = 0;
    p->form = (uint8_t)target;
    add_need(&p->mood, 20000);
    add_need(&p->bond, 5000);
    return target;
}

/* ---- Quests & daily --------------------------------------------------- */
void wf_quest_bump(wf_game *g, int kind, int amount) {
    for (int i = 0; i < WF_QUESTS; i++) {
        wf_quest *q = &g->quests[i];
        if (q->kind != kind || q->claimed) continue;
        int v = q->progress + amount;
        q->progress = (uint8_t)(v > q->goal ? q->goal : v);
    }
}

int wf_quest_claim(wf_game *g, int index) {
    if (index < 0 || index >= WF_QUESTS) return 0;
    wf_quest *q = &g->quests[index];
    if (q->claimed || q->progress < q->goal) return 0;
    q->claimed = 1;
    g->coins += q->coins;
    wf_give_item(g, q->item, 1);
    wf_gain_xp(g, 10u + q->goal * 2u);
    return q->coins;
}

const char *wf_quest_text(const wf_quest *q, char *buf, int cap) {
    static const char *const verbs[WF_Q_COUNT] = {
        "Dai da mangiare %d volte", "Gioca %d volte", "Vinci %d lotte", "Esplora %d volte",
        "Saluta %d amici", "Pulisci %d volte", "Allenati %d volte",
    };
    snprintf(buf, (size_t)cap, verbs[q->kind % WF_Q_COUNT], q->goal);
    return buf;
}

int wf_daily_claim(wf_game *g, int *item_out) {
    if (g->daily_claimed) return 0;
    g->daily_claimed = 1;
    if (g->streak < 7) g->streak++;
    int coins = 10 + g->streak * 5;
    g->coins += (uint32_t)coins;
    int item = g->streak >= 7 ? WF_IT_STEW : g->streak >= 4 ? WF_IT_CANDY : WF_IT_BERRY;
    wf_give_item(g, item, g->streak >= 7 ? 2 : 1);
    if (item_out) *item_out = item;
    return coins;
}

/* ---- Friends ---------------------------------------------------------- */
wf_friend *wf_friend_find(wf_game *g, const uint8_t id[6]) {
    for (int i = 0; i < WF_FRIENDS; i++)
        if (g->friends[i].used && !memcmp(g->friends[i].id, id, 6)) return &g->friends[i];
    return NULL;
}

wf_friend *wf_friend_upsert(wf_game *g, const uint8_t id[6], const char *name, int form, int level) {
    wf_friend *f = wf_friend_find(g, id);
    if (!f) {
        int slot = -1, weakest = 0x7fffffff;
        for (int i = 0; i < WF_FRIENDS; i++) {
            if (!g->friends[i].used) { slot = i; break; }
            int score = g->friends[i].greets + g->friends[i].duels;
            if (score < weakest) { weakest = score; slot = i; }
        }
        f = &g->friends[slot];
        memset(f, 0, sizeof *f);
        f->used = 1;
        memcpy(f->id, id, 6);
    }
    if (name) wf_copy_name(f->name, name);
    f->form = (uint8_t)form;
    f->level = (uint8_t)level;
    return f;
}

bool wf_greet(wf_game *g, const uint8_t id[6], const char *name, int form, int level, uint32_t now) {
    wf_friend *f = wf_friend_upsert(g, id, name, form, level);
    uint32_t today = wf_day_of(now) + 1;   /* +1 so that day 0 is distinguishable */
    if (f->last_greet_day == today) return false;
    f->last_greet_day = today;
    if (f->greets < 0xffff) f->greets++;
    add_need(&g->pet.mood, 8000);
    add_need(&g->pet.bond, 1500);
    wf_quest_bump(g, WF_Q_GREET, 1);
    return true;
}

/* Expose rng helpers to world.c without widening the public header. */
wf_rng wf__game_rng(wf_game *g) { return game_rng(g); }
void wf__game_rng_store(wf_game *g, const wf_rng *r) { game_rng_store(g, r); }
