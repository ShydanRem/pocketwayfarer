#include "lantern_coach.h"
#include "timing.h"

int ln_coach_tip(const ln_coach *c, const rpg_lantern *l, bool skills) {
    if (c->disabled || l->paused) return LNT_NONE;
    if (l->state == LN_ATTACK && !(c->learned & LNC_ATTACK)) return LNT_ATTACK_TIMING;
    if (l->state == LN_TELEGRAPH && !(c->learned & LNC_PARRY) && l->foes[l->acting].intent != LN_MEND) return LNT_INTENT;
    if (l->state == LN_PARRY && !(c->learned & LNC_PARRY)) return LNT_PARRY_TIMING;
    if (l->state != LN_CHOOSE) return LNT_NONE;
    if (skills) return !(c->learned & LNC_SKILL) ? LNT_SKILL : LNT_NONE;
    if (!(c->learned & LNC_ATTACK)) return LNT_ATTACK;
    if (l->resonance && !(c->learned & LNC_RESONANCE)) return LNT_RESONANCE;
    if (l->sp <= l->max_sp / 2 && !(c->learned & LNC_GUARD)) return LNT_GUARD;
    return LNT_NONE;
}
void ln_coach_observe(ln_coach *c, const rpg_lantern *a, const rpg_lantern *b) {
    if (a->state == LN_ATTACK && b->state == LN_IMPACT && b->feedback == LN_FEEDBACK_HERO && b->quality >= WF_Q_GOOD) c->learned |= LNC_ATTACK;
    if (a->state == LN_PARRY && b->state == LN_IMPACT && b->feedback == LN_FEEDBACK_FOE && b->quality >= WF_Q_GOOD) c->learned |= LNC_PARRY;
    if (a->state == LN_CHOOSE && b->state != LN_CHOOSE) {
        if (b->action == LN_GUARD) c->learned |= LNC_GUARD;
        if (b->action == LN_POWER || b->action == LN_SUPPORT) c->learned |= LNC_SKILL;
    }
    if (a->state == LN_ATTACK && b->state == LN_IMPACT && (b->effect_flags & LN_EFFECT_RESONANCE)) c->learned |= LNC_RESONANCE;
}
void ln_coach_ack(ln_coach *c, int tip) {
    if (tip == LNT_RESONANCE) c->learned |= LNC_RESONANCE;
}
wf_text ln_coach_text(int tip) {
    static const wf_text text[] = {
        {"", ""},
        {"Inizia da Attacco.\nPoi scegli il momento.", "Start with Attack.\nThen choose your moment."},
        {"Tocca sull'arco dorato.\nPuoi usare anche BOOT.", "Tap at the golden arc.\nBOOT works too."},
        {"Leggi l'intento.\nPoi attendi l'onda.", "Read the intent.\nThen wait for the wave."},
        {"Tocca quando l'onda\nincontra il cerchio.", "Tap when the wave\nmeets the white circle."},
        {"Guardia recupera PM\ne dimezza i danni.", "Guard restores SP\nand halves damage."},
        {"6 PM: colpo potenziato.\n5 PM: mossa di supporto.", "6 SP: stronger strike.\n5 SP: support move."},
        {"Tre parate perfette:\noffensiva potenziata.", "Three perfect parries:\npower skill empowered."}
    };
    return text[(unsigned)tip <= LNT_RESONANCE ? tip : LNT_NONE];
}
