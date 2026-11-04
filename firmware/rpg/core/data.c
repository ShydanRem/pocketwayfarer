/* Static game data: races, classes, skills, items, monsters, zones. */
#include <stdio.h>
#include <string.h>
#include "rpg.h"

#define T(it, en) {it, en}

/* ---- Races, stats, elements ------------------------------------------------------- */
static const rpg_race_info races[RACE_COUNT] = {
    {T("Umano", "Human"), T("Versatile: impara in fretta.", "Versatile: learns quickly."), {0, 0, 0, 0, 0, 0}, 5},
    {T("Elfo", "Elf"), T("Mira infallibile e mente acuta.", "Keen aim and a sharp mind."), {0, 0, 0, 1, 2, 0}, 0},
    {T("Nano", "Dwarf"), T("Robusto come la roccia.", "Sturdy as bedrock."), {1, 0, 2, 0, 0, 0}, 0},
    {T("Ferino", "Beastkin"), T("Riflessi felini, coda inclusa.", "Feline reflexes, tail included."), {1, 2, 0, 0, 0, 0}, 0},
};
const rpg_race_info *rpg_race(int r) { return &races[(unsigned)r < RACE_COUNT ? r : 0]; }

wf_text rpg_stat_name(int s) {
    static const wf_text n[ST_COUNT] = {T("FOR", "STR"), T("AGI", "AGI"), T("VIT", "VIT"),
                                        T("INT", "INT"), T("DES", "DEX"), T("FRT", "LUK")};
    return n[(unsigned)s < ST_COUNT ? s : 0];
}

wf_text rpg_element_name(int e) {
    static const wf_text n[EL_COUNT] = {T("Neutro", "Neutral"), T("Fuoco", "Fire"), T("Acqua", "Water"),
                                        T("Terra", "Earth"), T("Vento", "Wind"), T("Sacro", "Holy"),
                                        T("Oscuro", "Dark")};
    return n[(unsigned)e < EL_COUNT ? e : 0];
}

wf_text rpg_status_name(int s) {
    static const wf_text n[STS_COUNT] = {T("", ""), T("Ustione", "Burn"), T("Veleno", "Poison"), T("Gelo", "Freeze"),
                                         T("Stordito", "Stun"), T("Sanguina", "Bleed"), T("Debole", "Weak"),
                                         T("Cecità", "Blind")};
    return n[(unsigned)s < STS_COUNT ? s : 0];
}

int rpg_element_mult(int a, int d) {
    if (a == EL_NEUTRAL || d == EL_NEUTRAL) return 100;
    if (a == d) return 50;
    static const uint8_t beats[EL_COUNT] = {0, EL_EARTH, EL_FIRE, EL_WIND, EL_WATER, EL_DARK, EL_HOLY};
    if (beats[a] == d) return 150;
    if (beats[d] == a && a != EL_HOLY && a != EL_DARK) return 75;
    return 100;
}

/* ---- Classes ------------------------------------------------------------------------ */
static const struct { wf_text name, blurb; uint8_t main, second; } class_names[RPG_CLASS_COUNT] = {
    {T("Guerriero", "Warrior"), T("Spada, scudo e coraggio.", "Sword, shield and nerve."), ST_STR, ST_VIT},
    {T("Mago", "Mage"), T("Piega gli elementi al suo volere.", "Bends the elements to their will."), ST_INT, ST_DEX},
    {T("Arciere", "Archer"), T("Colpisce da lontano, sempre.", "Strikes from afar, every time."), ST_DEX, ST_AGI},
    {T("Ladro", "Rogue"), T("Veloce, letale, un po' tirchio.", "Quick, deadly, slightly greedy."), ST_AGI, ST_LUK},
    {T("Chierico", "Cleric"), T("Cura gli amici, castiga le tenebre.", "Heals friends, smites the dark."), ST_INT, ST_VIT},
    {T("Monaco", "Monk"), T("Pugni d'acciaio, mente di vetro.", "Fists of steel, mind of glass."), ST_STR, ST_AGI},
    {T("Artefice", "Artificer"), T("Martello, bombe e ingegno.", "Hammers, bombs and wit."), ST_STR, ST_DEX},
    {T("Occultista", "Occultist"), T("Patti con ciò che sta nell'ombra.", "Pacts with what hides in shadow."), ST_INT, ST_LUK},
    /* tier 2 */
    {T("Cavaliere", "Knight"), T("Un muro d'acciaio tra te e il male.", "A wall of steel against evil."), ST_STR, ST_VIT},
    {T("Berserker", "Berserker"), T("Più si arrabbia, più fa male.", "The angrier, the deadlier."), ST_STR, ST_AGI},
    {T("Elementalista", "Elementalist"), T("Tempeste e terremoti a comando.", "Storms and quakes on command."), ST_INT, ST_DEX},
    {T("Arcanista", "Arcanist"), T("Magia pura, senza fronzoli.", "Raw magic, no frills."), ST_INT, ST_VIT},
    {T("Ramingo", "Ranger"), T("Mai da solo: c'è sempre il lupo.", "Never alone: the wolf is near."), ST_DEX, ST_AGI},
    {T("Cecchino", "Sharpshooter"), T("Un colpo, un bersaglio.", "One shot, one target."), ST_DEX, ST_LUK},
    {T("Assassino", "Assassin"), T("L'ombra ha i denti.", "The shadow has teeth."), ST_AGI, ST_LUK},
    {T("Duellante", "Duelist"), T("Eleganza e punta di fioretto.", "Grace and a rapier's point."), ST_AGI, ST_STR},
    {T("Paladino", "Paladin"), T("La fede come armatura.", "Faith worn as armour."), ST_STR, ST_INT},
    {T("Sacerdote", "Priest"), T("Nessuno cade finché prega.", "None fall while they pray."), ST_INT, ST_VIT},
    {T("Campione", "Champion"), T("Il ring è ovunque.", "The ring is everywhere."), ST_STR, ST_AGI},
    {T("Mistico", "Mystic"), T("Il chi diventa luce.", "Chi turned to light."), ST_INT, ST_AGI},
    {T("Fabbro", "Blacksmith"), T("Ogni colpo è una forgiatura.", "Every blow is a forging."), ST_STR, ST_VIT},
    {T("Alchimista", "Alchemist"), T("Fiale, acidi e piccoli miracoli.", "Flasks, acids and small miracles."), ST_INT, ST_DEX},
    {T("Negromante", "Necromancer"), T("I morti lavorano gratis.", "The dead work for free."), ST_INT, ST_VIT},
    {T("Stregone", "Warlock"), T("Fiamme dell'abisso.", "Flames from the abyss."), ST_INT, ST_LUK},
    /* tier 3 */
    {T("Cavaliere del Drago", "Dragon Knight"), T("Sangue di drago nelle vene.", "Dragon blood in the veins."), ST_STR, ST_VIT},
    {T("Signore della Guerra", "Warlord"), T("Gli eserciti tremano.", "Armies tremble."), ST_STR, ST_AGI},
    {T("Arcimago", "Archmage"), T("Fa piovere stelle.", "Makes the stars fall."), ST_INT, ST_DEX},
    {T("Cronomante", "Chronomancer"), T("Ha sempre un turno in più.", "Always has one more turn."), ST_INT, ST_AGI},
    {T("Signore delle Bestie", "Beastmaster"), T("Il branco risponde.", "The pack answers."), ST_DEX, ST_AGI},
    {T("Arco della Tempesta", "Stormbow"), T("Frecce come fulmini.", "Arrows like lightning."), ST_DEX, ST_LUK},
    {T("Lama Notturna", "Nightblade"), T("Non l'hai visto arrivare.", "You never saw it coming."), ST_AGI, ST_LUK},
    {T("Maestro di Lame", "Blademaster"), T("Una danza di acciaio.", "A dance of steel."), ST_AGI, ST_STR},
    {T("Templare", "Templar"), T("Giuramento infrangibile.", "An unbreakable oath."), ST_STR, ST_VIT},
    {T("Ierofante", "Hierophant"), T("Voce degli dei.", "Voice of the gods."), ST_INT, ST_VIT},
    {T("Gran Maestro", "Grandmaster"), T("Cento colpi in un respiro.", "A hundred blows in a breath."), ST_STR, ST_AGI},
    {T("Asceso", "Ascendant"), T("Oltre il corpo.", "Beyond the body."), ST_INT, ST_AGI},
    {T("Forgiarune", "Runesmith"), T("Rune incise nel tuono.", "Runes carved in thunder."), ST_STR, ST_INT},
    {T("Trasmutatore", "Transmuter"), T("Il piombo diventa oro.", "Lead becomes gold."), ST_INT, ST_LUK},
    {T("Signore della Morte", "Deathlord"), T("Comanda le legioni dei caduti.", "Commands the fallen legions."), ST_INT, ST_VIT},
    {T("Evocavuoto", "Voidcaller"), T("Apre squarci nel nulla.", "Tears holes in nothing."), ST_INT, ST_LUK},
};

const rpg_class_info *rpg_class(int c) {
    static rpg_class_info out[RPG_CLASS_COUNT];
    static bool ready;
    if (!ready) {
        for (int i = 0; i < RPG_CLASS_COUNT; i++) {
            out[i].name = class_names[i].name;
            out[i].blurb = class_names[i].blurb;
            out[i].main_stat = class_names[i].main;
            out[i].second_stat = class_names[i].second;
            int first = i < 8 ? i * 3 : i < 24 ? 24 + (i - 8) * 3 : 72 + (i - 24) * 2;
            int n = i < 24 ? 3 : 2;
            for (int k = 0; k < 3; k++) out[i].skills[k] = (uint8_t)(k < n ? first + k : 0xff);
        }
        ready = true;
    }
    return &out[(unsigned)c < RPG_CLASS_COUNT ? c : 0];
}

int rpg_class_next(int c, int out[2]) {
    if (c < 8) { out[0] = 8 + c * 2; out[1] = 9 + c * 2; return 2; }
    if (c < 24) { out[0] = c + 16; return 1; }
    return 0;
}

int rpg_job_max(int c) { return c < 8 ? 30 : c < 24 ? 40 : 50; }

/* ---- Skills ------------------------------------------------------------------------- */
#define S(it, en, k, el, p, plv, h, sp, splv, cd, st, ch, bf, tu) \
    {T(it, en), k, el, p, plv, h, sp, splv, cd, st, ch, bf, tu}
static const rpg_skill_info skills[RPG_SKILL_COUNT] = {
    /* 0 Warrior */
    S("Fendente", "Bash", SK_STRIKE, EL_NEUTRAL, 160, 20, 1, 6, 1, 0, STS_STUN, 10, 0, 1),
    S("Grido di guerra", "War Cry", SK_BUFF, EL_NEUTRAL, 25, 5, 0, 8, 1, 4, 0, 0, BF_ATK, 3),
    S("Vigore", "Vigor", SK_PASSIVE, 0, 6, 3, 0, 0, 0, 0, 0, 0, PS_HP, 0),
    /* 1 Mage */
    S("Dardo di fuoco", "Fire Bolt", SK_BOLT, EL_FIRE, 170, 20, 1, 8, 2, 0, STS_BURN, 15, 0, 3),
    S("Lancia di ghiaccio", "Ice Lance", SK_BOLT, EL_WATER, 150, 18, 1, 9, 2, 1, STS_FREEZE, 25, 0, 1),
    S("Mente arcana", "Arcane Mind", SK_PASSIVE, 0, 6, 3, 0, 0, 0, 0, 0, 0, PS_MATK, 0),
    /* 2 Archer */
    S("Doppia freccia", "Double Shot", SK_MULTI, EL_NEUTRAL, 90, 10, 2, 7, 1, 0, 0, 0, 0, 0),
    S("Pioggia di frecce", "Arrow Rain", SK_CLEAVE, EL_NEUTRAL, 100, 12, 1, 12, 2, 2, 0, 0, 0, 0),
    S("Occhio di falco", "Hawk Eye", SK_PASSIVE, 0, 3, 2, 0, 0, 0, 0, 0, 0, PS_CRIT, 0),
    /* 3 Rogue */
    S("Pugnalata", "Backstab", SK_STRIKE, EL_NEUTRAL, 150, 20, 1, 6, 1, 0, STS_BLEED, 30, 0, 3),
    S("Veleno", "Envenom", SK_DEBUFF, EL_NEUTRAL, 60, 10, 1, 7, 1, 2, STS_POISON, 90, 0, 4),
    S("Agilità felina", "Feline Grace", SK_PASSIVE, 0, 3, 2, 0, 0, 0, 0, 0, 0, PS_DODGE, 0),
    /* 4 Cleric */
    S("Cura", "Heal", SK_HEAL, EL_HOLY, 120, 25, 0, 10, 2, 1, 0, 0, 0, 0),
    S("Luce sacra", "Holy Light", SK_BOLT, EL_HOLY, 160, 20, 1, 8, 2, 0, STS_BLIND, 15, 0, 2),
    S("Fede", "Faith", SK_PASSIVE, 0, 8, 4, 0, 0, 0, 0, 0, 0, PS_HEAL, 0),
    /* 5 Monk */
    S("Raffica di pugni", "Flurry", SK_MULTI, EL_NEUTRAL, 60, 8, 3, 7, 1, 0, 0, 0, 0, 0),
    S("Concentrazione", "Focus Chi", SK_BUFF, EL_NEUTRAL, 15, 3, 0, 6, 1, 4, 0, 0, BF_CRIT, 3),
    S("Corpo di ferro", "Iron Body", SK_PASSIVE, 0, 6, 3, 0, 0, 0, 0, 0, 0, PS_DEF, 0),
    /* 6 Artificer */
    S("Martellata", "Hammer Blow", SK_STRIKE, EL_NEUTRAL, 170, 20, 1, 7, 1, 1, STS_STUN, 20, 0, 1),
    S("Bomba", "Bomb", SK_CLEAVE, EL_FIRE, 120, 15, 1, 10, 2, 2, STS_BURN, 20, 0, 2),
    S("Ingegno", "Ingenuity", SK_PASSIVE, 0, 6, 3, 0, 0, 0, 0, 0, 0, PS_GOLD, 0),
    /* 7 Occultist */
    S("Dardo d'ombra", "Shadow Bolt", SK_BOLT, EL_DARK, 160, 20, 1, 7, 2, 0, 0, 0, 0, 0),
    S("Maledizione", "Curse", SK_DEBUFF, EL_DARK, 40, 10, 1, 8, 1, 2, STS_WEAK, 85, 0, 3),
    S("Patto oscuro", "Dark Pact", SK_PASSIVE, 0, 6, 3, 0, 0, 0, 0, 0, 0, PS_MATK, 0),
    /* 8 Knight */
    S("Muro di scudi", "Shield Wall", SK_BUFF, EL_NEUTRAL, 50, 8, 0, 10, 1, 4, 0, 0, BF_DEF, 3),
    S("Lancia perforante", "Piercing Lance", SK_STRIKE, EL_NEUTRAL, 220, 25, 1, 10, 2, 1, 0, 0, 0, 0),
    S("Armatura pesante", "Heavy Armour", SK_PASSIVE, 0, 8, 3, 0, 0, 0, 0, 0, 0, PS_DEF, 0),
    /* 9 Berserker */
    S("Furia", "Rage", SK_BUFF, EL_NEUTRAL, 45, 8, 0, 8, 1, 4, 0, 0, BF_RAGE, 3),
    S("Turbine", "Whirlwind", SK_CLEAVE, EL_NEUTRAL, 140, 15, 1, 12, 2, 1, STS_BLEED, 20, 0, 3),
    S("Sete di sangue", "Bloodthirst", SK_PASSIVE, 0, 4, 2, 0, 0, 0, 0, 0, 0, PS_LIFESTEAL, 0),
    /* 10 Elementalist */
    S("Tempesta di fulmini", "Thunderstorm", SK_NOVA, EL_WIND, 140, 15, 1, 14, 3, 1, STS_STUN, 15, 0, 1),
    S("Terremoto", "Earthquake", SK_NOVA, EL_EARTH, 160, 18, 1, 16, 3, 2, 0, 0, 0, 0),
    S("Sintonia elementale", "Attunement", SK_PASSIVE, 0, 8, 3, 0, 0, 0, 0, 0, 0, PS_MATK, 0),
    /* 11 Arcanist */
    S("Missili arcani", "Arcane Missiles", SK_MULTI, EL_NEUTRAL, 70, 10, 3, 10, 2, 0, 0, 0, 0, 0),
    S("Barriera di mana", "Mana Shield", SK_BUFF, EL_NEUTRAL, 30, 6, 0, 12, 2, 4, 0, 0, BF_SHIELD, 3),
    S("Riserva arcana", "Arcane Reserve", SK_PASSIVE, 0, 10, 4, 0, 0, 0, 0, 0, 0, PS_SP, 0),
    /* 12 Ranger */
    S("Lupo compagno", "Wolf Companion", SK_SUMMON, EL_NEUTRAL, 60, 10, 0, 14, 3, 6, 0, 0, 0, 5),
    S("Trappola", "Snare", SK_DEBUFF, EL_EARTH, 80, 15, 1, 9, 1, 3, STS_STUN, 60, 0, 1),
    S("Istinto", "Instinct", SK_PASSIVE, 0, 3, 2, 0, 0, 0, 0, 0, 0, PS_DODGE, 0),
    /* 13 Sharpshooter */
    S("Colpo mirato", "Aimed Shot", SK_STRIKE, EL_NEUTRAL, 260, 30, 1, 11, 2, 1, 0, 0, 0, 0),
    S("Freccia esplosiva", "Explosive Arrow", SK_CLEAVE, EL_FIRE, 150, 18, 1, 13, 2, 2, STS_BURN, 25, 0, 2),
    S("Precisione", "Precision", SK_PASSIVE, 0, 4, 2, 0, 0, 0, 0, 0, 0, PS_CRIT, 0),
    /* 14 Assassin */
    S("Lama nell'ombra", "Shadow Strike", SK_STRIKE, EL_DARK, 230, 25, 1, 10, 2, 1, 0, 0, 0, 0),
    S("Nube tossica", "Toxic Cloud", SK_DEBUFF, EL_NEUTRAL, 50, 10, 9, 12, 2, 3, STS_POISON, 85, 0, 4),
    S("Letale", "Lethal", SK_PASSIVE, 0, 4, 2, 0, 0, 0, 0, 0, 0, PS_CRIT, 0),
    /* 15 Duelist */
    S("Affondo", "Lunge", SK_STRIKE, EL_NEUTRAL, 200, 25, 1, 8, 2, 0, 0, 0, 0, 0),
    S("Parata e risposta", "Riposte", SK_BUFF, EL_NEUTRAL, 25, 5, 0, 8, 1, 4, 0, 0, BF_DODGE, 2),
    S("Scherma", "Fencing", SK_PASSIVE, 0, 7, 3, 0, 0, 0, 0, 0, 0, PS_ATK, 0),
    /* 16 Paladin */
    S("Martello sacro", "Holy Smite", SK_STRIKE, EL_HOLY, 200, 25, 1, 10, 2, 0, 0, 0, 0, 0),
    S("Benedizione", "Blessing", SK_BUFF, EL_HOLY, 30, 6, 0, 12, 2, 5, 0, 0, BF_DEF, 4),
    S("Aura di luce", "Aura of Light", SK_PASSIVE, 0, 7, 3, 0, 0, 0, 0, 0, 0, PS_DEF, 0),
    /* 17 Priest */
    S("Rigenerazione", "Renew", SK_BUFF, EL_HOLY, 8, 2, 0, 10, 2, 4, 0, 0, BF_REGEN, 4),
    S("Esorcismo", "Exorcism", SK_NOVA, EL_HOLY, 150, 18, 1, 14, 3, 2, 0, 0, 0, 0),
    S("Grazia", "Grace", SK_PASSIVE, 0, 2, 1, 0, 0, 0, 0, 0, 0, PS_SP_REGEN, 0),
    /* 18 Champion */
    S("Pugno del drago", "Dragon Fist", SK_STRIKE, EL_FIRE, 240, 25, 1, 10, 2, 1, STS_BURN, 25, 0, 2),
    S("Calcio rotante", "Spin Kick", SK_CLEAVE, EL_NEUTRAL, 130, 15, 1, 10, 2, 1, STS_STUN, 15, 0, 1),
    S("Disciplina", "Discipline", SK_PASSIVE, 0, 7, 3, 0, 0, 0, 0, 0, 0, PS_ATK, 0),
    /* 19 Mystic */
    S("Onda di chi", "Chi Wave", SK_BOLT, EL_NEUTRAL, 180, 20, 1, 9, 2, 0, 0, 0, 0, 0),
    S("Meditazione", "Meditate", SK_HEAL, EL_NEUTRAL, 90, 20, 0, 4, 1, 3, 0, 0, 0, 0),
    S("Equilibrio", "Balance", SK_PASSIVE, 0, 7, 3, 0, 0, 0, 0, 0, 0, PS_MATK, 0),
    /* 20 Blacksmith */
    S("Tempra", "Temper", SK_BUFF, EL_NEUTRAL, 40, 8, 0, 10, 2, 4, 0, 0, BF_ATK, 3),
    S("Colpo d'incudine", "Anvil Strike", SK_STRIKE, EL_NEUTRAL, 250, 25, 1, 12, 2, 1, STS_STUN, 30, 0, 1),
    S("Maestria", "Mastery", SK_PASSIVE, 0, 7, 3, 0, 0, 0, 0, 0, 0, PS_ATK, 0),
    /* 21 Alchemist */
    S("Fiala acida", "Acid Flask", SK_DEBUFF, EL_EARTH, 120, 15, 1, 9, 2, 1, STS_WEAK, 70, 0, 3),
    S("Elisir", "Elixir", SK_HEAL, EL_NEUTRAL, 140, 25, 0, 12, 2, 2, 0, 0, 0, 0),
    S("Distillazione", "Distillation", SK_PASSIVE, 0, 10, 4, 0, 0, 0, 0, 0, 0, PS_HEAL, 0),
    /* 22 Necromancer */
    S("Rianima scheletro", "Raise Skeleton", SK_SUMMON, EL_DARK, 70, 10, 0, 16, 3, 6, 0, 0, 0, 5),
    S("Risucchio vitale", "Life Drain", SK_DRAIN, EL_DARK, 150, 18, 1, 11, 2, 0, 0, 0, 0, 0),
    S("Legione", "Legion", SK_PASSIVE, 0, 15, 5, 0, 0, 0, 0, 0, 0, PS_SUMMON, 0),
    /* 23 Warlock */
    S("Fiamma infernale", "Hellfire", SK_NOVA, EL_FIRE, 140, 16, 1, 14, 3, 1, STS_BURN, 30, 0, 3),
    S("Tormento", "Agony", SK_DEBUFF, EL_DARK, 100, 15, 1, 10, 2, 2, STS_POISON, 100, 0, 5),
    S("Vuoto interiore", "Inner Void", SK_PASSIVE, 0, 3, 2, 0, 0, 0, 0, 0, 0, PS_CRIT, 0),
    /* 24 Dragon Knight */
    S("Sangue di drago", "Dragon Blood", SK_PASSIVE, 0, 10, 4, 0, 0, 0, 0, 0, 0, PS_HP, 0),
    S("Soffio del drago", "Dragon Breath", SK_CLEAVE, EL_FIRE, 260, 30, 1, 22, 4, 3, STS_BURN, 40, 0, 3),
    /* 25 Warlord */
    S("Comando", "Command", SK_PASSIVE, 0, 5, 2, 0, 0, 0, 0, 0, 0, PS_CRIT, 0),
    S("Colpo devastante", "Earthshatter", SK_CLEAVE, EL_EARTH, 300, 30, 1, 24, 4, 3, STS_STUN, 35, 0, 1),
    /* 26 Archmage */
    S("Flusso arcano", "Arcane Flow", SK_PASSIVE, 0, 3, 1, 0, 0, 0, 0, 0, 0, PS_SP_REGEN, 0),
    S("Meteora", "Meteor", SK_NOVA, EL_FIRE, 320, 35, 1, 30, 5, 3, STS_BURN, 50, 0, 3),
    /* 27 Chronomancer */
    S("Accelerazione", "Haste", SK_PASSIVE, 0, 6, 2, 0, 0, 0, 0, 0, 0, PS_DOUBLE, 0),
    S("Ferma tempo", "Time Stop", SK_DEBUFF, EL_NEUTRAL, 150, 20, 9, 28, 4, 5, STS_STUN, 100, 0, 1),
    /* 28 Beastmaster */
    S("Branco", "Pack Leader", SK_PASSIVE, 0, 20, 6, 0, 0, 0, 0, 0, 0, PS_SUMMON, 0),
    S("Carica selvaggia", "Stampede", SK_CLEAVE, EL_EARTH, 260, 30, 1, 22, 4, 3, STS_STUN, 25, 0, 1),
    /* 29 Stormbow */
    S("Vento in poppa", "Tailwind", SK_PASSIVE, 0, 6, 2, 0, 0, 0, 0, 0, 0, PS_DOUBLE, 0),
    S("Freccia della tempesta", "Storm Arrow", SK_MULTI, EL_WIND, 80, 10, 5, 24, 4, 3, STS_STUN, 10, 0, 1),
    /* 30 Nightblade */
    S("Velo d'ombra", "Shadow Veil", SK_PASSIVE, 0, 4, 2, 0, 0, 0, 0, 0, 0, PS_DODGE, 0),
    S("Mille tagli", "Thousand Cuts", SK_MULTI, EL_DARK, 60, 8, 6, 24, 4, 3, STS_BLEED, 30, 0, 3),
    /* 31 Blademaster */
    S("Danza delle lame", "Blade Dance", SK_PASSIVE, 0, 6, 2, 0, 0, 0, 0, 0, 0, PS_DOUBLE, 0),
    S("Tempesta di lame", "Blade Storm", SK_CLEAVE, EL_WIND, 240, 30, 1, 22, 4, 3, STS_BLEED, 30, 0, 3),
    /* 32 Templar */
    S("Giuramento", "Oath", SK_PASSIVE, 0, 10, 4, 0, 0, 0, 0, 0, 0, PS_HP, 0),
    S("Giudizio", "Judgement", SK_NOVA, EL_HOLY, 280, 30, 1, 26, 4, 3, STS_STUN, 25, 0, 1),
    /* 33 Hierophant */
    S("Santità", "Sanctity", SK_PASSIVE, 0, 15, 5, 0, 0, 0, 0, 0, 0, PS_HEAL, 0),
    S("Miracolo", "Miracle", SK_HEAL, EL_HOLY, 400, 50, 0, 30, 5, 5, 0, 0, 0, 0),
    /* 34 Grandmaster */
    S("Flusso del chi", "Chi Flow", SK_PASSIVE, 0, 6, 2, 0, 0, 0, 0, 0, 0, PS_DOUBLE, 0),
    S("Palmo dei cento", "Hundred Palms", SK_MULTI, EL_NEUTRAL, 45, 6, 8, 24, 4, 3, 0, 0, 0, 0),
    /* 35 Ascendant */
    S("Illuminazione", "Enlightenment", SK_PASSIVE, 0, 3, 1, 0, 0, 0, 0, 0, 0, PS_SP_REGEN, 0),
    S("Nirvana", "Nirvana", SK_NOVA, EL_HOLY, 300, 30, 1, 28, 4, 3, 0, 0, 0, 0),
    /* 36 Runesmith */
    S("Rune protettive", "Warding Runes", SK_PASSIVE, 0, 10, 3, 0, 0, 0, 0, 0, 0, PS_DEF, 0),
    S("Martello runico", "Rune Hammer", SK_STRIKE, EL_WIND, 380, 40, 1, 24, 4, 3, STS_STUN, 40, 0, 1),
    /* 37 Transmuter */
    S("Pietra filosofale", "Philosopher's Stone", SK_PASSIVE, 0, 15, 5, 0, 0, 0, 0, 0, 0, PS_GOLD, 0),
    S("Trasmutazione", "Transmute", SK_BOLT, EL_NEUTRAL, 350, 35, 1, 26, 4, 3, STS_WEAK, 50, 0, 3),
    /* 38 Deathlord */
    S("Non morte", "Undeath", SK_PASSIVE, 0, 5, 2, 0, 0, 0, 0, 0, 0, PS_LIFESTEAL, 0),
    S("Esercito dei morti", "Army of the Dead", SK_NOVA, EL_DARK, 260, 30, 1, 28, 4, 3, STS_WEAK, 40, 0, 3),
    /* 39 Voidcaller */
    S("Abisso", "Abyss", SK_PASSIVE, 0, 10, 3, 0, 0, 0, 0, 0, 0, PS_MATK, 0),
    S("Squarcio del vuoto", "Void Rift", SK_NOVA, EL_DARK, 330, 35, 1, 30, 5, 3, STS_BLIND, 40, 0, 2),
};

const rpg_skill_info *rpg_skill(int s) { return &skills[(unsigned)s < RPG_SKILL_COUNT ? s : 0]; }
int rpg_skill_power(int s, int lv) {
    const rpg_skill_info *k = rpg_skill(s);
    if (lv < 1) lv = 1;
    return k->power + k->power_lv * (lv - 1);
}
int rpg_skill_cost(int s, int lv) {
    const rpg_skill_info *k = rpg_skill(s);
    if (lv < 1) lv = 1;
    return k->sp + k->sp_lv * (lv - 1);
}

/* ---- Items -------------------------------------------------------------------------- */
#define FAMS(...) (uint8_t)(__VA_ARGS__)
#define F(x) (1u << FAM_##x)
static const rpg_item_type item_types[IT_TYPE_COUNT] = {
    [IT_SWORD]   = {T("Spada", "Sword"), SLOT_WEAPON, 0, FAMS(F(WARRIOR) | F(ROGUE)), 0, 0, 14, 0, 0, 0, 0},
    [IT_AXE]     = {T("Ascia", "Axe"), SLOT_WEAPON, 0, FAMS(F(WARRIOR) | F(ARTIFICER)), 1, 0, 18, 0, 0, 0, 0},
    [IT_STAFF]   = {T("Bastone", "Staff"), SLOT_WEAPON, 1, FAMS(F(MAGE) | F(CLERIC) | F(OCCULTIST)), 2, 1, 5, 17, 0, 2, 0},
    [IT_BOW]     = {T("Arco", "Bow"), SLOT_WEAPON, 1, FAMS(F(ARCHER)), 3, 0, 15, 0, 0, 0, 0},
    [IT_DAGGER]  = {T("Pugnale", "Dagger"), SLOT_WEAPON, 0, FAMS(F(ROGUE)), 4, 0, 11, 0, 0, 0, 0},
    [IT_MACE]    = {T("Mazza", "Mace"), SLOT_WEAPON, 0, FAMS(F(CLERIC)), 5, 0, 12, 8, 0, 0, 0},
    [IT_KNUCKLE] = {T("Tirapugni", "Knuckles"), SLOT_WEAPON, 0, FAMS(F(MONK)), 6, 0, 13, 4, 0, 0, 0},
    [IT_HAMMER]  = {T("Martello", "Hammer"), SLOT_WEAPON, 0, FAMS(F(ARTIFICER)), 7, 0, 17, 0, 0, 0, 0},
    [IT_SCYTHE]  = {T("Falce", "Scythe"), SLOT_WEAPON, 0, FAMS(F(OCCULTIST)), 8, 1, 8, 15, 0, 0, 0},
    [IT_SHIELD]  = {T("Scudo", "Shield"), SLOT_OFFHAND, 0, FAMS(F(WARRIOR) | F(CLERIC) | F(ARTIFICER)), 0, 0, 0, 0, 6, 2, 0},
    [IT_QUIVER]  = {T("Faretra", "Quiver"), SLOT_OFFHAND, 2, FAMS(F(ARCHER)), 1, 0, 5, 0, 0, 0, 0},
    [IT_TOME]    = {T("Tomo", "Tome"), SLOT_OFFHAND, 3, FAMS(F(MAGE) | F(OCCULTIST) | F(CLERIC)), 2, 1, 0, 7, 0, 2, 0},
    [IT_CHARM]   = {T("Amuleto da polso", "Charm"), SLOT_OFFHAND, 4, 0xff, 3, 0, 2, 2, 1, 1, 12},
    [IT_HELM]    = {T("Elmo", "Helm"), SLOT_HEAD, 0, 0xff, 0, 0, 0, 0, 4, 1, 0},
    [IT_HOOD]    = {T("Cappuccio", "Hood"), SLOT_HEAD, 2, 0xff, 1, 0, 0, 0, 2, 2, 0},
    [IT_CIRCLET] = {T("Diadema", "Circlet"), SLOT_HEAD, 4, 0xff, 2, 0, 0, 3, 1, 4, 0},
    [IT_PLATE]   = {T("Corazza", "Plate"), SLOT_BODY, 0, 0xff, 0, 0, 0, 0, 10, 2, 10},
    [IT_LEATHER] = {T("Giubba", "Jerkin"), SLOT_BODY, 2, 0xff, 1, 0, 0, 0, 6, 4, 6},
    [IT_ROBE]    = {T("Tunica", "Robe"), SLOT_BODY, 3, 0xff, 2, 0, 0, 4, 3, 7, 4},
    [IT_BOOTS]   = {T("Stivali", "Boots"), SLOT_FEET, 2, 0xff, 0, 0, 0, 0, 3, 1, 0},
    [IT_RING]    = {T("Anello", "Ring"), SLOT_ACC1, 4, 0xff, 0, 0, 3, 3, 0, 0, 0},
    [IT_AMULET]  = {T("Ciondolo", "Amulet"), SLOT_ACC1, 4, 0xff, 1, 0, 0, 0, 2, 2, 15},
};
const rpg_item_type *rpg_item_type_get(int t) { return &item_types[(unsigned)t < IT_TYPE_COUNT ? t : 0]; }

wf_text rpg_material_name(int m, int tier) {
    static const wf_text mats[5][RPG_TIERS] = {
        {T("Bronzo", "Bronze"), T("Ferro", "Iron"), T("Acciaio", "Steel"), T("Mithril", "Mithril"), T("Astrale", "Astral")},
        {T("Quercia", "Oak"), T("Frassino", "Ash"), T("Tasso", "Yew"), T("Ebano", "Ebony"), T("Legno lunare", "Moonwood")},
        {T("Cuoio", "Leather"), T("Cuoio borchiato", "Studded"), T("Viverna", "Wyvern"), T("Scaglie di drago", "Dragonscale"), T("Ombra", "Shadow")},
        {T("Lino", "Linen"), T("Seta", "Silk"), T("Velluto arcano", "Arcane"), T("Seta stellare", "Starsilk"), T("Etere", "Ether")},
        {T("Rame", "Copper"), T("Argento", "Silver"), T("Oro", "Golden"), T("Platino", "Platinum"), T("Stella", "Star")},
    };
    return mats[m % 5][tier % RPG_TIERS];
}

wf_text rpg_rarity_name(int r) {
    static const wf_text n[RAR_COUNT] = {T("Comune", "Common"), T("Non comune", "Uncommon"), T("Raro", "Rare"),
                                         T("Epico", "Epic"), T("Leggendario", "Legendary")};
    return n[(unsigned)r < RAR_COUNT ? r : 0];
}

wf_text rpg_affix_name(int a) {
    static const wf_text n[AF_COUNT] = {
        T("", ""), T("FOR", "STR"), T("AGI", "AGI"), T("VIT", "VIT"), T("INT", "INT"), T("DES", "DEX"),
        T("FRT", "LUK"), T("ATK %", "ATK %"), T("MATK %", "MATK %"), T("DIF", "DEF"), T("PV %", "HP %"),
        T("Critico %", "Crit %"), T("Risucchio %", "Lifesteal %"), T("PM/turno", "SP/turn"),
        T("Danno Fuoco %", "Fire dmg %"), T("Danno Acqua %", "Water dmg %"), T("Danno Terra %", "Earth dmg %"),
        T("Danno Vento %", "Wind dmg %"), T("Danno Sacro %", "Holy dmg %"), T("Danno Oscuro %", "Dark dmg %"),
        T("Oro %", "Gold %")};
    return n[(unsigned)a < AF_COUNT ? a : 0];
}

static const rpg_consumable_info consumables[CO_COUNT] = {
    [CO_POTION]   = {T("Pozione", "Potion"), T("Cura il 35% dei PV.", "Restores 35% HP."), 25, 35, 0, 0, 0, 0},
    [CO_HIPOTION] = {T("Gran pozione", "Hi-Potion"), T("Cura il 70% dei PV.", "Restores 70% HP."), 80, 70, 0, 0, 0, 0},
    [CO_ETHER]    = {T("Etere", "Ether"), T("Recupera il 50% dei PM.", "Restores 50% SP."), 60, 0, 50, 0, 0, 0},
    [CO_ANTIDOTE] = {T("Antidoto", "Antidote"), T("Cura ogni stato alterato.", "Cures any status."), 20, 0, 0, 0, 0, 0},
    [CO_BREAD]    = {T("Pane", "Bread"), T("Sazia un po'.", "A little filling."), 8, 0, 0, 25000, 2, 0},
    [CO_STEW]     = {T("Stufato", "Stew"), T("Sazia molto, ridà stamina.", "Very filling, restores stamina."), 30, 10, 0, 60000, 5, 15},
    [CO_PIE]      = {T("Torta di mele", "Apple Pie"), T("Morale alle stelle.", "Morale through the roof."), 22, 0, 0, 35000, 15, 0},
    [CO_ORE]      = {T("Minerale", "Ore"), T("Serve alla forgia.", "Used at the forge."), 15, 0, 0, 0, 0, 0},
    [CO_RETURN]   = {T("Pergamena di ritorno", "Return Scroll"), T("Esci dal dungeon tenendo tutto.", "Leave a dungeon keeping everything."), 40, 0, 0, 0, 0, 0},
};
const rpg_consumable_info *rpg_consumable(int c) { return &consumables[(unsigned)c < CO_COUNT ? c : 0]; }

/* ---- Monsters ----------------------------------------------------------------------- */
#define M(it, en, el, hp, atk, def, agi, sp, st, ch, mg, boss, gaf, gv) \
    {T(it, en), el, hp, atk, def, agi, sp, st, ch, mg, boss, gaf, gv}
static const rpg_species_info species[SP_COUNT] = {
    [SP_SLIME]        = M("Gelatino", "Jelly Slime", EL_WATER, 9, 8, 8, 6, FOE_HEAVY, 0, 0, 0, 0, AF_VIT, 2),
    [SP_SHROOM]       = M("Fungotto", "Shroomkin", EL_EARTH, 10, 8, 10, 6, FOE_CURSE, STS_POISON, 40, 1, 0, AF_HP_PCT, 3),
    [SP_WOLF]         = M("Lupo grigio", "Grey Wolf", EL_NEUTRAL, 9, 11, 8, 13, FOE_MULTI, STS_BLEED, 20, 0, 0, AF_AGI, 2),
    [SP_WISP]         = M("Scintillo", "Wisp", EL_FIRE, 7, 10, 7, 12, FOE_BREATH, STS_BURN, 30, 1, 0, AF_FIRE, 5),
    [SP_SKELETON]     = M("Scheletro", "Skeleton", EL_DARK, 10, 11, 10, 8, FOE_HEAVY, 0, 0, 0, 0, AF_STR, 2),
    [SP_BAT]          = M("Pipistrello", "Cave Bat", EL_WIND, 7, 9, 6, 15, FOE_MULTI, STS_BLIND, 25, 0, 0, AF_DEX, 2),
    [SP_GHOST]        = M("Spettro", "Wraith", EL_DARK, 8, 10, 12, 10, FOE_CURSE, STS_WEAK, 40, 1, 0, AF_DARK, 5),
    [SP_MIMIC]        = M("Mimic", "Mimic", EL_NEUTRAL, 15, 13, 14, 6, FOE_HEAVY, STS_BLEED, 30, 0, 0, AF_GOLD, 8),
    [SP_SLIME_KING]   = M("Re Gelatino", "Slime King", EL_WATER, 14, 11, 10, 6, FOE_HEAVY, STS_STUN, 20, 0, 1, AF_VIT, 5),
    [SP_BONE_KING]    = M("Re delle Ossa", "Bone King", EL_DARK, 15, 12, 12, 8, FOE_CURSE, STS_WEAK, 45, 1, 1, AF_STR, 5),
    [SP_CRAB]         = M("Granchietto", "Shore Crab", EL_WATER, 10, 10, 14, 6, FOE_HEAVY, 0, 0, 0, 0, AF_DEF, 4),
    [SP_JELLY]        = M("Medusina", "Jellyfish", EL_WATER, 8, 10, 7, 11, FOE_CURSE, STS_STUN, 25, 1, 0, AF_INT, 2),
    [SP_BLUE_SLIME]   = M("Gelatino blu", "Blue Slime", EL_WATER, 10, 9, 9, 7, FOE_HEAL, 0, 0, 1, 0, AF_SP_REGEN, 1),
    [SP_GOBLIN]       = M("Goblin pirata", "Goblin Pirate", EL_NEUTRAL, 9, 11, 8, 11, FOE_MULTI, STS_BLEED, 20, 0, 0, AF_GOLD, 5),
    [SP_SERPENT]      = M("Serpe marina", "Sea Serpent", EL_WATER, 10, 11, 9, 10, FOE_CURSE, STS_POISON, 45, 0, 0, AF_WATER, 5),
    [SP_DEEP_GHOST]   = M("Annegato", "Drowned", EL_DARK, 9, 11, 11, 9, FOE_CURSE, STS_FREEZE, 30, 1, 0, AF_DARK, 5),
    [SP_CRAB_KING]    = M("Granchio corazzato", "Armoured Crab", EL_WATER, 15, 12, 16, 6, FOE_HEAVY, STS_STUN, 25, 0, 1, AF_DEF, 8),
    [SP_LEVIATHAN]    = M("Leviatano", "Leviathan", EL_WATER, 16, 13, 12, 10, FOE_BREATH, STS_FREEZE, 30, 1, 1, AF_WATER, 10),
    [SP_TREANT]       = M("Radicotto", "Rootling", EL_EARTH, 12, 10, 13, 5, FOE_HEAVY, STS_STUN, 15, 0, 0, AF_VIT, 3),
    [SP_OWL]          = M("Gufo lunare", "Moon Owl", EL_WIND, 8, 11, 8, 13, FOE_BREATH, STS_BLIND, 30, 1, 0, AF_INT, 3),
    [SP_DIRE_WOLF]    = M("Lupo nero", "Dire Wolf", EL_DARK, 10, 12, 9, 13, FOE_MULTI, STS_BLEED, 30, 0, 0, AF_AGI, 3),
    [SP_SPIDER]       = M("Ragno tessitore", "Weaver Spider", EL_EARTH, 9, 11, 9, 12, FOE_CURSE, STS_POISON, 50, 0, 0, AF_DEX, 3),
    [SP_TOXIC_SHROOM] = M("Fungo velenoso", "Toxic Cap", EL_EARTH, 10, 10, 10, 6, FOE_BREATH, STS_POISON, 60, 1, 0, AF_EARTH, 6),
    [SP_DARK_FAIRY]   = M("Fata oscura", "Dark Fairy", EL_DARK, 7, 12, 8, 14, FOE_CURSE, STS_WEAK, 40, 1, 0, AF_MATK_PCT, 3),
    [SP_ELDER_STUMP]  = M("Antico Ceppo", "Elder Stump", EL_EARTH, 16, 12, 14, 5, FOE_HEAL, 0, 0, 1, 1, AF_HP_PCT, 6),
    [SP_SPIDER_QUEEN] = M("Regina Ragno", "Spider Queen", EL_EARTH, 15, 13, 11, 12, FOE_MULTI, STS_POISON, 50, 0, 1, AF_CRIT, 3),
    [SP_SALAMANDER]   = M("Salamandra", "Salamander", EL_FIRE, 10, 12, 9, 11, FOE_BREATH, STS_BURN, 40, 1, 0, AF_FIRE, 6),
    [SP_LAVA_GOLEM]   = M("Golem di lava", "Lava Golem", EL_FIRE, 13, 12, 15, 4, FOE_HEAVY, STS_BURN, 25, 0, 0, AF_DEF, 6),
    [SP_FIRE_BAT]     = M("Pipistrello ardente", "Fire Bat", EL_FIRE, 8, 11, 7, 15, FOE_MULTI, STS_BURN, 25, 0, 0, AF_AGI, 3),
    [SP_GOBLIN_MINER] = M("Goblin minatore", "Goblin Miner", EL_EARTH, 10, 12, 11, 9, FOE_HEAVY, STS_STUN, 20, 0, 0, AF_GOLD, 8),
    [SP_IMP]          = M("Diavoletto", "Imp", EL_FIRE, 8, 12, 8, 14, FOE_BREATH, STS_BURN, 35, 1, 0, AF_LUK, 3),
    [SP_HELL_KNIGHT]  = M("Cavaliere infernale", "Hell Knight", EL_DARK, 12, 13, 14, 8, FOE_HEAVY, STS_BLEED, 30, 0, 0, AF_ATK_PCT, 4),
    [SP_ASH_DRAGON]   = M("Drago di cenere", "Ash Dragon", EL_FIRE, 17, 13, 13, 9, FOE_BREATH, STS_BURN, 50, 1, 1, AF_FIRE, 10),
    [SP_DEMON_LORD]   = M("Signore dei Demoni", "Demon Lord", EL_DARK, 18, 14, 13, 10, FOE_CURSE, STS_WEAK, 50, 1, 1, AF_ATK_PCT, 6),
    [SP_STORM_WISP]   = M("Tempestino", "Storm Wisp", EL_WIND, 8, 12, 8, 15, FOE_BREATH, STS_STUN, 25, 1, 0, AF_WIND, 6),
    [SP_GRIFFIN]      = M("Grifone", "Griffin", EL_WIND, 11, 13, 10, 13, FOE_MULTI, STS_BLEED, 20, 0, 0, AF_AGI, 4),
    [SP_CRYSTAL]      = M("Cristallo errante", "Wandering Crystal", EL_HOLY, 10, 11, 15, 7, FOE_BREATH, STS_BLIND, 30, 1, 0, AF_MATK_PCT, 4),
    [SP_HARPY]        = M("Arpia", "Harpy", EL_WIND, 9, 12, 8, 15, FOE_CURSE, STS_WEAK, 30, 0, 0, AF_DEX, 4),
    [SP_SKY_KNIGHT]   = M("Cavaliere celeste", "Sky Knight", EL_HOLY, 12, 14, 14, 10, FOE_HEAVY, STS_STUN, 25, 0, 0, AF_HOLY, 6),
    [SP_CRYSTAL_GOLEM]= M("Golem di cristallo", "Crystal Golem", EL_HOLY, 14, 12, 17, 5, FOE_HEAVY, STS_STUN, 20, 0, 0, AF_DEF, 8),
    [SP_GUARDIAN]     = M("Custode Celeste", "Celestial Warden", EL_HOLY, 18, 14, 15, 10, FOE_BREATH, STS_BLIND, 40, 1, 1, AF_HOLY, 10),
    [SP_STAR_DRAGON]  = M("Drago Stellare", "Star Dragon", EL_HOLY, 20, 15, 14, 11, FOE_BREATH, STS_STUN, 30, 1, 1, AF_MATK_PCT, 8),
};
const rpg_species_info *rpg_species(int sp) { return &species[(unsigned)sp < SP_COUNT ? sp : 0]; }

static const rpg_zone_info zones[RPG_ZONES] = {
    {T("Prati di Lumen", "Lumen Meadows"), T("Cripta dei Lumi", "Lantern Crypt"),
     T("Erba alta, lucciole e una vecchia cripta.", "Tall grass, fireflies and an old crypt."), 1, 10,
     {SP_SLIME, SP_SHROOM, SP_WOLF, SP_WISP}, {SP_SKELETON, SP_BAT, SP_GHOST, SP_SHROOM}, SP_SLIME_KING, SP_BONE_KING, 0, 0},
    {T("Riva di Vetro", "Glass Shore"), T("Tempio Sommerso", "Sunken Temple"),
     T("Sabbia che tintinna e rovine sotto il mare.", "Chiming sand and ruins under the sea."), 10, 20,
     {SP_CRAB, SP_JELLY, SP_BLUE_SLIME, SP_GOBLIN}, {SP_SERPENT, SP_DEEP_GHOST, SP_JELLY, SP_CRAB}, SP_CRAB_KING, SP_LEVIATHAN, 1, 1},
    {T("Bosco Sussurrante", "Whispering Wood"), T("Radici Antiche", "Elder Roots"),
     T("Gli alberi parlano. Sotto, qualcosa tesse.", "The trees whisper. Below, something weaves."), 20, 32,
     {SP_TREANT, SP_OWL, SP_DIRE_WOLF, SP_SPIDER}, {SP_SPIDER, SP_TOXIC_SHROOM, SP_DARK_FAIRY, SP_TREANT}, SP_ELDER_STUMP, SP_SPIDER_QUEEN, 2, 2},
    {T("Caverne di Brace", "Ember Caves"), T("Fucina Infernale", "Infernal Forge"),
     T("Lava, goblin e una forgia che non si spegne.", "Lava, goblins and a forge that never cools."), 32, 48,
     {SP_SALAMANDER, SP_LAVA_GOLEM, SP_FIRE_BAT, SP_GOBLIN_MINER}, {SP_IMP, SP_HELL_KNIGHT, SP_SALAMANDER, SP_LAVA_GOLEM}, SP_ASH_DRAGON, SP_DEMON_LORD, 3, 3},
    {T("Guglie del Cielo", "Sky Spires"), T("Cittadella Celeste", "Celestial Citadel"),
     T("Sopra le nuvole, dove nascono le stelle.", "Above the clouds, where stars are born."), 48, 70,
     {SP_STORM_WISP, SP_GRIFFIN, SP_CRYSTAL, SP_HARPY}, {SP_SKY_KNIGHT, SP_CRYSTAL_GOLEM, SP_CRYSTAL, SP_STORM_WISP}, SP_GUARDIAN, SP_STAR_DRAGON, 4, 4},
};
const rpg_zone_info *rpg_zone(int z) { return &zones[(unsigned)z < RPG_ZONES ? z : 0]; }

const char *rpg_err(int e) {
    switch (e) {
    case RPG_OK: return TR("Fatto!", "Done!");
    case RPG_ERR_GOLD: return TR("Oro insufficiente", "Not enough gold");
    case RPG_ERR_NONE: return TR("Non ne hai", "You have none");
    case RPG_ERR_FULL: return TR("Sei già sazio", "You are full");
    case RPG_ERR_STAMINA: return TR("Troppo stanco: riposa", "Too tired: rest first");
    case RPG_ERR_LEVEL: return TR("Livello troppo basso", "Level too low");
    case RPG_ERR_CLASS: return TR("Non adatto alla tua classe", "Not for your class");
    case RPG_ERR_BAG: return TR("Zaino pieno", "Bag full");
    case RPG_ERR_BUSY: return TR("Sei già impegnato", "Already busy");
    case RPG_ERR_HUNGRY: return TR("Hai troppa fame", "Too hungry");
    case RPG_ERR_LOCKED: return TR("Non ancora disponibile", "Not available yet");
    case RPG_ERR_DEAD: return TR("Sei esausto: cura i PV", "Exhausted: heal first");
    default: return "?";
    }
}
