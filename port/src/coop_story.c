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

#define COOP_FOES 4        /* rivals on the field at once (the room: 2 players + these) */
#define COOP_RESERVES 16   /* reinforcements: each comes in when a rival on the field falls */
#define COOP_PER_ACTOR 4   /* behind one rival (a battle side has 5 member slots) */

typedef struct PortCoopFoe { int chara, costume, level, health, behind, transform; char name[40]; } PortCoopFoe;
#define COOP_DIALOG 8      /* lines of a chapter's conversation */
typedef struct PortCoopChapter {
    char id[32], saga[48], title[64], pair[64];
    char intro[400], outro[256], defeat[256];
    int stage, bgm, time;
    int foes, reserves, dialogs;
    PortCoopFoe foe[COOP_FOES], reserve[COOP_RESERVES];
    char dlgWho[COOP_DIALOG][32], dlgText[COOP_DIALOG][200];
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

static int field_int(lua_State *L, const char *key, int def, int lo, int hi);

/* A list of fighters { {personaje=..., traje=..., nivel=..., vida=..., nombre=...}, ... } under `key`; how many. */
static int read_foes(lua_State *L, const char *key, PortCoopFoe *out, int max) {
    int n = 0, k;
    lua_getfield(L, -1, key);
    for (k = 1; lua_istable(L, -1) && n < max; k++) {
        PortCoopFoe *foe = &out[n];
        lua_rawgeti(L, -1, k);
        if (!lua_istable(L, -1)) { lua_pop(L, 1); break; }
        foe->chara = field_int(L, "personaje", -1, -1, 511);
        foe->costume = field_int(L, "traje", 0, 0, 9);
        foe->level = field_int(L, "nivel", 2, 0, 4);
        foe->health = field_int(L, "vida", 100, 10, 400);
        foe->behind = field_int(L, "detras", 0, 0, 4) - 1; /* (reinforcements: which rival, 1-based; else in turns) */
        foe->transform = field_int(L, "transforma", 0, 0, 90); /* (below this % of health: its next form comes in) */
        field_str(L, "nombre", foe->name, sizeof(foe->name));
        lua_pop(L, 1);
        if (foe->chara >= 0) n++;
    }
    lua_pop(L, 1);
    return n;
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
        c->foes = read_foes(L, "rivales", c->foe, COOP_FOES);
        c->reserves = read_foes(L, "refuerzos", c->reserve, COOP_RESERVES);
        lua_getfield(L, -1, "dialogo"); /* the conversation: { {"who", "what"}, ... } */
        for (k = 1; lua_istable(L, -1) && c->dialogs < COOP_DIALOG; k++) {
            lua_rawgeti(L, -1, k);
            if (!lua_istable(L, -1)) { lua_pop(L, 1); break; }
            lua_rawgeti(L, -1, 1);
            if (lua_type(L, -1) == LUA_TSTRING) snprintf(c->dlgWho[c->dialogs], 32, "%s", lua_tostring(L, -1));
            lua_pop(L, 1);
            lua_rawgeti(L, -1, 2);
            if (lua_type(L, -1) == LUA_TSTRING) snprintf(c->dlgText[c->dialogs], 200, "%s", lua_tostring(L, -1));
            lua_pop(L, 1);
            if (c->dlgText[c->dialogs][0]) c->dialogs++;
            lua_pop(L, 1);
        }
        lua_pop(L, 1);
        for (k = 0; k < c->reserves; k++) /* behind which rival each reinforcement comes in */
            if (c->reserve[k].behind < 0 || c->reserve[k].behind >= c->foes) c->reserve[k].behind = c->foes > 0 ? k % c->foes : 0;
        lua_pop(L, 1);
        if (!c->title[0]) snprintf(c->title, sizeof(c->title), "Capítulo %d", i);
        if (!c->id[0]) snprintf(c->id, sizeof(c->id), "cap%d", i);
        if (c->foes < 1) {
            snprintf(sError, sizeof(sError), "El capítulo %d (%s) necesita al menos un rival con «personaje».", i, c->title);
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

/* The chapter the room is for (-1: none, an ordinary Tag Team room). sJoining: Join by code was chosen in the co-op
   screen; the chapter is not known yet and comes from the host (PortCoop_Announced, through the room's chat). */
static int sJoining;
/* The endless mode (src/battle/coop_infinite.c generates its rivals): 0 = off, else the rivals at once (2 or 3). */
static int sInfinite, sInfWaves;
extern int CoopInf_FirstChara(int k), CoopInf_Wave(void);
static int sDiff;
void PortCoop_Select(int i) { sSelected = i >= 0 && i < sCount ? i : -1; sOutcome = 0; sJoining = 0; sInfinite = 0; sDiff = Port_Setting("coop_dificultad", 0); if (sDiff < 0 || sDiff > 2) sDiff = 0; }
void PortCoop_SelectInfinite(int foes) { sSelected = -1; sOutcome = 0; sJoining = 0; sInfinite = foes == 3 ? 3 : 2; }
void PortCoop_JoinBegin(void) { sSelected = -1; sOutcome = 0; sJoining = 1; sInfinite = 0; sDiff = 0; }
int PortCoop_Selected(void) { return sSelected; }
int PortCoop_Infinite(void) { return sInfinite; }
int PortCoop_Active(void) { return sSelected >= 0 || sJoining || sInfinite; }
int PortCoop_InfRecord(int foes) { char key[32]; snprintf(key, sizeof(key), "coop_inf_record_%d", foes); return Port_Setting(key, 0); }
int PortCoop_Joining(void) { return sJoining; }

/* The room's content fingerprint: both players must have the same capitulos.lua (not the same chapter: the joiner
   learns it from the host). */
unsigned long long PortCoop_MixContent(unsigned long long base) {
    unsigned long long h;
    if (!PortCoop_Active()) return base;
    h = (sFileHash ^ 0xC0035709ull) * 0x100000001B3ull;
    return base ^ h ^ (h << 29);
}

/* The host's announcement, "#coop:<id>", sent in the room's chat (port/src/gs/tag_team_net.c). */
/* The chapter's difficulty (0 normal, 1 hard, 2 extreme): the host's choice (setting coop_dificultad), the joiner's
   from the announcement. The rivals' health and damage and the AI's level go up, and so do the rewards. */
static const int kDiffHealth[3] = {100, 140, 180}, kDiffDamage[3] = {100, 125, 150}, kDiffXp[3] = {100, 125, 150};
static const int kDiffZp[3] = {100, 150, 200}, kDiffAi[3] = {0, 1, 2};
int PortCoop_Difficulty(void) { return sSelected >= 0 ? sDiff : 0; }
void PortCoop_SetDifficulty(int d) { sDiff = d < 0 ? 0 : d > 2 ? 2 : d; if (!sJoining) { Port_SettingSave("coop_dificultad", sDiff); Port_SettingsWrite(); } }
int PortCoop_FoeDamagePct(void) { return kDiffDamage[PortCoop_Difficulty()]; }
const char *PortCoop_DifficultyName(int d) { static const char *n[3] = {"Normal", "Dif\303\255cil", "Extremo"}; return n[d < 0 ? 0 : d > 2 ? 2 : d]; }
const char *PortCoop_AnnounceId(void) {
    static char buf[80];
    if (sInfinite) return sInfinite == 3 ? "__inf3" : "__inf2";
    if (sSelected < 0) return NULL;
    snprintf(buf, sizeof(buf), "%s@%d", sCh[sSelected].id, sDiff);
    return buf;
}
void PortCoop_Announced(const char *text) {
    int i;
    char id[72];
    const char *at;
    if (!sJoining || !text) return;
    snprintf(id, sizeof(id), "%s", text);
    at = strchr(text, '@');
    if (at) { id[at - text < (int)sizeof(id) ? at - text : (int)sizeof(id) - 1] = 0; if (at[1] >= '0' && at[1] <= '2') sDiff = at[1] - '0'; }
    else sDiff = 0;
    if (!strcmp(id, "__inf2") || !strcmp(id, "__inf3")) { /* the endless mode (the joiner stays "joining": no chapter) */
        int foes = id[5] - '0';
        if (sInfinite != foes) { sInfinite = foes; sSelected = -1; sOutcome = 0; fprintf(stderr, "coop: the host's mode is endless, %d rivals\n", foes); }
        return;
    }
    for (i = 0; i < sCount; i++)
        if (!strcmp(sCh[i].id, id)) {
            if (sSelected != i) { sSelected = i; sOutcome = 0; fprintf(stderr, "coop: the host's chapter is %s\n", id); }
            return;
        }
    fprintf(stderr, "coop: the host's chapter %s is not in this capitulos.lua\n", id);
}

static const PortCoopFoe *foe(int k) { return sSelected >= 0 && k >= 0 && k < sCh[sSelected].foes ? &sCh[sSelected].foe[k] : NULL; }
int PortCoop_FoeCount(void) { return sInfinite ? sInfinite : sSelected >= 0 ? sCh[sSelected].foes : 2; }

/* The reinforcements behind rival k (they enter one after another when it falls): reinforcement r of the list goes
   behind rival r % foes, so they come in turns. How many; the j-th through the pointers. */
static int reserve_index(int k, int j) { /* the list index of the j-th reinforcement behind rival k, or -1 */
    int r, n = 0;
    if (sSelected < 0 || k < 0 || k >= sCh[sSelected].foes) return -1;
    for (r = 0; r < sCh[sSelected].reserves; r++)
        if (sCh[sSelected].reserve[r].behind == k && n++ == j) return n > COOP_PER_ACTOR ? -1 : r;
    return -1;
}
int PortCoop_ReserveCount(int k) {
    int n = 0;
    while (reserve_index(k, n) >= 0) n++;
    return n;
}
int PortCoop_Reserve(int k, int j, int *chara, int *costume, int *level, int *health) {
    int r = reserve_index(k, j);
    if (r < 0) return 0;
    *chara = sCh[sSelected].reserve[r].chara;
    *costume = sCh[sSelected].reserve[r].costume;
    *level = sCh[sSelected].reserve[r].level + kDiffAi[PortCoop_Difficulty()];
    if (*level > 4) *level = 4;
    *health = sCh[sSelected].reserve[r].health * kDiffHealth[PortCoop_Difficulty()] / 100;
    return 1;
}
int PortCoop_FoeChara(int k) { return sInfinite ? (k >= 0 && k < sInfinite ? CoopInf_FirstChara(k) : -1) : foe(k) ? foe(k)->chara : -1; }
int PortCoop_FoeCostume(int k) { return foe(k) ? foe(k)->costume : 0; }
/* The % of health below which rival k (j < 0) or its j-th reinforcement transforms into the one behind it; 0: never. */
int PortCoop_Transform(int k, int j) {
    int r;
    if (j < 0) return foe(k) ? foe(k)->transform : 0;
    r = reserve_index(k, j);
    return r >= 0 ? sCh[sSelected].reserve[r].transform : 0;
}
int PortCoop_FoeLevel(int k) { int l = (foe(k) ? foe(k)->level : 2) + kDiffAi[PortCoop_Difficulty()]; return l > 4 ? 4 : l; }
int PortCoop_FoeHealth(int k) { return (foe(k) ? foe(k)->health : 100) * kDiffHealth[PortCoop_Difficulty()] / 100; }
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
/* ------------------------------------------------------------------------------------------------------------------
 * Levels, grades and Z points.
 * Each player has a fighter of their own: a copy of a character they choose (the co-op screen, "Mi personaje"),
 * offered as an extra cell of the character select. Picked from that cell it has its level (1..100) from the
 * experience it earned, and the battle makes it weaker or stronger by it (src/battle/coop_infinite.c, CoopLv_*);
 * the same character from its own cell is the original, as always.
 * After each battle the player's fighter earns experience by how it went: the share of the rivals' health it took,
 * the partners it revived, the times it fell, the victory. A chapter won also gives a grade, E D C B A S Z, the best
 * one kept for the chapter list, and Z points (the story mode's money) by the grade.
 * ---------------------------------------------------------------------------------------------------------------- */
#define RPG_MAX_LEVEL 100
static char sRewardLine[200];
static int sRewardZp;
static const char *const kGrades[8] = {"", "E", "D", "C", "B", "A", "S", "Z"};

static int sUsingMine; /* the last pick was my own fighter's cell (src/menu/tag_team_select.inc) */
static int sPickedLevel; /* its level when picked: kept for a rematch with the same picks (the partner was told this one) */
int PortRpg_MyChara(void) { int c = Port_Setting("coop_mi_chara", 1); return c >= 0 && c < 512 ? c : 1; }
void PortRpg_SetMyChara(int chara) { Port_SettingSave("coop_mi_chara", chara); Port_SettingsWrite(); }
int PortRpg_MyLevel(void);
void PortRpg_PickMine(int mine) { sUsingMine = mine != 0; sPickedLevel = sUsingMine ? PortRpg_MyLevel() : 0; if (getenv("BT4_RENDER_TEST")) fprintf(stderr, "coop: pick from my own cell: %d\n", sUsingMine); }
int PortRpg_UsingMine(void) { return sUsingMine; }
void Port_TraceLine(const char *s) { if (getenv("BT4_RENDER_TEST")) { fprintf(stderr, "%s\n", s); fflush(stderr); } }
void Port_TraceInt(const char *s, int v) { if (getenv("BT4_RENDER_TEST")) { fprintf(stderr, "%s %d\n", s, v); fflush(stderr); } }
static int PortRpg_Xp(void) { return Port_Setting("coop_mi_xp", 0); }
/* experience from level L to L+1 */
static int rpg_need(int level) { return 40 + 5 * level; }
int PortRpg_LevelOfXp(int xp) {
    int level = 1;
    while (level < RPG_MAX_LEVEL && xp >= rpg_need(level)) { xp -= rpg_need(level); level++; }
    return level;
}
int PortRpg_MyLevel(void) { return PortRpg_LevelOfXp(PortRpg_Xp()); }
/* the level the battle uses for my pick: my own fighter's, or 0 (the original) */
int PortRpg_PickLevel(void) { return sUsingMine ? sPickedLevel : 0; }
/* the experience inside the current level, in thousandths of what the next one needs (for a bar) */
int PortRpg_LevelMille(void) {
    int xp = PortRpg_Xp(), level = 1;
    while (level < RPG_MAX_LEVEL && xp >= rpg_need(level)) { xp -= rpg_need(level); level++; }
    return level >= RPG_MAX_LEVEL ? 1000 : xp * 1000 / rpg_need(level);
}

static void grade_key(char *out, int cap, int i) { done_key(out, cap, i); memcpy(out, "coop_nota_", 10); }
/* The best grade of chapter i: "" without one. */
const char *PortCoop_ChGrade(int i) {
    char key[64]; int g;
    if (i < 0 || i >= sCount) return "";
    grade_key(key, sizeof(key), i);
    g = Port_Setting(key, 0);
    return g >= 1 && g <= 7 ? kGrades[g] : "";
}
/* The hardest difficulty chapter i was won at, + 1 (0: never won since difficulties came). */
int PortCoop_ChBestDiff(int i) {
    char key[64];
    if (i < 0 || i >= sCount) return 0;
    done_key(key, sizeof(key), i); memcpy(key, "coop_difi_", 10);
    return Port_Setting(key, 0);
}
/* The last battle's results, for the results panel (src/menu/port_tbo.c): whole numbers by field. */
enum { RES_VALID, RES_WON, RES_INFINITE, RES_WAVES, RES_GRADE, RES_SCORE, RES_SHARE, RES_REVIVES, RES_DOWNS, RES_ZP, RES_XP,
       RES_BEFORE, RES_AFTER, RES_MILLE, RES_DIFF, RES_MINE, RES_COUNT };
static int sRes[RES_COUNT], sResCaps;
int PortCoop_ResultCapsules(void) { return sResCaps; }
int PortCoop_Result(int field) { return field >= 0 && field < RES_COUNT ? sRes[field] : 0; }
const char *PortCoop_ResultGrade(void) { return kGrades[sRes[RES_GRADE] >= 0 && sRes[RES_GRADE] <= 7 ? sRes[RES_GRADE] : 0]; }
void PortCoop_ResultSeen(void) { sRes[RES_VALID] = 0; }
const char *PortCoop_RewardLine(void) { return sRewardLine; }
/* The Z points earned by the last battle, once (src/menu/port_tbo.c pays them into the save). */
int PortCoop_TakeZPoints(void) { int z = sRewardZp; sRewardZp = 0; return z; }

/* ------------------------------------------------------------------------------------------------------------------
 * Statistics, missions and capsules (src/menu/port_coop.c shows them). Everything in bt3_settings.txt:
 * - coop_st_*: the totals of every co-op battle this player played.
 * - Missions: three a day and three a week, the same for everyone that day (picked from the date), counted from the
 *   battles' results; once done, claimed on the co-op screen for Z points, experience and capsules.
 * - Capsules: an EXP capsule doubles the experience of the next battle with "Mi personaje", a Z capsule the Z points
 *   of the next battle that gives some; each is used by itself, one at a time.
 * ---------------------------------------------------------------------------------------------------------------- */
#include <time.h>
enum { MT_WIN, MT_REVIVE, MT_GRADE_A, MT_HARD, MT_WAVES, MT_HALF, MT_NODOWN, MT_MINE, MT_GRADE_S, MT_EXTREME, MT_COUNT };
static const char *const kMisText[MT_COUNT] = {
    "Gana %d capítulo%s", "Revive a tu compañero %d ve%s", "Consigue nota A o mejor %d ve%s",
    "Gana %d capítulo%s en Difícil o Extremo", "Supera %d oleadas en el modo infinito",
    "Haz más de la mitad del daño en %d victoria%s", "Gana %d capítulo%s sin caer",
    "Juega %d combate%s con Mi personaje", "Consigue nota S o Z %d ve%s", "Gana %d capítulo%s en Extremo"};
static const int kDayPool[8][2] = {{MT_WIN, 2}, {MT_REVIVE, 2}, {MT_GRADE_A, 1}, {MT_HARD, 1}, {MT_WAVES, 5}, {MT_HALF, 1},
                                   {MT_NODOWN, 1}, {MT_MINE, 3}};
static const int kWeekPool[8][2] = {{MT_WIN, 10}, {MT_REVIVE, 10}, {MT_GRADE_S, 3}, {MT_EXTREME, 1}, {MT_WAVES, 25},
                                    {MT_NODOWN, 5}, {MT_MINE, 15}, {MT_HARD, 5}};
static long mis_day(void) { /* days since 1970 in local time */
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    long y = t ? t->tm_year + 1900 : 1970, d = t ? t->tm_yday : 0;
    return (y - 1970) * 365 + (y - 1969) / 4 + d;
}
static long mis_week(void) { return (mis_day() + 3) / 7; } /* (weeks from a Monday) */
static unsigned mis_rand(unsigned *s) { *s = *s * 1103515245u + 12345u; return (*s >> 16) & 0x7FFF; }
/* mission k (0..2 today's, 3..5 this week's): its kind and its goal */
static void mis_pick(int k, int *kind, int *goal) {
    int weekly = k >= 3, slot = k % 3, used[3], i, n = 0;
    unsigned s = (unsigned)(weekly ? mis_week() * 7919 + 17 : mis_day() * 104729 + 3);
    const int (*pool)[2] = weekly ? kWeekPool : kDayPool;
    while (n <= slot) {
        int j = (int)(mis_rand(&s) % 8), dup = 0;
        for (i = 0; i < n; i++) if (used[i] == j) dup = 1;
        if (!dup) used[n++] = j;
    }
    *kind = pool[used[slot]][0];
    *goal = pool[used[slot]][1];
}
static void mis_key(char *out, int cap, int k, const char *what) { snprintf(out, cap, "coop_mis%d_%s", k, what); }
/* today's and this week's counters, made new when the day or the week changed */
static void mis_fresh(void) {
    char key[32];
    int k;
    long day = mis_day(), week = mis_week();
    if (Port_Setting("coop_mis_day", -1) != day) {
        for (k = 0; k < 3; k++) { mis_key(key, sizeof(key), k, "n"); Port_SettingSave(key, 0); mis_key(key, sizeof(key), k, "ok"); Port_SettingSave(key, 0); }
        Port_SettingSave("coop_mis_day", (int)day);
    }
    if (Port_Setting("coop_mis_week", -1) != week) {
        for (k = 3; k < 6; k++) { mis_key(key, sizeof(key), k, "n"); Port_SettingSave(key, 0); mis_key(key, sizeof(key), k, "ok"); Port_SettingSave(key, 0); }
        Port_SettingSave("coop_mis_week", (int)week);
    }
}
int PortMis_Goal(int k) { int kind, goal; if (k < 0 || k > 5) return 0; mis_pick(k, &kind, &goal); return goal; }
int PortMis_Progress(int k) {
    char key[32];
    int n;
    if (k < 0 || k > 5) return 0;
    mis_fresh();
    mis_key(key, sizeof(key), k, "n");
    n = Port_Setting(key, 0);
    return n > PortMis_Goal(k) ? PortMis_Goal(k) : n;
}
int PortMis_Claimed(int k) { char key[32]; if (k < 0 || k > 5) return 0; mis_fresh(); mis_key(key, sizeof(key), k, "ok"); return Port_Setting(key, 0); }
const char *PortMis_Text(int k) {
    static char buf[6][96];
    int kind, goal;
    if (k < 0 || k > 5) return "";
    mis_pick(k, &kind, &goal);
    if (kind == MT_WAVES) snprintf(buf[k], sizeof(buf[k]), kMisText[kind], goal);
    else if (kind == MT_REVIVE || kind == MT_GRADE_A || kind == MT_GRADE_S) snprintf(buf[k], sizeof(buf[k]), kMisText[kind], goal, goal == 1 ? "z" : "ces");
    else snprintf(buf[k], sizeof(buf[k]), kMisText[kind], goal, goal == 1 ? "" : "s");
    return buf[k];
}
/* the rewards: Z points, experience, capsules (0 EXP, 1 Z) */
static void mis_reward(int k, int *zp, int *xp, int *capExp, int *capZp) {
    *zp = k >= 3 ? 20000 : k == 2 ? 1500 : 3000;
    *xp = k >= 3 ? 400 : 80;
    *capExp = k == 2 ? (int)(mis_day() & 1) : k == 3 ? 1 : 0;
    *capZp = k == 2 ? (int)!(mis_day() & 1) : k == 3 ? 1 : 0;
}
const char *PortMis_RewardText(int k) {
    static char buf[6][96];
    int zp, xp, ce, cz, n;
    if (k < 0 || k > 5) return "";
    mis_reward(k, &zp, &xp, &ce, &cz);
    n = snprintf(buf[k], sizeof(buf[k]), "%d Puntos Z, %d EXP", zp, xp);
    if (ce) n += snprintf(buf[k] + n, sizeof(buf[k]) - n, ", cápsula EXP");
    if (cz) snprintf(buf[k] + n, sizeof(buf[k]) - n, ", cápsula Z");
    return buf[k];
}
/* Hours left for today's missions, days for this week's. */
int PortMis_HoursLeft(void) { time_t now = time(NULL); struct tm *t = localtime(&now); return t ? 24 - t->tm_hour : 24; }
int PortMis_DaysLeft(void) { return 7 - (int)((mis_day() + 3) % 7); }
int PortItem_Count(int which) { return Port_Setting(which ? "coop_cap_zp" : "coop_cap_exp", 0); }
/* Claims a mission done: 1 when it paid now (its Z points through src/menu/menu_p_b.c, into the save). */
int PortMis_Claim(int k) {
    extern void PortCoop_AddMoney(int n);
    char key[32];
    int zp, xp, ce, cz;
    if (k < 0 || k > 5 || PortMis_Claimed(k) || PortMis_Progress(k) < PortMis_Goal(k)) return 0;
    mis_reward(k, &zp, &xp, &ce, &cz);
    mis_key(key, sizeof(key), k, "ok");
    Port_SettingSave(key, 1);
    Port_SettingSave("coop_mi_xp", Port_Setting("coop_mi_xp", 0) + xp);
    if (ce) Port_SettingSave("coop_cap_exp", PortItem_Count(0) + ce);
    if (cz) Port_SettingSave("coop_cap_zp", PortItem_Count(1) + cz);
    Port_SettingsWrite();
    PortCoop_AddMoney(zp);
    fprintf(stderr, "coop: mission %d claimed: %d Z points, %d EXP, capsules %d/%d\n", k, zp, xp, ce, cz);
    return 1;
}
static void mis_add(int kind, int n) {
    char key[32];
    int k, mk, goal;
    if (n <= 0) return;
    mis_fresh();
    for (k = 0; k < 6; k++) {
        mis_pick(k, &mk, &goal);
        if (mk != kind) continue;
        mis_key(key, sizeof(key), k, "n");
        Port_SettingSave(key, Port_Setting(key, 0) + n);
    }
}
/* The statistics (src/menu/port_coop.c). */
enum { ST_BATTLES, ST_WINS, ST_MINE, ST_DAMAGE, ST_REVIVES, ST_DOWNS, ST_BEST, ST_GRADE_S, ST_WAVES, ST_EXTREME, ST_ZP, ST_COUNT };
static const char *const kStatKey[ST_COUNT] = {"coop_st_battles", "coop_st_wins", "coop_st_mine", "coop_st_damage",
    "coop_st_revives", "coop_st_downs", "coop_st_best", "coop_st_grade_s", "coop_st_waves", "coop_st_extreme", "coop_st_zp"};
int PortStat(int k) { return k >= 0 && k < ST_COUNT ? Port_Setting(kStatKey[k], 0) : 0; }
const char *PortStat_BestGrade(void) { int g = PortStat(ST_BEST); return g >= 1 && g <= 7 ? kGrades[g] : "-"; }
static void stat_add(int k, int n) { if (n) Port_SettingSave(kStatKey[k], PortStat(k) + n); }
/* After each co-op battle (rpg_reward): the statistics and the missions. */
static void rpg_count(int won, int grade, int share, int revives, int downs, int dealt) {
    stat_add(ST_BATTLES, 1);
    stat_add(ST_WINS, won);
    stat_add(ST_MINE, sUsingMine);
    stat_add(ST_DAMAGE, dealt);
    stat_add(ST_REVIVES, revives);
    stat_add(ST_DOWNS, downs);
    stat_add(ST_ZP, sRewardZp);
    if (grade > PortStat(ST_BEST)) Port_SettingSave(kStatKey[ST_BEST], grade);
    stat_add(ST_GRADE_S, grade >= 6);
    if (sInfinite && sInfWaves > PortStat(ST_WAVES)) Port_SettingSave(kStatKey[ST_WAVES], sInfWaves);
    stat_add(ST_EXTREME, won && sDiff == 2);
    mis_add(MT_WIN, won);
    mis_add(MT_REVIVE, revives);
    mis_add(MT_GRADE_A, grade >= 5);
    mis_add(MT_GRADE_S, grade >= 6);
    mis_add(MT_HARD, won && sDiff >= 1);
    mis_add(MT_EXTREME, won && sDiff == 2);
    mis_add(MT_WAVES, sInfinite ? sInfWaves : 0);
    mis_add(MT_HALF, won && share > 500);
    mis_add(MT_NODOWN, won && downs == 0);
    mis_add(MT_MINE, sUsingMine);
}

static void rpg_reward(int won) {
    extern int TagBattle_LocalActor(void), CoopLv_DamageShareMille(int), CoopLv_Revives(int), CoopLv_Downs(int);
    static const int zp[8] = {0, 500, 1000, 2000, 3000, 5000, 7500, 10000};
    int me = TagBattle_LocalActor();
    int share = CoopLv_DamageShareMille(me), revives = CoopLv_Revives(me), downs = CoopLv_Downs(me);
    int score, grade = 0, xp, before, after, n = 0;
    char key[32];

    /* the score, 0..100: winning, the share of the damage, the partners revived, not falling */
    score = 40 + share * 40 / 1000 + revives * 8 + (downs == 0 ? 20 : downs == 1 ? 8 : 0);
    if (score > 100) score = 100;
    if (sInfinite) {
        xp = 30 + sInfWaves * 25 + share * 30 / 1000 + revives * 10; /* (the endless mode: no difficulty) */
        sRewardZp = sInfWaves * 400;
    } else if (won) {
        grade = score >= 95 ? 7 : score >= 85 ? 6 : score >= 72 ? 5 : score >= 60 ? 4 : score >= 48 ? 3 : score >= 36 ? 2 : 1;
        xp = (50 + score + sSelected * 3) * kDiffXp[sDiff] / 100;
        sRewardZp = (zp[grade] + sSelected * 100) * kDiffZp[sDiff] / 100;
        grade_key(key, sizeof(key), sSelected);
        if (grade > Port_Setting(key, 0)) Port_SettingSave(key, grade);
        memcpy(key, "coop_difi_", 10);
        if (sDiff + 1 > Port_Setting(key, 0)) Port_SettingSave(key, sDiff + 1);
    } else {
        xp = 20 + share * 30 / 1000 + revives * 8;
        sRewardZp = 0;
    }
    if (!sUsingMine) xp = 0; /* (the original fighter: no experience; the grade and the Z points all the same) */
    {   int capExp = 0, capZp = 0; /* the capsules, by themselves: one each, when it doubles something */
        if (xp > 0 && PortItem_Count(0) > 0) { xp *= 2; capExp = 1; Port_SettingSave("coop_cap_exp", PortItem_Count(0) - 1); }
        if (sRewardZp > 0 && PortItem_Count(1) > 0) { sRewardZp *= 2; capZp = 1; Port_SettingSave("coop_cap_zp", PortItem_Count(1) - 1); }
        sResCaps = capExp | capZp << 1;
    }
    {   extern int CoopLv_Dealt(int);
        rpg_count(won, grade, share, revives, downs, CoopLv_Dealt(me));
    }
    before = PortRpg_MyLevel();
    if (xp) Port_SettingSave("coop_mi_xp", PortRpg_Xp() + xp);
    Port_SettingsWrite();
    after = PortRpg_MyLevel();
    if (grade) n += snprintf(sRewardLine + n, sizeof(sRewardLine) - n, "Nota %s  ", kGrades[grade]);
    if (sRewardZp) n += snprintf(sRewardLine + n, sizeof(sRewardLine) - n, "+%d Puntos Z  ", sRewardZp);
    if (xp && n < (int)sizeof(sRewardLine))
        n += snprintf(sRewardLine + n, sizeof(sRewardLine) - n, after > before ? "+%d EXP: ¡nivel %d!" : "+%d EXP (nivel %d)", xp, after);
    if (sResCaps && n < (int)sizeof(sRewardLine))
        n += snprintf(sRewardLine + n, sizeof(sRewardLine) - n, "  (cápsula x2)");
    sRes[RES_VALID] = 1; sRes[RES_WON] = won; sRes[RES_INFINITE] = sInfinite; sRes[RES_WAVES] = sInfWaves; sRes[RES_GRADE] = grade;
    sRes[RES_SCORE] = score; sRes[RES_SHARE] = share; sRes[RES_REVIVES] = revives; sRes[RES_DOWNS] = downs; sRes[RES_ZP] = sRewardZp;
    sRes[RES_XP] = xp; sRes[RES_BEFORE] = before; sRes[RES_AFTER] = after; sRes[RES_MILLE] = PortRpg_LevelMille();
    sRes[RES_DIFF] = sInfinite ? 0 : sDiff; sRes[RES_MINE] = sUsingMine;
    fprintf(stderr, "coop: reward score=%d share=%d revives=%d downs=%d grade=%s xp=%d level %d->%d zp=%d\n", score, share,
            revives, downs, kGrades[grade], xp, before, after, sRewardZp);
}

void PortCoop_BattleEnded(int winner) {
    if (sInfinite && !gPortResim) {
        char key[32];
        sInfWaves = CoopInf_Wave();
        snprintf(key, sizeof(key), "coop_inf_record_%d", sInfinite);
        if (sInfWaves > Port_Setting(key, 0)) { Port_SettingSave(key, sInfWaves); Port_SettingsWrite(); sOutcome = 4; }
        else sOutcome = 3;
        rpg_reward(0);
        fprintf(stderr, "coop: endless mode over: %d waves\n", sInfWaves);
        return;
    }
    if (gPortResim || sSelected < 0) return;
    sOutcome = (winner & 1) ? 1 : 2;
    if (sOutcome == 1) {
        char key[64];
        done_key(key, sizeof(key), sSelected);
        Port_SettingSave(key, 1);
        Port_SettingsWrite();
    }
    rpg_reward(sOutcome == 1);
}

/* The line to show after the battle, once: the closing text or the defeat line; NULL when there is none. */
const char *PortCoop_TakeOutcome(void) {
    if (sInfinite && sOutcome >= 3) {
        static char line[256];
        int o = sOutcome, rec = PortCoop_InfRecord(sInfinite);
        sOutcome = 0;
        if (o == 4) snprintf(line, sizeof(line), "¡Nuevo récord! Habéis superado %d oleadas.  %s", sInfWaves, sRewardLine);
        else snprintf(line, sizeof(line), "Habéis superado %d oleadas. Vuestro récord: %d.  %s", sInfWaves, rec, sRewardLine);
        return line;
    }
    int o = sOutcome;
    sOutcome = 0;
    if (sSelected < 0 || o == 0) return NULL;
    {   static char line[256];
        snprintf(line, sizeof(line), "%s  %s", o == 1 ? (sCh[sSelected].outro[0] ? sCh[sSelected].outro : "¡Capítulo superado!")
                 : (sCh[sSelected].defeat[0] ? sCh[sSelected].defeat : "Habéis caído... ¡Inténtalo otra vez!"), sRewardLine);
        return line;
    }
}

/* The chosen chapter's title (the room's heading on the online screens), NULL with none. */
const char *PortCoop_Title(void) { return sInfinite == 3 ? "Modo infinito (2 contra 3)" : sInfinite ? "Modo infinito (2 contra 2)" : sSelected >= 0 ? sCh[sSelected].title : NULL; }

/* Chapter i's fields, for the chapter screen (src/menu/port_coop.c): plain strings and numbers, no structure crosses
   into the game code. */
static const PortCoopChapter *ch(int i) { return i >= 0 && i < sCount ? &sCh[i] : NULL; }
const char *PortCoop_ChTitle(int i) { return ch(i) ? ch(i)->title : ""; }
const char *PortCoop_ChSaga(int i) { return ch(i) ? ch(i)->saga : ""; }
const char *PortCoop_ChPair(int i) { return ch(i) ? ch(i)->pair : ""; }
const char *PortCoop_ChIntro(int i) { return ch(i) ? ch(i)->intro : ""; }
const char *PortCoop_ChFoeName(int i, int k) { return ch(i) && k >= 0 && k < COOP_FOES ? ch(i)->foe[k].name : ""; }
int PortCoop_ChFoeChara(int i, int k) { return ch(i) && k >= 0 && k < COOP_FOES ? ch(i)->foe[k].chara : -1; }
int PortCoop_ChFoeLevel(int i, int k) { return ch(i) && k >= 0 && k < COOP_FOES ? ch(i)->foe[k].level : 2; }
int PortCoop_ChFoeHealth(int i, int k) { return ch(i) && k >= 0 && k < COOP_FOES ? ch(i)->foe[k].health : 100; }
int PortCoop_ChFoeCount(int i) { return ch(i) ? ch(i)->foes : 0; }
int PortCoop_ChReserveCount(int i) { return ch(i) ? ch(i)->reserves : 0; }
int PortCoop_ChDialogCount(int i) { return ch(i) ? ch(i)->dialogs : 0; }
const char *PortCoop_ChDialogWho(int i, int k) { return ch(i) && k >= 0 && k < ch(i)->dialogs ? ch(i)->dlgWho[k] : ""; }
const char *PortCoop_ChDialogText(int i, int k) { return ch(i) && k >= 0 && k < ch(i)->dialogs ? ch(i)->dlgText[k] : ""; }
/* A chapter can be played once the one before it is won (the first always). */
int PortCoop_Unlocked(int i) { return i <= 0 || PortCoop_Done(i) || PortCoop_Done(i - 1); }
