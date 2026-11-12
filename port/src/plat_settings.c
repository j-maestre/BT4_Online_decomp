/*
 * The player's settings, kept between runs in a small text file of `name=number` lines:
 *
 *     bt3_settings.txt in the current directory, or the path in $BT3_SETTINGS (an empty $BT3_SETTINGS: no file)
 *
 * The settings overlay (port/src/gs/gs_gpu.c) writes it whenever a setting changes. At start an environment
 * variable (BT3_SCALE, BT3_ASPECT, ...) still wins over the file, so a single run can be started differently.
 * Names: scale, aspect_milli, fullscreen, fx_off, glow, music, effects, voices, stage_sounds, hud_layout, game_speed, music_follows_speed, display; the bindings of port/src/gs/gs_input.c
 * (key_p1_cross = an SDL scancode, pad_p1_cross = a controller button, padslot_p1 = which controller,
 * deadzone_p1_ls / deadzone_p1_rs = a stick's deadzone, vibration_p1 and vibration_p1_strength).
 */
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define MAX_SETTINGS 200
static struct { char name[24]; int value; } sSet[MAX_SETTINGS];
/* The few settings that are text (online name, menu language): `name="text"` in the file, after the numbers. A
   release from before these reads past such a line. */
#define MAX_TEXTS 16
static struct { char name[24]; char value[64]; } sText[MAX_TEXTS];
static int sTextCount;
static int sDirty;
static int sCount;
static pthread_once_t sOnce = PTHREAD_ONCE_INIT;
static pthread_mutex_t sLock = PTHREAD_MUTEX_INITIALIZER;

static const char *path(void) {
    const char *p = getenv("BT3_SETTINGS");
    return p == NULL ? "bt3_settings.txt" : p[0] == '\0' ? NULL : p;
}

static void load(void);
/* Historia cooperativa's progress (every setting named "coop_..."): kept apart, one file for each game save, so
   that each save has its own fighter and a new game starts from nothing:
       <saves folder>/coop_<id>.txt      (the saves folder: saves, or $BT3_SAVES; the updates never touch it)
   The id is written in the save itself (src/menu/menu_p_b.c, PortCoop_SaveSync); id 1 is the save that was in use
   when this came: its file starts with the "coop_" settings of bt3_settings.txt. Until a save is chosen the
   "coop_" settings stay in bt3_settings.txt, as before. */
#define MAX_COOP 1500
static struct { char name[40]; int value; } sCoop[MAX_COOP];
static struct { char name[40]; char value[64]; } sCoopText[32];
static int sCoopCount, sCoopTextCount, sCoopDirty;
static unsigned sCoopId; /* 0: none chosen */
static int is_coop(const char *name) { return sCoopId != 0 && strncmp(name, "coop_", 5) == 0; }
static void coop_path(char *out, int size, unsigned id) {
    const char *root = getenv("BT3_SAVES") != NULL ? getenv("BT3_SAVES") : "saves";
    snprintf(out, (size_t)size, "%s/coop_%08x.txt", root, id);
}
static void coop_write_locked(void) {
    char file[600];
    FILE *fp;
    int i;
    if (!sCoopDirty || sCoopId == 0) return;
    coop_path(file, sizeof(file), sCoopId);
    fp = fopen(file, "w");
    if (fp == NULL) return;
    for (i = 0; i < sCoopCount; i++) fprintf(fp, "%s=%d\n", sCoop[i].name, sCoop[i].value);
    for (i = 0; i < sCoopTextCount; i++) fprintf(fp, "%s=\"%s\"\n", sCoopText[i].name, sCoopText[i].value);
    fclose(fp);
    sCoopDirty = 0;
}
/* The game save in use (its co-op id): its progress is loaded. */
void Port_CoopUseSave(unsigned id) {
    char file[600], line[160], name[40];
    FILE *fp;
    int value, i;

    pthread_once(&sOnce, load);
    pthread_mutex_lock(&sLock);
    if (id == sCoopId) { pthread_mutex_unlock(&sLock); return; }
    coop_write_locked();
    sCoopId = id;
    sCoopCount = sCoopTextCount = 0;
    sCoopDirty = 0;
    coop_path(file, sizeof(file), id);
    fp = fopen(file, "r");
    if (fp != NULL) {
        while (fgets(line, sizeof(line), fp) != NULL) {
            int start = 0;
            sscanf(line, " %39[a-z0-9_]=\"%n", name, &start);
            if (start > 0) {
                char *end = strchr(line + start, '"');
                int length = end != NULL ? (int)(end - line - start) : -1;
                if (length < 0 || length >= 64 || sCoopTextCount >= 32) continue;
                strcpy(sCoopText[sCoopTextCount].name, name);
                memcpy(sCoopText[sCoopTextCount].value, line + start, length);
                sCoopText[sCoopTextCount++].value[length] = '\0';
            } else if (sscanf(line, " %39[a-z0-9_]=%d", name, &value) == 2 && sCoopCount < MAX_COOP) {
                strcpy(sCoop[sCoopCount].name, name);
                sCoop[sCoopCount++].value = value;
            }
        }
        fclose(fp);
    } else if (id == 1) { /* the save in use when this came: the progress kept so far */
        for (i = 0; i < sCount && sCoopCount < MAX_COOP; i++)
            if (strncmp(sSet[i].name, "coop_", 5) == 0) { strcpy(sCoop[sCoopCount].name, sSet[i].name); sCoop[sCoopCount++].value = sSet[i].value; }
        for (i = 0; i < sTextCount && sCoopTextCount < 32; i++)
            if (strncmp(sText[i].name, "coop_", 5) == 0) { strcpy(sCoopText[sCoopTextCount].name, sText[i].name); strcpy(sCoopText[sCoopTextCount++].value, sText[i].value); }
        sCoopDirty = 1;
        coop_write_locked();
    }
    pthread_mutex_unlock(&sLock);
}
unsigned Port_CoopSaveId(void) { return sCoopId; }
/* A new id for a save that has none: random, never 0 or 1. */
unsigned Port_CoopNewId(void) {
    static unsigned mix = 0x9E3779B9u; /* (not the game's own generator: its sequence is part of the battles) */
    unsigned id;
    do {
        mix = (mix ^ (unsigned)time(NULL) ^ ((unsigned)clock() << 7) ^ (unsigned)(size_t)&id) * 2654435761u + 0x6D2B79F5u;
        id = mix ^ (mix >> 15);
    } while (id <= 1);
    return id;
}
static int coop_get(const char *name, int def) {
    int i;
    for (i = 0; i < sCoopCount; i++) if (strcmp(sCoop[i].name, name) == 0) return sCoop[i].value;
    return def;
}
static void coop_set(const char *name, int value) {
    int i;
    for (i = 0; i < sCoopCount && strcmp(sCoop[i].name, name) != 0; i++) {}
    if (i >= MAX_COOP) return;
    if (i == sCoopCount) { snprintf(sCoop[sCoopCount++].name, sizeof(sCoop[0].name), "%s", name); sCoopDirty = 1; }
    if (sCoop[i].value != value) { sCoop[i].value = value; sCoopDirty = 1; }
}

static void load(void) {
    char line[128], name[24];
    int value;
    FILE *fp = path() != NULL ? fopen(path(), "r") : NULL;

    while (fp != NULL && fgets(line, sizeof(line), fp) != NULL && sCount < MAX_SETTINGS) {
        int start = 0;
        sscanf(line, " %23[a-z0-9_]=\"%n", name, &start);
        if (start > 0) {
            char *end = strchr(line + start, '"');
            int i, length = end != NULL ? (int)(end - line - start) : -1;
            if (length < 0 || length >= (int)sizeof(sText[0].value)) continue;
            for (i = 0; i < sTextCount && strcmp(sText[i].name, name) != 0; ++i) {}
            if (i < MAX_TEXTS) {
                if (i == sTextCount) ++sTextCount;
                strcpy(sText[i].name, name);
                memcpy(sText[i].value, line + start, length);
                sText[i].value[length] = '\0';
            }
        } else if (sscanf(line, " %23[a-z0-9_]=%d", name, &value) == 2) {
            strcpy(sSet[sCount].name, name);
            sSet[sCount++].value = value;
        }
    }
    if (fp != NULL) {
        fclose(fp);
    }
}

/* The saved value of a setting, or `def` when the file does not have it. */
int Port_Setting(const char *name, int def) {
    int i, v = def;

    pthread_once(&sOnce, load);
    pthread_mutex_lock(&sLock);
    if (is_coop(name)) { v = coop_get(name, def); pthread_mutex_unlock(&sLock); return v; } /* (Historia cooperativa: this save's) */
    for (i = 0; i < sCount; i++) {
        if (strcmp(sSet[i].name, name) == 0) {
            v = sSet[i].value;
        }
    }
    pthread_mutex_unlock(&sLock);
    return v;
}

/* Stores a setting; Port_SettingsWrite puts the changes into the file. */
void Port_SettingSave(const char *name, int value) {
    int i;

    pthread_once(&sOnce, load);
    pthread_mutex_lock(&sLock);
    if (is_coop(name)) { coop_set(name, value); pthread_mutex_unlock(&sLock); return; }
    for (i = 0; i < sCount && strcmp(sSet[i].name, name) != 0; i++) {
    }
    if (i < MAX_SETTINGS) {
        if (i == sCount) {
            snprintf(sSet[sCount++].name, sizeof(sSet[0].name), "%s", name);
            sDirty = 1;
        }
        if (sSet[i].value != value) {
            sSet[i].value = value;
            sDirty = 1;
        }
    }
    pthread_mutex_unlock(&sLock);
}

/* A saved text setting copied into out (size bytes), or `def` when the file does not have it. */
void Port_SettingText(const char *name, const char *def, char *out, int size) {
    int i;

    pthread_once(&sOnce, load);
    pthread_mutex_lock(&sLock);
    snprintf(out, (size_t)size, "%s", def);
    if (is_coop(name)) {
        for (i = 0; i < sCoopTextCount; i++) if (strcmp(sCoopText[i].name, name) == 0) snprintf(out, (size_t)size, "%s", sCoopText[i].value);
        pthread_mutex_unlock(&sLock);
        return;
    }
    for (i = 0; i < sTextCount; i++) {
        if (strcmp(sText[i].name, name) == 0) {
            snprintf(out, (size_t)size, "%s", sText[i].value);
        }
    }
    pthread_mutex_unlock(&sLock);
}

/* Stores a text setting (a double quote or a line break in it becomes an apostrophe or a space: the file's form). */
void Port_SettingTextSave(const char *name, const char *value) {
    char clean[64];
    int i;

    snprintf(clean, sizeof(clean), "%s", value);
    for (i = 0; clean[i] != '\0'; i++) {
        clean[i] = clean[i] == '"' ? '\'' : clean[i] == '\n' || clean[i] == '\r' ? ' ' : clean[i];
    }
    pthread_once(&sOnce, load);
    pthread_mutex_lock(&sLock);
    if (is_coop(name)) {
        for (i = 0; i < sCoopTextCount && strcmp(sCoopText[i].name, name) != 0; i++) {}
        if (i < 32) {
            if (i == sCoopTextCount) { snprintf(sCoopText[i].name, sizeof(sCoopText[i].name), "%s", name); sCoopText[i].value[0] = 0; sCoopTextCount++; }
            if (strcmp(sCoopText[i].value, clean) != 0) { snprintf(sCoopText[i].value, sizeof(sCoopText[i].value), "%s", clean); sCoopDirty = 1; }
        }
        pthread_mutex_unlock(&sLock);
        return;
    }
    for (i = 0; i < sTextCount && strcmp(sText[i].name, name) != 0; i++) {
    }
    if (i < MAX_TEXTS) {
        if (i == sTextCount) {
            snprintf(sText[i].name, sizeof(sText[i].name), "%s", name);
            sText[i].value[0] = '\0';
            sTextCount++;
            sDirty = 1;
        }
        if (strcmp(sText[i].value, clean) != 0) {
            strcpy(sText[i].value, clean);
            sDirty = 1;
        }
    }
    pthread_mutex_unlock(&sLock);
}

void Port_SettingsWrite(void) {
    FILE *fp;
    int i;

    pthread_mutex_lock(&sLock);
    coop_write_locked();
    fp = sDirty && path() != NULL ? fopen(path(), "w") : NULL;
    if (fp != NULL) {
        for (i = 0; i < sCount; i++) {
            fprintf(fp, "%s=%d\n", sSet[i].name, sSet[i].value);
        }
        for (i = 0; i < sTextCount; i++) {
            fprintf(fp, "%s=\"%s\"\n", sText[i].name, sText[i].value);
        }
        fclose(fp);
    }
    sDirty = 0;
    pthread_mutex_unlock(&sLock);
}
