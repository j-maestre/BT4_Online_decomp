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
#define REVIVE_RANGE 40.0f  /* this close (the battle's units: the fighters start some 40 apart) */
#define REVIVE_HEALTH 40.0f /* %, of the fighter's own maximum */

extern s32 BtlCharApi_GetHp(s32 actor);
extern void BtlCharApi_GetPos(s32 objId, f32 *out);
extern s32 BtlInput_IsHeld(BtlMemberChr *chr, u32 mask);
extern s32 BtlMember_CountAlive(BtlMemberChr *chr);
extern BtlMemberChr *BtlChar_Get(s32 side);
extern s32 TagBattle_Count(void);

static s32 sRevOn;           /* reviving is on in this battle */
static s32 sRevChara[10];    /* each player's fighter as set up (-1: no reviving for that side) */
static s32 sRevCostume[10];
static s32 sRevProgress[10]; /* how long a partner has held guard next to this fallen player */
static s32 sRevGranted[10];  /* a revive gave the member behind its health: not taken back until it comes in */
static s32 sRevActive[10];   /* the member fighting last step (a change: the revived one came in) */

void CoopRevive_Configure(s32 on) {
    s32 i;

    sRevOn = on;
    for (i = 0; i < 10; i++) { sRevChara[i] = -1; sRevCostume[i] = 0; sRevProgress[i] = 0; sRevGranted[i] = 0; sRevActive[i] = 0; }
}
s32 CoopRevive_Active(void) { return sRevOn; }
void CoopRevive_SetupPlayer(s32 side, s32 chara, s32 costume, s32 level) {
    if (side < 0 || side >= 10) return;
    sRevChara[side] = chara;
    sRevCostume[side] = costume;
    BattleSetup_SetMember(side, 1, chara, costume, 0, level, 0.0f, NULL); /* (down from the start: only a revive raises it) */
}

/* Every battle step (src/battle/battle.c): the fallen players and the partners next to them. */
void CoopRevive_Update(void) {
    s32 d, h, n = TagBattle_Count();

    if (!sRevOn) return;
    for (d = 0; d < n && d < 10; d++) {
        BtlMemberChr *down = BtlChar_Get(d);
        f32 pd[4], ph[4], dx, dz, dy;
        s32 helped = 0;

        if (sRevChara[d] < 0 || TagBattle_Team(d) != 0) continue;
        if (down->active != sRevActive[d]) { sRevActive[d] = down->active; sRevGranted[d] = 0; } /* (the revived one is in) */
        if (!sRevGranted[d] && down->memberCount > 1) /* the member behind stays down until a revive (the set-up and a
                                                         battle's start give it some health) */
            BtlMember_GetGauge(down, down->active ? 0 : 1)->health = 0;
        if (BtlCharApi_GetHp(d) > 0 || BtlMember_CountAlive(down) > 0) { sRevProgress[d] = 0; continue; }
        BtlCharApi_GetPos(d, pd);
        for (h = 0; h < n && !helped; h++) {
            if (h == d || TagBattle_Team(h) != 0 || BtlCharApi_GetHp(h) <= 0) continue;
            BtlCharApi_GetPos(h, ph);
            dx = ph[0] - pd[0]; dy = ph[1] - pd[1]; dz = ph[2] - pd[2];
            if ((dx * dx + dy * dy + dz * dz <= REVIVE_RANGE * REVIVE_RANGE && BtlInput_IsHeld(BtlChar_Get(h), 1 /* BTLB_GUARD */)) ||
                (getenv("BT4_RENDER_TEST") && getenv("BT4_COOP_REVIVE_AUTO")))
                helped = 1; /* (the test variable: any partner revives from anywhere, as a CPU partner never holds guard) */
        }
        if (helped) sRevProgress[d]++;
        else if (sRevProgress[d] > 0) sRevProgress[d] -= sRevProgress[d] > 1 ? 2 : 1; /* (it fades when let go) */
        if (sRevProgress[d] >= REVIVE_TICKS) {
            BattleMember m;
            BtlMember_Init(&m, sRevChara[d], sRevCostume[d], 0, 0, REVIVE_HEALTH, NULL);
            h = BtlMember_Replace(down, (BtlMemberSetup *)&m);
            sRevProgress[d] = 0;
            sRevGranted[d] = h >= 0;
            fprintf(stderr, "coop: player %d revived (slot %d)\n", d, h);
        }
    }
}

/* A player of the team that is down and can be revived: the partner may lock on to it (src/battle/battle_work.c,
   TagBattle_Target) to find it. */
s32 CoopRevive_IsDown(s32 d) {
    if (!sRevOn || d < 0 || d >= 10 || d >= TagBattle_Count() || sRevChara[d] < 0) return 0;
    return BtlCharApi_GetHp(d) <= 0 && BtlMember_CountAlive(BtlChar_Get(d)) <= 0;
}

/* For the HUD (mods: game.coop_down, game.coop_revive): a player of the team is down (and can be revived); the
   furthest revive under way, 0..1. */
s32 CoopRevive_AnyDown(void) {
    s32 d;

    if (!sRevOn) return 0;
    for (d = 0; d < TagBattle_Count() && d < 10; d++)
        if (sRevChara[d] >= 0 && BtlCharApi_GetHp(d) <= 0 && BtlMember_CountAlive(BtlChar_Get(d)) <= 0) return 1;
    return 0;
}
f32 CoopRevive_Progress(void) {
    s32 d, best = 0;

    for (d = 0; d < 10; d++) if (sRevProgress[d] > best) best = sRevProgress[d];
    return sRevOn ? (f32)best / REVIVE_TICKS : 0.0f;
}
#endif
