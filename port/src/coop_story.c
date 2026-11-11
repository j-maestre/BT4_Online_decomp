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

typedef struct PortCoopFoe { int chara, costume, level, health, behind; char name[40]; } PortCoopFoe;
typedef struct PortCoopChapter {
    char id[32], saga[48], title[64], pair[64];
    char intro[400], outro[256], defeat[256];
    int stage, bgm, time;
    int foes, reserves;
    PortCoopFoe foe[COOP_FOES], reserve[COOP_RESERVES];
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
void PortCoop_Select(int i) { sSelected = i >= 0 && i < sCount ? i : -1; sOutcome = 0; sJoining = 0; sInfinite = 0; }
void PortCoop_SelectInfinite(int foes) { sSelected = -1; sOutcome = 0; sJoining = 0; sInfinite = foes == 3 ? 3 : 2; }
void PortCoop_JoinBegin(void) { sSelected = -1; sOutcome = 0; sJoining = 1; sInfinite = 0; }
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
const char *PortCoop_AnnounceId(void) { return sInfinite == 3 ? "__inf3" : sInfinite ? "__inf2" : sSelected >= 0 ? sCh[sSelected].id : NULL; }
void PortCoop_Announced(const char *id) {
    int i;
    if (!sJoining || !id) return;
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
    *level = sCh[sSelected].reserve[r].level;
    *health = sCh[sSelected].reserve[r].health;
    return 1;
}
int PortCoop_FoeChara(int k) { return sInfinite ? (k >= 0 && k < sInfinite ? CoopInf_FirstChara(k) : -1) : foe(k) ? foe(k)->chara : -1; }
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
int PortRpg_MyChara(void) { int c = Port_Setting("coop_mi_chara", 1); return c >= 0 && c < 512 ? c : 1; }
void PortRpg_SetMyChara(int chara) { Port_SettingSave("coop_mi_chara", chara); Port_SettingsWrite(); }
void PortRpg_PickMine(int mine) { sUsingMine = mine != 0; if (getenv("BT4_RENDER_TEST")) fprintf(stderr, "coop: pick from my own cell: %d\n", sUsingMine); }
int PortRpg_UsingMine(void) { return sUsingMine; }
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
int PortRpg_PickLevel(void) { return sUsingMine ? PortRpg_MyLevel() : 0; }
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
const char *PortCoop_RewardLine(void) { return sRewardLine; }
/* The Z points earned by the last battle, once (src/menu/port_tbo.c pays them into the save). */
int PortCoop_TakeZPoints(void) { int z = sRewardZp; sRewardZp = 0; return z; }

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
        xp = 30 + sInfWaves * 25 + share * 30 / 1000 + revives * 10;
        sRewardZp = sInfWaves * 400;
    } else if (won) {
        grade = score >= 95 ? 7 : score >= 85 ? 6 : score >= 72 ? 5 : score >= 60 ? 4 : score >= 48 ? 3 : score >= 36 ? 2 : 1;
        xp = 50 + score + sSelected * 3;
        sRewardZp = zp[grade] + sSelected * 100;
        grade_key(key, sizeof(key), sSelected);
        if (grade > Port_Setting(key, 0)) Port_SettingSave(key, grade);
    } else {
        xp = 20 + share * 30 / 1000 + revives * 8;
        sRewardZp = 0;
    }
    if (!sUsingMine) xp = 0; /* (the original fighter: no experience; the grade and the Z points all the same) */
    before = PortRpg_MyLevel();
    if (xp) Port_SettingSave("coop_mi_xp", PortRpg_Xp() + xp);
    Port_SettingsWrite();
    after = PortRpg_MyLevel();
    if (grade) n += snprintf(sRewardLine + n, sizeof(sRewardLine) - n, "Nota %s  ", kGrades[grade]);
    if (sRewardZp) n += snprintf(sRewardLine + n, sizeof(sRewardLine) - n, "+%d Puntos Z  ", sRewardZp);
    if (xp && n < (int)sizeof(sRewardLine))
        n += snprintf(sRewardLine + n, sizeof(sRewardLine) - n, after > before ? "+%d EXP: ¡nivel %d!" : "+%d EXP (nivel %d)", xp, after);
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
/* A chapter can be played once the one before it is won (the first always). */
int PortCoop_Unlocked(int i) { return i <= 0 || PortCoop_Done(i) || PortCoop_Done(i - 1); }
