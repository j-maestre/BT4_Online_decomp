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
extern s32 gPortTboCoop;
char gPortCoopNote[160]; /* why the last room could not be made or joined (src/menu/port_tbo.c), shown on the first page */

enum { CO_START, CO_LIST, CO_CHAPTER };
#define CO_ROWS 7 /* chapters on the list at once */

typedef struct CoopScreen {
    s32 page, cur, top, act, start;
    s32 frames, pageFrom;
    s32 leaving, go;
    void *winRes;
} CoopScreen;

static CoopScreen *sCo;

/* The game's 16-bit text for the message box, from UTF-8. */
static u16 *Co_Box(const char *utf8) {
    static u16 buf[2][224];
    static const char *last;
    static s32 cur;
    const u8 *s = (const u8 *)utf8;
    s32 n = 1;

    if (utf8 == last) return buf[cur];
    last = utf8;
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
    Ps_Move(&sCo->start, 4);
    if (pad->gamePressed & (PADG_CROSS | PADG_START)) {
        Snd_PlaySe(1, 1);
        gPortCoopNote[0] = 0;
        if (sCo->start == 0) Co_SetPage(CO_LIST);
        else if (sCo->start == 1) { PortCoop_JoinBegin(); gPortTboCoop = 2; Co_Leave(1); }
        else { PortCoop_SelectInfinite(sCo->start == 2 ? 2 : 3); gPortTboCoop = 1; Co_Leave(1); }
    } else if (pad->gamePressed & PADG_TRIANGLE) {
        Snd_PlaySe(1, 2);
        Co_Leave(0);
    }
}

static void Co_ListUpdate(void) {
    Pad *pad = &gPad[0];
    s32 count = PortCoop_Count();

    Ps_Prompt(0, count > 0 ? "Elegir" : "Volver");
    Ps_Prompt(2, "Volver");
    Ps_Prompt(1, "Releer");
    if (count > 0) {
        Ps_Move(&sCo->cur, count);
        if (sCo->cur < sCo->top) sCo->top = sCo->cur;
        if (sCo->cur >= sCo->top + CO_ROWS) sCo->top = sCo->cur - CO_ROWS + 1;
    }
    if ((pad->gamePressed & (PADG_CROSS | PADG_START)) && count > 0 && !PortCoop_Unlocked(sCo->cur)) {
        Snd_PlaySe(1, 2); /* (locked: the one before it first) */
    } else if ((pad->gamePressed & (PADG_CROSS | PADG_START)) && count > 0) {
        Snd_PlaySe(1, 1);
        sCo->act = 0;
        Co_SetPage(CO_CHAPTER);
    } else if (pad->gamePressed & (PADG_TRIANGLE | ((count == 0) ? PADG_CROSS : 0))) {
        Snd_PlaySe(1, 2);
        Co_SetPage(CO_START);
    } else if (pad->gamePressed & PADG_SQUARE) { /* the file read again (after editing it) */
        Snd_PlaySe(1, 0);
        PortCoop_Load();
        sCo->cur = sCo->top = 0;
    }
}

static void Co_ChapterUpdate(void) {
    Pad *pad = &gPad[0];

    Ps_Prompt(0, "Crear sala");
    Ps_Prompt(2, "Volver");
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
        Co_Wrap(50.0f + slide, 82.0f, "T\303\272 y un amigo, cada uno en su casa, contra los rivales de la CPU de cada cap\303\255tulo. "
                "Uno crea la sala y elige el cap\303\255tulo; el otro se une con el c\303\263digo.", 0.55f, 410.0f, 20.0f, 4, 0);
        {   char rec[2][40];
            sprintf(rec[0], "R\303\251cord: %d", PortCoop_InfRecord(2));
            sprintf(rec[1], "R\303\251cord: %d", PortCoop_InfRecord(3));
            Co_Row(60.0f + slide, 380.0f + slide, 170.0f, "Crear sala (historia)", NULL, sCo->start == 0, 3);
            Co_Row(60.0f + slide, 380.0f + slide, 200.0f, "Unirse a una sala", NULL, sCo->start == 1, 3);
            Co_Row(60.0f + slide, 380.0f + slide, 236.0f, "Modo infinito: 2 contra 2", rec[0], sCo->start == 2, 3);
            Co_Row(60.0f + slide, 380.0f + slide, 266.0f, "Modo infinito: 2 contra 3", rec[1], sCo->start == 3, 3);
        }
        if (gPortCoopNote[0]) Co_Wrap(50.0f + slide, 302.0f, gPortCoopNote, 0.5f, 410.0f, 18.0f, 3, 5);
        box = sCo->start == 0
            ? "Elige el cap\303\255tulo y crea la sala. Despu\303\251s dale el c\303\263digo a tu amigo."
            : sCo->start == 1
            ? "Escribe el c\303\263digo que te d\303\251 tu amigo. El cap\303\255tulo o el modo lo elige quien crea la sala."
            : "Oleadas sin fin: cuando cae un rival entra otro, cada vez con m\303\241s vida y m\303\241s da\303\261o. Cada 5 oleadas, un jefe. Crea la sala y pasa el c\303\263digo.";
    } else if (sCo->page == CO_LIST) {
        Ps_Heading(Ps_Left() + 50.0f, Ps_TextTop(32.0f, 1.3f), "Historia cooperativa", 1.3f, 300.0f, 0x80);
        if (count == 0) {
            Co_Wrap(50.0f + slide, 100.0f, PortCoop_Error(), 0.55f, 400.0f, 20.0f, 6, 4);
            box = "Revisa historia_coop/capitulos.lua junto al juego. Cuadrado: leerlo otra vez.";
        } else {
            for (i = sCo->top; i < count && i < sCo->top + CO_ROWS; i++) {
                s32 open = PortCoop_Unlocked(i);
                char *value = PortCoop_Done(i) ? "Completado" : open ? (char *)PortCoop_ChSaga(i) : "Bloqueado";
                Co_Row(40.0f + slide, 420.0f + slide, 82.0f + 30.0f * (i - sCo->top), open ? (char *)PortCoop_ChTitle(i) : "? ? ?", value,
                       i == sCo->cur, PortCoop_Done(i) ? 5 : open ? 3 : 4);
            }
            if (sCo->top > 0) Ps_Text(220.0f + slide, 66.0f, "...", 0.5f, 40.0f, 4);
            if (sCo->top + CO_ROWS < count) Ps_Text(220.0f + slide, 82.0f + 30.0f * CO_ROWS, "...", 0.5f, 40.0f, 4);
            box = PortCoop_Unlocked(sCo->cur)
                ? "Elige un cap\303\255tulo. Cada uno que super\303\251is desbloquea el siguiente. Cuadrado: volver a leer los cap\303\255tulos."
                : "Bloqueado: primero ten\303\251is que superar el cap\303\255tulo anterior.";
        }
    } else {
        s32 c = sCo->cur, k;
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
        Co_Row(60.0f + slide, 300.0f + slide, 296.0f, "Crear sala", NULL, 1, 3);
        box = "Crea la sala y dale el c\303\263digo a tu amigo: \303\251l solo tiene que elegir \"Unirse a una sala\".";
    }
    PortOnline_Raw(0);
    MsgWin_PortDrawStringHd(Co_Box(box));
}

/* ---------------------------------------------------------------------------------------------- the frame loop */
/* Returns 1 when left for the online screens (gPortTboCoop says what to do there), 0 back to the main menu. */
s32 PortCoop_Run(void) {
    s32 go, before = PortCoop_Selected(), joined = PortCoop_Joining();

    sCo = Heap_Alloc(sizeof(CoopScreen), 0x20, 0, 2);
    memset(sCo, 0, sizeof(CoopScreen));
    PortCoop_Load();
    if (joined) sCo->start = 1; /* back from a room joined: the first page, on Join */
    else if (PortCoop_Infinite()) sCo->start = PortCoop_Infinite() == 3 ? 3 : 2; /* back from the endless mode: on it */
    else if (before >= 0 && before < PortCoop_Count()) { /* back from a room made: its chapter on the list */
        sCo->cur = before;
        sCo->top = before >= CO_ROWS ? before - CO_ROWS + 1 : 0;
        sCo->page = CO_LIST;
    }
    PortCoop_Select(-1); /* (an ordinary room until a chapter is chosen again) */
    Co_WinInit();
    ColorFade_StartIn(0, 0, 0, 0x14);
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
            else Co_ChapterUpdate();
        }
        Port_MenuPage(52);
        Ps_PageSet();
        Ps_SetShift(0);
        Co_Draw();
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
