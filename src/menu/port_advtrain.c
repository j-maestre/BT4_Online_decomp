/*
 * PC only: "Entrenamiento avanzado" (Advanced Training), a screen of the port's own reached from the main menu
 * (src/menu/menu_a.c, item MAINMENU_ADVTRAIN). Two pages:
 *   the list of trainings (for now one: the "fium fium" trainer, the circle evasion), and
 *   the chosen training's options and its Start.
 * Start leaves for the game's own training character select (progress mode 45, TrainMode_Main) with the defence drill
 * on for those fights (port/src/mods_game.c, PortDrill_*): the CPU attacks without a break. Cancelling that select
 * comes back here (src/menu/menu_h_c.c); the main menu forgets the drill (MainMenu_Init).
 * Drawn with the kit of menu/port_screen.c and the windows of the versus menu, as the online screens are.
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
extern int Port_Setting(const char *name, int def);
extern void Port_SettingSave(const char *name, int value);
extern void Port_SettingsWrite(void);
extern int Port_PageAspectMilli(void);
extern int Port_GameProfileEnglish(void);
extern void PortDrill_Force(int on);

enum { AT_LIST, AT_FIUM };
#define AT_TRAININGS 1
#define AT_FIUM_ROWS 3 /* attacks, pause, start */

typedef struct AdvTrain {
    s32 page;
    s32 cur[2];
    s32 mix, rest;
    s32 frames, pageFrom;
    s32 leaving, start;
    void *winRes;
} AdvTrain;

static AdvTrain *sAt;

static char *At_L(char *es, char *en) {
    return Port_GameProfileEnglish() ? en : es;
}

/* The game's 16-bit text for the message box, from UTF-8 (the font has the Spanish letters). */
static u16 *At_Box(char *utf8) {
    static u16 buf[2][160];
    static char last[160];
    static s32 cur;
    const u8 *s = (const u8 *)utf8;
    s32 n = 1;

    if (Ps_Len(last) > 0 && Ps_Len(utf8) == Ps_Len(last)) {
        s32 i, same = 1;
        for (i = 0; utf8[i]; i++) if (utf8[i] != last[i]) { same = 0; break; }
        if (same) return buf[cur];
    }
    Ps_Copy(last, utf8, 160);
    cur = !cur;
    buf[cur][0] = 0xFEFF;
    while (*s && n < 158) {
        u32 c = *s++;
        if (c >= 0xC0 && c < 0xE0 && *s) { c = ((c & 0x1F) << 6) | (*s++ & 0x3F); }
        else if (c >= 0xE0 && s[0] && s[1]) { c = ((c & 0x0F) << 12) | ((s[0] & 0x3F) << 6) | (s[1] & 0x3F); s += 2; }
        if (c == '<') c = '(';
        buf[cur][n++] = (u16)c;
    }
    buf[cur][n] = 0;
    return buf[cur];
}

static void At_WinInit(void) {
    void *icons;

    sAt->winRes = Sprite_Unpack(MPACK_AT(gMenuArc5, 1), NULL, NULL); /* the versus menu's windows, as Tb_WinInit */
    icons = MPACK_AT(sAt->winRes, 24);
    Res_RelocateOffsets(&icons, icons, icons);
    IconWin_Init(MPACK_AT(sAt->winRes, 23), icons);
    IconWin_PortHideWords();
    IconWin_Open();
    MsgWin_Init(MPACK_AT(sAt->winRes, 22), MPACK_AT(sAt->winRes, 21), 1, 0);
    MsgWin_SetText(NULL);
    MsgWin_PortSameTop();
    MsgWin_Open();
}

static void At_WinTerm(void) {
    if (sAt->winRes == NULL) return;
    MsgWin_Term();
    IconWin_Term();
    Heap_Free(sAt->winRes);
    sAt->winRes = NULL;
}

static void At_SetPage(s32 page) {
    sAt->page = page;
    sAt->pageFrom = sAt->frames;
}

/* 0..1 as a page slides in. */
static f32 At_In(void) {
    f32 t = (f32)(sAt->frames - sAt->pageFrom) / 12.0f;
    if (t > 1.0f) t = 1.0f;
    return 1.0f - (1.0f - t) * (1.0f - t);
}

static char *At_MixName(s32 mix) {
    switch (mix) {
    case 1: return At_L("Cuerpo a cuerpo y t\303\251cnicas", "Melee and techniques");
    case 2: return At_L("Solo t\303\251cnicas", "Techniques only");
    default: return At_L("Cuerpo a cuerpo", "Melee");
    }
}

/* ---------------------------------------------------------------------------------------------- the pages' input */
static void At_ListUpdate(void) {
    Pad *pad = &gPad[0];

    Ps_Prompt(0, At_L("Elegir", "Select"));
    Ps_Prompt(2, At_L("Volver", "Back"));
    Ps_Move(&sAt->cur[0], AT_TRAININGS);
    if (pad->gamePressed & (PADG_CROSS | PADG_START)) {
        Snd_PlaySe(1, 1);
        sAt->cur[1] = AT_FIUM_ROWS - 1;
        At_SetPage(AT_FIUM);
    } else if (pad->gamePressed & PADG_TRIANGLE) {
        Snd_PlaySe(1, 2);
        sAt->leaving = 1;
        ColorFade_StartOut(0, 0, 0, 0x14);
    }
}

static void At_FiumUpdate(void) {
    Pad *pad = &gPad[0];
    s32 side;

    Ps_Prompt(3, At_L("Cambiar", "Change"));
    Ps_Prompt(0, At_L("Empezar", "Start"));
    Ps_Prompt(2, At_L("Volver", "Back"));
    Ps_Move(&sAt->cur[1], AT_FIUM_ROWS);
    side = Ps_Side();
    if (side != 0 && sAt->cur[1] == 0) {
        sAt->mix = (sAt->mix + side + 3) % 3;
        Snd_PlaySe(1, 0);
    } else if (side != 0 && sAt->cur[1] == 1) {
        s32 r = sAt->rest + side * 5;
        sAt->rest = r < 10 ? 10 : r > 90 ? 90 : r;
        Snd_PlaySe(1, 0);
    }
    if (pad->gamePressed & (PADG_CROSS | PADG_START)) {
        Snd_PlaySe(1, 1);
        Port_SettingSave("train_drill_mix", sAt->mix);
        Port_SettingSave("train_drill_rest", sAt->rest);
        Port_SettingsWrite();
        sAt->start = 1;
        sAt->leaving = 1;
        ColorFade_StartOut(0, 0, 0, 0x14);
    } else if (pad->gamePressed & PADG_TRIANGLE) {
        Snd_PlaySe(1, 2);
        At_SetPage(AT_LIST);
    }
}

/* ---------------------------------------------------------------------------------------------- drawing */
static void At_Backdrop(void) {
    f32 aspect = (f32)Port_PageAspectMilli() / 1000.0f;
    f32 pic = Ps_PicW(TBO_BG) / Ps_PicH(TBO_BG);
    f32 u = aspect < pic ? aspect / pic : 1.0f;
    f32 v = aspect < pic ? 1.0f : pic / aspect;

    Ps_SpriteScreen(TBO_BG, 0.0f, 0.0f, 512.0f, 448.0f, 0.5f - u * 0.5f, 0.5f - v * 0.5f, 0.5f + u * 0.5f, 0.5f + v * 0.5f,
                    PS_PLAIN, 0x80);
    Ps_Rect(Ps_Left() - 200.0f, 0.0f, 712.0f, 448.0f, Ps_Rgba(0, 0, 0, 0x30), Ps_Rgba(0, 0, 0, 0x50));
}

/* One row of a list: a bar (the chosen one gold and longer), its text, and an optional value at its right. */
static void At_Row(f32 y, char *text, char *value, s32 chosen, f32 slide) {
    f32 x0 = 40.0f + slide, x1 = 400.0f + slide;

    if (chosen) Ps_Pill(TBO_KIT_SEL, x0 - 6.0f, y - 3.0f, x1 + 14.0f, y + 27.0f, 0x80);
    else Ps_Pill(TBO_KIT_BAR, x0, y, x1, y + 24.0f, 0x78);
    PortOnline_Raw(1);
    Ps_Text(x0 + 16.0f, Ps_TextTop(y + 12.0f, 0.62f), text, 0.62f, value ? 170.0f : 330.0f, chosen ? 1 : 0);
    if (value) {
        Ps_Align(2);
        Ps_Text(x1 - 12.0f, Ps_TextTop(y + 12.0f, 0.58f), value, 0.58f, 180.0f, 3);
        Ps_Align(0);
    }
    PortOnline_Raw(0);
}

static void At_Draw(void) {
    f32 slide = (1.0f - At_In()) * -160.0f;
    char *box;
    char rest[48];

    At_Backdrop();
    IconWin_Draw();
    PortOnline_Raw(1);
    Ps_Heading(Ps_Left() + 50.0f, Ps_TextTop(32.0f, 1.3f),
               sAt->page == AT_LIST ? At_L("Entrenamiento avanzado", "Advanced Training")
                                    : At_L("Entrenador de fium fium", "Fium Fium Trainer"), 1.3f, 300.0f, 0x80);
    PortOnline_Raw(0);
    if (sAt->page == AT_LIST) {
        At_Row(110.0f, At_L("Entrenador de fium fium", "Fium Fium Trainer"), NULL, sAt->cur[0] == 0, slide);
        PortOnline_Raw(1);
        Ps_Text(60.0f + slide, 150.0f, At_L("M\303\241s entrenamientos pr\303\263ximamente", "More trainings coming soon"), 0.5f, 330.0f, 4);
        PortOnline_Raw(0);
        box = At_L("La CPU te ataca sin parar y t\303\272 tienes que esquivar con c\303\255rculo (fium fium). "
                   "El mod Entrenador de defensa te dice si fue a tiempo.",
                   "The CPU attacks without a break and you evade with circle. "
                   "The Defence trainer mod tells you if it was in time.");
    } else {
        sprintf(rest, At_L("%d ticks (%d.%d s)", "%d ticks (%d.%d s)"), sAt->rest, sAt->rest / 30, (sAt->rest % 30) * 10 / 30);
        At_Row(100.0f, At_L("Ataques", "Attacks"), At_MixName(sAt->mix), sAt->cur[1] == 0, slide);
        At_Row(134.0f, At_L("Pausa entre ataques", "Pause between attacks"), rest, sAt->cur[1] == 1, slide);
        At_Row(180.0f, At_L("Empezar: elegir personajes", "Start: choose fighters"), NULL, sAt->cur[1] == 2, slide);
        box = sAt->cur[1] == 0 ? At_L("Qu\303\251 lanza la CPU: golpes cuerpo a cuerpo, t\303\251cnicas o ambos.",
                                      "What the CPU throws: melee hits, techniques or both.")
            : sAt->cur[1] == 1 ? At_L("Tiempo de descanso entre ataque y ataque. Menos pausa es m\303\241s dif\303\255cil.",
                                      "The rest between attacks. Less is harder.")
                               : At_L("Elige tu personaje, el rival y el escenario. Fija al rival (lock-on) para que el c\303\255rculo esquive.",
                                      "Choose your fighter, the rival and the stage. Lock on so that circle evades.");
    }
    MsgWin_PortDrawStringHd(At_Box(box));
}

/* ---------------------------------------------------------------------------------------------- the frame loop */
/* Returns 1 when left for the training character select (the drill is on), 0 back to the main menu. */
s32 PortAdvTrain_Run(void) {
    s32 start;

    sAt = Heap_Alloc(sizeof(AdvTrain), 0x20, 0, 2);
    memset(sAt, 0, sizeof(AdvTrain));
    sAt->mix = Port_Setting("train_drill_mix", 0);
    if (sAt->mix < 0 || sAt->mix > 2) sAt->mix = 0;
    sAt->rest = Port_Setting("train_drill_rest", 30);
    if (sAt->rest < 10 || sAt->rest > 90) sAt->rest = 30;
    At_WinInit();
    ColorFade_StartIn(0, 0, 0, 0x14);
    while (1) {
        s32 live;

        Job_Run();
        Gfx_BeginFrame();
        Pad_Update();
        Snd_Update();
        ColorFade_Update();
        live = !sAt->leaving && ColorFade_IsInDone();
        Ps_PromptsClear();
        if (live) {
            if (sAt->page == AT_LIST) At_ListUpdate();
            else At_FiumUpdate();
        }
        Port_MenuPage(52);
        Ps_PageSet();
        Ps_SetShift(0);
        At_Draw();
        Ps_PromptsDraw();
        ColorFade_Draw();
        Port_MenuPage(0);
        File_Stub264D90();
        Gfx_EndFrame(1);
        Dma_Flush();
        sAt->frames++;
        if (sAt->leaving && ColorFade_IsOutDone()) break;
    }
    sceGsSyncPath(0, 0);
    At_WinTerm();
    start = sAt->start;
    Heap_Free(sAt);
    sAt = NULL;
    Dma_ResetBuffers();
    PortDrill_Force(start);
    return start;
}
#endif
