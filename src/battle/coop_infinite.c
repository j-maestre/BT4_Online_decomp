/*
 * PC only: Historia cooperativa's endless mode ("Modo infinito"), the battle side.
 *
 * Two players against 2 (or 3) CPU rivals at once, in one battle that does not end while the players stand. Each
 * rival is a team of five fighters set at the start (weak to strong, the fifth a boss): when the one fighting falls,
 * the battle brings in the next, as a team battle does, and the one that fell before is brought back to the end of
 * the line with more health (BtlMember_Replace, as the survival mode's opponent pool does, always into that same
 * member slot with that same fighter: the HUD's faces and the models are the ones loaded at the start). Every wave
 * the rivals come back stronger and hit harder (CoopInf_ScaleDamage); their AI level stays the same.
 *
 * The counters are globals of this battle file: part of the game state that online rollback saves and restores, so
 * both machines do the same thing at the same moment. Everything here comes from the wave number alone.
 * (ASCII only in this file: the build's pointer tool reads it by byte offset.)
 */
#include "common.h"
#include "battle/battle.h"
#include "battle/battle_setup.h"
#include "battle/btl_char_member.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef PORT
extern s32 TagBattle_Team(s32 a);
extern s32 BtlMember_Replace(BtlMemberChr *chr, BtlMemberSetup *src);
extern BtlMember *BtlMember_Get(BtlMemberChr *chr, s32 n);
extern BtlMemberGauge *BtlMember_GetGauge(BtlMemberChr *chr, s32 n);

#define INF_TEAM 5 /* fighters behind each rival: the five member slots */

static s32 sInfFoes;     /* rivals at once: 0 = not the endless mode */
static s32 sInfDefeated; /* rivals fallen */
static s32 sInfRival[10]; /* each battle side's place among the rivals (-1: a player), for its fighters */

/* Fighters by strength: each rival's team takes one of each, then a boss. */
static const s16 sInfTier0[] = {184, 79, 84, 88, 89, 90, 77, 82, 194};                   /* Saibaman .. Spopovich */
static const s16 sInfTier1[] = {80, 29, 87, 86, 83, 103, 185, 109, 121, 123};            /* Nappa .. Slug */
static const s16 sInfTier2[] = {91, 92, 93, 100, 101, 102, 105, 106, 111, 126, 166, 97}; /* Freezer 1 .. King Cold */
static const s16 sInfTier3[] = {94, 95, 96, 107, 112, 127, 128, 136, 114, 137};          /* Freezer final .. Janemba */
static const s16 sInfBoss[] = {190, 117, 138, 141, 147, 115, 116, 95};                   /* Broly, Bu puro.. (no giants) */
#define INF_COUNT(t) ((s32)(sizeof(t) / sizeof((t)[0])))

/* Rival k's j-th fighter (j 0..4, the fifth the boss). Rivals take different ones. */
static s32 CoopInf_Chara(s32 k, s32 j) {
    switch (j) {
    case 0: return sInfTier0[(k * 3 + 1) % INF_COUNT(sInfTier0)];
    case 1: return sInfTier1[(k * 4 + 0) % INF_COUNT(sInfTier1)];
    case 2: return sInfTier2[(k * 5 + 2) % INF_COUNT(sInfTier2)];
    case 3: return sInfTier3[(k * 3 + 3) % INF_COUNT(sInfTier3)];
    default: return sInfBoss[k % INF_COUNT(sInfBoss)];
    }
}

/* Health (%) and CPU level of a fighter that comes in around `wave`. */
static f32 CoopInf_Health(s32 wave, s32 boss) {
    s32 hp = 100 + 12 * wave + (boss ? 150 : 0);

    if (getenv("BT4_RENDER_TEST") && getenv("BT4_COOP_INF_WEAK")) return 1.0f; /* (test only: rivals that fall at once) */
    return (f32)(hp > 600 ? 600 : hp);
}
static s32 CoopInf_Level(s32 boss) { return boss ? 21 : 13; } /* (difficult for the bosses, normal for the rest) */

/* The battle's set-up (src/battle/battle_work.c, TagBattle_ConfigureLocal): `foes` rivals at once, 0 = off. */
void CoopInf_Configure(s32 foes) {
    s32 i;

    sInfFoes = foes;
    sInfDefeated = 0;
    for (i = 0; i < 10; i++) sInfRival[i] = -1;
}
void CoopInf_End(void) { sInfFoes = 0; }
s32 CoopInf_Active(void) { return sInfFoes; }
s32 CoopInf_Wave(void) { return sInfFoes ? sInfDefeated / sInfFoes : 0; } /* waves beaten */
s32 CoopInf_Defeated(void) { return sInfDefeated; }
s32 CoopInf_TeamSize(void) { return INF_TEAM; }

/* Rival k's team (k-th of the rivals' team), into battle side `side`: the caller sets the side to INF_TEAM members. */
void CoopInf_SetupRival(s32 side, s32 k) {
    s32 j;

    if (side >= 0 && side < 10) sInfRival[side] = k;
    for (j = 0; j < INF_TEAM; j++) {
        s32 boss = j == INF_TEAM - 1;
        BattleSetup_SetMember(side, j, CoopInf_Chara(k, j), 0, 0, CoopInf_Level(boss), CoopInf_Health(j, boss), NULL);
    }
}
s32 CoopInf_FirstChara(s32 k) { return CoopInf_Chara(k, 0); }

/* A member fell (BtlMember_Damage): a rival's. The one that fell before it (now out of the fight, not the one that just
   fell: that one is still the active member) goes back to the end of the line, stronger. */
void CoopInf_OnKO(BtlMemberChr *chr) {
    BattleMember m;
    s32 i, slot = -1, boss;

    if (sInfFoes <= 0 || TagBattle_Team(chr->side) != 1 || chr->side < 0 || chr->side >= 10 || sInfRival[chr->side] < 0) return;
    sInfDefeated++;
    for (i = 0; i < INF_TEAM && i < chr->memberCount; i++) /* (the slot BtlMember_Replace takes) */
        if (i != chr->active && BtlMember_GetGauge(chr, i)->health <= 0) { slot = i; break; }
    if (slot < 0) return;
    boss = slot == INF_TEAM - 1;
    BtlMember_Init(&m, CoopInf_Chara(sInfRival[chr->side], slot), 0, 0, CoopInf_Level(boss),
                   CoopInf_Health(CoopInf_Wave() + INF_TEAM - 1, boss), NULL);
    i = BtlMember_Replace(chr, (BtlMemberSetup *)&m);
    if (getenv("BT4_RENDER_TEST"))
        fprintf(stderr, "coop inf: rival %d fell (%d down, wave %d); slot %d (chara %d) back in line -> %d\n", chr->side,
                sInfDefeated, CoopInf_Wave() + 1, slot, BtlMember_Get(chr, slot)->chara, i);
}

/* Damage to the players grows with the waves: 7% more each, up to three times. */
s32 CoopInf_ScaleDamage(BtlMemberChr *chr, s32 amount) {
    s32 pct;

    if (sInfFoes <= 0 || amount <= 0 || TagBattle_Team(chr->side) != 0) return amount;
    pct = 100 + 7 * CoopInf_Wave();
    if (pct > 300) pct = 300;
    return amount * pct / 100;
}

/* ------------------------------------------------------------------------------------------------------------------
 * Reviving (every co-op battle, story and endless): a player that falls stays down; the partner comes close and holds
 * guard for three seconds to bring them back with 40% health. Each player's side has a second member, the same
 * fighter, set up with no health (so the battle does not bring it in by itself, and a side with both down is out);
 * reviving gives that member its health back, and the battle brings it in as a team battle brings in the next
 * member: the fallen one gets up. All from the fighters' positions and their buttons, which online play shares.
 * ---------------------------------------------------------------------------------------------------------------- */
#define REVIVE_TICKS 90     /* held this long (the battle's 30 steps a second): three seconds */
#define REVIVE_RANGE 120.0f /* this close (the battle's units: the fighters start some 40 apart) */
#define REVIVE_HEALTH 40.0f /* %, of the fighter's own maximum */
#define REVIVE_WARP_TICKS 30 /* guard held this long far away: instant transmission next to the partner */

extern s32 BtlCharApi_GetHp(s32 actor);
extern void BtlCharApi_GetPos(s32 objId, f32 *out);
extern s32 BattleSide_GetObjId(s32 side);
extern void CoopRevive_Warp(s32 who, s32 to, f32 gap);
/* guard held (the test variable: as if held, a CPU partner never holds it) */
#define REV_GUARD(h) (BtlInput_IsHeld(BtlChar_Get(h), 1 /* BTLB_GUARD */) || (getenv("BT4_RENDER_TEST") && getenv("BT4_COOP_REVIVE_AUTO")))
extern f32 CoopRevive_Delta(s32 from, s32 to, s32 axis), CoopRevive_Yaw(s32 who);
extern s32 BtlInput_IsHeld(BtlMemberChr *chr, u32 mask);
extern s32 BtlMember_CountAlive(BtlMemberChr *chr);
extern BtlMemberChr *BtlChar_Get(s32 side);
extern s32 TagBattle_Count(void);

static s32 sRevOn;           /* reviving is on in this battle */
static s32 sRevHelper[10];  /* the partner reviving this player */
static s32 sLvOn, sLvLevel[10], sLvDealt[10], sLvRevives[10], sLvDowns[10], sLvRivalDamage, sLvAttacker = -1;
s32 CoopLv_Level(s32 actor);
s32 CoopLv_HealthPct(s32 level);
static s32 sRevChara[10];    /* each player's fighter as set up (-1: no reviving for that side) */
static s32 sRevCostume[10];
static s32 sRevProgress[10]; /* how long a partner has held guard next to this fallen player */
static s32 sRevGranted[10];  /* a revive gave the member behind its health: not taken back until it comes in */
static s32 sRevActive[10];   /* the member fighting last step (a change: the revived one came in) */
static s32 sRevWarp[10];     /* how long a partner far away has held guard: a second, and it appears beside */
static s32 sRevNeed[10];     /* how long this fallen player's revive takes (its helper's level reward) */
static s32 sLvFoePct = 100;  /* the chapter's difficulty: the rivals' damage, % */
static s32 sBossPct[10][6], sBossInit[10][6], sBossFlash; /* bosses that transform (CoopBoss_Set) */
/* The level rewards of "Mi personaje" (port/src/coop_story.c lists them): level 10 revives a partner in two seconds,
   25 gets up with 60% health, 40 appears beside a fallen partner in half a second, 60 takes 10% less damage, 80
   revives from half as far again. */
static s32 Rev_Lv(s32 a) { return sLvOn && a >= 0 && a < 10 ? sLvLevel[a] : 0; }
static s32 Rev_Ticks(s32 h) { return Rev_Lv(h) >= 10 ? 60 : REVIVE_TICKS; }
static f32 Rev_Range(s32 h) { return Rev_Lv(h) >= 80 ? REVIVE_RANGE * 1.5f : REVIVE_RANGE; }
static s32 Rev_WarpTicks(s32 h) { return Rev_Lv(h) >= 40 ? 15 : REVIVE_WARP_TICKS; }
static f32 Rev_Health(s32 d) { return Rev_Lv(d) >= 25 ? 60.0f : REVIVE_HEALTH; }

void CoopRevive_Configure(s32 on) {
    s32 i;

    sRevOn = on;
    for (i = 0; i < 10; i++) { sRevChara[i] = -1; sRevCostume[i] = 0; sRevProgress[i] = 0; sRevGranted[i] = 0; sRevActive[i] = 0; sRevWarp[i] = 0; sRevHelper[i] = -1; sRevNeed[i] = REVIVE_TICKS; }
}
s32 CoopRevive_Active(void) { return sRevOn; }
extern s32 TagBattle_Active(void);
void CoopRevive_SetupPlayer(s32 side, s32 chara, s32 costume, s32 level) {
    if (side < 0 || side >= 10) return;
    sRevChara[side] = chara;
    sRevCostume[side] = costume;
    BattleSetup_SetMember(side, 1, chara, costume, 0, level, 0.0f, NULL); /* (down from the start: only a revive raises it) */
}

/* Every battle step (src/battle/battle.c): the fallen players and the partners next to them. */
void CoopRevive_Update(void) {
    s32 d, h, n = TagBattle_Count();

    if (sBossFlash > 0) sBossFlash--;
    if (!sRevOn || !TagBattle_Active()) return;
    for (d = 0; d < n && d < 10; d++) {
        BtlMemberChr *down = BtlChar_Get(d);
        f32 dx, dz, dy;
        s32 helped = 0;

        if (sRevChara[d] < 0 || TagBattle_Team(d) != 0) continue;
        if (down->active != sRevActive[d]) { sRevActive[d] = down->active; sRevGranted[d] = 0; } /* (the revived one is in) */
        if (!sRevGranted[d] && down->memberCount > 1) /* the member behind stays down until a revive (the set-up and a
                                                         battle's start give it some health) */
            BtlMember_GetGauge(down, down->active ? 0 : 1)->health = 0;
        if (BtlCharApi_GetHp(d) > 0 || BtlMember_CountAlive(down) > 0) { sRevProgress[d] = 0; continue; }
        for (h = 0; h < n && !helped; h++) {
            if (h == d || TagBattle_Team(h) != 0 || BtlCharApi_GetHp(h) <= 0) continue;
            dx = CoopRevive_Delta(d, h, 0); dy = CoopRevive_Delta(d, h, 1); dz = CoopRevive_Delta(d, h, 2);
            if (dx * dx + dy * dy + dz * dz > Rev_Range(h) * Rev_Range(h) && REV_GUARD(h)) {
                if (++sRevWarp[d] >= Rev_WarpTicks(h)) { /* instant transmission next to the partner */
                    CoopRevive_Warp(h, d, REVIVE_RANGE * 0.3f);
                    sRevWarp[d] = 0;
                    fprintf(stderr, "coop: player %d appeared next to %d\n", h, d);
                }
                continue;
            }
            if (dx * dx + dy * dy + dz * dz <= Rev_Range(h) * Rev_Range(h) && REV_GUARD(h))
                helped = 1;
        }
        if (helped) { sRevHelper[d] = h - 1; sRevNeed[d] = Rev_Ticks(h - 1); } /* (the loop stepped past the helper) */
        {   extern s32 gPortResim;
            static s32 logTick;
            if (!gPortResim && ++logTick % 30 == 0) /* (a line a second: what the revive sees) */
                for (h = 0; h < n; h++) {
                    f32 x, y, z;
                    if (h == d || TagBattle_Team(h) != 0) continue;
                    x = CoopRevive_Delta(d, h, 0); y = CoopRevive_Delta(d, h, 1); z = CoopRevive_Delta(d, h, 2);
                    fprintf(stderr, "coop revive: down=%d progress=%d helper=%d hp=%d dist=%d guard=%d range=%d\n", d,
                            sRevProgress[d], h, BtlCharApi_GetHp(h), (s32)__builtin_sqrtf(x * x + y * y + z * z),
                            BtlInput_IsHeld(BtlChar_Get(h), 1), (s32)REVIVE_RANGE); /* (whole numbers: floats print wrong here) */
                }
        }
        if (helped) sRevProgress[d]++;
        else if (sRevProgress[d] > 0) sRevProgress[d] -= sRevProgress[d] > 1 ? 2 : 1; /* (it fades when let go) */
        if (sRevProgress[d] >= sRevNeed[d]) {
            BattleMember m;
            BtlMember_Init(&m, sRevChara[d], sRevCostume[d], 0, 0, Rev_Health(d) * CoopLv_HealthPct(CoopLv_Level(d)) / 100, NULL);
            h = BtlMember_Replace(down, (BtlMemberSetup *)&m);
            sRevProgress[d] = 0;
            sRevGranted[d] = h >= 0;
            if (h >= 0 && sRevHelper[d] >= 0 && sRevHelper[d] < 10) sLvRevives[sRevHelper[d]]++;
            fprintf(stderr, "coop: player %d revived (slot %d)\n", d, h);
        }
    }
}

/* A player of the team that is down and can be revived: the partner may lock on to it (src/battle/battle_work.c,
   TagBattle_Target) to find it. */
s32 CoopRevive_IsDown(s32 d) {
    if (!sRevOn || !TagBattle_Active() || d < 0 || d >= 10 || d >= TagBattle_Count() || sRevChara[d] < 0) return 0;
    return BtlCharApi_GetHp(d) <= 0 && BtlMember_CountAlive(BtlChar_Get(d)) <= 0;
}

/* For the HUD (mods: game.coop_down, game.coop_revive): a player of the team is down (and can be revived); the
   furthest revive under way, 0..1. */
/* From fighter `from`: the nearest fallen partner's distance (in the battle's units) and its bearing (radians, 0 ahead
   of the fighter's camera, positive to the right); 0 without one. */
s32 CoopRevive_Guide(s32 from, f32 *dist, f32 *bearing) {
    extern f32 sqrtf(f32), atan2f(f32, f32);
    f32 best = -1.0f, dx, dz, d, ang;
    s32 i;

    if (!sRevOn || !TagBattle_Active() || from < 0 || from >= TagBattle_Count() || BtlCharApi_GetHp(from) <= 0) return 0;
    for (i = 0; i < TagBattle_Count() && i < 10; i++) {
        if (i == from || !CoopRevive_IsDown(i)) continue;
        dx = CoopRevive_Delta(from, i, 0); dz = CoopRevive_Delta(from, i, 2);
        d = sqrtf(dx * dx + dz * dz);
        if (best >= 0.0f && d >= best) continue;
        best = d;
        ang = atan2f(dx, dz) - CoopRevive_Yaw(from); /* (from where the fighter faces: its opponent, the camera's way) */
        while (ang > 3.14159265f) ang -= 6.2831853f;
        while (ang < -3.14159265f) ang += 6.2831853f;
        *bearing = ang;
    }
    if (best < 0.0f) return 0;
    *dist = best;
    return 1;
}
s32 CoopRevive_Range(void) { return (s32)REVIVE_RANGE; }
/* The furthest revive under way, in thousandths (a whole number: a float returned to the port's code arrives wrong). */
s32 CoopRevive_ProgressMille(void) {
    s32 d, best = 0;

    for (d = 0; d < 10; d++) {
        s32 m = sRevNeed[d] > 0 ? sRevProgress[d] * 1000 / sRevNeed[d] : 0;
        if (m > best) best = m;
    }
    best = sRevOn ? best : 0;
    return best > 1000 ? 1000 : best;
}

s32 CoopRevive_AnyDown(void) {
    s32 d;

    if (!sRevOn || !TagBattle_Active()) return 0;
    for (d = 0; d < TagBattle_Count() && d < 10; d++)
        if (sRevChara[d] >= 0 && BtlCharApi_GetHp(d) <= 0 && BtlMember_CountAlive(BtlChar_Get(d)) <= 0) return 1;
    return 0;
}
f32 CoopRevive_Progress(void) {
    s32 d, best = 0;

    for (d = 0; d < 10; d++) {
        s32 m = sRevNeed[d] > 0 ? sRevProgress[d] * 1000 / sRevNeed[d] : 0;
        if (m > best) best = m;
    }
    return sRevOn ? (f32)(best > 1000 ? 1000 : best) / 1000.0f : 0.0f;
}
/* ------------------------------------------------------------------------------------------------------------------
 * Levels (every co-op battle): each player's fighter is their own copy of the character, from level 1 to 100
 * (port/src/coop_story.c keeps the experience). Level 1 is weaker than the original and level 100 stronger: health
 * 75%..125%, the damage it deals 75%..130% and the damage it takes 125%..80% (the original at about level 45). Both
 * machines know both levels before the battle (port/src/gs/tag_team_net.c), so the battle stays the same on both.
 * The battle also counts what each player does, for the experience: the damage dealt to the rivals, the partners
 * revived and the times fallen.
 * ---------------------------------------------------------------------------------------------------------------- */

static s32 CoopLv_Lerp(s32 level, s32 at1, s32 at100) {
    if (level < 1) level = 1;
    if (level > 100) level = 100;
    return at1 + (at100 - at1) * (level - 1) / 99;
}
s32 CoopLv_HealthPct(s32 level) { return level > 0 ? CoopLv_Lerp(level, 75, 125) : 100; }
static s32 CoopLv_AttackPct(s32 level) { return level > 0 ? CoopLv_Lerp(level, 75, 130) : 100; }
static s32 CoopLv_TakenPct(s32 level) { return level > 0 ? CoopLv_Lerp(level, 125, 80) : 100; }

void CoopLv_Configure(s32 on) {
    s32 i;

    sLvOn = on;
    sLvRivalDamage = 0;
    sLvAttacker = -1;
    sLvFoePct = 100;
    memset(sBossPct, 0, sizeof(sBossPct)); memset(sBossInit, 0, sizeof(sBossInit)); sBossFlash = 0;
    for (i = 0; i < 10; i++) { sLvLevel[i] = 0; sLvDealt[i] = 0; sLvRevives[i] = 0; sLvDowns[i] = 0; }
}
void CoopLv_SetLevel(s32 actor, s32 level) { if (actor >= 0 && actor < 10) sLvLevel[actor] = level < 0 ? 0 : level > 100 ? 100 : level; }
s32 CoopLv_Active(void) { return sLvOn && TagBattle_Active(); }
/* the chapter's difficulty (src/battle/battle_work.c): the rivals' damage to the players, % */
/* A rival's member that transforms (port/src/coop_story.c, "transforma"): below `pct` % of the health it started
   with, the hit that takes it there finishes it, and its next form, the member behind, comes in at once. */
void CoopBoss_Set(s32 actor, s32 member, s32 pct) {
    if (actor >= 0 && actor < 10 && member >= 0 && member < 6) sBossPct[actor][member] = pct;
}
/* For the HUD (port/src/gs/mods.c): steps left of the "transforms" notice. */
s32 CoopBoss_Flash(void) { return sBossFlash; }
void CoopLv_SetFoeDamage(s32 pct) { sLvFoePct = pct < 50 ? 50 : pct > 300 ? 300 : pct; }
s32 CoopLv_Level(s32 actor) { return sLvOn && actor >= 0 && actor < 10 ? sLvLevel[actor] : 0; }
/* who strikes, around one hit (src/battle/btl_char_hit.c, btl_char_coll_b.c): set and cleared within the same call */
s32 CoopLv_SetAttacker(s32 actor) { s32 before = sLvAttacker; sLvAttacker = actor; return before; }

/* Every health loss (src/battle/btl_char_member.c, BtlMember_Damage). */
s32 CoopLv_ScaleDamage(BtlMemberChr *chr, s32 amount) {
    s32 v = chr->side, a = sLvAttacker;

    if (!sLvOn || amount <= 0 || v < 0 || v >= 10) return amount;
    if (a >= 0 && a < 10 && a != v && TagBattle_Team(a) != TagBattle_Team(v)) {
        if (sLvLevel[a] > 0) amount = amount * CoopLv_AttackPct(sLvLevel[a]) / 100;
        if (TagBattle_Team(v) == 1) sLvDealt[a] += amount;
        if (TagBattle_Team(a) == 1 && sLvFoePct != 100) amount = amount * sLvFoePct / 100; /* (the difficulty) */
    }
    if (sLvLevel[v] > 0) amount = amount * CoopLv_TakenPct(sLvLevel[v]) / 100;
    if (sLvLevel[v] >= 60) amount = amount * 90 / 100; /* (the level 60 reward) */
    if (TagBattle_Team(v) == 1 && chr->active >= 0 && chr->active < 6 && sBossPct[v][chr->active] > 0
        && chr->active + 1 < chr->memberCount) { /* a boss that transforms */
        s32 m = chr->active, hp = BtlMember_GetGauge(chr, m)->health;
        if (sBossInit[v][m] <= 0) sBossInit[v][m] = hp;
        if (hp > 0 && (long long)(hp - amount) * 100 < (long long)sBossInit[v][m] * sBossPct[v][m]) {
            amount = hp;
            sBossPct[v][m] = 0;
            sBossFlash = 120;
            fprintf(stderr, "coop: rival %d transforms (member %d)\n", v, m);
        }
    }
    if (amount < 1) amount = 1;
    if (TagBattle_Team(v) == 1) sLvRivalDamage += amount;
    return amount;
}
void CoopLv_OnKO(BtlMemberChr *chr) {
    if (sLvOn && chr->side >= 0 && chr->side < 10 && TagBattle_Team(chr->side) == 0) sLvDowns[chr->side]++;
}
/* For the experience (port/src/coop_story.c), whole numbers: the share of the rivals' lost health this player took
   (thousandths), the partners it revived, the times it fell. */
s32 CoopLv_DamageShareMille(s32 actor) {
    if (!sLvOn || actor < 0 || actor >= 10 || sLvRivalDamage <= 0) return 0;
    return (s32)((long long)sLvDealt[actor] * 1000 / sLvRivalDamage);
}
/* the health this player took from the rivals in this battle (the statistics, port/src/coop_story.c) */
s32 CoopLv_Dealt(s32 actor) { return sLvOn && actor >= 0 && actor < 10 ? sLvDealt[actor] : 0; }
s32 CoopLv_Revives(s32 actor) { return sLvOn && actor >= 0 && actor < 10 ? sLvRevives[actor] : 0; }
s32 CoopLv_Downs(s32 actor) { return sLvOn && actor >= 0 && actor < 10 ? sLvDowns[actor] : 0; }
#endif
