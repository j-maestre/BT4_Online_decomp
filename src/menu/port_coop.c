/*
 * PC only: "Historia cooperativa" (co-op story), a screen of the port's own reached from the main menu
 * (src/menu/menu_a.c, item MAINMENU_COOP). The chapters come from historia_coop/capitulos.lua (port/src/coop_story.c).
 * Three pages: Make a room / Join a room; the list of chapters; a chapter (its story, the rivals) with "Make the room".
 * Both leave for the online screens (src/menu/port_tbo.c, gPortTboCoop): the room is an ordinary Tag Team room whose
 * content fingerprint carries capitulos.lua; the joiner learns the chapter from the host (port/src/coop_story.c).
 * (ASCII only in this file: the build's pointer tool reads it by byte offset. Accents are octal escapes.)
 */
#include "common.h"
#include "../../port/src/gs/online_language.h"
#include "menu/menu_e.h"
#include "sys/pad.h"
#include "sys/save.h"

#ifdef PORT
#include "menu/port_screen.h"
#include "menu/tbo_art.h"

extern void MsgWin_SetText(void *text);
extern void Job_Run(void);
extern void IconWin_PortHideWords(void);
extern void MsgWin_PortDrawStringHd(u16 *str);
extern void MsgWin_PortSameTop(void);
extern void Port_MenuPage(int screen);
extern int Port_PageAspectMilli(void);

extern int PortCoop_Load(void), PortCoop_Count(void), PortCoop_Selected(void), PortCoop_Done(int i);
extern void PortCoop_Select(int i), PortCoop_JoinBegin(void), PortCoop_SelectInfinite(int foes);
extern int PortCoop_InfRecord(int foes);
extern int PortCoop_Joining(void), PortCoop_Infinite(void);
extern const char *PortCoop_Error(void);
extern const char *PortCoop_ChTitle(int i), *PortCoop_ChSaga(int i), *PortCoop_ChPair(int i), *PortCoop_ChIntro(int i);
extern const char *PortCoop_ChFoeName(int i, int k);
extern int PortCoop_ChFoeChara(int i, int k), PortCoop_ChFoeLevel(int i, int k), PortCoop_ChFoeHealth(int i, int k);
extern int PortCoop_ChFoeCount(int i), PortCoop_ChReserveCount(int i), PortCoop_Unlocked(int i);
extern const char *PortCoop_ChGrade(int i), *PortCoop_RewardLine(void);
extern s32 gPortTboCoop;
char gPortCoopNote[160]; /* why the last room could not be made or joined (src/menu/port_tbo.c), shown on the first page */

enum { CO_START, CO_LIST, CO_CHAPTER, CO_PICK, CO_MISSIONS, CO_MAP, CO_ME, CO_NAME, CO_POINTS, CO_TREE, CO_OBJ };
#define CO_ROWS 7 /* chapters on the list at once */

typedef struct CoopScreen {
    s32 page, cur, top, act, start;
    s32 frames, pageFrom;
    s32 leaving, go;
    void *winRes;
} CoopScreen;

static CoopScreen *sCo;
static void Co_PickOpen(void);
static void Co_MapOpen(void);
static s32 sCoPickBack, sCoPickTested, sCoFromMe; /* back from "Mi personaje": on its row */
static s32 sCoRoom, sCoRoomDone; /* "Mi personaje" opened from inside a room (PortCoop_RoomMe) */

/* The game's 16-bit text for the message box, from UTF-8. */
static u16 *Co_Box(const char *utf8) {
    static u16 buf[2][224];
    static char last[512]; /* (the text itself: a buffer written again each frame keeps its address) */
    static s32 cur;
    const u8 *s = (const u8 *)utf8;
    s32 n = 1;

    if (!strcmp(utf8, last)) return buf[cur];
    Ps_Copy(last, (char *)utf8, 512);
    cur = !cur;
    buf[cur][0] = 0xFEFF;
    while (*s && n < 222) {
        u32 c = *s++;
        if (c >= 0xC0 && c < 0xE0 && *s) { c = ((c & 0x1F) << 6) | (*s++ & 0x3F); }
        else if (c >= 0xE0 && s[0] && s[1]) { c = ((c & 0x0F) << 12) | ((s[0] & 0x3F) << 6) | (s[1] & 0x3F); s += 2; }
        if (c == '<') c = '(';
        buf[cur][n++] = (u16)c;
    }
    buf[cur][n] = 0;
    return buf[cur];
}

static void Co_WinInit(void) {
    void *icons;

    sCo->winRes = Sprite_Unpack(MPACK_AT(gMenuArc5, 1), NULL, NULL); /* the versus menu's windows, as Tb_WinInit */
    icons = MPACK_AT(sCo->winRes, 24);
    Res_RelocateOffsets(&icons, icons, icons);
    IconWin_Init(MPACK_AT(sCo->winRes, 23), icons);
    IconWin_PortHideWords();
    IconWin_Open();
    MsgWin_Init(MPACK_AT(sCo->winRes, 22), MPACK_AT(sCo->winRes, 21), 1, 0);
    MsgWin_SetText(NULL);
    MsgWin_PortSameTop();
    MsgWin_Open();
}

static void Co_WinTerm(void) {
    if (sCo->winRes == NULL) return;
    MsgWin_Term();
    IconWin_Term();
    Heap_Free(sCo->winRes);
    sCo->winRes = NULL;
}

static void Co_SetPage(s32 page) {
    sCo->page = page;
    sCo->pageFrom = sCo->frames;
}

static f32 Co_In(void) {
    f32 t = (f32)(sCo->frames - sCo->pageFrom) / 12.0f;
    if (t > 1.0f) t = 1.0f;
    return 1.0f - (1.0f - t) * (1.0f - t);
}

static char *Co_Level(s32 level) {
    static char *names[5] = {"muy f\303\241cil", "f\303\241cil", "normal", "dif\303\255cil", "muy dif\303\255cil"};
    return names[level < 0 ? 0 : level > 4 ? 4 : level];
}

/* ---------------------------------------------------------------------------------------------- input */
static void Co_Leave(s32 go) {
    sCo->go = go;
    sCo->leaving = 1;
    ColorFade_StartOut(0, 0, 0, 0x14);
}

static void Co_StartUpdate(void) {
    Pad *pad = &gPad[0];

    Ps_Prompt(0, "Confirmar");
    Ps_Prompt(2, "Volver");
    Ps_Move(&sCo->start, 6);
    if (pad->gamePressed & (PADG_CROSS | PADG_START)) {
        Snd_PlaySe(1, 1);
        gPortCoopNote[0] = 0;
        if (sCo->start == 5) { sCo->cur = 0; Co_SetPage(CO_ME); } /* "Mi personaje" */
        else if (sCo->start == 4) { sCo->cur = 0; sCo->act = 0; sCoFromMe = 0; Co_SetPage(CO_MISSIONS); }
        else if (sCo->start == 0) Co_MapOpen();
        else if (sCo->start == 1) { PortCoop_JoinBegin(); gPortTboCoop = 2; Co_Leave(1); }
        else { PortCoop_SelectInfinite(sCo->start == 2 ? 2 : 3); gPortTboCoop = 1; Co_Leave(1); }
    } else if (pad->gamePressed & PADG_TRIANGLE) {
        Snd_PlaySe(1, 2);
        Co_Leave(0);
    }
}

/* "Mi personaje": the list of characters (their first form only), to choose the player's own fighter. */
extern void PortTbo_CharaName(s32 chara, char *out, s32 cap);
extern s32 PortTbo_CharaCount(void);
extern int PortRpg_MyChara(void), PortRpg_MyLevel(void);
extern void PortRpg_SetMyChara(int chara);
static s32 sPickIds[256], sPickCount;
static void Co_PickBuild(void) {
    char name[48], base[48];
    s32 c, n = PortTbo_CharaCount(), len;

    sPickCount = 0;
    base[0] = 0;
    for (c = 0; c < n && sPickCount < 256; c++) {
        PortTbo_CharaName(c, name, 48);
        if (!name[0] || name[0] == '?') continue;
        len = (s32)strlen(base);
        if (len > 0 && !strncmp(name, base, len) && name[len] == ' ') continue; /* a form of the one before ("... Super Saiyan") */
        Ps_Copy(base, name, 48);
        sPickIds[sPickCount++] = c;
    }
}
static void Co_PickUpdate(void) {
    Pad *pad = &gPad[0];

    Ps_Prompt(0, "Elegir");
    Ps_Prompt(2, "Volver");
    if (sPickCount > 0) {
        Ps_Move(&sCo->cur, sPickCount);
        if (sCo->cur < sCo->top) sCo->top = sCo->cur;
        if (sCo->cur >= sCo->top + CO_ROWS) sCo->top = sCo->cur - CO_ROWS + 1;
    }
    if ((pad->gamePressed & (PADG_CROSS | PADG_START)) && sPickCount > 0) {
        Snd_PlaySe(1, 1);
        PortRpg_SetMyChara(sPickIds[sCo->cur]);
        sCo->cur = sCo->top = 0;
        Co_SetPage(CO_START);
    } else if (pad->gamePressed & PADG_TRIANGLE) {
        Snd_PlaySe(1, 2);
        sCo->cur = sCo->top = 0;
        Co_SetPage(CO_START);
    }
}
static void Co_PickOpen(void) {
    s32 i, mine = PortRpg_MyChara();

    Co_PickBuild();
    sCo->cur = 0;
    for (i = 0; i < sPickCount; i++) if (sPickIds[i] == mine) sCo->cur = i;
    sCo->top = sCo->cur >= CO_ROWS ? sCo->cur - CO_ROWS / 2 : 0;
    if (sCo->top > sPickCount - CO_ROWS) sCo->top = sPickCount - CO_ROWS > 0 ? sPickCount - CO_ROWS : 0;
    Co_SetPage(CO_PICK);
}

/* "Misiones y estad\303\255sticas": today's and this week's missions (claimed with cross), and, left / right, the totals
   (port/src/coop_story.c keeps both). */
extern int PortMis_Goal(int k), PortMis_Progress(int k), PortMis_Claimed(int k), PortMis_Claim(int k);
extern int PortMis_HoursLeft(void), PortMis_DaysLeft(void), PortItem_Count(int which), PortStat(int k);
extern const char *PortMis_Text(int k), *PortMis_RewardText(int k), *PortStat_BestGrade(void);
/* The shop: capsules, experience and titles for the save's Z points. Items: 0 EXP capsule, 1 Z capsule, 2 an
   experience pack, then the titles that are sold. */
extern s32 PortCoop_Money(void), PortCoop_SpendMoney(s32 n);
extern void PortRpg_TitleBought(int i), PortRpg_SetTitle(int i);
extern int PortRpg_TitleCount(void), PortRpg_TitlePrice(int), PortRpg_TitleUnlocked(int);
extern const char *PortRpg_TitleName(int);
extern int PortRpg_Abilities(void);
extern void PortRpg_AbilityBought(int bit);
#define CO_SHOP_BASE 5
extern int PortAch_Count(void), PortAch_Done(int), PortAch_Claimed(int), PortAch_Claim(int), PortAch_Reward(int), PortAch_ReadyCount(void);
extern void PortAch_Progress(int i, int *now, int *goal);
extern const char *PortAch_Name(int);
extern int PortMis_Streak(void), PortMis_StreakBonus(void);
static s32 sShopTitle[16], sShopTitles;
static void Co_ShopBuild(void) {
    s32 t;
    sShopTitles = 0;
    for (t = 0; t < PortRpg_TitleCount() && sShopTitles < 16; t++) if (PortRpg_TitlePrice(t) > 0) sShopTitle[sShopTitles++] = t;
}
static s32 Co_ShopCount(void) { return CO_SHOP_BASE + sShopTitles; }
static s32 Co_ShopPrice(s32 i) { return i == 0 || i == 1 ? 4000 : i == 2 ? 6000 : i == 3 ? 25000 : i == 4 ? 35000 : PortRpg_TitlePrice(sShopTitle[i - CO_SHOP_BASE]); }
static s32 Co_ShopOwned(s32 i) { return i == 3 ? (PortRpg_Abilities() & 1) != 0 : i == 4 ? (PortRpg_Abilities() & 2) != 0 : i >= CO_SHOP_BASE && PortRpg_TitleUnlocked(sShopTitle[i - CO_SHOP_BASE]); }
static char sCoNote[160];
static s32 sCoNoteFrames;
static void Co_Note(const char *text) { Ps_Copy(sCoNote, (char *)text, 160); sCoNoteFrames = 200; }
extern const char *PortMis_LastClaim(void);
extern int PortObj_TakeGiven(void);
extern const char *PortObj_Name(int);
static void Co_ShopBuy(s32 i) {
    if (Co_ShopOwned(i) || !PortCoop_SpendMoney(Co_ShopPrice(i))) { Snd_PlaySe(1, 2); return; }
    if (i == 0) Port_SettingSave("coop_cap_exp", PortItem_Count(0) + 1);
    else if (i == 1) Port_SettingSave("coop_cap_zp", PortItem_Count(1) + 1);
    else if (i == 2) Port_SettingSave("coop_mi_xp", Port_Setting("coop_mi_xp", 0) + 250);
    else if (i == 3) PortRpg_AbilityBought(1);
    else if (i == 4) PortRpg_AbilityBought(2);
    else { PortRpg_TitleBought(sShopTitle[i - CO_SHOP_BASE]); PortRpg_SetTitle(sShopTitle[i - CO_SHOP_BASE]); }
    Port_SettingsWrite();
    Snd_PlaySe(1, 1);
    {   char t[160];
        if (i == 0 || i == 1) sprintf(t, "Comprado: c\303\241psula %s. Tienes %d.", i ? "Z" : "de EXP", PortItem_Count(i));
        else if (i == 2) sprintf(t, "+250 EXP. Mi personaje: nivel %d.", PortRpg_MyLevel());
        else if (i == 3 || i == 4) sprintf(t, "\302\241Habilidad conseguida! Ya vale en tu pr\303\263ximo combate.");
        else sprintf(t, "T\303\255tulo conseguido y puesto: %.60s", PortRpg_TitleName(sShopTitle[i - CO_SHOP_BASE]));
        Co_Note(t);
    }
}
static void Co_MissionsUpdate(void) { /* act: 0 the missions, 2 the shop, 1 the statistics (left / right in that order) */
    Pad *pad = &gPad[0];

    Ps_Prompt(0, sCo->act == 1 ? "Volver" : sCo->act == 2 ? "Comprar" : "Reclamar"); /* (the achievements: claim too) */
    Ps_Prompt(2, "Volver");
    if (pad->gamePressed & (PADG_LEFT | PADG_RIGHT | PADG_L1 | PADG_R1)) { /* the tabs */
        static const s32 next[4] = {2, 0, 3, 1}, prev[4] = {1, 3, 0, 2};
        sCo->act = (pad->gamePressed & (PADG_RIGHT | PADG_R1)) ? next[sCo->act % 4] : prev[sCo->act % 4];
        sCo->cur = sCo->top = 0; Co_ShopBuild(); Snd_PlaySe(1, 0);
    }
    if (sCo->act == 0) Ps_Move(&sCo->cur, 6);
    else if (sCo->act == 2 || sCo->act == 3) {
        s32 n = sCo->act == 2 ? Co_ShopCount() : PortAch_Count();
        Ps_Move(&sCo->cur, n);
        if (sCo->cur < sCo->top) sCo->top = sCo->cur;
        if (sCo->cur >= sCo->top + 8) sCo->top = sCo->cur - 7;
    }
    if ((pad->gamePressed & (PADG_CROSS | PADG_START)) && sCo->act == 0) {
        if (PortMis_Claim(sCo->cur)) {
            char t[200]; s32 o = PortObj_TakeGiven();
            Ps_Copy(t, (char *)PortMis_LastClaim(), 200);
            if (o >= 0) { Ps_Cat(t, " y el objeto ", 200); Ps_Cat(t, (char *)PortObj_Name(o), 200); }
            Snd_PlaySe(1, 1); Co_Note(t);
        } else Snd_PlaySe(1, 2);
    } else if ((pad->gamePressed & (PADG_CROSS | PADG_START)) && sCo->act == 2) {
        Co_ShopBuy(sCo->cur);
    } else if ((pad->gamePressed & (PADG_CROSS | PADG_START)) && sCo->act == 3) {
        if (PortAch_Claim(sCo->cur)) { char t[160]; sprintf(t, "\302\241Logro conseguido! +%d Puntos Z", PortAch_Reward(sCo->cur)); Co_Note(t); Snd_PlaySe(1, 1); }
        else Snd_PlaySe(1, 2);
    } else if ((pad->gamePressed & PADG_TRIANGLE) || ((pad->gamePressed & PADG_CROSS) && sCo->act == 1)) {
        Snd_PlaySe(1, 2);
        sCo->cur = sCoFromMe ? 6 : 0; sCo->act = 0;
        Co_SetPage(sCoFromMe ? CO_ME : CO_START);
        sCoFromMe = 0;
    }
}

/* The saga map: the chapters' sagas (consecutive chapters with the same "saga") along a path, as the story mode's
   map; a saga opens once its first chapter is unlocked, and entering it lists its chapters. */
extern int PortCoop_ChDialogCount(int i);
extern const char *PortCoop_ChDialogWho(int i, int k), *PortCoop_ChDialogText(int i, int k);
#define CO_SAGAS 16
static s32 sSagaFirst[CO_SAGAS], sSagaLen[CO_SAGAS], sSagaN, sCoSaga;
static void Co_SagasBuild(void) {
    s32 i, n = PortCoop_Count();
    sSagaN = 0;
    for (i = 0; i < n; i++) {
        if (sSagaN > 0 && !strcmp(PortCoop_ChSaga(i), PortCoop_ChSaga(sSagaFirst[sSagaN - 1]))) { sSagaLen[sSagaN - 1]++; continue; }
        if (sSagaN >= CO_SAGAS) { sSagaLen[sSagaN - 1]++; continue; }
        sSagaFirst[sSagaN] = i; sSagaLen[sSagaN] = 1; sSagaN++;
    }
}
static s32 Co_SagaOf(s32 chapter) {
    s32 s;
    for (s = 0; s < sSagaN; s++) if (chapter >= sSagaFirst[s] && chapter < sSagaFirst[s] + sSagaLen[s]) return s;
    return 0;
}
static s32 Co_SagaOpen(s32 s) { s32 i; for (i = 0; i < sSagaLen[s]; i++) if (PortCoop_Unlocked(sSagaFirst[s] + i)) return 1; return 0; }
static s32 Co_SagaDone(s32 s) { s32 i, d = 0; for (i = 0; i < sSagaLen[s]; i++) d += PortCoop_Done(sSagaFirst[s] + i) != 0; return d; }
static void Co_MapOpen(void) {
    s32 s;
    Co_SagasBuild();
    sCoSaga = 0;
    for (s = 0; s < sSagaN; s++) if (Co_SagaOpen(s) && Co_SagaDone(s) < sSagaLen[s]) { sCoSaga = s; break; }
    Co_SetPage(CO_MAP);
}
static void Co_ListOpen(s32 saga) { /* on the first chapter of the saga still to win */
    s32 i, first = sSagaFirst[saga];
    sCoSaga = saga;
    sCo->cur = first;
    for (i = 0; i < sSagaLen[saga]; i++) if (PortCoop_Unlocked(first + i) && !PortCoop_Done(first + i)) { sCo->cur = first + i; break; }
    sCo->top = sCo->cur - first >= CO_ROWS ? sCo->cur - CO_ROWS + 1 : first;
    Co_SetPage(CO_LIST);
}
static void Co_MapUpdate(void) {
    Pad *pad = &gPad[0];

    Ps_Prompt(0, "Entrar");
    Ps_Prompt(2, "Volver");
    Ps_Prompt(1, "Releer");
    if (sSagaN > 0) {
        if ((pad->gameRepeat & (PADG_RIGHT | PADG_DOWN)) && sCoSaga + 1 < sSagaN) { sCoSaga++; Snd_PlaySe(1, 0); }
        if ((pad->gameRepeat & (PADG_LEFT | PADG_UP)) && sCoSaga > 0) { sCoSaga--; Snd_PlaySe(1, 0); }
    }
    if ((pad->gamePressed & (PADG_CROSS | PADG_START)) && sSagaN > 0) {
        if (Co_SagaOpen(sCoSaga)) { Snd_PlaySe(1, 1); Co_ListOpen(sCoSaga); }
        else Snd_PlaySe(1, 2);
    } else if (pad->gamePressed & PADG_TRIANGLE) {
        Snd_PlaySe(1, 2);
        Co_SetPage(CO_START);
    } else if (pad->gamePressed & PADG_SQUARE) { /* the file read again (after editing it) */
        Snd_PlaySe(1, 0);
        PortCoop_Load();
        Co_MapOpen();
    }
}

/* "Mi personaje": who it is (the fight's select), its name, its title, its improvement points and its statistics. */
extern const char *PortRpg_Name(void), *PortRpg_TitleName(int);
extern void PortRpg_SetName(const char *name), PortRpg_SetTitle(int i), PortRpg_PointsReset(void);
extern int PortRpg_Title(void), PortRpg_TitleCount(void), PortRpg_TitleUnlocked(int), PortRpg_TitleLevel(int), PortRpg_TitlePrice(int);
extern int PortRpg_Points(int w), PortRpg_PointsFree(void), PortRpg_PointAdd(int w, int d);
static const char *const kCoKeys[5] = {"ABCDEFGHIJKLM", "NOPQRSTUVWXYZ", "abcdefghijklm", "nopqrstuvwxyz", "0123456789-. "};
#define CO_KEY_COLS 14 /* (13 letters and OK on the last row) */
static char sCoName[20];
static void Co_MeUpdate(void) {
    Pad *pad = &gPad[0];

    Ps_Prompt(0, "Elegir");
    Ps_Prompt(2, "Volver");
    Ps_Move(&sCo->cur, 7);
    if (sCo->cur == 2 && (pad->gameRepeat & (PADG_LEFT | PADG_RIGHT))) { /* the title: the next one unlocked */
        s32 n = PortRpg_TitleCount(), t = PortRpg_Title(), k;
        for (k = 1; k < n; k++) {
            s32 c = (t + ((pad->gameRepeat & PADG_RIGHT) ? k : n - k)) % n;
            if (PortRpg_TitleUnlocked(c)) { PortRpg_SetTitle(c); Snd_PlaySe(1, 0); break; }
        }
    }
    if (pad->gamePressed & (PADG_CROSS | PADG_START)) {
        Snd_PlaySe(1, 1);
        if (sCo->cur == 0 && sCoRoom) { Co_Note("El personaje se cambia fuera de la sala (aqu\303\255 s\303\255: nombre, t\303\255tulo, mejoras, \303\241rbol y objetos)."); Snd_PlaySe(1, 2); }
        else if (sCo->cur == 0) { sCoPickBack = 1; Co_Leave(2); } /* the fight's select (port_screen.c) */
        else if (sCo->cur == 1) { Ps_Copy(sCoName, (char *)PortRpg_Name(), 20); sCo->act = 0; Co_SetPage(CO_NAME); }
        else if (sCo->cur == 3) { sCo->act = 0; Co_SetPage(CO_POINTS); }
        else if (sCo->cur == 4) { sCo->act = 0; Co_SetPage(CO_TREE); }
        else if (sCo->cur == 5) { sCo->act = 0; Co_SetPage(CO_OBJ); }
        else if (sCo->cur == 6) { sCo->cur = 0; sCo->act = 1; sCoFromMe = 1; Co_SetPage(CO_MISSIONS); }
    } else if (pad->gamePressed & PADG_TRIANGLE) {
        Snd_PlaySe(1, 2);
        sCo->cur = 0;
        if (sCoRoom) sCoRoomDone = 1; /* back to the room */
        else Co_SetPage(CO_START);
    }
}
static void Co_NameUpdate(void) { /* the keys: act = row * CO_KEY_COLS + column */
    Pad *pad = &gPad[0];
    s32 r = sCo->act / CO_KEY_COLS, c = sCo->act % CO_KEY_COLS, len = (s32)strlen(sCoName), done = 0;

    Ps_Prompt(0, "Escribir");
    Ps_Prompt(1, "Borrar");
    Ps_Prompt(2, "Cancelar");
    if (pad->gameRepeat & PADG_UP) r = (r + 4) % 5;
    if (pad->gameRepeat & PADG_DOWN) r = (r + 1) % 5;
    if (pad->gameRepeat & PADG_LEFT) c = (c + CO_KEY_COLS - 1) % CO_KEY_COLS;
    if (pad->gameRepeat & PADG_RIGHT) c = (c + 1) % CO_KEY_COLS;
    if (r < 4 && c >= 13) c = (pad->gameRepeat & PADG_LEFT) ? 12 : 0;
    if (pad->gameRepeat & (PADG_UP | PADG_DOWN | PADG_LEFT | PADG_RIGHT)) Snd_PlaySe(1, 0);
    sCo->act = r * CO_KEY_COLS + c;
    if (pad->gamePressed & PADG_CROSS) {
        if (r == 4 && c == 13) done = 1;
        else if (len < 16) { sCoName[len] = kCoKeys[r][c]; sCoName[len + 1] = 0; Snd_PlaySe(1, 0); }
    } else if ((pad->gamePressed & PADG_SQUARE) && len > 0) { sCoName[len - 1] = 0; Snd_PlaySe(1, 2); }
    else if (pad->gamePressed & PADG_START) done = 1;
    else if (pad->gamePressed & PADG_TRIANGLE) { Snd_PlaySe(1, 2); sCo->cur = 1; Co_SetPage(CO_ME); }
    if (done) {
        while ((len = (s32)strlen(sCoName)) > 0 && sCoName[len - 1] == ' ') sCoName[len - 1] = 0;
        PortRpg_SetName(sCoName);
        Snd_PlaySe(1, 1);
        sCo->cur = 1;
        Co_SetPage(CO_ME);
    }
}
static void Co_PointsUpdate(void) {
    Pad *pad = &gPad[0];

    Ps_Prompt(0, sCo->act == 4 ? "Repartir de nuevo" : "Volver");
    Ps_Prompt(2, "Volver");
    Ps_Move(&sCo->act, 5);
    if (sCo->act < 4 && (pad->gameRepeat & (PADG_LEFT | PADG_RIGHT)))
        Snd_PlaySe(1, PortRpg_PointAdd(sCo->act, (pad->gameRepeat & PADG_RIGHT) ? 1 : -1) ? 0 : 2);
    if ((pad->gamePressed & PADG_CROSS) && sCo->act == 4) { PortRpg_PointsReset(); Snd_PlaySe(1, 1); }
    else if (pad->gamePressed & (PADG_TRIANGLE | PADG_CROSS)) { Snd_PlaySe(1, 2); sCo->cur = 3; Co_SetPage(CO_ME); }
}

/* The skill tree (act: branch * 3 + tier; left / right the branch, up / down the tier) and the objects. */
extern int PortTree_Has(int), PortTree_Cost(int), PortTree_Points(void), PortTree_Free(void), PortTree_Buy(int);
extern void PortTree_Reset(void);
extern const char *PortTree_Name(int), *PortTree_What(int);
extern int PortObj_Count(void), PortObj_Owned(int), PortObj_Equipped(int), PortObj_TakeGiven(void);
extern void PortObj_Toggle(int);
extern const char *PortObj_Name(int), *PortObj_What(int);
static void Co_TreeUpdate(void) {
    Pad *pad = &gPad[0];
    s32 br = sCo->act / 3, ti = sCo->act % 3;

    Ps_Prompt(0, "Aprender");
    Ps_Prompt(1, "Olvidar todo");
    Ps_Prompt(2, "Volver");
    if (pad->gameRepeat & PADG_LEFT) br = (br + 2) % 3;
    if (pad->gameRepeat & PADG_RIGHT) br = (br + 1) % 3;
    if (pad->gameRepeat & PADG_UP) ti = (ti + 2) % 3;
    if (pad->gameRepeat & PADG_DOWN) ti = (ti + 1) % 3;
    if (pad->gameRepeat & (PADG_LEFT | PADG_RIGHT | PADG_UP | PADG_DOWN)) Snd_PlaySe(1, 0);
    sCo->act = br * 3 + ti;
    if (pad->gamePressed & PADG_CROSS) {
        if (PortTree_Buy(sCo->act)) { char t[120]; sprintf(t, "Aprendida: %.60s", PortTree_Name(sCo->act)); Co_Note(t); Snd_PlaySe(1, 1); }
        else Snd_PlaySe(1, 2);
    } else if (pad->gamePressed & PADG_SQUARE) { PortTree_Reset(); Co_Note("Habilidades olvidadas: tus puntos vuelven a estar libres."); Snd_PlaySe(1, 2); }
    else if (pad->gamePressed & PADG_TRIANGLE) { Snd_PlaySe(1, 2); sCo->cur = 4; Co_SetPage(CO_ME); }
}
static void Co_ObjUpdate(void) {
    Pad *pad = &gPad[0];

    Ps_Prompt(0, "Equipar / quitar");
    Ps_Prompt(2, "Volver");
    Ps_Move(&sCo->act, PortObj_Count());
    if (pad->gamePressed & PADG_CROSS) {
        if (PortObj_Owned(sCo->act)) { PortObj_Toggle(sCo->act); Snd_PlaySe(1, 1); } else Snd_PlaySe(1, 2);
    } else if (pad->gamePressed & PADG_TRIANGLE) { Snd_PlaySe(1, 2); sCo->cur = 5; Co_SetPage(CO_ME); }
}

static void Co_ListUpdate(void) {
    Pad *pad = &gPad[0];
    s32 count = PortCoop_Count();

    s32 first = sSagaN > 0 ? sSagaFirst[sCoSaga] : 0, len = sSagaN > 0 ? sSagaLen[sCoSaga] : count;

    Ps_Prompt(0, count > 0 ? "Elegir" : "Volver");
    Ps_Prompt(2, "Volver");
    if (count > 0) { /* within the saga */
        s32 local = sCo->cur - first;
        Ps_Move(&local, len);
        sCo->cur = first + local;
        if (sCo->cur < sCo->top) sCo->top = sCo->cur;
        if (sCo->cur >= sCo->top + CO_ROWS) sCo->top = sCo->cur - CO_ROWS + 1;
        if (sCo->top < first) sCo->top = first;
    }
    if ((pad->gamePressed & (PADG_CROSS | PADG_START)) && count > 0 && !PortCoop_Unlocked(sCo->cur)) {
        Snd_PlaySe(1, 2); /* (locked: the one before it first) */
    } else if ((pad->gamePressed & (PADG_CROSS | PADG_START)) && count > 0) {
        Snd_PlaySe(1, 1);
        sCo->act = 0;
        Co_SetPage(CO_CHAPTER);
    } else if (pad->gamePressed & (PADG_TRIANGLE | ((count == 0) ? PADG_CROSS : 0))) {
        Snd_PlaySe(1, 2);
        Co_SetPage(CO_MAP); /* (back to the map, on this saga) */
    }
}

extern int PortCoop_Difficulty(void), PortCoop_ChBestDiff(int), PortRpg_LevelMille(void);
extern void PortCoop_SetDifficulty(int d);
extern const char *PortCoop_DifficultyName(int d);
static s32 Co_Diff(void) { s32 d = Port_Setting("coop_dificultad", 0); return d < 0 ? 0 : d > 2 ? 2 : d; }
static void Co_ChapterUpdate(void) {
    Pad *pad = &gPad[0];

    if (sCo->act < PortCoop_ChDialogCount(sCo->cur)) { /* the conversation before the battle */
        Ps_Prompt(0, "Siguiente");
        Ps_Prompt(2, "Saltar");
        if (pad->gamePressed & (PADG_CROSS | PADG_START)) { sCo->act++; Snd_PlaySe(1, 0); sCo->pageFrom = sCo->frames - 6; }
        else if (pad->gamePressed & PADG_TRIANGLE) { sCo->act = PortCoop_ChDialogCount(sCo->cur); Snd_PlaySe(1, 2); }
        return;
    }
    Ps_Prompt(0, "Crear sala");
    Ps_Prompt(2, "Volver");
    if (pad->gameRepeat & (PADG_LEFT | PADG_RIGHT)) { /* the difficulty */
        s32 d = Co_Diff() + ((pad->gameRepeat & PADG_RIGHT) ? 1 : -1);
        if (d >= 0 && d <= 2) { PortCoop_SetDifficulty(d); Snd_PlaySe(1, 0); }
    }
    if (pad->gamePressed & (PADG_CROSS | PADG_START)) {
        Snd_PlaySe(1, 1);
        PortCoop_Select(sCo->cur);
        gPortTboCoop = 1;
        Co_Leave(1);
    } else if (pad->gamePressed & PADG_TRIANGLE) {
        Snd_PlaySe(1, 2);
        Co_SetPage(CO_LIST);
    }
}

/* ---------------------------------------------------------------------------------------------- drawing */
static void Co_Backdrop(void) {
    f32 aspect = (f32)Port_PageAspectMilli() / 1000.0f;
    f32 pic = Ps_PicW(TBO_BG) / Ps_PicH(TBO_BG);
    f32 u = aspect < pic ? aspect / pic : 1.0f;
    f32 v = aspect < pic ? 1.0f : pic / aspect;

    Ps_SpriteScreen(TBO_BG, 0.0f, 0.0f, 512.0f, 448.0f, 0.5f - u * 0.5f, 0.5f - v * 0.5f, 0.5f + u * 0.5f, 0.5f + v * 0.5f,
                    PS_PLAIN, 0x80);
    Ps_Rect(Ps_Left() - 200.0f, 0.0f, 712.0f, 448.0f, Ps_Rgba(0, 0, 0, 0x38), Ps_Rgba(0, 0, 0, 0x58));
}

static void Co_Row(f32 x0, f32 x1, f32 y, char *text, char *value, s32 chosen, s32 valueStyle) {
    if (chosen) Ps_Pill(TBO_KIT_SEL, x0 - 6.0f, y - 3.0f, x1 + 14.0f, y + 27.0f, 0x80);
    else Ps_Pill(TBO_KIT_BAR, x0, y, x1, y + 24.0f, 0x78);
    Ps_Text(x0 + 14.0f, Ps_TextTop(y + 12.0f, 0.58f), text, 0.58f, value ? (x1 - x0) * 0.62f : x1 - x0 - 30.0f, chosen ? 1 : 0);
    if (value) {
        Ps_Align(2);
        Ps_Text(x1 - 10.0f, Ps_TextTop(y + 12.0f, 0.46f), value, 0.46f, (x1 - x0) * 0.34f, valueStyle);
        Ps_Align(0);
    }
}

/* Text in lines of at most `width` units, from (x, y), `step` apart; at most `lines` lines. */
static void Co_Wrap(f32 x, f32 y, const char *text, f32 scale, f32 width, f32 step, s32 lines, s32 style) {
    char line[256], word[128];
    s32 at = 0, n = 0, drawn = 0;

    line[0] = 0;
    while (drawn < lines) {
        s32 w = 0;
        while (text[at] == ' ') at++;
        while (text[at] && text[at] != ' ' && w < 126) word[w++] = text[at++];
        word[w] = 0;
        if (w > 0) {
            char trial[256];
            Ps_Copy(trial, line, 256);
            if (n > 0) Ps_Cat(trial, " ", 256);
            Ps_Cat(trial, word, 256);
            if (n > 0 && Ps_TextWidth(trial, scale) > width) {
                Ps_Text(x, y + step * drawn++, line, scale, width, style);
                Ps_Copy(line, word, 256);
                n = 1;
                continue;
            }
            Ps_Copy(line, trial, 256);
            n++;
        }
        if (!text[at]) {
            if (n > 0) Ps_Text(x, y + step * drawn, line, scale, width, style);
            break;
        }
    }
}

/* "Mi personaje"'s box: the experience and the level rewards (src/battle/coop_infinite.c applies them). */
static const char *Co_RewardsBox(void) {
    static char text[400];
    static const struct { s32 level; const char *what; } rw[5] = {
        {10, "revives a tu compa\303\261ero en 2 s"}, {25, "te levantas con 60% de vida"},
        {40, "teletransporte junto a tu compa\303\261ero en 0,5 s"}, {60, "recibes 10% menos da\303\261o"},
        {80, "revives desde m\303\241s lejos"}};
    s32 lv = PortRpg_MyLevel(), i, have = 0, next = -1;
    char one[120];
    for (i = 0; i < 5; i++) { if (lv >= rw[i].level) have = i + 1; else if (next < 0) next = i; }
    sprintf(text, "Nivel %d (%d%% hasta el siguiente). Recompensas: %d de 5.", lv, PortRpg_LevelMille() / 10, have);
    if (have > 0) { sprintf(one, " La \303\272ltima: %s.", rw[have - 1].what); Ps_Cat(text, one, 400); }
    if (next >= 0) { sprintf(one, " Al Nv %d: %s.", rw[next].level, rw[next].what); Ps_Cat(text, one, 400); }
    return text; /* (the box takes some 220 letters: the latest reward and the next only) */
}

/* The tabs of "Misiones y tienda": the missions, the shop, the statistics (act 0, 2, 1). */
static void Co_NoteDraw(void) {
    if (sCoNoteFrames <= 0) return;
    sCoNoteFrames--;
    Ps_Pill(TBO_KIT_SEL, 40.0f, 300.0f, 470.0f, 326.0f, sCoNoteFrames > 20 ? 0x80 : sCoNoteFrames * 6);
    Ps_Align(1);
    Ps_Text(255.0f, Ps_TextTop(313.0f, 0.5f), sCoNote, 0.5f, 420.0f, 1);
    Ps_Align(0);
}
static void Co_Tabs(f32 slide) {
    static const char *names[4] = {"Misiones", "Tienda", "Logros", "Estad\303\255sticas"};
    static const s32 acts[4] = {0, 2, 3, 1};
    s32 k;
    for (k = 0; k < 4; k++) {
        f32 x0 = 168.0f + slide + 76.0f * k;
        s32 on = sCo->act == acts[k];
        Ps_Pill(on ? TBO_KIT_SEL : TBO_KIT_BAR, x0, 22.0f, x0 + 72.0f, 46.0f, on ? 0x80 : 0x60);
        Ps_Align(1);
        Ps_Text(x0 + 36.0f, Ps_TextTop(34.0f, 0.42f), (char *)names[k], 0.42f, 68.0f, on ? 1 : 4);
        Ps_Align(0);
    }
    Ps_Align(1);
    Ps_Text(168.0f + slide + 152.0f, 48.0f, "<  izquierda / derecha  >", 0.32f, 260.0f, 4);
    Ps_Align(0);
}

extern const char *PortCoop_ChSecret(int);
extern int PortCoop_ChWeekly(int);
extern const char *PortCoop_ChRequire(int);
static const char *Co_SecretBox(s32 c) {
    static char text[160];
    if (PortCoop_ChWeekly(c)) { sprintf(text, "Este jefe semanal volver\303\241 otra semana. El de esta semana est\303\241 abierto en esta misma lista."); return text; }
    if (PortCoop_ChRequire(c)[0]) { sprintf(text, "Ruta \302\277Y si...?: se abre al completar todos los cap\303\255tulos de la %.60s.", PortCoop_ChRequire(c)); return text; }
    sprintf(text, "Cap\303\255tulo secreto: se abre al sacar nota S o Z en todos los cap\303\255tulos de la %.60s.", PortCoop_ChSecret(c));
    return text;
}

static void Co_Draw(void) {
    f32 slide = (1.0f - Co_In()) * -160.0f;
    s32 count = PortCoop_Count(), i;
    const char *box;
    char line[160];

    Co_Backdrop();
    IconWin_Draw();
    PortOnline_Raw(1);
    if (sCo->page == CO_START) {
        Ps_Heading(Ps_Left() + 50.0f, Ps_TextTop(32.0f, 1.3f), "Historia cooperativa", 1.3f, 300.0f, 0x80);
        Co_Wrap(50.0f + slide, 82.0f, "T\303\272 y un amigo, cada uno en su casa, contra la CPU. "
                "Uno crea la sala y el otro se une con el c\303\263digo.", 0.55f, 410.0f, 20.0f, 2, 0);
        {   char rec[2][40];
            sprintf(rec[0], "R\303\251cord: %d", PortCoop_InfRecord(2));
            sprintf(rec[1], "R\303\251cord: %d", PortCoop_InfRecord(3));
            char mine[80], mineName[48];
            char mis[40];
            s32 k, ready = 0;
            for (k = 0; k < 6; k++) if (!PortMis_Claimed(k) && PortMis_Progress(k) >= PortMis_Goal(k)) ready++;
            ready += PortAch_ReadyCount();
            if (ready) sprintf(mis, "\302\241%d para reclamar!", ready); else mis[0] = 0;
            Co_Row(60.0f + slide, 380.0f + slide, 132.0f, "Crear sala (historia)", NULL, sCo->start == 0, 3);
            Co_Row(60.0f + slide, 380.0f + slide, 160.0f, "Unirse a una sala", NULL, sCo->start == 1, 3);
            Co_Row(60.0f + slide, 380.0f + slide, 192.0f, "Modo infinito: 2 contra 2", rec[0], sCo->start == 2, 3);
            Co_Row(60.0f + slide, 380.0f + slide, 220.0f, "Modo infinito: 2 contra 3", rec[1], sCo->start == 3, 3);
            Co_Row(60.0f + slide, 380.0f + slide, 252.0f, "Misiones y tienda", ready ? mis : NULL, sCo->start == 4, 1);
            PortTbo_CharaName(PortRpg_MyChara(), mineName, 48);
            sprintf(mine, "%.40s  Nv %d", mineName, PortRpg_MyLevel());
            Co_Row(60.0f + slide, 380.0f + slide, 280.0f, "Mi personaje", mine, sCo->start == 5, 5);
            {   f32 x0 = 74.0f + slide, x1 = 366.0f + slide, fill = x0 + (x1 - x0) * PortRpg_LevelMille() / 1000.0f;
                Ps_Rect(x0, 307.0f, x1, 311.0f, Ps_Rgba(23, 50, 60, 0x80), Ps_Rgba(23, 50, 60, 0x80));
                Ps_Rect(x0, 307.0f, fill, 311.0f, Ps_Rgba(255, 213, 91, 0x80), Ps_Rgba(255, 190, 60, 0x80));
                {   extern const char *PortRpg_XpText(void);
                    Ps_Text(x1 + 8.0f, Ps_TextTop(309.0f, 0.36f), (char *)PortRpg_XpText(), 0.36f, 110.0f, 3); }
            }
        }
        {   s32 k, won = 0, top = 0;
            char prog[96];
            for (k = 0; k < count; k++) {
                if (PortCoop_Done(k)) won++;
                if (PortCoop_ChGrade(k)[0] == 'S' || PortCoop_ChGrade(k)[0] == 'Z') top++;
            }
            sprintf(prog, "Cap\303\255tulos superados: %d de %d   \302\267   Notas S o Z: %d", won, count, top);
            Ps_Text(60.0f + slide, 116.0f, prog, 0.42f, 360.0f, 3);
        }
        if (gPortCoopNote[0]) Co_Wrap(50.0f + slide, 318.0f, gPortCoopNote, 0.5f, 410.0f, 18.0f, 2, 5);
        else if (PortCoop_RewardLine()[0]) { /* the last battle's grade, Z points and experience */
            sprintf(line, "\303\232ltimo combate: %.120s", PortCoop_RewardLine());
            Co_Wrap(50.0f + slide, 318.0f, line, 0.5f, 410.0f, 18.0f, 2, 1);
        }
        box = sCo->start == 5
            ? Co_RewardsBox()
            : sCo->start == 4
            ? "Misiones diarias y semanales, la tienda (c\303\241psulas, experiencia y t\303\255tulos) y las estad\303\255sticas."
            : sCo->start == 0
            ? "Elige el cap\303\255tulo y crea la sala. Despu\303\251s dale el c\303\263digo a tu amigo."
            : sCo->start == 1
            ? "Escribe el c\303\263digo que te d\303\251 tu amigo. El cap\303\255tulo o el modo lo elige quien crea la sala."
            : "Oleadas sin fin: cuando cae un rival entra otro, cada vez con m\303\241s vida y m\303\241s da\303\261o. Cada 5 oleadas, un jefe. Crea la sala y pasa el c\303\263digo.";
    } else if (sCo->page == CO_PICK) {
        char name[48];
        Ps_Heading(Ps_Left() + 50.0f, Ps_TextTop(32.0f, 1.3f), "Mi personaje", 1.3f, 300.0f, 0x80);
        for (i = sCo->top; i < sPickCount && i < sCo->top + CO_ROWS; i++) {
            PortTbo_CharaName(sPickIds[i], name, 48);
            Co_Row(40.0f + slide, 420.0f + slide, 82.0f + 30.0f * (i - sCo->top), name,
                   sPickIds[i] == PortRpg_MyChara() ? "Elegido" : NULL, i == sCo->cur, 5);
        }
        if (sCo->top > 0) Ps_Text(220.0f + slide, 66.0f, "...", 0.5f, 40.0f, 4);
        if (sCo->top + CO_ROWS < sPickCount) Ps_Text(220.0f + slide, 82.0f + 30.0f * CO_ROWS, "...", 0.5f, 40.0f, 4);
        box = "Elige qui\303\251n quieres ser. En la selecci\303\263n de personaje de la cooperativa saldr\303\241 una casilla m\303\241s, "
              "al final, con \303\251l: es tu personaje, con tu nivel. Su casilla normal sigue siendo el original.";
    } else if (sCo->page == CO_MISSIONS && sCo->act == 2) {
        Ps_Heading(Ps_Left() + 50.0f, Ps_TextTop(32.0f, 1.3f), "Tienda", 1.2f, 110.0f, 0x80);
        Co_Tabs(slide);
        sprintf(line, "Tus Puntos Z: %d   C\303\241psulas: %d EXP, %d Z   Nv %d", PortCoop_Money(), PortItem_Count(0), PortItem_Count(1), PortRpg_MyLevel());
        Ps_Text(50.0f + slide, 60.0f, line, 0.46f, 410.0f, 1);
        {   extern const char *PortRpg_XpText(void);
            f32 x0 = 50.0f + slide, x1 = 330.0f + slide, fill = x0 + (x1 - x0) * PortRpg_LevelMille() / 1000.0f;
            Ps_Rect(x0, 78.0f, x1, 82.0f, Ps_Rgba(23, 50, 60, 0x80), Ps_Rgba(23, 50, 60, 0x80));
            Ps_Rect(x0, 78.0f, fill, 82.0f, Ps_Rgba(255, 213, 91, 0x80), Ps_Rgba(255, 190, 60, 0x80));
            Ps_Text(x1 + 8.0f, Ps_TextTop(80.0f, 0.36f), (char *)PortRpg_XpText(), 0.36f, 110.0f, 3);
        }
        if (sShopTitles == 0) Co_ShopBuild();
        for (i = sCo->top; i < Co_ShopCount() && i < sCo->top + 8; i++) {
            char what[80], value[40];
            if (i == 0) sprintf(what, "C\303\241psula de EXP (x2 EXP un combate)");
            else if (i == 1) sprintf(what, "C\303\241psula Z (x2 Puntos Z un combate)");
            else if (i == 2) sprintf(what, "Entrenamiento: +250 EXP");
            else if (i == 3) sprintf(what, "Habilidad: empiezas con 1 barra de Ki m\303\241s");
            else if (i == 4) sprintf(what, "Habilidad: cada golpe recupera Ki");
            else sprintf(what, "T\303\255tulo: %.50s", PortRpg_TitleName(sShopTitle[i - CO_SHOP_BASE]));
            if (Co_ShopOwned(i)) sprintf(value, "Comprado"); else sprintf(value, "%d Z", Co_ShopPrice(i));
            Co_Row(40.0f + slide, 420.0f + slide, 88.0f + 26.0f * (i - sCo->top), what, value, i == sCo->cur,
                   Co_ShopOwned(i) ? 4 : PortCoop_Money() >= Co_ShopPrice(i) ? 1 : 4);
        }
        if (sCo->top > 0) Ps_Text(220.0f + slide, 76.0f, "...", 0.5f, 40.0f, 4);
        if (sCo->top + 8 < Co_ShopCount()) Ps_Text(220.0f + slide, 88.0f + 26.0f * 8, "...", 0.5f, 40.0f, 4);
        box = sCo->cur == 3 || sCo->cur == 4 ? "Las habilidades son para siempre y valen en la cooperativa con la casilla de tu personaje."
            : "X: comprar. Los Puntos Z son los de tu partida (los mismos del modo historia).";
    } else if (sCo->page == CO_MISSIONS && sCo->act == 3) {
        Ps_Heading(Ps_Left() + 50.0f, Ps_TextTop(32.0f, 1.2f), "Logros", 1.2f, 110.0f, 0x80);
        Co_Tabs(slide);
        for (i = sCo->top; i < PortAch_Count() && i < sCo->top + 8; i++) {
            char value[48];
            s32 now, goal;
            PortAch_Progress(i, &now, &goal);
            if (PortAch_Claimed(i)) sprintf(value, "Conseguido");
            else if (now >= goal) sprintf(value, "\302\241Reclamar %d Z!", PortAch_Reward(i));
            else if (goal > 1) sprintf(value, "%d / %d", now, goal);
            else sprintf(value, "%d Z", PortAch_Reward(i));
            Co_Row(40.0f + slide, 420.0f + slide, 74.0f + 28.0f * (i - sCo->top), (char *)PortAch_Name(i), value, i == sCo->cur,
                   PortAch_Claimed(i) ? 5 : now >= goal ? 1 : 4);
        }
        if (sCo->top + 8 < PortAch_Count()) Ps_Text(220.0f + slide, 74.0f + 28.0f * 8, "...", 0.5f, 40.0f, 4);
        if (sCo->top > 0) Ps_Text(220.0f + slide, 62.0f, "...", 0.5f, 40.0f, 4);
        box = "Logros de la cooperativa: cada uno se reclama una vez con X. Algunos dan t\303\255tulos (\303\201ngel guardi\303\241n, Maestro de la historia, Guerrero secreto).";
    } else if (sCo->page == CO_MISSIONS && !sCo->act) {
        char head[80];
        Ps_Heading(Ps_Left() + 50.0f, Ps_TextTop(32.0f, 1.3f), "Misiones", 1.2f, 110.0f, 0x80);
        Co_Tabs(slide);
        for (i = 0; i < 6; i++) {
            char value[40];
            f32 y = (i < 3 ? 92.0f : 216.0f) + 28.0f * (i % 3);
            if (i == 0 || i == 3) {
                if (i == 0) sprintf(head, "Diarias  (se renuevan en %d h)", PortMis_HoursLeft());
                else sprintf(head, "Semanales  (se renuevan en %d d\303\255a%s)", PortMis_DaysLeft(), PortMis_DaysLeft() == 1 ? "" : "s");
                Ps_Text(50.0f + slide, y - 20.0f, head, 0.46f, 400.0f, 3);
            }
            if (PortMis_Claimed(i)) sprintf(value, "Hecho");
            else if (PortMis_Progress(i) >= PortMis_Goal(i)) sprintf(value, "\302\241Reclamar!");
            else sprintf(value, "%d / %d", PortMis_Progress(i), PortMis_Goal(i));
            Co_Row(40.0f + slide, 420.0f + slide, y, (char *)PortMis_Text(i), value, i == sCo->cur,
                   PortMis_Claimed(i) ? 4 : PortMis_Progress(i) >= PortMis_Goal(i) ? 1 : 3);
        }
        if (PortMis_Streak() > 0)
            sprintf(line, "Racha: %d d\303\255a%s seguido%s%s   \302\267   C\303\241psulas: %d EXP, %d Z", PortMis_Streak(), PortMis_Streak() == 1 ? "" : "s",
                    PortMis_Streak() == 1 ? "" : "s", PortMis_StreakBonus() ? " (+Puntos Z)" : "", PortItem_Count(0), PortItem_Count(1));
        else sprintf(line, "C\303\241psulas: %d de EXP, %d de Puntos Z (se usan solas en el siguiente combate)", PortItem_Count(0), PortItem_Count(1));
        Ps_Text(50.0f + slide, 302.0f, line, 0.42f, 400.0f, 0);
        sprintf(line, "Premio: %s%s. X: reclamar. Reclama una misi\303\263n cada d\303\255a para mantener la racha (hasta +50%% Puntos Z).",
                PortMis_RewardText(sCo->cur), PortMis_StreakBonus() ? " y la racha" : "");
        box = line;
    } else if (sCo->page == CO_MISSIONS) {
        static const char *names[11] = {"Combates jugados", "Victorias", "Con Mi personaje", "Da\303\261o total a los rivales",
            "Compa\303\261eros revividos", "Veces ca\303\255do", "Mejor nota", "Notas S o Z", "Mejor modo infinito (oleadas)",
            "Victorias en Extremo", "Puntos Z ganados"};
        static const s32 order[10] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 10};
        char mineName[48];
        PortTbo_CharaName(PortRpg_MyChara(), mineName, 48);
        sprintf(line, "Estad\303\255sticas de %.40s  (Nv %d)", mineName, PortRpg_MyLevel());
        Ps_Heading(Ps_Left() + 50.0f, Ps_TextTop(32.0f, 1.1f), "Estad\303\255sticas", 0.9f, 110.0f, 0x80);
        Co_Tabs(slide);
        Ps_Text(50.0f + slide, 52.0f, line + 14, 0.42f, 110.0f, 3); /* (who: "de <name> (Nv X)") */
        for (i = 0; i < 10; i++) {
            char value[40];
            s32 k = order[i];
            if (k == 6) sprintf(value, "%s", PortStat_BestGrade());
            else if (k == 1 && PortStat(0) > 0) sprintf(value, "%d (%d%%)", PortStat(1), PortStat(1) * 100 / PortStat(0));
            else sprintf(value, "%d", PortStat(k));
            Co_Row(40.0f + slide, 420.0f + slide, 70.0f + 24.0f * i, (char *)names[k], value, 0, k == 6 ? 1 : 3);
        }
        sprintf(line, "Victorias en Extremo: %d. Izquierda/derecha: tienda, misiones.", PortStat(9));
        box = line;
    } else if (sCo->page == CO_MAP) {
        s32 s, rows = (sSagaN + 2) / 3;
        f32 step = rows > 3 ? 236.0f / (f32)rows : 78.0f, nw = 136.0f, nh = step - 26.0f;
        Ps_Heading(Ps_Left() + 50.0f, Ps_TextTop(32.0f, 1.3f), "Mapa de sagas", 1.3f, 300.0f, 0x80);
        for (s = 0; s < sSagaN; s++) { /* the path first, under the sagas */
            s32 r = s / 3, c = (r & 1) ? 2 - s % 3 : s % 3, r2, c2;
            f32 x = 40.0f + slide + c * 156.0f + nw * 0.5f, y = 76.0f + r * step + nh * 0.5f, x2, y2;
            u32 col = Co_SagaOpen(s) ? Ps_Rgba(255, 213, 91, 0x70) : Ps_Rgba(120, 120, 120, 0x50);
            if (s + 1 >= sSagaN) break;
            r2 = (s + 1) / 3; c2 = (r2 & 1) ? 2 - (s + 1) % 3 : (s + 1) % 3;
            x2 = 40.0f + slide + c2 * 156.0f + nw * 0.5f; y2 = 76.0f + r2 * step + nh * 0.5f;
            if (r2 == r) Ps_Rect(x < x2 ? x : x2, y - 2.0f, x < x2 ? x2 : x, y + 2.0f, col, col);
            else Ps_Rect(x - 2.0f, y, x + 2.0f, y2, col, col);
        }
        for (s = 0; s < sSagaN; s++) {
            s32 r = s / 3, c = (r & 1) ? 2 - s % 3 : s % 3, open = Co_SagaOpen(s), done = Co_SagaDone(s);
            f32 x0 = 40.0f + slide + c * 156.0f, y0 = 76.0f + r * step;
            const char *name = PortCoop_ChSaga(sSagaFirst[s]);
            char prog[48];
            if (!strncmp(name, "Saga ", 5)) name += 5;
            if (!strncmp(name, "de las ", 7)) name += 7; else if (!strncmp(name, "de los ", 7)) name += 7; else if (!strncmp(name, "de ", 3)) name += 3;
            Ps_Pill(s == sCoSaga ? TBO_KIT_SEL : TBO_KIT_BAR, x0 - (s == sCoSaga ? 4.0f : 0.0f), y0 - (s == sCoSaga ? 3.0f : 0.0f),
                    x0 + nw + (s == sCoSaga ? 4.0f : 0.0f), y0 + nh + (s == sCoSaga ? 3.0f : 0.0f), open ? 0x80 : 0x48);
            Ps_Align(1);
            Ps_Text(x0 + nw * 0.5f, y0 + 4.0f, open ? (char *)name : "? ? ?", 0.48f, nw - 10.0f, s == sCoSaga ? 1 : open ? 0 : 4);
            if (!open) sprintf(prog, "Bloqueada");
            else if (done >= sSagaLen[s]) sprintf(prog, "Completada");
            else sprintf(prog, "%d / %d", done, sSagaLen[s]);
            Ps_Text(x0 + nw * 0.5f, y0 + nh - 18.0f, prog, 0.40f, nw - 10.0f, !open ? 4 : done >= sSagaLen[s] ? 5 : 3);
            Ps_Align(0);
        }
        if (sSagaN > 0) {
            s32 open = Co_SagaOpen(sCoSaga);
            sprintf(line, open ? "%.40s: %d cap\303\255tulos, %d superados. X: entrar. Cuadrado: volver a leer los cap\303\255tulos."
                               : "%.40s: bloqueada. Superad la saga anterior para abrirla.",
                    PortCoop_ChSaga(sSagaFirst[sCoSaga]), sSagaLen[sCoSaga], Co_SagaDone(sCoSaga));
            box = line;
        } else box = "Revisa historia_coop/capitulos.lua junto al juego. Cuadrado: leerlo otra vez.";
    } else if (sCo->page == CO_ME) {
        char cn[48], value[64];
        static const char *rows[7] = {"Personaje", "Nombre", "T\303\255tulo", "Mejoras", "\303\201rbol de habilidades", "Objetos", "Estad\303\255sticas"};
        Ps_Heading(Ps_Left() + 50.0f, Ps_TextTop(32.0f, 1.3f), "Mi personaje", 1.3f, 300.0f, 0x80);
        PortTbo_CharaName(PortRpg_MyChara(), cn, 48);
        sprintf(line, "%s%s%.40s  \302\267  Nv %d", PortRpg_Name()[0] ? PortRpg_Name() : "", PortRpg_Name()[0] ? "  \302\267  " : "", cn, PortRpg_MyLevel());
        Ps_Text(50.0f + slide, 66.0f, line, 0.55f, 410.0f, 1);
        for (i = 0; i < 7; i++) {
            if (i == 0) sprintf(value, "%.40s", cn);
            else if (i == 1) sprintf(value, "%.20s", PortRpg_Name()[0] ? PortRpg_Name() : "(sin nombre)");
            else if (i == 2) sprintf(value, "<  %.40s  >", PortRpg_TitleName(PortRpg_Title()));
            else if (i == 3 && PortRpg_PointsFree() > 0) sprintf(value, "\302\241%d puntos sin gastar!", PortRpg_PointsFree());
            else if (i == 3) sprintf(value, "V %d  A %d  D %d  Ki %d", PortRpg_Points(0), PortRpg_Points(1), PortRpg_Points(2), PortRpg_Points(3));
            else if (i == 4) sprintf(value, PortTree_Free() > 0 ? "\302\241%d puntos libres!" : "%d puntos", PortTree_Free() > 0 ? PortTree_Free() : PortTree_Points());
            else if (i == 5) { s32 o, n = 0; for (o = 0; o < PortObj_Count(); o++) n += PortObj_Owned(o) != 0; sprintf(value, "%d de %d", n, PortObj_Count()); }
            else sprintf(value, "%d combates", PortStat(0));
            Co_Row(50.0f + slide, 410.0f + slide, 88.0f + 27.0f * i, (char *)rows[i], value, i == sCo->cur, i == 3 && PortRpg_PointsFree() > 0 ? 1 : 5);
        }
        {   f32 x0 = 64.0f + slide, x1 = 396.0f + slide, fill = x0 + (x1 - x0) * PortRpg_LevelMille() / 1000.0f;
            Ps_Rect(x0, 284.0f, x1, 289.0f, Ps_Rgba(23, 50, 60, 0x80), Ps_Rgba(23, 50, 60, 0x80));
            Ps_Rect(x0, 284.0f, fill, 289.0f, Ps_Rgba(255, 213, 91, 0x80), Ps_Rgba(255, 190, 60, 0x80));
            {   extern const char *PortRpg_XpText(void);
                Ps_Text(x1 + 8.0f, Ps_TextTop(286.5f, 0.36f), (char *)PortRpg_XpText(), 0.36f, 110.0f, 3); }
        }
        if (sCo->cur == 2) {
            s32 t, n = PortRpg_TitleCount(), got = 0;
            for (t = 0; t < n; t++) got += PortRpg_TitleUnlocked(t) != 0;
            sprintf(line, "Izquierda/derecha: cambiar de t\303\255tulo (tienes %d de %d). Los dem\303\241s se ganan subiendo de nivel o en la tienda.", got, n);
            box = line;
        } else if (sCo->cur == 1) box = "El nombre de tu personaje: lo ver\303\241 tu compa\303\261ero en la sala y en el combate.";
        else if (sCo->cur == 3) box = "Cada nivel da un punto. Rep\303\241rtelos entre vida, ataque, defensa y Ki (cada punto, un 0,5%).";
        else if (sCo->cur == 4) box = "Tres ramas (tanque, atacante y apoyo) con habilidades que se aprenden en orden. Un punto cada 5 niveles.";
        else if (sCo->cur == 5) box = "Objetos que equipas (hasta 2) para tu personaje.";
        else if (sCo->cur == 6) box = "Todo lo que has hecho en la cooperativa.";
        else box = Co_RewardsBox();
    } else if (sCo->page == CO_NAME) {
        s32 r, c;
        Ps_Heading(Ps_Left() + 50.0f, Ps_TextTop(32.0f, 1.3f), "Nombre", 1.3f, 300.0f, 0x80);
        Ps_Pill(TBO_KIT_BAR, 60.0f + slide, 72.0f, 420.0f + slide, 104.0f, 0x80);
        sprintf(line, "%s%s", sCoName, (sCo->frames / 16) & 1 ? "_" : " ");
        Ps_TextRaw(76.0f + slide, Ps_TextTop(88.0f, 0.7f), line, 0.7f, 330.0f, 1);
        for (r = 0; r < 5; r++)
            for (c = 0; c < (r == 4 ? 14 : 13); c++) {
                f32 x = 40.0f + slide + c * 30.0f, y = 120.0f + r * 34.0f, w = (r == 4 && c == 13) ? 50.0f : 26.0f;
                char key[4];
                s32 on = sCo->act == r * CO_KEY_COLS + c;
                Ps_Pill(on ? TBO_KIT_SEL : TBO_KIT_BAR, x, y, x + w, y + 28.0f, 0x80);
                if (r == 4 && c == 13) sprintf(key, "OK"); else if (kCoKeys[r][c] == ' ') sprintf(key, "_"); else { key[0] = kCoKeys[r][c]; key[1] = 0; }
                Ps_Align(1);
                Ps_TextRaw(x + w * 0.5f, Ps_TextTop(y + 14.0f, 0.55f), key, 0.55f, w, on ? 1 : 0);
                Ps_Align(0);
            }
        box = "X: escribir. Cuadrado: borrar. Start u OK: guardar. Tri\303\241ngulo: cancelar. (Hasta 16 letras; _ es un espacio)";
    } else if (sCo->page == CO_POINTS) {
        static const char *names[4] = {"Vida", "Ataque", "Defensa", "Ki (gastas menos)"};
        Ps_Heading(Ps_Left() + 50.0f, Ps_TextTop(32.0f, 1.3f), "Mejoras", 1.3f, 300.0f, 0x80);
        sprintf(line, "Puntos sin gastar: %d   (un punto por nivel; nivel %d)", PortRpg_PointsFree(), PortRpg_MyLevel());
        Ps_Text(50.0f + slide, 70.0f, line, 0.5f, 410.0f, PortRpg_PointsFree() > 0 ? 1 : 3);
        for (i = 0; i < 4; i++) {
            char value[48];
            sprintf(value, "<  %d  >   %s%d,%d%%", PortRpg_Points(i), i == 3 ? "-" : "+", PortRpg_Points(i) / 2, (PortRpg_Points(i) & 1) * 5);
            Co_Row(50.0f + slide, 410.0f + slide, 100.0f + 34.0f * i, (char *)names[i], value, i == sCo->act, 5);
        }
        Co_Row(50.0f + slide, 410.0f + slide, 246.0f, "Repartir de nuevo", NULL, sCo->act == 4, 3);
        box = sCo->act == 3 ? "Ki: cada punto hace que las t\303\251cnicas (y todo lo que gasta Ki) cuesten un 0,5% menos. M\303\241ximo 40: un 20% menos."
            : "Izquierda/derecha: quitar o poner un punto (m\303\241ximo 40 en cada una). Se aplican en el siguiente combate con tu casilla.";
    } else if (sCo->page == CO_TREE) {
        static const char *branches[3] = {"Tanque", "Atacante", "Apoyo"};
        s32 br, ti;
        Ps_Heading(Ps_Left() + 50.0f, Ps_TextTop(32.0f, 1.2f), "\303\201rbol de habilidades", 1.2f, 300.0f, 0x80);
        sprintf(line, "Puntos de habilidad: %d libres de %d (uno cada 5 niveles)", PortTree_Free(), PortTree_Points());
        Ps_Text(50.0f + slide, 62.0f, line, 0.46f, 410.0f, PortTree_Free() > 0 ? 1 : 3);
        f32 xb = 30.0f + slide; /* (a float that grows: no int to float conversion here) */
        for (br = 0; br < 3; br++, xb += 150.0f) {
            f32 x0 = xb, yb = 110.0f;
            Ps_Align(1);
            Ps_Text(x0 + 70.0f, 84.0f, (char *)branches[br], 0.6f, 140.0f, 3);
            Ps_Align(0);
            for (ti = 0; ti < 3; ti++, yb += 58.0f) {
                s32 b = br * 3 + ti, has = PortTree_Has(b), open = ti == 0 || PortTree_Has(b - 1);
                f32 y0 = yb;
                char cost[24];
                if (ti > 0) Ps_Rect(x0 + 68.0f, y0 - 14.0f, x0 + 72.0f, y0, has ? Ps_Rgba(255, 213, 91, 0x80) : Ps_Rgba(120, 120, 120, 0x60), has ? Ps_Rgba(255, 213, 91, 0x80) : Ps_Rgba(120, 120, 120, 0x60));
                Ps_Pill(b == sCo->act ? TBO_KIT_SEL : TBO_KIT_BAR, x0, y0, x0 + 140.0f, y0 + 44.0f, has || open ? 0x80 : 0x48);
                Ps_Align(1);
                Ps_Text(x0 + 70.0f, y0 + 5.0f, (char *)PortTree_Name(b), 0.44f, 132.0f, has ? 5 : b == sCo->act ? 1 : open ? 0 : 4);
                if (has) sprintf(cost, "Aprendida"); else sprintf(cost, "%d punto%s", PortTree_Cost(b), PortTree_Cost(b) == 1 ? "" : "s");
                Ps_Text(x0 + 70.0f, y0 + 24.0f, cost, 0.38f, 132.0f, has ? 5 : 3);
                Ps_Align(0);
            }
        }
        sprintf(line, "%.60s: %.120s%s", PortTree_Name(sCo->act), PortTree_What(sCo->act),
                sCo->act % 3 && !PortTree_Has(sCo->act - 1) ? " (antes, la de arriba)" : "");
        box = line;
    } else if (sCo->page == CO_OBJ) {
        s32 eq = 0;
        Ps_Heading(Ps_Left() + 50.0f, Ps_TextTop(32.0f, 1.2f), "Objetos", 1.2f, 300.0f, 0x80);
        for (i = 0; i < PortObj_Count(); i++) eq += PortObj_Equipped(i) && PortObj_Owned(i);
        sprintf(line, "Equipados: %d de 2", eq);
        Ps_Text(50.0f + slide, 62.0f, line, 0.46f, 410.0f, 3);
        for (i = 0; i < PortObj_Count(); i++) {
            char value[48];
            s32 own = PortObj_Owned(i);
            if (!own) sprintf(value, "Sin conseguir");
            else if (PortObj_Equipped(i)) sprintf(value, "Equipado  \302\267  %.30s", PortObj_What(i));
            else sprintf(value, "%.40s", PortObj_What(i));
            Co_Row(40.0f + slide, 420.0f + slide, 82.0f + 28.0f * i, own ? (char *)PortObj_Name(i) : "? ? ?", value, i == sCo->act,
                   !own ? 4 : PortObj_Equipped(i) ? 1 : 5);
        }
        box = "Se consiguen con las misiones semanales, los jefes semanales y las rutas \302\277Y si...? Equipa hasta 2: suman a tus mejoras con tu casilla.";
    } else if (sCo->page == CO_LIST) {
        s32 first = sSagaN > 0 ? sSagaFirst[sCoSaga] : 0, end = sSagaN > 0 ? first + sSagaLen[sCoSaga] : count;
        Ps_Heading(Ps_Left() + 50.0f, Ps_TextTop(32.0f, 1.3f), sSagaN > 0 ? (char *)PortCoop_ChSaga(first) : "Historia cooperativa", 1.3f, 380.0f, 0x80);
        if (count == 0) {
            Co_Wrap(50.0f + slide, 100.0f, PortCoop_Error(), 0.55f, 400.0f, 20.0f, 6, 4);
            box = "Revisa historia_coop/capitulos.lua junto al juego. Cuadrado: leerlo otra vez.";
        } else {
            for (i = sCo->top; i < end && i < sCo->top + CO_ROWS; i++) {
                s32 open = PortCoop_Unlocked(i);
                char done[40], *value;
                sprintf(done, PortCoop_ChGrade(i)[0] ? "Nota %s" : "Completado", PortCoop_ChGrade(i));
                if (PortCoop_ChBestDiff(i) >= 2) Ps_Cat(done, PortCoop_ChBestDiff(i) >= 3 ? " (Extremo)" : " (Dif\303\255cil)", 40);
                {   extern const char *PortCoop_ChSecret(int);
                    extern int PortCoop_ChWeekly(int);extern const char *PortCoop_ChRequire(int);
                    value = PortCoop_ChWeekly(i) ? (open ? "\302\241Esta semana!" : "Otra semana") : PortCoop_Done(i) ? done : open ? (char *)PortCoop_ChSaga(i)
                          : PortCoop_ChSecret(i)[0] ? "Secreto" : PortCoop_ChRequire(i)[0] ? "Ruta cerrada" : "Bloqueado"; }
                Co_Row(40.0f + slide, 420.0f + slide, 82.0f + 30.0f * (i - sCo->top), open ? (char *)PortCoop_ChTitle(i) : "? ? ?", value,
                       i == sCo->cur, PortCoop_Done(i) ? 5 : open ? 3 : 4);
            }
            if (sCo->top > first) Ps_Text(220.0f + slide, 66.0f, "...", 0.5f, 40.0f, 4);
            if (sCo->top + CO_ROWS < end) Ps_Text(220.0f + slide, 82.0f + 30.0f * CO_ROWS, "...", 0.5f, 40.0f, 4);
            box = PortCoop_Unlocked(sCo->cur)
                ? "Elige un cap\303\255tulo. Cada uno que super\303\251is desbloquea el siguiente. Tri\303\241ngulo: volver al mapa."
                : PortCoop_ChSecret(sCo->cur)[0] || PortCoop_ChRequire(sCo->cur)[0] || PortCoop_ChWeekly(sCo->cur) ? Co_SecretBox(sCo->cur)
                : "Bloqueado: primero ten\303\251is que superar el cap\303\255tulo anterior.";
        }
    } else {
        s32 c = sCo->cur, k;
        if (sCo->act < PortCoop_ChDialogCount(c)) { /* the conversation */
            Ps_Heading(Ps_Left() + 50.0f, Ps_TextTop(32.0f, 1.1f), (char *)PortCoop_ChTitle(c), 1.1f, 380.0f, 0x80);
            Ps_Text(50.0f + slide, 62.0f, (char *)PortCoop_ChSaga(c), 0.5f, 400.0f, 3);
            Ps_Pill(TBO_KIT_BAR, 36.0f + slide, 112.0f, 446.0f + slide, 250.0f, 0x70);
            Ps_Text(54.0f + slide, 122.0f, (char *)PortCoop_ChDialogWho(c, sCo->act), 0.72f, 380.0f, 1);
            Co_Wrap(54.0f + slide, 152.0f, PortCoop_ChDialogText(c, sCo->act), 0.6f, 380.0f, 22.0f, 4, 0);
            sprintf(line, "%d / %d", sCo->act + 1, PortCoop_ChDialogCount(c));
            Ps_Align(2); Ps_Text(436.0f + slide, 230.0f, line, 0.4f, 60.0f, 4); Ps_Align(0);
            box = "X: siguiente. Tri\303\241ngulo: saltar la conversaci\303\263n.";
            PortOnline_Raw(0);
            MsgWin_PortDrawStringHd(Co_Box(box));
            return;
        }
        Ps_Heading(Ps_Left() + 50.0f, Ps_TextTop(32.0f, 1.1f), (char *)PortCoop_ChTitle(c), 1.1f, 380.0f, 0x80);
        Ps_Text(50.0f + slide, 62.0f, (char *)PortCoop_ChSaga(c), 0.5f, 400.0f, 3);
        Co_Wrap(50.0f + slide, 82.0f, PortCoop_ChIntro(c), 0.52f, 410.0f, 18.0f, 6, 0);
        if (PortCoop_ChPair(c)[0]) {
            Ps_Copy(line, "Pareja sugerida: ", 160);
            Ps_Cat(line, (char *)PortCoop_ChPair(c), 160);
            Ps_Text(50.0f + slide, 196.0f, line, 0.5f, 410.0f, 3);
        }
        for (k = 0; k < PortCoop_ChFoeCount(c) && k < 3; k++) {
            char num[48];
            if (PortCoop_ChFoeName(c, k)[0]) Ps_Copy(line, (char *)PortCoop_ChFoeName(c, k), 160);
            else { sprintf(num, "Personaje %d", PortCoop_ChFoeChara(c, k)); Ps_Copy(line, num, 160); }
            sprintf(num, "  (%s, vida %d%%)", Co_Level(PortCoop_ChFoeLevel(c, k)), PortCoop_ChFoeHealth(c, k));
            Ps_Cat(line, num, 160);
            Ps_Text(50.0f + slide, 216.0f + 18.0f * k, k == 0 ? "Rivales:" : "", 0.5f, 80.0f, 2);
            Ps_Text(120.0f + slide, 216.0f + 18.0f * k, line, 0.5f, 340.0f, 0);
        }
        {   char more[96];
            if (PortCoop_ChReserveCount(c) > 0) {
                sprintf(more, "+ %d refuerzos: entran cuando cae un rival", PortCoop_ChReserveCount(c));
                Ps_Text(120.0f + slide, 216.0f + 18.0f * (PortCoop_ChFoeCount(c) < 3 ? PortCoop_ChFoeCount(c) : 3), more, 0.5f, 340.0f, 1);
            }
        }
        {   static const char *more[3] = {"recompensas normales", "rivales m\303\241s duros: +50% Puntos Z, +25% EXP",
                                          "rivales mucho m\303\241s duros: x2 Puntos Z, +50% EXP"};
            char dv[64];
            sprintf(dv, "<  %s  >", PortCoop_DifficultyName(Co_Diff()));
            Co_Row(60.0f + slide, 380.0f + slide, 262.0f, "Dificultad", dv, 0, Co_Diff() == 0 ? 3 : Co_Diff() == 1 ? 1 : 4);
            Ps_Text(70.0f + slide, 288.0f, (char *)more[Co_Diff()], 0.42f, 320.0f, 0);
        }
        {   extern int PortCoop_ChBestTime(int), PortCoop_ChRivalTime(int), PortCoop_ChRivalGrade(int), PortCoop_ChWeekly(int);
            extern const char *PortCoop_GradeName(int), *PortCoop_RivalName(void);
            char me[64], rv[80];
            s32 t = PortCoop_ChBestTime(c), rt = PortCoop_ChRivalTime(c);
            if (t > 0) sprintf(me, "Tu r\303\251cord: %s  %d:%02d", PortCoop_ChGrade(c)[0] ? PortCoop_ChGrade(c) : "-", t / 60, t % 60);
            else sprintf(me, "Sin r\303\251cord todav\303\255a");
            if (rt > 0 || PortCoop_ChRivalGrade(c) > 0) sprintf(rv, "   \302\267   %.16s: %s  %d:%02d", PortCoop_RivalName()[0] ? PortCoop_RivalName() : "tu amigo",
                                                         PortCoop_GradeName(PortCoop_ChRivalGrade(c)), rt / 60, rt % 60);
            else rv[0] = 0;
            Ps_Cat(me, rv, 64);
            Ps_Text(330.0f + slide - 270.0f, 334.0f, me, 0.42f, 410.0f, PortCoop_ChWeekly(c) ? 1 : 3);
        }
        Co_Row(60.0f + slide, 300.0f + slide, 306.0f, "Crear sala", NULL, 1, 3);
        {   extern int PortCoop_ChWeekly(int);
            box = PortCoop_ChWeekly(c) ? "Jefe semanal: premio triple y un objeto. Cuanto antes ganes, mejor nota (contrarreloj). Izquierda/derecha: dificultad."
                : "Izquierda/derecha: dificultad. Cuanto antes ganes, mejor nota. Crea la sala y dale el c\303\263digo a tu amigo."; }
    }
    PortOnline_Raw(0);
    MsgWin_PortDrawStringHd(Co_Box(box));
}

/* ---------------------------------------------------------------------------------------------- inside a room */
/* "Mi personaje" over a room (menu/port_tbo.c): its pages without leaving, the room kept alive by `poll` each frame;
   back with Triangle, or at once when `stop` says the room moves on (the fight starts, or it closed). The room's own
   windows stay (the same ones as this screen's), so nothing is loaded here. */
static void Co_TestPoll(void) {}
static s32 Co_TestStop(void) { return 0; }
void PortCoop_RoomMe(void (*poll)(void), s32 (*stop)(void)) {
    CoopScreen *outer = sCo;

    sCo = Heap_Alloc(sizeof(CoopScreen), 0x20, 0, 2);
    memset(sCo, 0, sizeof(CoopScreen));
    sCo->page = CO_ME;
    sCo->cur = 3; /* on the points */
    sCoRoom = 1;
    sCoRoomDone = 0;
    sCoFromMe = 0;
    while (!sCoRoomDone) {
        Job_Run();
        Gfx_BeginFrame();
        Pad_Update();
        Snd_Update();
        ColorFade_Update();
        poll();
        if (getenv("BT4_RENDER_TEST") && getenv("BT4_COOP_ROOMME_TEST")) { /* (test: the page, its points, back) */
            extern void Port_TagUiCapture(int);
            if (sCo->frames == 30) Port_TagUiCapture(2);
            if (sCo->frames == 40) gPad[0].gamePressed = PADG_CROSS;
            if (sCo->frames == 70) Port_TagUiCapture(3);
            if (sCo->frames == 80) gPad[0].gamePressed = PADG_TRIANGLE;
            if (sCo->frames == 90) gPad[0].gamePressed = PADG_TRIANGLE;
        }
        if (stop()) sCoRoomDone = 1;
        Ps_PromptsClear();
        if (!sCoRoomDone) {
            if (sCo->page == CO_ME) Co_MeUpdate();
            else if (sCo->page == CO_NAME) Co_NameUpdate();
            else if (sCo->page == CO_POINTS) Co_PointsUpdate();
            else if (sCo->page == CO_TREE) Co_TreeUpdate();
            else if (sCo->page == CO_OBJ) Co_ObjUpdate();
            else if (sCo->page == CO_MISSIONS) Co_MissionsUpdate();
            else Co_SetPage(CO_ME);
        }
        Port_MenuPage(52);
        Ps_PageSet();
        Ps_SetShift(0);
        Co_Draw();
        Co_NoteDraw();
        Ps_PromptsDraw();
        ColorFade_Draw();
        Port_MenuPage(0);
        File_Stub264D90();
        Gfx_EndFrame(1);
        Dma_Flush();
        sCo->frames++;
    }
    sCoRoom = 0;
    sCoFromMe = 0;
    Heap_Free(sCo);
    sCo = outer;
    gPad[0].gamePressed = 0; /* (the Triangle that closed it is not the room's) */
}

/* ---------------------------------------------------------------------------------------------- the frame loop */
/* Returns 1 when left for the online screens (gPortTboCoop says what to do there), 2 for "Mi personaje"'s select,
   0 back to the main menu. */
s32 PortCoop_Run(void) {
    s32 go, before = PortCoop_Selected(), joined = PortCoop_Joining();

    { extern void PortCoop_SaveSync(void); PortCoop_SaveSync(); } /* (this save's own progress) */
    sCo = Heap_Alloc(sizeof(CoopScreen), 0x20, 0, 2);
    memset(sCo, 0, sizeof(CoopScreen));
    PortCoop_Load();
    if (sCoPickBack) { sCo->start = 5; sCoPickBack = 0; sCo->page = CO_ME; sCo->cur = 0; }
    else if (joined) sCo->start = 1; /* back from a room joined: the first page, on Join */
    else if (PortCoop_Infinite()) sCo->start = PortCoop_Infinite() == 3 ? 3 : 2; /* back from the endless mode: on it */
    else if (before >= 0 && before < PortCoop_Count()) { /* back from a room made: its chapter on its saga's list */
        Co_SagasBuild();
        sCoSaga = Co_SagaOf(before);
        sCo->cur = before;
        sCo->top = before - sSagaFirst[sCoSaga] >= CO_ROWS ? before - CO_ROWS + 1 : sSagaFirst[sCoSaga];
        sCo->page = CO_LIST;
    }
    PortCoop_Select(-1); /* (an ordinary room until a chapter is chosen again) */
    Co_WinInit();
    ColorFade_StartIn(0, 0, 0, 0x14);
    if (getenv("BT4_RENDER_TEST") && getenv("BT4_COOP_ROOMME_TEST") && !sCoPickTested) { sCoPickTested = 1; PortCoop_RoomMe(Co_TestPoll, Co_TestStop); } /* (test: as from a room) */
    if (getenv("BT4_RENDER_TEST") && getenv("BT4_COOP_PICK_TEST") && !sCoPickTested) { sCoPickTested = 1; sCoPickBack = 1; Co_Leave(2); } /* (test) */
    while (1) {
        s32 live;

        Job_Run();
        Gfx_BeginFrame();
        Pad_Update();
        Snd_Update();
        ColorFade_Update();
        live = !sCo->leaving && ColorFade_IsInDone();
        Ps_PromptsClear();
        if (live) {
            if (sCo->page == CO_START) Co_StartUpdate();
            else if (sCo->page == CO_LIST) Co_ListUpdate();
            else if (sCo->page == CO_PICK) Co_PickUpdate();
            else if (sCo->page == CO_MISSIONS) Co_MissionsUpdate();
            else if (sCo->page == CO_MAP) Co_MapUpdate();
            else if (sCo->page == CO_ME) Co_MeUpdate();
            else if (sCo->page == CO_NAME) Co_NameUpdate();
            else if (sCo->page == CO_POINTS) Co_PointsUpdate();
            else if (sCo->page == CO_TREE) Co_TreeUpdate();
            else if (sCo->page == CO_OBJ) Co_ObjUpdate();
            else Co_ChapterUpdate();
        }
        Port_MenuPage(52);
        Ps_PageSet();
        Ps_SetShift(0);
        Co_Draw();
        Co_NoteDraw(); /* (what was just won, bought or learnt) */
        Ps_PromptsDraw();
        ColorFade_Draw();
        Port_MenuPage(0);
        File_Stub264D90();
        Gfx_EndFrame(1);
        Dma_Flush();
        sCo->frames++;
        if (sCo->leaving && ColorFade_IsOutDone()) break;
    }
    sceGsSyncPath(0, 0);
    Co_WinTerm();
    go = sCo->go;
    Heap_Free(sCo);
    sCo = NULL;
    Dma_ResetBuffers();
    return go;
}
#endif
