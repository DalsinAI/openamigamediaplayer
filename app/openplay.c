/* OpenPlay: the Open media player (DESIGN.md). Plays anything a datatype
 * opens: pictures (a slideshow), sounds and tunes, animations and video,
 * through datatypes.library, so a new datatype is a new format for free.
 * Drawn with OpenGadTools in the OpenLook theme, by the rules of Open Apps
 * Look and Feel: the buttons as icons (the default), icons and text, or
 * text; every action a menu item too; Workbench by default, its own screen
 * when asked.
 *
 *   [Open] | [Previous] [Play] [Stop] [Next] | [Repeat] [Full screen]   [Playlist] [Info] [Save CDXL]
 *   +--------------------------------------------+  Playlist   6 items
 *   |  the picture, the film or the sound          |  Boing 2026.mp4
 *   +--------------------------------------------+  Lake at dusk.heic ...
 *   1:36 ======O----------------------- 4:12        [Add...][Remove][Save...]
 *   status: what opened it and how fast
 *
 *   OpenPlay [FILES ...] [BUTTONS=ICONS|TEXT|BOTH] [OWNSCREEN]
 *
 * MIT, Copyright (c) 2026 Dalsin Limited. */
#include <exec/types.h>
#include <exec/memory.h>
#include <exec/execbase.h>
#include <dos/dos.h>
#include <dos/dostags.h>
#include <dos/var.h>
#include <intuition/intuition.h>
#include <intuition/gadgetclass.h>
#include <intuition/icclass.h>
#include <libraries/gadtools.h>
#include <libraries/asl.h>
#include <libraries/iffparse.h>
#include <graphics/text.h>
#include <datatypes/datatypes.h>
#include <datatypes/datatypesclass.h>
#include <datatypes/pictureclass.h>
#include <datatypes/soundclass.h>
#include <datatypes/animationclass.h>
#include <devices/inputevent.h>
#include <workbench/startup.h>
#include <workbench/workbench.h>
#include <workbench/icon.h>
#include <utility/tagitem.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/gadtools.h>
#include <proto/graphics.h>
#include <proto/datatypes.h>
#include <proto/asl.h>
#include <proto/iffparse.h>
#include <proto/icon.h>
#include <proto/wb.h>
#include <proto/utility.h>
#include <proto/expansion.h>

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ogt_theme.h"
#include "ogt_draw.h"
#include "ogt_font.h"
#include "ogt_icons.h"
#include "ogt_list.h"
#include "ogt_toolbar.h"

#include "op_stack.h"
#include "op_icons.h"

#define VERSION_TEXT "OpenPlay 0.1.1 (10.10.2026)"
static const char version[] __attribute__((used)) = "$VER: " VERSION_TEXT " MIT, Copyright (c) 2026 Dalsin Limited";

struct Library *DataTypesBase, *AslBase, *IFFParseBase, *IconBase, *WorkbenchBase;
extern struct WBStartup *_WBenchMsg;

#define THEME_DIR "SYS:Prefs/Presets/Themes/"
#define PREFS_DIR "OpenPlay"
#define MAX_ITEMS 512
#define PATH_LEN 256
#define SLIDE_TICKS 60                  /* six seconds a picture (IntuiTicks come ten a second) */

/* The theme when none is set or found: OpenLook's Open, light. */
static const char fallback_theme[] =
    "name Open\nversion 1\nfont \"DejaVu Sans\" 12\n[light]\n"
    "window #e8eaee\ntext #121825\nlabel #2a3342\nmuted #5d6676\n"
    "button #ffffff 0, #d5dae2 100\nbutton.text #121825\nbutton.border #8a94a6\nbutton.highlight #ffffff\n"
    "string #ffffff\nstring.shadow #8a94a6\nstring.shine #c9cfd9\n"
    "accent #365fa3\naccent.text #ffffff\nfill #a4bde6 0, #7d9fd5 100\nfill.text #121825\n"
    "selection.inactive #cfdaee\nlist #ffffff\nlist.alternate #f1f4f8\nlist.header #f7f8fa 0, #dde2e9 100\n"
    "group.line #a3abb8\ngroup.highlight #ffffff\ntab #e3e7ed 0, #cfd5de 100\ntab.text #2a3342\n"
    "track #cdd3dc\nframe.active #3d4a63\n";

/* ---- commands: toolbar buttons, menu items and keys ---- */

enum { C_OPEN = 1, C_PREV, C_PLAY, C_STOP, C_NEXT, C_REPEAT, C_FULL, C_LIST, C_INFO, C_CDXL,
       C_ADD, C_REMOVE, C_SAVELIST, C_CLEAR, C_ABOUT, C_QUIT, C_OWNSCREEN,
       C_BT_LOOK, C_BT_BOTH, C_BT_ICONS, C_BT_TEXT };

/* The toolbar draws the buttons and their names; OpenPlay draws the
 * player icons on them (op_icons) until OpenGadTools has its own. */
static ogt_tool tools[] = {
    { C_OPEN, "Open", OGT_ICON_NONE, 0, 0, 0 },
    { C_PREV, "Previous", OGT_ICON_NONE, 1, 0, 0 },
    { C_PLAY, "Play", OGT_ICON_NONE, 0, 0, 1 },
    { C_STOP, "Stop", OGT_ICON_NONE, 0, 0, 0 },
    { C_NEXT, "Next", OGT_ICON_NONE, 0, 0, 0 },
    { C_REPEAT, "Repeat", OGT_ICON_NONE, 1, 0, 0 },
    { C_FULL, "Full screen", OGT_ICON_NONE, 0, 0, 0 },
    { C_LIST, "Playlist", OGT_ICON_NONE, 1, 0, 0 },
    { C_INFO, "Info", OGT_ICON_NONE, 0, 0, 0 },
    { C_CDXL, "Save CDXL", OGT_ICON_NONE, 0, 0, 0 },
};
#define NTOOLS ((int)(sizeof tools / sizeof tools[0]))
static int tool_icon[NTOOLS] = { OP_ICON_OPEN, OP_ICON_PREV, OP_ICON_PLAY, OP_ICON_STOP, OP_ICON_NEXT, OP_ICON_REPEAT,
                                 OP_ICON_FULLSCREEN, OP_ICON_PLAYLIST, OP_ICON_INFO, OP_ICON_CONVERT };
#define TB_PADX 6           /* ogt_toolbar's own spacing, to put the icons where it would */
#define TB_PADY 3
/* The key for each, shown in the help line: Open Apps Look and Feel, Keys. */
static const char *const tool_keys[NTOOLS] = { "O", "V or Left", "P or Space", "T or Esc", "N or Right", "R", "F", "L", "I", "C" };

static struct NewMenu newmenus[] = {
    { NM_TITLE, (STRPTR)"Project", 0, 0, 0, 0 },
    { NM_ITEM, (STRPTR)"Open...", (STRPTR)"O", 0, 0, (APTR)C_OPEN },
    { NM_ITEM, (STRPTR)"Save CDXL...", 0, 0, 0, (APTR)C_CDXL },
    { NM_ITEM, NM_BARLABEL, 0, 0, 0, 0 },
    { NM_ITEM, (STRPTR)"About...", (STRPTR)"?", 0, 0, (APTR)C_ABOUT },
    { NM_ITEM, (STRPTR)"Quit", (STRPTR)"Q", 0, 0, (APTR)C_QUIT },
    { NM_TITLE, (STRPTR)"Control", 0, 0, 0, 0 },
    { NM_ITEM, (STRPTR)"Play / Pause", (STRPTR)"P", 0, 0, (APTR)C_PLAY },
    { NM_ITEM, (STRPTR)"Stop", (STRPTR)"T", 0, 0, (APTR)C_STOP },
    { NM_ITEM, (STRPTR)"Previous", (STRPTR)"V", 0, 0, (APTR)C_PREV },
    { NM_ITEM, (STRPTR)"Next", (STRPTR)"N", 0, 0, (APTR)C_NEXT },
    { NM_ITEM, (STRPTR)"Repeat", (STRPTR)"R", CHECKIT | MENUTOGGLE, 0, (APTR)C_REPEAT },
    { NM_TITLE, (STRPTR)"Playlist", 0, 0, 0, 0 },
    { NM_ITEM, (STRPTR)"Add...", (STRPTR)"A", 0, 0, (APTR)C_ADD },
    { NM_ITEM, (STRPTR)"Remove", 0, 0, 0, (APTR)C_REMOVE },
    { NM_ITEM, (STRPTR)"Save...", (STRPTR)"S", 0, 0, (APTR)C_SAVELIST },
    { NM_ITEM, (STRPTR)"Clear", 0, 0, 0, (APTR)C_CLEAR },
    { NM_TITLE, (STRPTR)"View", 0, 0, 0, 0 },
    { NM_ITEM, (STRPTR)"Playlist", (STRPTR)"L", CHECKIT | MENUTOGGLE, 0, (APTR)C_LIST },
    { NM_ITEM, (STRPTR)"Full screen", (STRPTR)"F", 0, 0, (APTR)C_FULL },
    { NM_ITEM, (STRPTR)"Own screen", 0, CHECKIT | MENUTOGGLE, 0, (APTR)C_OWNSCREEN },
    { NM_ITEM, (STRPTR)"About this file...", (STRPTR)"I", 0, 0, (APTR)C_INFO },
    { NM_ITEM, NM_BARLABEL, 0, 0, 0, 0 },
    { NM_ITEM, (STRPTR)"Buttons", 0, 0, 0, 0 },
    { NM_SUB, (STRPTR)"As in Look prefs", 0, CHECKIT, ~1 & 15, (APTR)C_BT_LOOK },
    { NM_SUB, NM_BARLABEL, 0, 0, 0, 0 },
    { NM_SUB, (STRPTR)"Icons and text", 0, CHECKIT, ~4 & 15, (APTR)C_BT_BOTH },
    { NM_SUB, (STRPTR)"Icons", 0, CHECKIT, ~8 & 15, (APTR)C_BT_ICONS },
    { NM_SUB, (STRPTR)"Text", 0, CHECKIT, ~16 & 31, (APTR)C_BT_TEXT },
    { NM_END, 0, 0, 0, 0, 0 }
};

/* ---- the playlist ---- */

enum { K_UNKNOWN = 0, K_PICTURE, K_SOUND, K_VIDEO, K_TEXT, K_DOCUMENT, K_OTHER };
static const char *const kind_word[] = { "FILE", "PICTURE", "SOUND", "VIDEO", "TEXT", "DOCUMENT", "FILE" };

typedef struct item {
    char path[PATH_LEN];
    int kind;                       /* known once it has been opened */
} item;

static item *items;
static int nitems, cur = -1;

/* ---- the window ---- */

static struct Screen *scr, *own_scr;
static APTR vi;
static struct Window *win;
static struct Menu *menus;
static struct Gadget *glist, *g_add, *g_remove, *g_save;
static struct TextFont *font;
static struct TextAttr ta;
static ogt_theme theme;
static ogt_ctx ctx;
static int ctx_ok, theme_mode;
static ogt_toolbar tb;
static ogt_list *list;
static int fh;
static struct MsgPort *appport;
static struct AppWindow *appwin;

static int bt_choice = -1;          /* -1 as in Look prefs, else OGT_TB_* */
static int bt_forced = -1;          /* a ToolType or BUTTONS= */
static int show_list = 1, repeat_on, want_own, volume = 64;
static int win_box[4];              /* the place last remembered */

static struct { int x, y, w, h; } media_box, seek_box, status_box, side_box, head_box, bar_box, vol_box;
static int tb_y;

#define GID_DT 50
#define GID_LIST 60
#define GID_ADD 70
#define GID_REMOVE 71
#define GID_SAVE 72
#define GID_TOOLS 100

/* ---- what is playing ---- */

static Object *dto;
static int kind, playing, frames, frame, fps, slide_ticks, hover = -1, pending_play, pending_ticks;
static ULONG pic_w, pic_h, snd_len, snd_rate;
static char dt_name[48], dt_base[32], status[200], opened_in[24], win_title[PATH_LEN + 16];
static struct DateStamp t_open;

static LONG pen(const char *key) { return ogt_pen(&ctx, key); }

/* OPENPLAY_DEBUG set to a file's name: a line for each step in that file. */
static int debug_on = -1;
static char debug_file[100];
static void dbg(const char *fmt, ...)
{
    char buf[200];
    va_list ap;
    BPTR f;
    if (debug_on < 0) debug_on = GetVar((CONST_STRPTR)"OPENPLAY_DEBUG", (STRPTR)debug_file, sizeof debug_file, 0) > 0;
    if (!debug_on) return;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof buf - 1, fmt, ap);
    va_end(ap);
    strcat(buf, "\n");
    if ((f = Open((CONST_STRPTR)debug_file, MODE_READWRITE))) {
        Seek(f, 0, OFFSET_END);
        Write(f, buf, (LONG)strlen(buf));
        Close(f);
    }
}

/* ---- small files in ENV: and ENVARC: ---- */

static int read_small(const char *path, char *buf, int size)
{
    BPTR f = Open((CONST_STRPTR)path, MODE_OLDFILE);
    LONG n;
    if (!f) return 0;
    n = Read(f, buf, size - 1);
    Close(f);
    buf[n > 0 ? n : 0] = 0;
    return 1;
}

static void write_small(const char *name, const char *text)
{
    static const char *const roots[2] = { "ENV:", "ENVARC:" };
    int i;
    for (i = 0; i < 2; i++) {
        char path[128], dir[64];
        BPTR f, l;
        snprintf(dir, sizeof dir, "%s%s", roots[i], PREFS_DIR);
        if ((l = Lock((CONST_STRPTR)dir, SHARED_LOCK))) UnLock(l);
        else if ((l = CreateDir((CONST_STRPTR)dir))) UnLock(l);
        snprintf(path, sizeof path, "%s%s/%s", roots[i], PREFS_DIR, name);
        if ((f = Open((CONST_STRPTR)path, MODE_NEWFILE))) {
            Write(f, (APTR)text, (LONG)strlen(text));
            Close(f);
        }
    }
}

static char *load_text(const char *path)
{
    BPTR f = Open((CONST_STRPTR)path, MODE_OLDFILE);
    struct FileInfoBlock *fib;
    char *buf = NULL;
    if (!f) return NULL;
    if ((fib = AllocDosObject(DOS_FIB, NULL))) {
        if (ExamineFH(f, fib) && fib->fib_Size > 0 && fib->fib_Size < 65536 && (buf = malloc(fib->fib_Size + 1))) {
            LONG n = Read(f, buf, fib->fib_Size);
            buf[n > 0 ? n : 0] = 0;
        }
        FreeDosObject(DOS_FIB, fib);
    }
    Close(f);
    return buf;
}

/* ---- the look: ENV:OpenGadTools/Look, as OpenLook reads it ---- */

static char *word(char **p, char *out, int size)
{
    char *s = *p;
    int n = 0;
    while (*s == ' ' || *s == '\t') s++;
    if (!*s || *s == '\n' || *s == '\r') { out[0] = 0; *p = s; return NULL; }
    if (*s == '"') {
        s++;
        while (*s && *s != '"' && *s != '\n' && n < size - 1) out[n++] = *s++;
        if (*s == '"') s++;
    } else
        while (*s && *s != ' ' && *s != '\t' && *s != '\n' && *s != '\r' && n < size - 1) out[n++] = *s++;
    out[n] = 0;
    *p = s;
    return out;
}

static int on_amigachrome(void)
{
    struct Library *ExpansionBase = OpenLibrary((CONST_STRPTR)"expansion.library", 37);
    int ac = 0;
    if (ExpansionBase) {
        if (FindConfigDev(NULL, 0xDA15, -1) || FindConfigDev(NULL, 2011, -1)) ac = 1;
        CloseLibrary(ExpansionBase);
    }
    return ac;
}

/* Lite by itself: a real 68040 or 68060 with no OpenGPU (OpenLook's rule). */
static int lite_by_itself(void)
{
    struct Library *gpu;
    if (!(SysBase->AttnFlags & (AFF_68040 | AFF_68060))) return 0;
    if ((gpu = OpenLibrary((CONST_STRPTR)"opengpu.library", 0))) { CloseLibrary(gpu); return 0; }
    return !on_amigachrome();
}

static int hour_now(void)
{
    struct DateStamp ds;
    DateStamp(&ds);
    return (int)(ds.ds_Minute / 60);
}

static int load_theme_file(const char *name)
{
    char path[200], err[96], *text;
    if (strchr(name, ':')) snprintf(path, sizeof path, "%s", name);
    else snprintf(path, sizeof path, "%s%s%s", THEME_DIR, name, strstr(name, ".theme") ? "" : ".theme");
    if (!(text = load_text(path))) return 0;
    {
        int ok = ogt_theme_parse(&theme, text, err, sizeof err);
        free(text);
        return ok;
    }
}

/* The theme, its mode, the accent and Lite, from OpenPrefs Look; else
 * ENV:OpenGadTools/Theme (OpenGadTools 0.1); else Open, light. */
static void load_look(void)
{
    char *text = load_text("ENV:OpenGadTools/Look"), *line, *p, w[112], v[112], name[160] = "";   /* name: a whole w[] or Theme buf[] */
    int mode = OGT_LIGHT, from = 19, to = 7, lite_mode = 2, has_accent = 0;
    unsigned lite = OGT_LITE_SHADOWS | OGT_LITE_ROUNDING | OGT_LITE_GRADIENTS;
    ogt_rgb accent = {0, 0, 0};

    if (text) {
        for (line = text; line && *line; ) {
            char *next = strchr(line, '\n');
            p = line;
            if (line == text) {
                if (word(&p, w, sizeof w) && w[0] != ';') snprintf(name, sizeof name, "%s", w);
                if (word(&p, w, sizeof w)) mode = !strcmp(w, "dark") ? OGT_DARK : !strcmp(w, "auto") ? 2 : OGT_LIGHT;
            } else if (word(&p, w, sizeof w) && w[0] != ';') {
                if (!strcmp(w, "mode") && word(&p, v, sizeof v)) {
                    if (!strcmp(v, "auto")) mode = 2;
                    if (word(&p, v, sizeof v)) from = atoi(v) % 24;
                    if (word(&p, v, sizeof v)) to = atoi(v) % 24;
                } else if (!strcmp(w, "accent") && word(&p, v, sizeof v)) {
                    has_accent = ogt_parse_colour(v, &accent);
                } else if (!strcmp(w, "lite") && word(&p, v, sizeof v)) {
                    lite_mode = !strcmp(v, "on") ? 1 : !strcmp(v, "off") ? 0 : 2;
                } else if (!strncmp(w, "lite.", 5) && word(&p, v, sizeof v)) {
                    unsigned bit = !strcmp(w + 5, "shadows") ? OGT_LITE_SHADOWS : !strcmp(w + 5, "rounding") ? OGT_LITE_ROUNDING :
                                   !strcmp(w + 5, "gradients") ? OGT_LITE_GRADIENTS : 0;
                    if (!strcmp(v, "on")) lite |= bit; else lite &= ~bit;
                }
            }
            line = next ? next + 1 : NULL;
        }
        free(text);
    } else {
        char buf[160], *nl;
        if (read_small("ENV:OpenGadTools/Theme", buf, sizeof buf)) {
            if ((nl = strchr(buf, '\n'))) { *nl = 0; if (!strncmp(nl + 1, "dark", 4)) mode = OGT_DARK; }
            snprintf(name, sizeof name, "%s", buf);
        }
    }
    if (mode == 2) {
        int h = hour_now();
        mode = from > to ? (h >= from || h < to) : (h >= from && h < to);
        mode = mode ? OGT_DARK : OGT_LIGHT;
    }
    if (!name[0] || !load_theme_file(name)) {
        char err[96];
        ogt_theme_parse(&theme, fallback_theme, err, sizeof err);
        mode = OGT_LIGHT;
    }
    if (mode == OGT_DARK && !theme.has_dark) mode = OGT_LIGHT;
    if (lite_mode == 1 || (lite_mode == 2 && lite_by_itself()))
        ogt_theme_set_lite(&theme, lite_mode == 1 ? lite : OGT_LITE_SHADOWS | OGT_LITE_ROUNDING | OGT_LITE_GRADIENTS);
    if (has_accent) ogt_theme_set_accent(&theme, accent);
    theme_mode = mode;
}

/* "icons+text" (or BOTH), "icons" or "text" as OGT_TB_*, else -1. */
static int parse_buttons(const char *s)
{
    char w[16];
    int n = 0;
    while (s && (*s == ' ' || *s == '\t' || *s == '=')) s++;
    while (s && *s && *s != ' ' && *s != '\t' && *s != '\n' && *s != '\r' && n < (int)sizeof w - 1) {
        char ch = *s++;
        w[n++] = (char)(ch >= 'A' && ch <= 'Z' ? ch - 'A' + 'a' : ch);
    }
    w[n] = 0;
    if (!strcmp(w, "icons+text") || !strcmp(w, "both") || !strcmp(w, "icons-text")) return OGT_TB_ICONS_TEXT;
    if (!strcmp(w, "icons")) return OGT_TB_ICONS;
    if (!strcmp(w, "text")) return OGT_TB_TEXT;
    return -1;
}

/* Look prefs' proposed "buttons" line (Open Apps Look and Feel), read here
 * until OpenGadTools reads it for every app; icons only when it says nothing
 * (every Open app starts with icons, the Team's rule of 10 October 2026). */
static int look_buttons(void)
{
    char *text = load_text("ENV:OpenGadTools/Look"), *l;
    int style = OGT_TB_ICONS;
    for (l = text; l && *l; l = strchr(l, '\n') ? strchr(l, '\n') + 1 : NULL)
        if (!strncmp(l, "buttons", 7) && (l[7] == ' ' || l[7] == '\t')) {
            int b = parse_buttons(l + 7);
            if (b >= 0) style = b;
        }
    free(text);
    return style;
}

/* The Buttons style: a ToolType or BUTTONS= wins, then View > Buttons, then Look prefs. */
static int buttons_style(void)
{
    if (bt_forced >= 0) return bt_forced;
    if (bt_choice >= 0) return bt_choice;
    return look_buttons();
}

static void load_prefs(void)
{
    char buf[256], *p;
    if (read_small("ENV:" PREFS_DIR "/Buttons", buf, sizeof buf)) bt_choice = parse_buttons(buf);
    if (read_small("ENV:" PREFS_DIR "/Playlist", buf, sizeof buf)) show_list = buf[0] != '0';
    if (read_small("ENV:" PREFS_DIR "/Repeat", buf, sizeof buf)) repeat_on = buf[0] == '1';
    if (read_small("ENV:" PREFS_DIR "/OwnScreen", buf, sizeof buf)) want_own = buf[0] == '1';
    if (read_small("ENV:" PREFS_DIR "/Volume", buf, sizeof buf)) { volume = atoi(buf); if (volume < 0 || volume > 64) volume = 64; }
    if (read_small("ENV:" PREFS_DIR "/Window", buf, sizeof buf)) {
        int i;
        p = buf;
        for (i = 0; i < 4; i++) win_box[i] = (int)strtol(p, &p, 10);
    }
}

static void save_prefs(void)
{
    char buf[64];
    write_small("Buttons", bt_choice == OGT_TB_ICONS ? "icons\n" : bt_choice == OGT_TB_TEXT ? "text\n" :
                           bt_choice == OGT_TB_ICONS_TEXT ? "icons+text\n" : "look\n");
    write_small("Playlist", show_list ? "1\n" : "0\n");
    write_small("Repeat", repeat_on ? "1\n" : "0\n");
    write_small("OwnScreen", want_own ? "1\n" : "0\n");
    snprintf(buf, sizeof buf, "%d\n", volume);
    write_small("Volume", buf);
    if (win && !own_scr) {
        snprintf(buf, sizeof buf, "%d %d %d %d\n", win->LeftEdge, win->TopEdge, win->Width, win->Height);
        write_small("Window", buf);
    }
}

/* ---- the playlist ---- */

static const char *base_name(const char *path)
{
    const char *s = (const char *)FilePart((CONST_STRPTR)path);
    return s && *s ? s : path;
}

static int ends_with(const char *s, const char *tail)
{
    size_t a = strlen(s), b = strlen(tail);
    return a >= b && !strcasecmp(s + a - b, tail);
}

static void add_path(const char *path);

/* A playlist file (.m3u or OpenPlay's own .opl): one path a line. */
static int add_playlist_file(const char *path)
{
    char *text, *line;
    if (!(ends_with(path, ".m3u") || ends_with(path, ".opl"))) return 0;
    if (!(text = load_text(path))) return 1;
    for (line = text; line && *line; ) {
        char *next = strchr(line, '\n');
        if (next) *next = 0;
        if (line[0] && line[0] != '#') {
            char *cr = strchr(line, '\r');
            if (cr) *cr = 0;
            add_path(line);
        }
        line = next ? next + 1 : NULL;
    }
    free(text);
    return 1;
}

static void add_one(const char *path)
{
    item *grown;
    if (ends_with(path, ".info")) return;
    if (add_playlist_file(path)) return;
    if (nitems >= MAX_ITEMS) return;
    if (!items || !(nitems % 32)) {
        if (!(grown = realloc(items, (nitems + 32) * sizeof *items))) return;
        items = grown;
    }
    snprintf(items[nitems].path, PATH_LEN, "%s", path);
    items[nitems].kind = K_UNKNOWN;
    nitems++;
}

/* A file, or a drawer's files (not its drawers: Open Apps Look and Feel,
 * "a drawer adds what it holds"). */
static void add_path(const char *path)
{
    BPTR l = Lock((CONST_STRPTR)path, SHARED_LOCK);
    struct FileInfoBlock *fib;
    if (!l) return;
    if ((fib = AllocDosObject(DOS_FIB, NULL))) {
        if (Examine(l, fib)) {
            if (fib->fib_DirEntryType > 0) {
                while (ExNext(l, fib))
                    if (fib->fib_DirEntryType < 0) {
                        char full[PATH_LEN];
                        snprintf(full, sizeof full, "%s", path);
                        AddPart((STRPTR)full, (CONST_STRPTR)fib->fib_FileName, sizeof full);
                        add_one(full);
                    }
            } else {
                add_one(path);
            }
        }
        FreeDosObject(DOS_FIB, fib);
    }
    UnLock(l);
}

static void add_lock(BPTR dir, const char *name)
{
    char path[PATH_LEN];
    if (!NameFromLock(dir, (STRPTR)path, sizeof path)) return;
    if (name && *name) AddPart((STRPTR)path, (CONST_STRPTR)name, sizeof path);
    add_path(path);
}

/* ---- drawing ---- */

static void fmt_time(char *out, int size, ULONG secs)
{
    snprintf(out, size, "%lu:%02lu", (unsigned long)(secs / 60), (unsigned long)(secs % 60));
}

static void draw_status(void)
{
    struct RastPort *rp;
    char line[240];
    if (!win) return;
    rp = win->RPort;
    ogt_fill(&ctx, rp, "window", status_box.x, status_box.y, status_box.w, status_box.h);
    if (hover >= 0 && hover < NTOOLS)
        snprintf(line, sizeof line, "%s  (%s)", tools[hover].label, tool_keys[hover]);
    else
        snprintf(line, sizeof line, "%s", status);
    if (kind != K_UNKNOWN && hover < 0) {
        int cw = ogt_text_width(rp, kind_word[kind]) + 12;
        ogt_box(rp, pen("accent"), status_box.x, status_box.y + 1, cw, fh + 2);
        ogt_text(rp, pen("accent.text"), status_box.x + 6, status_box.y + 2, kind_word[kind], 0);
        ogt_text(rp, pen("muted"), status_box.x + cw + 8, status_box.y + 2, line, status_box.w - cw - 8);
    } else {
        ogt_text(rp, pen(hover >= 0 ? "label" : "muted"), status_box.x, status_box.y + 2, line, status_box.w);
    }
}

static void set_status(const char *s)
{
    strlcpy(status, s, sizeof status);         /* one line: a longer message is cut to it */
    draw_status();
}

/* Position: a bar and the times for sound and video; for pictures, where
 * the slideshow is. */
static void draw_seek(void)
{
    struct RastPort *rp;
    char a[32] = "", b[32] = "";                /* "%d of %d" with any two ints */
    int x, w, bx, bw, filled = 0;
    if (!win) return;
    rp = win->RPort;
    x = seek_box.x; w = seek_box.w;
    ogt_fill(&ctx, rp, "window", x, seek_box.y, w, seek_box.h);
    if (kind == K_VIDEO && frames > 0) {
        int f = fps > 0 ? fps : 25;
        fmt_time(a, sizeof a, (ULONG)(frame / f));
        fmt_time(b, sizeof b, (ULONG)(frames / f));
        filled = frames > 1 ? frame * 1000 / (frames - 1) : 0;
    } else if (kind == K_SOUND && snd_rate) {
        fmt_time(a, sizeof a, 0);
        fmt_time(b, sizeof b, snd_len / snd_rate);
    } else if (kind == K_PICTURE && nitems) {
        snprintf(a, sizeof a, "%d of %d", cur + 1, nitems);
        filled = nitems > 1 ? cur * 1000 / (nitems - 1) : 1000;
    }
    bar_box.w = 0;
    /* the volume, at the right, for sound and films: SDTA_Volume, 0 to 64 */
    vol_box.w = 0;
    if (kind == K_SOUND || kind == K_VIDEO) {
        int vw = 64, vx = x + w - vw, vy = seek_box.y + seek_box.h / 2 - 4;
        ogt_text(rp, pen("muted"), vx - ogt_text_width(rp, "Volume") - 6, seek_box.y + (seek_box.h - fh) / 2, "Volume", 0);
        ogt_box(rp, pen("string"), vx, vy, vw, 9);
        ogt_frame(rp, pen("string.shadow"), vx, vy, vw, 9);
        if (volume > 0) ogt_fill(&ctx, rp, "fill", vx + 1, vy + 1, (vw - 2) * volume / 64, 7);
        vol_box.x = vx; vol_box.y = seek_box.y; vol_box.w = vw; vol_box.h = seek_box.h;
        w -= vw + ogt_text_width(rp, "Volume") + 18;
    }
    if (!a[0]) return;
    ogt_text(rp, pen("label"), x, seek_box.y + (seek_box.h - fh) / 2, a, 0);
    bx = x + ogt_text_width(rp, "00:00 of 000") + 8;
    bw = w - (bx - x) - ogt_text_width(rp, "00:00") - 12;
    if (bw < 20) return;
    bar_box.x = bx; bar_box.y = seek_box.y; bar_box.w = bw; bar_box.h = seek_box.h;
    {
        int by = seek_box.y + seek_box.h / 2 - 4, kx;
        ogt_box(rp, pen("string"), bx, by, bw, 9);
        ogt_frame(rp, pen("string.shadow"), bx, by, bw, 9);
        if (filled > 0) ogt_fill(&ctx, rp, "fill", bx + 1, by + 1, (bw - 2) * filled / 1000, 7);
        kx = bx + (bw - 12) * filled / 1000;
        ogt_fill(&ctx, rp, "button", kx, by - 3, 12, 15);
        ogt_frame(rp, pen("button.border"), kx, by - 3, 12, 15);
    }
    if (b[0]) ogt_text(rp, pen("label"), bx + bw + 8, seek_box.y + (seek_box.h - fh) / 2, b, 0);
}

static int row_h(void *user, int i) { (void)user; (void)i; return fh + 6; }

static void draw_row(void *user, int i, struct RastPort *rp, int x, int y, int w, int h, int sel)
{
    static const int kicon[] = { OP_ICON_FILE, OP_ICON_SLIDESHOW, OP_ICON_SOUND, OP_ICON_FILM, OP_ICON_FILE, OP_ICON_FILE, OP_ICON_FILE };
    int s = fh, now = i == cur;
    (void)user;
    if (sel) ogt_box(rp, pen("accent"), x, y, w, h);
    else ogt_fill(&ctx, rp, (i & 1) ? "list.alternate" : "list", x, y, w, h);
    op_icon_draw(&ctx, rp, kicon[items[i].kind], x + 3, y + (h - s) / 2, s, 0);
    ogt_bold(rp, now);
    ogt_text(rp, pen(sel ? "accent.text" : "text"), x + s + 8, y + (h - fh) / 2, base_name(items[i].path), w - s - 12);
    ogt_bold(rp, 0);
}

static void draw_side(void)
{
    struct RastPort *rp;
    char n[48];
    if (!win || !show_list) return;
    rp = win->RPort;
    ogt_fill(&ctx, rp, "window", head_box.x, head_box.y, head_box.w, head_box.h);
    ogt_bold(rp, 1);
    ogt_text(rp, pen("label"), head_box.x, head_box.y + 2, "Playlist", 0);
    ogt_bold(rp, 0);
    snprintf(n, sizeof n, "%d item%s", nitems, nitems == 1 ? "" : "s");
    ogt_text(rp, pen("muted"), head_box.x + head_box.w - ogt_text_width(rp, n), head_box.y + 2, n, 0);
    if (list) ogt_list_draw(list);
}

static void draw_media_back(void)
{
    struct RastPort *rp = win->RPort;
    ogt_rgb black = {0, 0, 0};
    ogt_frame(rp, pen("string.shadow"), media_box.x - 1, media_box.y - 1, media_box.w + 2, media_box.h + 2);
    if (dto) return;
    ogt_box(rp, kind == K_SOUND ? pen("list") : ogt_pen_rgb(&ctx, black), media_box.x, media_box.y, media_box.w, media_box.h);
    {
        const char *hint = nitems ? "Choose Play, or double-click an item in the playlist." :
                                    "Drop pictures, sounds, films or drawers here, or choose Open.";
        int tw = ogt_text_width(rp, hint);
        ogt_rgb grey = {0xb0, 0xb6, 0xc0};
        ogt_text(rp, ogt_pen_rgb(&ctx, grey), media_box.x + (media_box.w - tw) / 2, media_box.y + media_box.h / 2 - fh / 2, hint, media_box.w - 8);
    }
}

/* A button, then its icon where ogt_toolbar would put one. */
static void draw_one(int i, int down)
{
    int x = tb.box[i].x, y = tb.box[i].y, w = tb.box[i].w, h = tb.box[i].h, s = tb.icon_size;
    ogt_toolbar_draw_one(&tb, &ctx, win->RPort, i, down, "window");
    if (!w) return;
    switch (tb.style) {
    case OGT_TB_TEXT: break;
    case OGT_TB_INLINE: op_icon_draw(&ctx, win->RPort, tool_icon[i], x + TB_PADX, y + (h - s) / 2, s, tb.tool[i].disabled); break;
    default: op_icon_draw(&ctx, win->RPort, tool_icon[i], x + (w - s) / 2, y + TB_PADY + 1, s, tb.tool[i].disabled); break;
    }
}

static void draw_tools(void)
{
    int i;
    for (i = 0; i < tb.n; i++)
        if (tb.tool[i].id == C_PLAY) {
            tb.tool[i].label = playing ? "Pause" : (kind == K_PICTURE ? "Slideshow" : "Play");
            tool_icon[i] = playing ? OP_ICON_PAUSE : (kind == K_PICTURE ? OP_ICON_SLIDESHOW : OP_ICON_PLAY);
        }
    ogt_toolbar_enable(&tb, C_PREV, nitems > 1);
    ogt_toolbar_enable(&tb, C_NEXT, nitems > 1);
    ogt_toolbar_enable(&tb, C_PLAY, nitems > 0);
    ogt_toolbar_enable(&tb, C_STOP, dto != NULL);
    ogt_toolbar_enable(&tb, C_FULL, dto != NULL && kind != K_SOUND);
    ogt_toolbar_enable(&tb, C_INFO, dto != NULL);
    ogt_toolbar_enable(&tb, C_CDXL, dto != NULL && kind == K_VIDEO);
    ogt_fill(&ctx, win->RPort, "window", win->BorderLeft, tb_y, win->Width - win->BorderLeft - win->BorderRight, tb.h);
    for (i = 0; i < tb.n; i++) {
        if (tb.tool[i].sep_before && i && tb.box[i].w)
            ogt_vline(win->RPort, pen("group.line"), tb.box[i].x - 9 / 2 - 1, tb.box[i].y + 4, tb.box[i].h - 8);
        /* toggles drawn pressed while on */
        draw_one(i, i == tb.pressed || (tb.tool[i].id == C_REPEAT && repeat_on) || (tb.tool[i].id == C_LIST && show_list));
    }
    ogt_hline(win->RPort, pen("group.line"), win->BorderLeft, tb_y + tb.h + 2, win->Width - win->BorderLeft - win->BorderRight);
}

static void draw_all(void)
{
    ogt_fill(&ctx, win->RPort, "window", win->BorderLeft, win->BorderTop,
             win->Width - win->BorderLeft - win->BorderRight, win->Height - win->BorderTop - win->BorderBottom);
    draw_tools();
    draw_media_back();
    draw_seek();
    draw_side();
    draw_status();
    if (glist) RefreshGList(glist, win, NULL, -1);
    GT_RefreshWindow(win, NULL);
    if (dto) RefreshDTObjects(dto, win, NULL, TAG_DONE);
}

/* ---- layout ---- */

static void remove_gadgets(void)
{
    if (dto) RemoveDTObject(win, dto);
    if (glist) {
        RemoveGList(win, glist, -1);
        FreeGadgets(glist);
        glist = NULL;
    }
    g_add = g_remove = g_save = NULL;
}

static void place_dto(void)
{
    if (!dto) return;
    SetAttrs(dto, GA_Left, media_box.x, GA_Top, media_box.y, GA_Width, media_box.w, GA_Height, media_box.h, TAG_DONE);
    AddDTObject(win, NULL, dto, -1);
}

static void layout(void)
{
    int x0 = win->BorderLeft, y0 = win->BorderTop;
    int w = win->Width - win->BorderLeft - win->BorderRight, h = win->Height - win->BorderTop - win->BorderBottom;
    int m = 8, sw = show_list ? (w / 4 < 170 ? 170 : w / 4) : 0, bh = fh + 8, th;
    struct NewGadget ng;
    struct Gadget *g, *tbg;
    int i, count;

    remove_gadgets();
    ogt_toolbar_set(&tb, tools, NTOOLS, buttons_style());
    th = ogt_toolbar_layout(&tb, win->RPort, x0 + m, y0 + 4, w - 2 * m);
    tb_y = y0 + 4;

    status_box.x = x0 + m; status_box.w = w - 2 * m; status_box.h = fh + 4;
    status_box.y = y0 + h - status_box.h - 4;
    seek_box.x = x0 + m; seek_box.h = fh + 10;
    seek_box.y = status_box.y - seek_box.h - 4;
    seek_box.w = w - 2 * m - (sw ? sw + m : 0);
    media_box.x = x0 + m + 1; media_box.y = tb_y + th + 2 + m + 1;
    media_box.w = seek_box.w - 2; media_box.h = seek_box.y - 4 - media_box.y - 1;
    if (media_box.h < 40) media_box.h = 40;
    side_box.x = x0 + w - m - sw; side_box.y = tb_y + th + 2 + m; side_box.w = sw;
    side_box.h = status_box.y - 4 - side_box.y;
    head_box.x = side_box.x; head_box.y = side_box.y; head_box.w = sw; head_box.h = fh + 6;

    memset(&ng, 0, sizeof ng);
    ng.ng_TextAttr = &ta;
    ng.ng_VisualInfo = vi;
    g = CreateContext(&glist);
    if (show_list) {
        int bw = (sw - 8) / 3;
        ng.ng_TopEdge = side_box.y + side_box.h - bh;
        ng.ng_Height = bh;
        ng.ng_Width = bw;
        ng.ng_LeftEdge = side_box.x;
        ng.ng_GadgetText = (UBYTE *)"_Add...";
        ng.ng_GadgetID = GID_ADD;
        g = g_add = CreateGadget(BUTTON_KIND, g, &ng, GT_Underscore, '_', TAG_DONE);
        ng.ng_LeftEdge += bw + 4;
        ng.ng_GadgetText = (UBYTE *)"Re_move";
        ng.ng_GadgetID = GID_REMOVE;
        g = g_remove = CreateGadget(BUTTON_KIND, g, &ng, GT_Underscore, '_', TAG_DONE);
        ng.ng_LeftEdge += bw + 4;
        ng.ng_Width = sw - 2 * (bw + 4);
        ng.ng_GadgetText = (UBYTE *)"Sa_ve...";
        ng.ng_GadgetID = GID_SAVE;
        g = g_save = CreateGadget(BUTTON_KIND, g, &ng, GT_Underscore, '_', TAG_DONE);
    }
    if (g) AddGList(win, glist, (UWORD)~0, -1, NULL);
    tbg = ogt_toolbar_gadgets(&tb, GID_TOOLS);
    for (i = count = 0; i < tb.n; i++) count += tb.box[i].w != 0;
    if (tbg && count) AddGList(win, tbg, (UWORD)~0, count, NULL);
    if (show_list && list) {
        ogt_list_layout(list, side_box.x, head_box.y + head_box.h, sw, side_box.h - head_box.h - bh - 6);
        ogt_list_set_count(list, nitems);
    } else if (list) {
        ogt_list_layout(list, 0, 0, 0, 0);
    }
    place_dto();
}

static void relayout_and_draw(void)
{
    layout();
    draw_all();
}

/* ---- opening and playing ---- */

static void close_media(void)
{
    if (dto) {
        DoDTMethod(dto, win, NULL, DTM_TRIGGER, NULL, STM_STOP, NULL);
        RemoveDTObject(win, dto);
        DisposeDTObject(dto);
        dto = NULL;
    }
    kind = K_UNKNOWN;
    playing = pending_play = 0;
    frames = frame = fps = 0;
    pic_w = pic_h = snd_len = snd_rate = 0;
    dt_name[0] = dt_base[0] = 0;
}

static int ticks_since(const struct DateStamp *a)
{
    struct DateStamp b;
    DateStamp(&b);
    return (int)((b.ds_Days - a->ds_Days) * 24 * 60 * 60 * 50 + (b.ds_Minute - a->ds_Minute) * 60 * 50 + (b.ds_Tick - a->ds_Tick));
}

/* openamigaimage's own attributes (its Datatypes/include/datatypes/openimage.h):
 * what decodes or plays an object, and how that has gone, in words. A
 * datatype that doesn't know them answers OM_GET with FALSE. */
#define OIA_DecodedBy (TAG_USER + 0x0DA15000 + 1)
#define OIA_Stats     (TAG_USER + 0x0DA15000 + 2)

/* openamigaimage's datatypes that send the file to the Nursery
 * (openservice.device: the services card or a paired Cradle). */
static const char *const nursery_datatypes[] = { "openpicture", "opensound", "opendoc", "openvideo", NULL };

/* What did the work: what the datatype says (openmodule.datatype names the
 * way it plays: this CPU, a cores board core or media.decode/1); else the
 * Nursery for the datatypes above; else this Amiga (webp, webm, the
 * system's own). */
static const char *decoded_by(void)
{
    static char said[64];
    STRPTR by = NULL;
    int i;
    if (dto && GetDTAttrs(dto, OIA_DecodedBy, (ULONG)&by, TAG_DONE) && by && *by) {
        snprintf(said, sizeof said, "%s", (const char *)by);
        return said;
    }
    for (i = 0; nursery_datatypes[i]; i++)
        if (!strcmp(dt_base, nursery_datatypes[i])) {
            int found;
            Forbid();
            found = FindName(&SysBase->DeviceList, (CONST_STRPTR)"openservice.device") != NULL;
            Permit();
            return found ? "media.decode/1 through the Nursery" : "the Nursery (openservice.device)";
        }
    return "this Amiga";
}

static void describe(void)
{
    char what[80] = "";
    if (kind == K_PICTURE && pic_w) snprintf(what, sizeof what, "%lu x %lu", (unsigned long)pic_w, (unsigned long)pic_h);
    else if (kind == K_VIDEO && frames) snprintf(what, sizeof what, "%d frames, %d a second", frames, fps);
    else if (kind == K_SOUND && snd_rate) snprintf(what, sizeof what, "%lu Hz", (unsigned long)snd_rate);
    snprintf(status, sizeof status, "%s.datatype%s%s  -  %s  -  decoded by %s  -  opened in %s",
             dt_base[0] ? dt_base : "?", what[0] ? "  -  " : "", what, dt_name, decoded_by(), opened_in);
}

static void show_item(int i, int play)
{
    struct DataType *dtn = NULL;
    int t;
    char msg[PATH_LEN + 176];                   /* "Couldn't open ", a name, why[80] and the hint */
    if (i < 0 || i >= nitems) return;
    close_media();
    cur = i;
    if (list) { ogt_list_select(list, i); ogt_list_show(list, i); }
    snprintf(win_title, sizeof win_title, "OpenPlay \xb7 %s", base_name(items[i].path));
    SetWindowTitles(win, (CONST_STRPTR)win_title, (CONST_STRPTR)~0);
    snprintf(msg, sizeof msg, "Opening %s...", base_name(items[i].path));
    set_status(msg);
    SetWindowPointer(win, WA_BusyPointer, TRUE, TAG_DONE);
    DateStamp(&t_open);
    dbg("open %s", items[i].path);
    dto = NewDTObject((APTR)items[i].path,
                      GA_Left, media_box.x, GA_Top, media_box.y, GA_Width, media_box.w, GA_Height, media_box.h,
                      GA_ID, GID_DT, ICA_TARGET, ICTARGET_IDCMP,
                      PDTA_Remap, TRUE, PDTA_DestMode, PMODE_V43, DTA_ControlPanel, FALSE,
                      TAG_DONE);
    t = ticks_since(&t_open);
    snprintf(opened_in, sizeof opened_in, "%d.%02d s", t / 50, (t % 50) * 2);
    SetWindowPointer(win, TAG_DONE);
    if (!dto) {
        LONG err = IoErr();
        char why[80];
        if (err == DTERROR_UNKNOWN_DATATYPE) snprintf(why, sizeof why, "no datatype knows this kind of file");
        else if (err == ERROR_OBJECT_NOT_FOUND) snprintf(why, sizeof why, "the file isn't there any more");
        else Fault(err, (STRPTR)"", (STRPTR)why, sizeof why);
        snprintf(msg, sizeof msg, "Couldn't open %s: %s. A missing format may need a services card or a paired Cradle.",
                 base_name(items[i].path), why[0] == ':' ? why + 2 : why);
        items[i].kind = K_OTHER;
        set_status(msg);
        draw_all();
        return;
    }
    if (GetDTAttrs(dto, DTA_DataType, (ULONG)&dtn, TAG_DONE) && dtn) {
        snprintf(dt_name, sizeof dt_name, "%s", dtn->dtn_Header->dth_Name ? (char *)dtn->dtn_Header->dth_Name : "");
        snprintf(dt_base, sizeof dt_base, "%s", dtn->dtn_Header->dth_BaseName ? (char *)dtn->dtn_Header->dth_BaseName : "");
        switch (dtn->dtn_Header->dth_GroupID) {
        case GID_PICTURE: kind = K_PICTURE; break;
        case GID_SOUND: case GID_MUSIC: case GID_INSTRUMENT: kind = K_SOUND; break;
        case GID_ANIMATION: case GID_MOVIE: kind = K_VIDEO; break;
        case GID_TEXT: kind = K_TEXT; break;
        case GID_DOCUMENT: kind = K_DOCUMENT; break;
        default: kind = K_OTHER; break;
        }
    } else {
        kind = K_OTHER;
    }
    items[i].kind = kind;
    if (kind == K_PICTURE) {
        struct BitMapHeader *bmh = NULL;
        if (GetDTAttrs(dto, PDTA_BitMapHeader, (ULONG)&bmh, TAG_DONE) && bmh) { pic_w = bmh->bmh_Width; pic_h = bmh->bmh_Height; }
    } else if (kind == K_VIDEO) {
        ULONG n = 0, f = 0;
        GetDTAttrs(dto, ADTA_Frames, (ULONG)&n, ADTA_FramesPerSecond, (ULONG)&f, TAG_DONE);
        frames = (int)n; fps = (int)f;
    } else if (kind == K_SOUND) {
        ULONG len = 0, rate = 0, period = 0;
        GetDTAttrs(dto, SDTA_SampleLength, (ULONG)&len, SDTA_Period, (ULONG)&period, TAG_DONE);
        if (!GetDTAttrs(dto, SDTA_SamplesPerSec, (ULONG)&rate, TAG_DONE) || !rate)
            rate = period ? 3546895UL / period : 0;
        snd_len = len; snd_rate = rate;
    }
    describe();
    dbg("opened kind %d base %s", kind, dt_base);
    if (kind == K_SOUND || kind == K_VIDEO)
        SetAttrs(dto, SDTA_Volume, (ULONG)volume, DTA_Repeat, (ULONG)repeat_on, TAG_DONE);
    AddDTObject(win, NULL, dto, -1);
    dbg("added");
    draw_all();
    dbg("drawn");
    if (play && (kind == K_SOUND || kind == K_VIDEO)) {
        /* Play once the object has laid itself out (its first DTA_Sync), or
         * after a second: a trigger before that is lost. */
        pending_play = 1;
        pending_ticks = 0;
        playing = 1;
    } else if (play && kind == K_PICTURE && nitems > 1) {
        playing = 1;
        slide_ticks = 0;
    }
    draw_tools();
    draw_seek();
    if (list) ogt_list_draw(list);
}

static void step(int d, int play)
{
    int n;
    if (!nitems) return;
    n = cur + d;
    if (n < 0) n = repeat_on ? nitems - 1 : 0;
    if (n >= nitems) {
        if (!repeat_on) { playing = 0; draw_tools(); return; }
        n = 0;
    }
    show_item(n, play);
}

static void play_pause(void)
{
    if (!dto) {
        show_item(cur >= 0 ? cur : 0, 1);
        return;
    }
    if (kind == K_SOUND || kind == K_VIDEO) {
        if (pending_play) pending_play = 0;
        else DoDTMethod(dto, win, NULL, DTM_TRIGGER, NULL, playing ? STM_PAUSE : STM_PLAY, NULL);
        playing = !playing;
    } else if (kind == K_PICTURE) {
        playing = !playing;
        slide_ticks = 0;
    }
    draw_tools();
}

static void stop(void)
{
    if (dto && (kind == K_SOUND || kind == K_VIDEO)) DoDTMethod(dto, win, NULL, DTM_TRIGGER, NULL, STM_STOP, NULL);
    playing = 0;
    frame = 0;
    draw_tools();
    draw_seek();
}

/* ---- requesters ---- */

static int ask_file(const char *title, int save, int multi, char *out, int size, struct FileRequester **keep)
{
    struct FileRequester *fr;
    char def[108] = "";                         /* a whole file name, as the requester's default */
    if (!AslBase) return 0;
    if (save && cur >= 0) {
        strlcpy(def, base_name(items[cur].path), sizeof def);
        if (strrchr(def, '.')) *strrchr(def, '.') = 0;
        strncat(def, out[0] ? out : "", sizeof def - strlen(def) - 1);
    }
    if (!(fr = AllocAslRequestTags(ASL_FileRequest, ASLFR_Window, (ULONG)win, ASLFR_TitleText, (ULONG)title,
                                   ASLFR_DoSaveMode, save, ASLFR_DoMultiSelect, multi, ASLFR_InitialFile, (ULONG)def,
                                   ASLFR_RejectIcons, TRUE, TAG_DONE)))
        return 0;
    if (!AslRequest(fr, NULL)) { FreeAslRequest(fr); return 0; }
    snprintf(out, size, "%s", fr->fr_Drawer);
    AddPart((STRPTR)out, fr->fr_File, size);
    if (keep) *keep = fr; else FreeAslRequest(fr);
    return 1;
}

static void open_or_add(int replace)
{
    struct FileRequester *fr = NULL;
    char path[PATH_LEN] = "";
    int before = nitems, i;
    if (!ask_file(replace ? "Open" : "Add to the playlist", 0, 1, path, sizeof path, &fr)) return;
    if (replace) { close_media(); nitems = 0; cur = -1; before = 0; }
    if (fr->fr_NumArgs > 0) {
        for (i = 0; i < fr->fr_NumArgs; i++) {
            BPTR l = Lock((CONST_STRPTR)fr->fr_Drawer, SHARED_LOCK);
            if (l) { add_lock(l, (const char *)fr->fr_ArgList[i].wa_Name); UnLock(l); }
        }
    } else {
        add_path(path);
    }
    FreeAslRequest(fr);
    if (list) ogt_list_set_count(list, nitems);
    if (nitems > before && (replace || !dto)) show_item(before, 1);
    else draw_all();
}

static void save_list(void)
{
    char path[PATH_LEN] = ".opl";
    BPTR f;
    int i;
    if (!nitems || !ask_file("Save the playlist", 1, 0, path, sizeof path, NULL)) return;
    if (!ends_with(path, ".opl") && !ends_with(path, ".m3u")) strncat(path, ".opl", sizeof path - strlen(path) - 1);
    if (!(f = Open((CONST_STRPTR)path, MODE_NEWFILE))) { set_status("Couldn't write the playlist there. Is the disk write-protected?"); return; }
    for (i = 0; i < nitems; i++) { Write(f, items[i].path, (LONG)strlen(items[i].path)); Write(f, "\n", 1); }
    Close(f);
    set_status("Playlist saved. Open it, or drop it on OpenPlay, to play it again.");
}

static void remove_item(void)
{
    int i = list ? list->selected : -1;
    if (i < 0 || i >= nitems) return;
    if (i == cur) close_media();
    memmove(&items[i], &items[i + 1], (nitems - i - 1) * sizeof *items);
    nitems--;
    if (cur > i) cur--;
    else if (cur == i) cur = -1;
    if (list) { ogt_list_set_count(list, nitems); ogt_list_select(list, i < nitems ? i : nitems - 1); }
    draw_all();
}

/* Save CDXL: the film as a CDXL the chipset plays by itself (ToCDXL,
 * openamigaservice), for an AGA Amiga with no services. */
static void save_cdxl(void)
{
    char path[PATH_LEN] = ".cdxl", cmd[PATH_LEN * 2 + 64];
    LONG rc;
    BPTR nil;
    if (kind != K_VIDEO || cur < 0) return;
    if (!ask_file("Save as CDXL", 1, 0, path, sizeof path, NULL)) return;
    if (!ends_with(path, ".cdxl")) strncat(path, ".cdxl", sizeof path - strlen(path) - 1);
    snprintf(cmd, sizeof cmd, "C:ToCDXL \"%s\" \"%s\" PRESET=AGA", items[cur].path, path);
    set_status("Making the CDXL on the Nursery...");
    SetWindowPointer(win, WA_BusyPointer, TRUE, TAG_DONE);
    nil = Open((CONST_STRPTR)"NIL:", MODE_NEWFILE);
    rc = SystemTags((CONST_STRPTR)cmd, SYS_Output, (ULONG)nil, SYS_Input, 0, TAG_DONE);
    if (nil) Close(nil);
    SetWindowPointer(win, TAG_DONE);
    if (rc == 0) {
        add_one(path);
        if (list) ogt_list_set_count(list, nitems);
        draw_side();
        set_status("CDXL saved and added to the playlist.");
    } else {
        set_status(rc < 0 ? "ToCDXL isn't installed (C:ToCDXL comes with OpenUp's OpenService part)."
                          : "ToCDXL couldn't convert it. It needs the services card or a paired Cradle.");
    }
}

/* ---- About this file ---- */

static void copy_text(const char *text)
{
    struct IFFHandle *iff;
    if (!IFFParseBase || !(iff = AllocIFF())) return;
    if ((iff->iff_Stream = (ULONG)OpenClipboard(0))) {
        InitIFFasClip(iff);
        if (!OpenIFF(iff, IFFF_WRITE)) {
            if (!PushChunk(iff, MAKE_ID('F', 'T', 'X', 'T'), ID_FORM, IFFSIZE_UNKNOWN) &&
                !PushChunk(iff, 0, MAKE_ID('C', 'H', 'R', 'S'), IFFSIZE_UNKNOWN)) {
                WriteChunkBytes(iff, (APTR)text, (LONG)strlen(text));
                PopChunk(iff);
                PopChunk(iff);
            }
            CloseIFF(iff);
        }
        CloseClipboard((struct ClipboardHandle *)iff->iff_Stream);
    }
    FreeIFF(iff);
}

static void info_window(void)
{
    struct Window *iw;
    struct Gadget *ig = NULL, *gg;
    struct NewGadget ng;
    /* static: the File line holds a whole path, and all of them are too much
       for a program's stack */
    static char lines[8][PATH_LEN + 16], all[8 * (PATH_LEN + 32)];
    const char *labels[8] = { "File", "Datatype", "Kind", "Size", "Decoded by", "Runs on", "Screen", "Opened in" };
    int n = 8, i, lw = 0, vw = 0, done = 0, ww, wh, bh = fh + 8;
    if (!dto || cur < 0) return;
    all[0] = 0;
    snprintf(lines[0], sizeof lines[0], "%s", items[cur].path);
    snprintf(lines[1], sizeof lines[0], "%s.datatype (%s)", dt_base, dt_name);
    snprintf(lines[2], sizeof lines[0], "%s", kind_word[kind]);
    if (kind == K_PICTURE) snprintf(lines[3], sizeof lines[0], "%lu x %lu", (unsigned long)pic_w, (unsigned long)pic_h);
    else if (kind == K_VIDEO) snprintf(lines[3], sizeof lines[0], "%d frames, %d a second", frames, fps);
    else if (kind == K_SOUND) snprintf(lines[3], sizeof lines[0], "%lu samples at %lu Hz", (unsigned long)snd_len, (unsigned long)snd_rate);
    else snprintf(lines[3], sizeof lines[0], "-");
    {
        STRPTR how = NULL;
        if (GetDTAttrs(dto, OIA_Stats, (ULONG)&how, TAG_DONE) && how && *how)
            snprintf(lines[4], sizeof lines[0], "%s: %s", decoded_by(), (const char *)how);
        else
            snprintf(lines[4], sizeof lines[0], "%s", decoded_by());
    }
    snprintf(lines[5], sizeof lines[0], "%s", on_amigachrome() ? "AmigaChrome" : "this Amiga");
    {
        ULONG depth = GetBitMapAttr(win->WScreen->RastPort.BitMap, BMA_DEPTH);
        snprintf(lines[6], sizeof lines[0], "%dx%d, %lu-bit%s", win->WScreen->Width, win->WScreen->Height, (unsigned long)depth,
                 own_scr ? ", OpenPlay's own screen" : "");
    }
    snprintf(lines[7], sizeof lines[0], "%s", opened_in);
    for (i = 0; i < n; i++) {
        int a = ogt_text_width(win->RPort, labels[i]), b = ogt_text_width(win->RPort, lines[i]);
        if (a > lw) lw = a;
        if (b > vw) vw = b;
        strncat(all, labels[i], sizeof all - strlen(all) - 1);
        strncat(all, ": ", sizeof all - strlen(all) - 1);
        strncat(all, lines[i], sizeof all - strlen(all) - 1);
        strncat(all, "\n", sizeof all - strlen(all) - 1);
    }
    if (vw > win->WScreen->Width - lw - 80) vw = win->WScreen->Width - lw - 80;
    ww = lw + vw + 48;
    if (ww < 300) ww = 300;
    wh = n * (fh + 4) + bh + 40;
    memset(&ng, 0, sizeof ng);
    ng.ng_TextAttr = &ta;
    ng.ng_VisualInfo = vi;
    if (!(iw = OpenWindowTags(NULL, WA_Left, win->LeftEdge + 40, WA_Top, win->TopEdge + 40, WA_InnerWidth, ww, WA_InnerHeight, wh,
                              WA_Title, (ULONG)"About this file", WA_CustomScreen, (ULONG)win->WScreen,
                              WA_DragBar, TRUE, WA_DepthGadget, TRUE, WA_CloseGadget, TRUE, WA_Activate, TRUE,
                              WA_IDCMP, IDCMP_CLOSEWINDOW | IDCMP_GADGETUP | IDCMP_VANILLAKEY | IDCMP_REFRESHWINDOW, TAG_DONE)))
        return;
    SetFont(iw->RPort, font);
    gg = CreateContext(&ig);
    ng.ng_TopEdge = iw->BorderTop + wh - bh - 8;
    ng.ng_Height = bh;
    ng.ng_Width = ogt_text_width(iw->RPort, "Copy text") + 24;
    ng.ng_LeftEdge = iw->Width - iw->BorderRight - 8 - ng.ng_Width - 8 - (ogt_text_width(iw->RPort, "Close") + 32);
    ng.ng_GadgetText = (UBYTE *)"Cop_y text";
    ng.ng_GadgetID = 1;
    gg = CreateGadget(BUTTON_KIND, gg, &ng, GT_Underscore, '_', TAG_DONE);
    ng.ng_LeftEdge += ng.ng_Width + 8;
    ng.ng_Width = ogt_text_width(iw->RPort, "Close") + 32;
    ng.ng_GadgetText = (UBYTE *)"_Close";
    ng.ng_GadgetID = 2;
    gg = CreateGadget(BUTTON_KIND, gg, &ng, GT_Underscore, '_', TAG_DONE);
    ogt_fill(&ctx, iw->RPort, "window", iw->BorderLeft, iw->BorderTop, iw->Width - iw->BorderLeft - iw->BorderRight,
             iw->Height - iw->BorderTop - iw->BorderBottom);
    {
        int gx = iw->BorderLeft + 8, gy = iw->BorderTop + 10;
        ogt_frame(iw->RPort, pen("group.line"), gx, gy - 4, iw->Width - iw->BorderLeft - iw->BorderRight - 16, n * (fh + 4) + 12);
        for (i = 0; i < n; i++) {
            int y = gy + 2 + i * (fh + 4);
            ogt_text(iw->RPort, pen("label"), gx + 8 + lw - ogt_text_width(iw->RPort, labels[i]), y, labels[i], 0);
            ogt_text(iw->RPort, pen("text"), gx + 8 + lw + 12, y, lines[i], vw);
        }
    }
    if (gg) { AddGList(iw, ig, (UWORD)~0, -1, NULL); RefreshGList(ig, iw, NULL, -1); GT_RefreshWindow(iw, NULL); }
    while (!done) {
        struct IntuiMessage *m;
        WaitPort(iw->UserPort);
        while ((m = GT_GetIMsg(iw->UserPort))) {
            ULONG cl = m->Class;
            UWORD code = m->Code;
            struct Gadget *g = m->IAddress;
            GT_ReplyIMsg(m);
            if (cl == IDCMP_CLOSEWINDOW) done = 1;
            else if (cl == IDCMP_GADGETUP && g->GadgetID == 2) done = 1;
            else if (cl == IDCMP_GADGETUP && g->GadgetID == 1) copy_text(all);
            else if (cl == IDCMP_VANILLAKEY && (code == 27 || code == 'c' || code == 'C' || code == 13)) done = 1;
            else if (cl == IDCMP_VANILLAKEY && (code == 'y' || code == 'Y')) copy_text(all);
            else if (cl == IDCMP_REFRESHWINDOW) { GT_BeginRefresh(iw); GT_EndRefresh(iw, TRUE); }
        }
    }
    RemoveGList(iw, ig, -1);
    CloseWindow(iw);
    FreeGadgets(ig);
}

/* ---- full screen ---- */

/* The film or picture alone on a screen of its own, Workbench's mode;
 * Esc, a click or F goes back. OpenRTG scales it on the board. */
static void full_screen(void)
{
    struct Screen *fs;
    struct Window *fw;
    Object *o;
    int done = 0;
    ULONG fpen;
    if (!dto || cur < 0 || kind == K_SOUND) return;
    if (!(fs = OpenScreenTags(NULL, SA_LikeWorkbench, TRUE, SA_Title, (ULONG)"OpenPlay", SA_ShowTitle, FALSE,
                              SA_Quiet, TRUE, SA_Type, CUSTOMSCREEN, TAG_DONE))) {
        set_status("There isn't memory for a screen of its own. Close something and try again.");
        return;
    }
    if (!(fw = OpenWindowTags(NULL, WA_CustomScreen, (ULONG)fs, WA_Left, 0, WA_Top, 0, WA_Width, fs->Width, WA_Height, fs->Height,
                              WA_Borderless, TRUE, WA_Backdrop, TRUE, WA_Activate, TRUE, WA_RMBTrap, TRUE,
                              WA_IDCMP, IDCMP_VANILLAKEY | IDCMP_MOUSEBUTTONS | IDCMP_IDCMPUPDATE | IDCMP_RAWKEY, TAG_DONE))) {
        CloseScreen(fs);
        return;
    }
    fpen = ObtainBestPen(fs->ViewPort.ColorMap, 0, 0, 0, OBP_Precision, PRECISION_EXACT, TAG_DONE);
    SetAPen(fw->RPort, fpen);
    RectFill(fw->RPort, 0, 0, fs->Width - 1, fs->Height - 1);
    if (playing && kind != K_PICTURE) DoDTMethod(dto, win, NULL, DTM_TRIGGER, NULL, STM_PAUSE, NULL);
    o = NewDTObject((APTR)items[cur].path, GA_Left, 0, GA_Top, 0, GA_Width, fs->Width, GA_Height, fs->Height,
                    GA_ID, 1, ICA_TARGET, ICTARGET_IDCMP, PDTA_Remap, TRUE, PDTA_DestMode, PMODE_V43,
                    DTA_ControlPanel, FALSE, TAG_DONE);
    if (o) {
        AddDTObject(fw, NULL, o, -1);
        RefreshDTObjects(o, fw, NULL, TAG_DONE);
        if (kind == K_VIDEO) DoDTMethod(o, fw, NULL, DTM_TRIGGER, NULL, STM_PLAY, NULL);
    }
    while (!done) {
        struct IntuiMessage *m;
        WaitPort(fw->UserPort);
        while ((m = (struct IntuiMessage *)GetMsg(fw->UserPort))) {
            ULONG cl = m->Class;
            UWORD code = m->Code;
            struct TagItem *tags = cl == IDCMP_IDCMPUPDATE ? (struct TagItem *)m->IAddress : NULL;
            if (tags && o && FindTagItem(DTA_Sync, tags)) RefreshDTObjects(o, fw, NULL, TAG_DONE);
            ReplyMsg((struct Message *)m);
            if (cl == IDCMP_MOUSEBUTTONS && code == SELECTDOWN) done = 1;
            else if (cl == IDCMP_VANILLAKEY && (code == 27 || code == 'f' || code == 'F')) done = 1;
            else if (cl == IDCMP_VANILLAKEY && code == ' ' && o && kind == K_VIDEO)
                DoDTMethod(o, fw, NULL, DTM_TRIGGER, NULL, STM_PAUSE, NULL);
        }
    }
    if (o) {
        DoDTMethod(o, fw, NULL, DTM_TRIGGER, NULL, STM_STOP, NULL);
        RemoveDTObject(fw, o);
        DisposeDTObject(o);
    }
    ReleasePen(fs->ViewPort.ColorMap, fpen);
    CloseWindow(fw);
    CloseScreen(fs);
    playing = 0;
    draw_tools();
}

/* ---- the window: open, close, and the screen it is on ---- */

static void close_ui(void)
{
    close_media();
    if (appwin) { RemoveAppWindow(appwin); appwin = NULL; }
    if (list && win) { ogt_list_free(list); list = NULL; }
    if (win) {
        save_prefs();
        ClearMenuStrip(win);
        remove_gadgets();
        CloseWindow(win);
        win = NULL;
    }
    if (menus) { FreeMenus(menus); menus = NULL; }
    if (font) { CloseFont(font); font = NULL; }
    if (ctx_ok) { ogt_ctx_free(&ctx); ctx_ok = 0; }
    if (vi) { FreeVisualInfo(vi); vi = NULL; }
    if (scr) { UnlockPubScreen(NULL, scr); scr = NULL; }
    if (own_scr) {
        while (!CloseScreen(own_scr)) Delay(25);    /* visitors close first */
        own_scr = NULL;
    }
}

static void set_menu_checks(void)
{
    struct MenuItem *it;
    struct Menu *mm;
    for (mm = menus; mm; mm = mm->NextMenu)
        for (it = mm->FirstItem; it; it = it->NextItem) {
            struct MenuItem *sub;
            int id = (int)(ULONG)GTMENUITEM_USERDATA(it);
            if (id == C_LIST) it->Flags = show_list ? (it->Flags | CHECKED) : (it->Flags & ~CHECKED);
            if (id == C_REPEAT) it->Flags = repeat_on ? (it->Flags | CHECKED) : (it->Flags & ~CHECKED);
            if (id == C_OWNSCREEN) it->Flags = own_scr ? (it->Flags | CHECKED) : (it->Flags & ~CHECKED);
            for (sub = it->SubItem; sub; sub = sub->NextItem) {
                int sid = (int)(ULONG)GTMENUITEM_USERDATA(sub);
                int on = (sid == C_BT_LOOK && bt_choice < 0) || (sid == C_BT_BOTH && bt_choice == OGT_TB_ICONS_TEXT) ||
                         (sid == C_BT_ICONS && bt_choice == OGT_TB_ICONS) || (sid == C_BT_TEXT && bt_choice == OGT_TB_TEXT);
                sub->Flags = on ? (sub->Flags | CHECKED) : (sub->Flags & ~CHECKED);
            }
        }
}

/* The part of the screen a full-size window may have: below the title bar,
 * less the strip OpenDock takes along an edge (OpenFiles' free_area(), copied
 * here). OpenDock's window is the one whose screen title starts "OpenDock";
 * an ENV:OpenDock/Free of "left top width height" wins when the dock
 * publishes one. Without a dock it is the screen less its title bar. */
static void free_area(int *l, int *t, int *w, int *h)
{
    char buf[48];
    struct Window *dw;
    ULONG lock;
    LONG got;
    int top = scr->BarHeight + 1, bottom = scr->Height, left = 0, right = scr->Width, a, b, c, d;
    got = GetVar((STRPTR)"OpenDock/Free", (STRPTR)buf, sizeof buf, GVF_GLOBAL_ONLY);
    if (got > 0 && sscanf(buf, "%d %d %d %d", &a, &b, &c, &d) == 4 && c >= 400 && d >= 200 && a >= 0 && b >= 0 &&
        a + c <= scr->Width && b + d <= scr->Height) {
        *l = a;
        *t = b < top ? top : b;
        *w = c;
        *h = b + d - *t;
        return;
    }
    lock = LockIBase(0);
    for (dw = scr->FirstWindow; dw; dw = dw->NextWindow) {
        if (!dw->ScreenTitle || strncmp((const char *)dw->ScreenTitle, "OpenDock", 8) != 0)
            continue;
        if (dw->Width >= dw->Height) {              /* along the top or the bottom */
            if (dw->TopEdge + dw->Height / 2 > scr->Height / 2) {
                if (dw->TopEdge < bottom)
                    bottom = dw->TopEdge;
            } else if (dw->TopEdge + dw->Height > top)
                top = dw->TopEdge + dw->Height;
        } else {                                    /* down the left or the right */
            if (dw->LeftEdge + dw->Width / 2 > scr->Width / 2) {
                if (dw->LeftEdge < right)
                    right = dw->LeftEdge;
            } else if (dw->LeftEdge + dw->Width > left)
                left = dw->LeftEdge + dw->Width;
        }
    }
    UnlockIBase(lock);
    if (right - left < 400 || bottom - top < 200) { /* a dock that big: use the whole screen */
        left = 0;
        right = scr->Width;
        top = scr->BarHeight + 1;
        bottom = scr->Height;
    }
    *l = left;
    *t = top;
    *w = right - left;
    *h = bottom - top;
}

/* The first size (the user, 10 October 2026, as in OpenFiles 0.2.3): 800 x
 * 600, centred in the free area, and never bigger than it, so on a screen
 * smaller than 800 x 600 it is the free area itself. A size the user gives
 * the window is kept and given back by OpenWindows. */
#define START_W 800
#define START_H 600
static void start_box(int *l, int *t, int *w, int *h)
{
    int al, at, aw, ah;
    free_area(&al, &at, &aw, &ah);
    *w = aw < START_W ? aw : START_W;
    *h = ah < START_H ? ah : START_H;
    *l = al + (aw - *w) / 2;
    *t = at + (ah - *h) / 2;
}

static int open_ui(void)
{
    int w, h, l, t;
    if (want_own) {
        own_scr = OpenScreenTags(NULL, SA_LikeWorkbench, TRUE, SA_Title, (ULONG)VERSION_TEXT, SA_PubName, (ULONG)"OPENPLAY",
                                 SA_Type, PUBLICSCREEN, TAG_DONE);
        if (own_scr) PubScreenStatus(own_scr, 0);
        scr = LockPubScreen(own_scr ? (CONST_STRPTR)"OPENPLAY" : NULL);
    } else {
        scr = LockPubScreen(NULL);
    }
    if (!scr || !(vi = GetVisualInfoA(scr, NULL))) return 0;
    if (!(ctx_ok = ogt_ctx_init(&ctx, scr, &theme, theme_mode))) return 0;
    if (!(font = ogt_open_font(&theme, scr, &ta))) return 0;
    fh = font->tf_YSize;
    if (!(menus = CreateMenus(newmenus, GTMN_FrontPen, 0, TAG_DONE)) || !LayoutMenus(menus, vi, GTMN_NewLookMenus, TRUE, TAG_DONE))
        return 0;
    if (own_scr) {
        l = 0; t = scr->BarHeight + 1; w = scr->Width; h = scr->Height - t;
    } else if (win_box[2] >= 400 && win_box[3] >= 240) {
        l = win_box[0]; t = win_box[1]; w = win_box[2]; h = win_box[3];
        if (l + w > scr->Width) l = scr->Width - w;
        if (t + h > scr->Height) t = scr->Height - h;
        if (l < 0) l = 0;
        if (t < 0) t = 0;
    } else {
        start_box(&l, &t, &w, &h);      /* 800 x 600 in the free area, or the free area when smaller */
    }
    win = OpenWindowTags(NULL, WA_Left, l, WA_Top, t, WA_Width, w, WA_Height, h,
                         WA_MinWidth, w < 400 ? w : 400, WA_MinHeight, h < 240 ? h : 240, WA_MaxWidth, ~0, WA_MaxHeight, ~0,
                         WA_Title, (ULONG)"OpenPlay", WA_ScreenTitle, (ULONG)VERSION_TEXT, WA_PubScreen, (ULONG)scr,
                         WA_NewLookMenus, TRUE, WA_DragBar, TRUE, WA_DepthGadget, TRUE, WA_CloseGadget, TRUE,
                         WA_SizeGadget, TRUE, WA_SizeBBottom, TRUE, WA_Activate, TRUE, WA_SmartRefresh, TRUE,
                         WA_ReportMouse, TRUE,
                         WA_IDCMP, IDCMP_CLOSEWINDOW | IDCMP_NEWSIZE | IDCMP_REFRESHWINDOW | IDCMP_MOUSEBUTTONS |
                                   IDCMP_MOUSEMOVE | IDCMP_GADGETUP | IDCMP_GADGETDOWN | IDCMP_IDCMPUPDATE | IDCMP_MENUPICK |
                                   IDCMP_RAWKEY | IDCMP_VANILLAKEY | IDCMP_INTUITICKS | BUTTONIDCMP,
                         TAG_DONE);
    if (!win) return 0;
    SetFont(win->RPort, font);
    set_menu_checks();
    SetMenuStrip(win, menus);
    if (!(list = ogt_list_new(win, &ctx, GID_LIST, row_h, draw_row, NULL, NULL, "list"))) return 0;
    if (appport && WorkbenchBase && !own_scr) appwin = AddAppWindowA(1, 0, win, appport, NULL);
    relayout_and_draw();
    return 1;
}

static int reopen_ui(void)
{
    int keep = cur, was_playing = playing;
    close_ui();
    if (!open_ui()) return 0;
    if (keep >= 0) show_item(keep, was_playing);
    return 1;
}

static void about(void)
{
    struct EasyStruct es = { sizeof(struct EasyStruct), 0, (UBYTE *)"About OpenPlay",
        (UBYTE *)VERSION_TEXT "\n\nPlays anything a datatype opens: pictures, sounds,\n"
                 "tunes and films. Formats the Amiga can't decode by\nitself come through the Nursery: the services card\n"
                 "or a paired Cradle.\n\nMIT, Copyright (c) 2026 Dalsin Limited.\nhttps://github.com/DalsinAI/openamigamediaplayer",
        (UBYTE *)"Close" };
    EasyRequestArgs(win, &es, NULL, NULL);
}

/* ---- commands ---- */

static int command(int c)
{
    switch (c) {
    case C_OPEN: open_or_add(1); break;
    case C_ADD: open_or_add(0); break;
    case C_PREV: step(-1, playing || kind != K_PICTURE); break;
    case C_NEXT: step(1, playing || kind != K_PICTURE); break;
    case C_PLAY: play_pause(); break;
    case C_STOP: stop(); break;
    case C_REPEAT:
        repeat_on = !repeat_on;
        if (dto && (kind == K_SOUND || kind == K_VIDEO)) SetDTAttrs(dto, win, NULL, DTA_Repeat, (ULONG)repeat_on, TAG_DONE);
        set_menu_checks();
        draw_tools();
        break;
    case C_LIST: show_list = !show_list; set_menu_checks(); relayout_and_draw(); break;
    case C_FULL: full_screen(); break;
    case C_INFO: info_window(); break;
    case C_CDXL: save_cdxl(); break;
    case C_REMOVE: remove_item(); break;
    case C_SAVELIST: save_list(); break;
    case C_CLEAR: close_media(); nitems = 0; cur = -1; if (list) ogt_list_set_count(list, 0); draw_all(); break;
    case C_ABOUT: about(); break;
    case C_QUIT: return 1;
    case C_OWNSCREEN: want_own = !want_own; if (!reopen_ui()) return 1; break;
    case C_BT_LOOK: bt_choice = -1; set_menu_checks(); relayout_and_draw(); break;
    case C_BT_BOTH: bt_choice = OGT_TB_ICONS_TEXT; set_menu_checks(); relayout_and_draw(); break;
    case C_BT_ICONS: bt_choice = OGT_TB_ICONS; set_menu_checks(); relayout_and_draw(); break;
    case C_BT_TEXT: bt_choice = OGT_TB_TEXT; set_menu_checks(); relayout_and_draw(); break;
    }
    return 0;
}

static void set_volume(int v)
{
    volume = v < 0 ? 0 : v > 64 ? 64 : v;
    if (dto && (kind == K_SOUND || kind == K_VIDEO)) SetDTAttrs(dto, win, NULL, SDTA_Volume, (ULONG)volume, TAG_DONE);
    draw_seek();
}

/* The film has reached frame f: move the slider, and at the last frame go
 * on to the next item, or stop, unless Repeat is on. TRUE when the item
 * changed. */
static int at_frame(int f)
{
    if (f == frame) return 0;
    frame = f;
    dbg("frame %d", frame);
    draw_seek();
    if (playing && !repeat_on && frames > 1 && frame >= frames - 1) {
        if (cur < nitems - 1) {
            step(1, 1);
            return 1;
        }
        playing = 0;
        draw_tools();
    }
    return 0;
}

static void dt_update(struct TagItem *tags)
{
    struct TagItem *ti, *state = tags;
    while ((ti = NextTagItem(&state))) {
        switch (ti->ti_Tag) {
        case DTA_Sync:
            dbg("sync");
            RefreshDTObjects(dto, win, NULL, TAG_DONE);
            dbg("refreshed");
            if (pending_play) {
                pending_play = 0;
                dbg("play on sync");
                DoDTMethod(dto, win, NULL, DTM_TRIGGER, NULL, STM_PLAY, NULL);
                dbg("playing");
            }
            break;
        case ADTA_Frame:
            if (at_frame((int)ti->ti_Data)) return;
            break;
        }
    }
}

static int main_event(struct IntuiMessage *m)
{
    ULONG cl = m->Class, secs = m->Seconds, micros = m->Micros;
    if (m->Class != IDCMP_INTUITICKS && m->Class != IDCMP_MOUSEMOVE) dbg("event %08lx code %u", (unsigned long)m->Class, (unsigned)m->Code);
    UWORD code = m->Code, qual = m->Qualifier;
    APTR ia = m->IAddress;
    int mx = m->MouseX, my = m->MouseY, idx, r, i;
    /* the datatype's news; animation.datatype may leave GA_ID out */
    if (cl == IDCMP_IDCMPUPDATE && dto &&
        (FindTagItem(GA_ID, (struct TagItem *)ia) ? GetTagData(GA_ID, 0, (struct TagItem *)ia) == GID_DT
                                                  : FindTagItem(ADTA_Frame, (struct TagItem *)ia) || FindTagItem(DTA_Sync, (struct TagItem *)ia))) {
        struct TagItem *copy = CloneTagItems((struct TagItem *)ia);
        GT_ReplyIMsg(m);
        if (copy) { dt_update(copy); FreeTagItems(copy); }
        return 0;
    }
    GT_ReplyIMsg(m);
    switch (cl) {
    case IDCMP_CLOSEWINDOW:
        return 1;
    case IDCMP_NEWSIZE:
        relayout_and_draw();
        break;
    case IDCMP_REFRESHWINDOW:
        GT_BeginRefresh(win);
        GT_EndRefresh(win, TRUE);
        break;
    case IDCMP_INTUITICKS:
        if (pending_play && dto && ++pending_ticks >= 10) {
            pending_play = 0;
            DoDTMethod(dto, win, NULL, DTM_TRIGGER, NULL, STM_PLAY, NULL);
        }
        /* animation.datatype 47 doesn't always say which frame it is on:
         * ask it, so the slider and the buttons follow the film */
        if (playing && !pending_play && kind == K_VIDEO && dto) {
            ULONG f = 0;
            if (GetDTAttrs(dto, ADTA_Frame, (ULONG)&f, TAG_DONE) && at_frame((int)f)) break;
        }
        if (playing && kind == K_PICTURE && ++slide_ticks >= SLIDE_TICKS) {
            slide_ticks = 0;
            step(1, 1);
        }
        break;
    case IDCMP_MOUSEMOVE: {
        int h = -1;
        for (i = 0; i < tb.n; i++)
            if (tb.box[i].w && mx >= tb.box[i].x && mx < tb.box[i].x + tb.box[i].w && my >= tb.box[i].y && my < tb.box[i].y + tb.box[i].h)
                h = i;
        if (h != hover) { hover = h; draw_status(); }
        if (list) ogt_list_event(list, cl, code, ia, mx, my, secs, micros, &idx);
        break;
    }
    case IDCMP_MOUSEBUTTONS:
        if (code == SELECTDOWN && show_list && list && mx >= list->x && mx < list->x + list->w && my >= list->y && my < list->y + list->h) {
            r = ogt_list_event(list, cl, code, ia, mx, my, secs, micros, &idx);
            if (r == OGT_LIST_OPENED) show_item(idx, 1);
        } else if (code == SELECTDOWN && vol_box.w && mx >= vol_box.x && mx < vol_box.x + vol_box.w && my >= vol_box.y && my < vol_box.y + vol_box.h) {
            set_volume((mx - vol_box.x) * 64 / (vol_box.w - 1));
        } else if (code == SELECTDOWN && bar_box.w && mx >= bar_box.x && mx < bar_box.x + bar_box.w && my >= bar_box.y && my < bar_box.y + bar_box.h) {
            int at = (mx - bar_box.x) * 1000 / bar_box.w;
            if (kind == K_PICTURE && nitems > 1) {
                int n = at * nitems / 1000;
                show_item(n < nitems ? n : nitems - 1, playing);
            } else if (kind == K_VIDEO && dto && frames > 1) {
                /* seek: ADTA_Frame */
                frame = at * (frames - 1) / 1000;
                SetDTAttrs(dto, win, NULL, ADTA_Frame, (ULONG)frame, TAG_DONE);
                draw_seek();
            }
        }
        break;
    case IDCMP_IDCMPUPDATE:
        if (list) ogt_list_event(list, cl, code, ia, mx, my, secs, micros, &idx);
        break;
    case IDCMP_GADGETDOWN: {
        struct Gadget *g = ia;
        i = ogt_toolbar_index(&tb, g->GadgetID);
        if (i >= 0 && !tb.tool[i].disabled) {
            tb.pressed = i;
            draw_one(i, 1);
        }
        if (list) ogt_list_event(list, cl, code, ia, mx, my, secs, micros, &idx);
        break;
    }
    case IDCMP_GADGETUP: {
        struct Gadget *g = ia;
        i = ogt_toolbar_index(&tb, g->GadgetID);
        if (i >= 0) {
            tb.pressed = -1;
            draw_one(i, 0);
            if (!tb.tool[i].disabled) {
                r = command(tb.tool[i].id);
                if (win) draw_tools();
                return r;
            }
        } else if (g->GadgetID == GID_ADD) return command(C_ADD);
        else if (g->GadgetID == GID_REMOVE) return command(C_REMOVE);
        else if (g->GadgetID == GID_SAVE) return command(C_SAVELIST);
        else if (list) ogt_list_event(list, cl, code, ia, mx, my, secs, micros, &idx);
        break;
    }
    case IDCMP_MENUPICK:
        while (code != MENUNULL) {
            struct MenuItem *it = ItemAddress(menus, code);
            if (!it) break;
            if (command((int)(ULONG)GTMENUITEM_USERDATA(it))) return 1;
            if (!win || !menus) break;              /* the window was opened again */
            code = it->NextSelect;
        }
        break;
    case IDCMP_RAWKEY:
        switch (code) {
        case 0x4f: command(C_PREV); break;                    /* left */
        case 0x4e: command(C_NEXT); break;                    /* right */
        case 0x4c: if (list && (idx = ogt_list_step(list, -1)) >= 0) ogt_list_draw(list); break;
        case 0x4d: if (list && (idx = ogt_list_step(list, 1)) >= 0) ogt_list_draw(list); break;
        case 0x5f: about(); break;                            /* Help */
        }
        break;
    case IDCMP_VANILLAKEY:
        if (qual & (IEQUALIFIER_LCOMMAND | IEQUALIFIER_RCOMMAND)) break;
        switch (code) {
        case 'o': case 'O': return command(C_OPEN);
        case 'v': case 'V': return command(C_PREV);
        case 'p': case 'P': case ' ': return command(C_PLAY);
        case 't': case 'T': case 27: return command(C_STOP);
        case 'n': case 'N': return command(C_NEXT);
        case 'r': case 'R': return command(C_REPEAT);
        case 'f': case 'F': return command(C_FULL);
        case 'l': case 'L': return command(C_LIST);
        case 'i': case 'I': return command(C_INFO);
        case 'c': case 'C': return command(C_CDXL);
        case 'a': case 'A': return command(C_ADD);
        case 'm': case 'M': case 127: return command(C_REMOVE);
        case 13: if (list && list->selected >= 0) show_item(list->selected, 1); break;
        case '+': case '=': set_volume(volume + 8); break;
        case '-': set_volume(volume - 8); break;
        }
        break;
    }
    return 0;
}

static void app_news(void)
{
    struct AppMessage *am;
    int before = nitems, i;
    while ((am = (struct AppMessage *)GetMsg(appport))) {
        for (i = 0; i < am->am_NumArgs; i++)
            add_lock(am->am_ArgList[i].wa_Lock, (const char *)am->am_ArgList[i].wa_Name);
        ReplyMsg((struct Message *)am);
    }
    if (list) ogt_list_set_count(list, nitems);
    if (nitems > before) show_item(before, 1);
    else draw_side();
}

/* ---- start ---- */

static void tooltypes(struct WBStartup *wb)
{
    struct DiskObject *dob;
    BPTR old;
    STRPTR v;
    if (!IconBase || !wb || wb->sm_NumArgs < 1) return;
    old = CurrentDir(wb->sm_ArgList[0].wa_Lock);
    if ((dob = GetDiskObject((CONST_STRPTR)wb->sm_ArgList[0].wa_Name))) {
        if ((v = FindToolType((CONST_STRPTR *)dob->do_ToolTypes, (CONST_STRPTR)"BUTTONS"))) bt_forced = parse_buttons((char *)v);
        if (FindToolType((CONST_STRPTR *)dob->do_ToolTypes, (CONST_STRPTR)"OWNSCREEN")) want_own = 1;
        FreeDiskObject(dob);
    }
    CurrentDir(old);
}

static int op_main(void)
{
    LONG args[3] = { 0, 0, 0 };
    struct RDArgs *rda = NULL;
    int quit = 0, rc = RETURN_FAIL, i;

    if (!(DataTypesBase = OpenLibrary((CONST_STRPTR)"datatypes.library", 39))) goto out;
    AslBase = OpenLibrary((CONST_STRPTR)"asl.library", 38);
    IFFParseBase = OpenLibrary((CONST_STRPTR)"iffparse.library", 39);
    IconBase = OpenLibrary((CONST_STRPTR)"icon.library", 39);
    WorkbenchBase = OpenLibrary((CONST_STRPTR)"workbench.library", 39);
    load_prefs();
    load_look();

    if (_WBenchMsg) {
        tooltypes(_WBenchMsg);
        for (i = 1; i < _WBenchMsg->sm_NumArgs; i++)
            add_lock(_WBenchMsg->sm_ArgList[i].wa_Lock, (const char *)_WBenchMsg->sm_ArgList[i].wa_Name);
    } else if ((rda = ReadArgs((CONST_STRPTR)"FILES/M,BUTTONS/K,OWNSCREEN/S", args, NULL))) {
        if (args[0]) { char **f = (char **)args[0]; while (*f) add_path(*f++); }
        if (args[1]) bt_forced = parse_buttons((const char *)args[1]);
        if (args[2]) want_own = 1;
    }
    appport = CreateMsgPort();
    if (!open_ui()) goto out;
    if (nitems) show_item(0, 1);
    else set_status("Drop pictures, sounds, films or drawers on the window, or choose Open.");

    while (!quit) {
        ULONG got = Wait((1UL << win->UserPort->mp_SigBit) | SIGBREAKF_CTRL_C | (appport ? 1UL << appport->mp_SigBit : 0));
        struct IntuiMessage *m;
        if (got & SIGBREAKF_CTRL_C) quit = 1;
        if (appport && (got & (1UL << appport->mp_SigBit))) app_news();
        while (!quit && win && (m = GT_GetIMsg(win->UserPort)))
            quit = main_event(m);
        if (!win) break;
    }
    rc = RETURN_OK;
out:
    close_ui();
    if (appport) {
        struct Message *msg;
        while ((msg = GetMsg(appport))) ReplyMsg(msg);
        DeleteMsgPort(appport);
    }
    free(items);
    if (theme.text) ogt_theme_free(&theme);
    if (WorkbenchBase) CloseLibrary(WorkbenchBase);
    if (IconBase) CloseLibrary(IconBase);
    if (IFFParseBase) CloseLibrary(IFFParseBase);
    if (AslBase) CloseLibrary(AslBase);
    if (DataTypesBase) CloseLibrary(DataTypesBase);
    if (rda) FreeArgs(rda);
    if (rc != RETURN_OK) PutStr((CONST_STRPTR)"OpenPlay couldn't open its window (OS 3.2 and datatypes.library 39 needed).\n");
    return rc;
}

int main(void)
{
    return op_main_with_stack(op_main, 65536);
}
