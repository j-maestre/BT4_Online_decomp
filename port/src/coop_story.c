/* Historia cooperativa (co-op story): chapters for two players against two CPU fighters in a Tag Team room.
 *
 * The chapters are a Lua table in historia_coop/capitulos.lua next to the program (read here, once per visit of the
 * screen, with the Lua the mods use). Nothing of a chapter travels over the network: the room's content fingerprint
 * (which a joining player must match, shared/tag_kernel.c via port/src/tag_team.c, Tt_Join) is mixed with the
 * chapter file and the chosen chapter (PortCoop_MixContent), so only a player with the same file who chose the same
 * chapter can join, and both PCs apply the same chapter on their own:
 *   - the host's room keeps the friend on team 0 and the chapter's CPUs on team 1 (tag_team_net.c, coop_arrange);
 *   - the host's draft picks the CPUs' fighters and the stage (src/menu/tag_team_select.inc);
 *   - both set the CPUs' health when the battle is configured (src/battle/battle_work.c, TagBattle_ConfigureLocal);
 *   - the end of the battle gives the closing or the defeat line (src/battle/btl_seq.c -> PortCoop_BattleEnded).
 * The selection is cleared by the main menu (src/menu/menu_a.c), so ordinary Tag Team rooms are untouched.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../third_party/lua/lua.h"
#include "../third_party/lua/lauxlib.h"
#include "../third_party/lua/lualib.h"

#define COOP_MAX 64
#define COOP_FILE "historia_coop/capitulos.lua"

typedef struct PortCoopFoe { int chara, costume, level, health; char name[40]; } PortCoopFoe;
typedef struct PortCoopChapter {
    char id[32], saga[48], title[64], pair[64];
    char intro[400], outro[256], defeat[256];
    int stage, bgm, time;
    PortCoopFoe foe[2];
} PortCoopChapter;

static PortCoopChapter sCh[COOP_MAX];
static int sCount, sSelected = -1, sOutcome;
static unsigned long long sFileHash;
static char sError[200];

extern int Port_Setting(const char *name, int def);
extern void Port_SettingSave(const char *name, int value);
extern void Port_SettingsWrite(void);
extern int gPortResim;

static void field_str(lua_State *L, const char *key, char *out, int cap) {
    lua_getfield(L, -1, key);
    out[0] = 0;
    if (lua_type(L, -1) == LUA_TSTRING) snprintf(out, cap, "%s", lua_tostring(L, -1));
    lua_pop(L, 1);
}

static int field_int(lua_State *L, const char *key, int def, int lo, int hi) {
    int v = def;
    lua_getfield(L, -1, key);
    if (lua_isinteger(L, -1)) v = (int)lua_tointeger(L, -1);
    else if (lua_isnumber(L, -1)) v = (int)lua_tonumber(L, -1);
    lua_pop(L, 1);
    return v < lo ? lo : v > hi ? hi : v;
}

/* Reads the chapter file again; the number of chapters (0 with PortCoop_Error saying why). */
int PortCoop_Load(void) {
    lua_State *L;
    FILE *f;
    long n;
    char *text;
    int i;

    sCount = 0;
    sError[0] = 0;
    sFileHash = 0;
    f = fopen(COOP_FILE, "rb");
    if (!f) { snprintf(sError, sizeof(sError), "No encuentro %s junto al juego.", COOP_FILE); return 0; }
    fseek(f, 0, SEEK_END);
    n = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (n <= 0 || n > 1024 * 1024) { fclose(f); snprintf(sError, sizeof(sError), "%s está vacío o es demasiado grande.", COOP_FILE); return 0; }
    text = malloc((size_t)n + 1);
    if (!text || fread(text, 1, (size_t)n, f) != (size_t)n) { fclose(f); free(text); snprintf(sError, sizeof(sError), "No puedo leer %s.", COOP_FILE); return 0; }
    fclose(f);
    text[n] = 0;
    {   /* the file's fingerprint: part of the room's (two players must have the same chapters) */
        unsigned long long h = 0xCBF29CE484222325ull;
        for (i = 0; i < n; i++) h = (h ^ (unsigned char)text[i]) * 0x100000001B3ull;
        sFileHash = h;
    }
    L = luaL_newstate();
    if (!L) { free(text); snprintf(sError, sizeof(sError), "Sin memoria para Lua."); return 0; }
    /* the libraries the mods have (port/src/gs/mods.c): the program has no luaL_openlibs, nor io / os */
    luaL_requiref(L, LUA_GNAME, luaopen_base, 1);
    luaL_requiref(L, LUA_TABLIBNAME, luaopen_table, 1);
    luaL_requiref(L, LUA_STRLIBNAME, luaopen_string, 1);
    luaL_requiref(L, LUA_MATHLIBNAME, luaopen_math, 1);
    lua_settop(L, 0);
    if (luaL_loadbuffer(L, text, (size_t)n, "@" COOP_FILE) != LUA_OK || lua_pcall(L, 0, 1, 0) != LUA_OK) {
        snprintf(sError, sizeof(sError), "Error en %s: %s", COOP_FILE, lua_tostring(L, -1));
        lua_close(L);
        free(text);
        return 0;
    }
    free(text);
    if (!lua_istable(L, -1)) {
        snprintf(sError, sizeof(sError), "%s tiene que devolver una lista de capítulos (return { ... }).", COOP_FILE);
        lua_close(L);
        return 0;
    }
    for (i = 1; sCount < COOP_MAX; i++) {
        PortCoopChapter *c = &sCh[sCount];
        int k;
        lua_rawgeti(L, -1, i);
        if (!lua_istable(L, -1)) { lua_pop(L, 1); break; }
        memset(c, 0, sizeof(*c));
        field_str(L, "id", c->id, sizeof(c->id));
        field_str(L, "saga", c->saga, sizeof(c->saga));
        field_str(L, "titulo", c->title, sizeof(c->title));
        field_str(L, "pareja", c->pair, sizeof(c->pair));
        field_str(L, "intro", c->intro, sizeof(c->intro));
        field_str(L, "cierre", c->outro, sizeof(c->outro));
        field_str(L, "derrota", c->defeat, sizeof(c->defeat));
        c->stage = field_int(L, "escenario", -1, -1, 0x62);
        c->bgm = field_int(L, "musica", -1, -1, 65535);
        c->time = field_int(L, "tiempo", 4, 0, 4);
        lua_getfield(L, -1, "rivales");
        for (k = 0; k < 2; k++) {
            PortCoopFoe *foe = &c->foe[k];
            foe->chara = -1; foe->costume = 0; foe->level = 2; foe->health = 100; foe->name[0] = 0;
            if (lua_istable(L, -1)) {
                lua_rawgeti(L, -1, k + 1);
                if (lua_istable(L, -1)) {
                    foe->chara = field_int(L, "personaje", -1, -1, 511);
                    foe->costume = field_int(L, "traje", 0, 0, 9);
                    foe->level = field_int(L, "nivel", 2, 0, 4);
                    foe->health = field_int(L, "vida", 100, 10, 400);
                    field_str(L, "nombre", foe->name, sizeof(foe->name));
                }
                lua_pop(L, 1);
            }
        }
        lua_pop(L, 1);
        lua_pop(L, 1);
        if (!c->title[0]) snprintf(c->title, sizeof(c->title), "Capítulo %d", i);
        if (!c->id[0]) snprintf(c->id, sizeof(c->id), "cap%d", i);
        if (c->foe[0].chara < 0 || c->foe[1].chara < 0) {
            snprintf(sError, sizeof(sError), "El capítulo %d (%s) necesita dos rivales con «personaje».", i, c->title);
            continue;
        }
        sCount++;
    }
    lua_close(L);
    if (sCount == 0 && !sError[0]) snprintf(sError, sizeof(sError), "%s no tiene capítulos.", COOP_FILE);
    if (sSelected >= sCount) sSelected = -1;
    return sCount;
}

const char *PortCoop_Error(void) { return sError; }
int PortCoop_Count(void) { return sCount; }
const PortCoopChapter *PortCoop_Get(int i) { return i >= 0 && i < sCount ? &sCh[i] : NULL; }

/* The chapter the room is for (-1: none, an ordinary Tag Team room). */
void PortCoop_Select(int i) { sSelected = i >= 0 && i < sCount ? i : -1; sOutcome = 0; }
int PortCoop_Selected(void) { return sSelected; }
int PortCoop_Active(void) { return sSelected >= 0; }

/* The room's content fingerprint for the chosen chapter. */
unsigned long long PortCoop_MixContent(unsigned long long base) {
    unsigned long long h;
    const char *s;
    if (sSelected < 0) return base;
    h = sFileHash ^ 0xC0035709ull;
    for (s = sCh[sSelected].id; *s; s++) h = (h ^ (unsigned char)*s) * 0x100000001B3ull;
    h = (h ^ (unsigned)sSelected) * 0x100000001B3ull;
    return base ^ h ^ (h << 29);
}

static const PortCoopFoe *foe(int k) { return sSelected >= 0 && k >= 0 && k < 2 ? &sCh[sSelected].foe[k] : NULL; }
int PortCoop_FoeChara(int k) { return foe(k) ? foe(k)->chara : -1; }
int PortCoop_FoeCostume(int k) { return foe(k) ? foe(k)->costume : 0; }
int PortCoop_FoeLevel(int k) { return foe(k) ? foe(k)->level : 2; }
int PortCoop_FoeHealth(int k) { return foe(k) ? foe(k)->health : 100; }
int PortCoop_Stage(void) { return sSelected >= 0 ? sCh[sSelected].stage : -1; }
int PortCoop_Bgm(void) { return sSelected >= 0 ? sCh[sSelected].bgm : -1; }
int PortCoop_Time(void) { return sSelected >= 0 ? sCh[sSelected].time : 4; }

static void done_key(char *out, int cap, int i) {
    const char *s = sCh[i].id;
    int n = snprintf(out, cap, "coop_done_");
    for (; *s && n < cap - 1; s++) out[n++] = (*s >= 'a' && *s <= 'z') || (*s >= '0' && *s <= '9') || *s == '_' ? *s : '_';
    out[n] = 0;
}
int PortCoop_Done(int i) {
    char key[64];
    if (i < 0 || i >= sCount) return 0;
    done_key(key, sizeof(key), i);
    return Port_Setting(key, 0) != 0;
}

/* The end of a Tag Team battle (src/battle/btl_seq.c): winner bits of the result, 1 = team 0 (the players). */
void PortCoop_BattleEnded(int winner) {
    if (gPortResim || sSelected < 0) return;
    sOutcome = (winner & 1) ? 1 : 2;
    if (sOutcome == 1) {
        char key[64];
        done_key(key, sizeof(key), sSelected);
        Port_SettingSave(key, 1);
        Port_SettingsWrite();
    }
}

/* The line to show after the battle, once: the closing text or the defeat line; NULL when there is none. */
const char *PortCoop_TakeOutcome(void) {
    int o = sOutcome;
    sOutcome = 0;
    if (sSelected < 0 || o == 0) return NULL;
    if (o == 1) return sCh[sSelected].outro[0] ? sCh[sSelected].outro : "¡Capítulo superado!";
    return sCh[sSelected].defeat[0] ? sCh[sSelected].defeat : "Habéis caído... ¡Inténtalo otra vez!";
}

/* The chosen chapter's title (the room's heading on the online screens), NULL with none. */
const char *PortCoop_Title(void) { return sSelected >= 0 ? sCh[sSelected].title : NULL; }

/* Chapter i's fields, for the chapter screen (src/menu/port_coop.c): plain strings and numbers, no structure crosses
   into the game code. */
static const PortCoopChapter *ch(int i) { return i >= 0 && i < sCount ? &sCh[i] : NULL; }
const char *PortCoop_ChTitle(int i) { return ch(i) ? ch(i)->title : ""; }
const char *PortCoop_ChSaga(int i) { return ch(i) ? ch(i)->saga : ""; }
const char *PortCoop_ChPair(int i) { return ch(i) ? ch(i)->pair : ""; }
const char *PortCoop_ChIntro(int i) { return ch(i) ? ch(i)->intro : ""; }
const char *PortCoop_ChFoeName(int i, int k) { return ch(i) && k >= 0 && k < 2 ? ch(i)->foe[k].name : ""; }
int PortCoop_ChFoeChara(int i, int k) { return ch(i) && k >= 0 && k < 2 ? ch(i)->foe[k].chara : -1; }
int PortCoop_ChFoeLevel(int i, int k) { return ch(i) && k >= 0 && k < 2 ? ch(i)->foe[k].level : 2; }
int PortCoop_ChFoeHealth(int i, int k) { return ch(i) && k >= 0 && k < 2 ? ch(i)->foe[k].health : 100; }
