/*
 * Script mods, the game's side (the scripts themselves: port/src/gs/mods.c; docs/port/mods.md). Once per blank on the
 * game's thread this file reads what the scripts are given of a fight beyond the gauges (fight_panels.c reads those):
 * where each fighter is. And it is where the camera hook in src/battle/btl_cam.c (BtlCam_UpdateView) comes in: the
 * pose the game gives the battle view each frame is handed to the scripts' on_camera and the pose they return is
 * the one built and drawn.
 *
 * That pose is only drawn: nothing of the fight reads it back (include/battle/btl_char_cam.h: only
 * BtlCharApi_GetCamPose reads ChrCam.pos / rot; stick movement goes by ChrCam.yaw), and the game builds the view
 * from the fighter camera again at the start of the next frame. The other readers of the view are drawing, effects
 * and sound (include/battle/btl_cam.h). Blanks run again after a rollback, and self-check runs (BT3_SYNCTEST), get
 * the game's pose as it is. Cuts (techniques, transformations), fixed and frozen cameras are not handed over.
 *
 * Nothing here draws a game random number or writes anything but that view. This file's variables are not game
 * state (port/tools/make_state.py).
 */
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "gs/mods.h"
#include "port_addr.h"

#if (defined(__x86_64__) || defined(__aarch64__)) && defined(__clang__)
#undef __ptr32 /* the Windows headers of mingw define these two away */
#undef __uptr
#define GAME_PTR *__ptr32 __uptr
#else
#define GAME_PTR *
#endif

extern int gPortResim; /* gs/state.c: blanks being run again */
extern int gsSkipFrame; /* gs_draw.c: a presentation frame deliberately discarded */
extern void GAME_PTR gBtlChars;
extern void GAME_PTR gBtlSeq;
extern void GAME_PTR BtlChar_FindByObjId(int objId);
extern void GAME_PTR BtlChar_FindBySide(int side);
extern int BtlChar_TestFlag(void GAME_PTR chr, unsigned n);
extern int ChrCam_IsCutActive(void GAME_PTR chr);
extern int Battle_IsSplitScreen(void);
extern int TagBattle_Active(void),TagBattle_Count(void),TagBattle_ViewActor(void),TagBattle_Target(int actor);
extern int BattleSide_GetObjId(int side);
extern int PortSpectator_Mode(void);

int PortMods_ViewActor(int side) {
    int own,target,count;
    if(!TagBattle_Active())return side;
    count=TagBattle_Count();own=TagBattle_ViewActor();
    if(own<0 || own>=count)own=0;
    target=TagBattle_Target(own);
    return side==0?own:target>=0&&target<count?target:own;
}

#define CHR_POSE 0x10        /* include/battle/btl_act_a.h: the pose block; pos at +0x00 */
#define POSE_YAW 0x94        /* ... heading yaw */
#define CHR_CAM_TARGET 0x450 /* include/battle/btl_char_cam.h: ChrCam target */

static ModsGame sGame;
static ModsCam sCam;
static int sCamOut;
/* Presentation-only, one last requested pose per view, not a frame history. */
static struct { int valid,obj; ModsCam pose; } sPausedCamera[2];
struct NaturalPose { int valid,obj; float pos[3],rot[3],anchor[3]; };
static struct NaturalPose sNatural[10];
int PortMods_InBattle(void) { return sGame.battle; }

/* Host presentation preference, never included in gameplay snapshots. */
int Port_SpecialMoveCameras(int change) {
    extern int Port_Setting(const char *,int);
    extern void Port_SettingSave(const char *,int),Port_SettingsWrite(void);
    static int enabled=-1;
    if(enabled<0)enabled=Port_Setting("special_move_cameras",1)!=0;
    if(change>=0){enabled=change!=0;Port_SettingSave("special_move_cameras",enabled);Port_SettingsWrite();}
    return enabled;
}

static void rdf(float *out, uint8_t GAME_PTR p, int off, int n) {
    int i;
    uint32_t v;

    for (i = 0; i < n; i++) { /* (the bits as they are: no float operation in this software-float file) */
        v = *(uint32_t GAME_PTR)(p + off + i * 4);
        memcpy(&out[i], &v, 4);
    }
}

/* n floats at a game address (Port_Ptr) <-> here, as bits */
static void rda(float *out, uint32_t addr, int n) {
    memcpy(out, Port_Ptr(addr), (size_t)n * 4);
}
static void wra(uint32_t addr, const float *in, int n) {
    memcpy(Port_Ptr(addr), in, (size_t)n * 4);
}

/* Keep a regular chase framing during special-move cuts. The cached pose and
   its anchor are presentation data; no fighter camera or input is changed. */
void Port_NaturalFightCamera(int objId,int pos,int rot) {
    extern int BtlAct_GetCurrent(void GAME_PTR);
    uint8_t GAME_PTR chr=(uint8_t GAME_PTR)BtlChar_FindByObjId(objId);
    float anchor[3],p[3],r[3];int action,cut,i,slot=objId>=0?objId%10:0;
    struct NaturalPose *s=&sNatural[slot];
    if(gPortResim)return;
    if(!chr)return;
    action=BtlAct_GetCurrent(chr);
    cut=ChrCam_IsCutActive(chr)||(action>=0xFD && action<=0x13B);
    rdf(anchor,chr,CHR_POSE,3);rda(p,(uint32_t)pos,3);rda(r,(uint32_t)rot,3);
    if(!cut){s->valid=1;s->obj=objId;memcpy(s->pos,p,sizeof(p));memcpy(s->rot,r,sizeof(r));memcpy(s->anchor,anchor,sizeof(anchor));}
    else if(!Port_SpecialMoveCameras(-1)&&s->valid&&s->obj==objId){
        for(i=0;i<3;++i)p[i]=s->pos[i]+anchor[i]-s->anchor[i];
        wra((uint32_t)pos,p,3);wra((uint32_t)rot,s->rot,3);
    }
}

/* Defence trainer: what script mods are told about the defence windows: [0] Z counter (fighter +0x1070), [1] circle
   evasion of techniques (+0x106C), [2] of melee hits (+0x1068), [3] of rushing attacks (+0x107C). Fed by observers in
   src/battle/btl_char_move.c (BtlMove_UpdateDefenseTimers), btl_char_hit.c (BtlHit_CheckGuard, BtlHit_CheckDodge,
   BtlHit_CheckRush) and btl_char_coll_b.c (BtlColl_TryDodge); they only read. Not game state. */
#define TRAIN_FIGHTERS 10
#define TRAIN_KINDS 4
static struct ModsDefence sTrain[TRAIN_FIGHTERS][TRAIN_KINDS];
static int sTrainLast[TRAIN_FIGHTERS][TRAIN_KINDS], sTrainSeen[TRAIN_FIGHTERS], sTrainUnlocked[TRAIN_FIGHTERS];

static void train_timer(struct ModsDefence *d, int *last, int now, int seen) {
    /* the timers only count down; a jump up is a press that opened the window, -1 not reached from 0 a press that
       closed it again (src/battle/btl_char_move.c) */
    if (seen && now > *last && now > 0) {
        d->press_seq++; d->press_tick = d->tick; d->press_locked = 0; d->press_window = now;
    } else if (seen && now == -1 && *last != 0) {
        d->press_seq++; d->press_tick = d->tick; d->press_locked = 1; d->press_window = 0;
    }
    *last = now;
}

void PortTrain_Timers(int side, int zcounter, int guard, int dodge, int rush, int unlocked) {
    int k, now[TRAIN_KINDS];
    if (gPortResim || side < 0 || side >= TRAIN_FIGHTERS) return;
    now[0] = zcounter; now[1] = guard; now[2] = dodge; now[3] = rush;
    sTrain[side][0].tick++;
    for (k = 0; k < TRAIN_KINDS; k++) {
        sTrain[side][k].tick = sTrain[side][0].tick;
        train_timer(&sTrain[side][k], &sTrainLast[side][k], now[k], sTrainSeen[side]);
    }
    /* circle pressed without lock-on (the game ignores it): told on the circle kinds, once per press */
    if (unlocked && !sTrainUnlocked[side]) {
        for (k = 1; k < TRAIN_KINDS; k++) {
            sTrain[side][k].unlocked_seq++; sTrain[side][k].unlocked_tick = sTrain[side][k].tick;
        }
    }
    sTrainUnlocked[side] = unlocked;
    sTrainSeen[side] = 1;
}

void PortTrain_HitEx(int side, int kind, int eligible, int timer, int noKi) {
    struct ModsDefence *d;
    if (gPortResim || side < 0 || side >= TRAIN_FIGHTERS || kind < 0 || kind >= TRAIN_KINDS) return;
    d = &sTrain[side][kind];
    /* one record per tick (the Z Counter fix may ask again in the same tick); a rushing attack touches for several
       ticks: one record per contact */
    if (d->hit_seq && (d->hit_tick == d->tick || (kind == 3 && d->tick - d->hit_tick < 10))) {
        if (timer > 0 && eligible) { d->hit_ok = 1; d->hit_eligible = 1; }
        return;
    }
    d->hit_seq++; d->hit_tick = d->tick; d->hit_eligible = eligible; d->hit_ok = timer > 0; d->hit_noki = noKi;
}

void PortTrain_Hit(int side, int kind, int eligible, int timer) {
    PortTrain_HitEx(side, kind, eligible, timer, 0);
}

/* Defence drill (F1 > Tech): the CPU stops choosing its own buttons and attacks without a break, so that the player
   can practise evading with circle. Offline duels only (never online, in Tag Team or in a replay): set when a fight
   starts (src/battle/btl_char_mgr.c) and applied where the AI hands its buttons to its fighter
   (src/battle/btl_char_api.c, BtlCharApi_SetInjectedInput). Its random choices use its own generator, not the game's.
   Settings: train_drill (0/1), train_drill_mix (0 melee, 1 melee and techniques, 2 techniques only),
   train_drill_rest (ticks of pause between attacks, 10..90). */
#define DRILL_SIDES 10
#define DRILL_STEPS 24
#define B_DASH 0x2
#define B_BLAST 0x4
#define B_RUSH 0x8
#define B_CHARGE 0x200
#define B_LOCKON 0x4000
static int sDrillOn, sDrillMix, sDrillRest, sDrillForced;
/* Advanced Training (src/menu/port_advtrain.c) turns the drill on for its fights; the main menu turns it off. */
void PortDrill_Force(int on) { sDrillForced = on != 0; }
int PortDrill_Forced(void) { return sDrillForced; }
static struct Drill {
    int len, pos, left;            /* the attack being sent: step pos of len, ticks left of it */
    int rest, sinceTech;
    unsigned rng;
    struct { unsigned buttons; int ticks; } step[DRILL_STEPS];
} sDrill[DRILL_SIDES];

void PortDrill_BeginMatch(int offline) {
    extern int Port_Setting(const char *, int);
    int i;
    sDrillOn = offline && (sDrillForced || Port_Setting("train_drill", 0) != 0);
    sDrillMix = Port_Setting("train_drill_mix", 0);
    sDrillRest = Port_Setting("train_drill_rest", 30);
    if (sDrillRest < 10) sDrillRest = 10;
    if (sDrillRest > 90) sDrillRest = 90;
    memset(sDrill, 0, sizeof(sDrill));
    for (i = 0; i < DRILL_SIDES; i++) {
        sDrill[i].rng = 0x9E3779B9u ^ (unsigned)(i * 7919);
        sDrill[i].rest = 45;
    }
}

int PortDrill_Active(void) { return sDrillOn; }

static unsigned drill_rand(struct Drill *d, unsigned n) {
    d->rng = d->rng * 1664525u + 1013904223u;
    return (d->rng >> 16) % n;
}

static void drill_add(struct Drill *d, unsigned buttons, int ticks) {
    if (d->len < DRILL_STEPS) { d->step[d->len].buttons = buttons; d->step[d->len].ticks = ticks; d->len++; }
}

static void drill_presses(struct Drill *d, unsigned button, int count) {
    int i;
    for (i = 0; i < count; i++) { drill_add(d, button, 1); drill_add(d, 0, 5); }
}

static void drill_technique(struct Drill *d) {
    drill_add(d, B_CHARGE, 3); drill_add(d, B_CHARGE | B_BLAST, 2); drill_add(d, B_CHARGE, 2); drill_add(d, 0, 30);
}

/* Picks the next attack. */
static void drill_pick(struct Drill *d, int close) {
    unsigned r = drill_rand(d, 10);
    d->len = d->pos = 0;
    if (sDrillMix == 2 || (sDrillMix == 1 && (close ? r == 9 : d->sinceTech > 90))) {
        drill_technique(d); d->sinceTech = 0;
    } else if (r <= 3) {
        drill_presses(d, B_RUSH, 1 + (int)drill_rand(d, 5));                       /* rush of 1..5 hits */
    } else if (r <= 5) {
        drill_presses(d, B_RUSH, 1 + (int)drill_rand(d, 4));                       /* rush and a triangle finisher */
        drill_presses(d, B_BLAST, 1);
    } else if (r == 6) {
        drill_add(d, B_RUSH, 22); drill_add(d, 0, 8);                              /* charged smash */
    } else if (r == 7) {
        drill_presses(d, B_RUSH, 2); drill_add(d, B_RUSH, 18); drill_add(d, 0, 8); /* two hits and a charged one */
    } else {
        drill_presses(d, B_RUSH, 5); drill_presses(d, B_BLAST, 1);                 /* full rush */
    }
    d->left = d->len > 0 ? d->step[0].ticks : 0;
}

/* Sends the current step and moves on; at the end of the attack starts the pause. */
static unsigned drill_step(struct Drill *d) {
    unsigned b = d->step[d->pos].buttons;
    if (--d->left <= 0 && ++d->pos < d->len) d->left = d->step[d->pos].ticks;
    if (d->pos >= d->len) d->rest = sDrillRest + (int)drill_rand(d, (unsigned)sDrillRest);
    return b;
}

/* The buttons the CPU sends this tick while the drill is on. close: the fighters are in close range (flag 0x13). */
void PortDrill_Input(int objId, int close, unsigned *buttons, float *x, float *y) {
    struct Drill *d;
    unsigned out = B_LOCKON;
    if (!sDrillOn || objId < 0 || objId >= DRILL_SIDES) return;
    d = &sDrill[objId];
    d->sinceTech++;
    if (d->pos < d->len) {                                   /* sending an attack */
        out |= drill_step(d);
    } else if (!close && sDrillMix != 2 && !(sDrillMix == 1 && d->sinceTech > 90)) {
        out |= B_DASH;                                       /* far: dash at the opponent */
        if (d->rest > 8) d->rest = 8;
    } else if (--d->rest <= 0) {                             /* the pause is over: next attack */
        drill_pick(d, close);
        if (d->len > 0) out |= drill_step(d);
    }
    *buttons = out;
    *x = 0.0f;
    *y = 0.0f;
}

void PortMods_GameTick(void) {
    int side;
    uint8_t GAME_PTR chr;

    if (gPortResim) {
        return;
    }
    memset(&sGame, 0, sizeof(sGame));
    if (gBtlChars != NULL && gBtlSeq != NULL) {
        for (side = 0; side < 2; side++) {
            chr = (uint8_t GAME_PTR)BtlChar_FindBySide(PortMods_ViewActor(side));
            if (chr != NULL) {
                sGame.f[side].found = 1;
                rdf(sGame.f[side].pos, chr, CHR_POSE, 3);
                rdf(&sGame.f[side].yaw, chr, CHR_POSE + POSE_YAW, 1);
                {
                    int actor = PortMods_ViewActor(side);
                    if (actor >= 0 && actor < TRAIN_FIGHTERS)
                        memcpy(sGame.f[side].def, sTrain[actor], sizeof(sGame.f[side].def));
                }
            }
        }
        sGame.battle = sGame.f[0].found && sGame.f[1].found;
        sGame.split = sGame.battle && Battle_IsSplitScreen();
    } else { memset(sNatural,0,sizeof(sNatural)); memset(sPausedCamera,0,sizeof(sPausedCamera));
             memset(sTrain,0,sizeof(sTrain)); memset(sTrainSeen,0,sizeof(sTrainSeen)); memset(sTrainUnlocked,0,sizeof(sTrainUnlocked)); }
    Mods_Tick();
}

int PortMods_Read(ModsGame *out) {
    memcpy(out, &sGame, sizeof(*out));
    return sGame.battle;
}

int PortMods_CamGet(ModsCam *out) {
    memcpy(out, &sCam, sizeof(*out));
    return 1;
}

void PortMods_CamSet(const ModsCam *cam) {
    memcpy(sCam.pos, cam->pos, sizeof(sCam.pos));
    memcpy(sCam.rot, cam->rot, sizeof(sCam.rot));
    sCam.zoom=cam->zoom;
    sCam.collide=cam->collide;
    sCamOut = 1;
}

/* From BtlCam_UpdateView (src/battle/btl_cam.c): `pos` and `rot` are the addresses of the view's pose in game
   memory, just copied from the side's fighter camera. */
/* What the scripts asked of this side's camera besides its pose, for src/battle/btl_cam.c: the zoom (thousandths,
   1000 the game's) and whether the pose is to be kept out of the stage. The game's values when no mod changed it. */
static int sZoom[2] = {1000, 1000}, sCollide[2];
int Port_ModsCamZoom(int side) {
    return side >= 0 && side < 2 ? sZoom[side] : 1000;
}
int Port_ModsCamCollide(int side) {
    return side >= 0 && side < 2 ? sCollide[side] : 0;
}

void Port_ModsCamera(int side, int objId, int pos, int rot, int paused) {
    static int synctest = -1;
    uint8_t GAME_PTR chr;

    if (synctest < 0) {
        synctest = getenv("BT3_SYNCTEST") != NULL;
    }
    /* A render skip is chosen on the renderer thread. It must not decide
       whether the simulation builds a native or modified view: the choice
       can change before this tick's list is consumed. Only rollback skips
       presentation state. Renderer frame dropping remains independent. */
    if (gPortResim || side < 0 || side > 1) return;
    sZoom[side]=1000;sCollide[side]=0;
    if (synctest || PortSpectator_Mode() || !Mods_WantCamera()) {
        sPausedCamera[side].valid=0;
        return;
    }
    chr = (uint8_t GAME_PTR)BtlChar_FindByObjId(objId);
    if (chr == NULL) {
        return;
    }
    memset(&sCam, 0, sizeof(sCam));
    sCam.side = side;
    /* the game's own camera work: cuts, fixed and frozen cameras (fighter flags 0xCC, 0xCD, 0xDC, 0xB8) */
    sCam.cut = ChrCam_IsCutActive(chr) || BtlChar_TestFlag(chr, 0xCC) || BtlChar_TestFlag(chr, 0xDC) ||
               BtlChar_TestFlag(chr, 0xB8) || BtlChar_TestFlag(chr, 0xCD);
    rda(sCam.pos, (uint32_t)pos, 3);
    rda(sCam.rot, (uint32_t)rot, 3);
    rdf(sCam.target, chr, CHR_CAM_TARGET, 3);
    sCam.zoom = 1.0f;
    sCam.collide = 1;
    sCamOut = 0;
    if (paused && !sCam.cut && sPausedCamera[side].valid && sPausedCamera[side].obj==objId) {
        sCam=sPausedCamera[side].pose;
        sCamOut=1;
    } else {
        Mods_Camera();
        sPausedCamera[side].valid=sCamOut && !sCam.cut;
        sPausedCamera[side].obj=objId;
        sPausedCamera[side].pose=sCam;
    }
    if (sCamOut) {
        wra((uint32_t)pos, sCam.pos, 3);
        wra((uint32_t)rot, sCam.rot, 3);
        sZoom[side] = (int)(sCam.zoom * 1000.0f + 0.5f);
        sCollide[side] = sCam.collide;
    }
}
