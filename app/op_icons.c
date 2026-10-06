/* OpenPlay's player icons (op_icons.h).
 * MIT, Copyright (c) 2026 Dalsin Limited. */
#include <exec/types.h>
#include <graphics/rastport.h>
#include <proto/graphics.h>

#include "op_icons.h"

static const ogt_rgb colours[] = {
    {0, 0, 0},                                                   /* 0: none */
    {0x3f, 0x9a, 0x3a}, {0x1d, 0x4d, 0x1b}, {0xff, 0xff, 0xff},  /* 1 green, 2 its edge, 3 white */
    {0xe8, 0x57, 0x4a}, {0x6b, 0x1d, 0x16}, {0x6f, 0x9a, 0xd8},  /* 4 red, 5 its edge, 6 blue */
    {0x1d, 0x3b, 0x6b}, {0xd9, 0xdd, 0xe3}, {0x3a, 0x3f, 0x47},  /* 7 navy, 8 light grey, 9 slate */
    {0x36, 0x5f, 0xa3}, {0xa8, 0x6c, 0xe0}, {0x3e, 0x16, 0x70},  /* 10 accent blue, 11 purple, 12 its edge */
    {0xf2, 0xa1, 0x3a}, {0x6a, 0x3c, 0x06}, {0xf0, 0xc2, 0x5e},  /* 13 orange, 14 its edge, 15 folder */
    {0x5a, 0x41, 0x10}, {0xf4, 0xd3, 0x6b},                      /* 16 folder edge, 17 folder front */
};

enum { S_END, S_POLY, S_LINE, S_RECT, S_CIRCLE };

typedef struct shape {
    unsigned char kind, fill, line, thick, n;
    signed char p[24];
} shape;

#define END { S_END, 0, 0, 0, 0, {0} }

static const shape icons[OP_ICON_COUNT][6] = {
    [OP_ICON_OPEN] = {
        { S_POLY, 15, 16, 1, 6, {4, 8, 18, 8, 22, 13, 40, 13, 40, 40, 4, 40} },
        { S_POLY, 17, 16, 1, 4, {11, 21, 46, 21, 40, 40, 4, 40} }, END },
    [OP_ICON_PLAY] = {
        { S_CIRCLE, 1, 2, 1, 0, {24, 24, 20} },
        { S_POLY, 3, 2, 1, 3, {19, 13, 35, 24, 19, 35} }, END },
    [OP_ICON_PAUSE] = {
        { S_CIRCLE, 1, 2, 1, 0, {24, 24, 20} },
        { S_RECT, 3, 2, 1, 0, {16, 14, 21, 34} },
        { S_RECT, 3, 2, 1, 0, {27, 14, 32, 34} }, END },
    [OP_ICON_STOP] = {
        { S_RECT, 4, 5, 1, 0, {10, 10, 38, 38} }, END },
    [OP_ICON_PREV] = {
        { S_RECT, 6, 7, 1, 0, {8, 12, 13, 36} },
        { S_POLY, 6, 7, 1, 3, {40, 12, 40, 36, 16, 24} }, END },
    [OP_ICON_NEXT] = {
        { S_RECT, 6, 7, 1, 0, {35, 12, 40, 36} },
        { S_POLY, 6, 7, 1, 3, {8, 12, 8, 36, 32, 24} }, END },
    [OP_ICON_REPEAT] = {
        { S_LINE, 0, 7, 3, 4, {10, 26, 10, 14, 34, 14, 34, 18} },
        { S_POLY, 6, 7, 1, 3, {28, 18, 40, 18, 34, 26} },
        { S_LINE, 0, 7, 3, 4, {38, 22, 38, 34, 14, 34, 14, 30} },
        { S_POLY, 6, 7, 1, 3, {8, 30, 20, 30, 14, 22} }, END },
    [OP_ICON_FULLSCREEN] = {
        { S_RECT, 8, 9, 1, 0, {4, 7, 44, 37} },
        { S_RECT, 6, 7, 1, 0, {8, 11, 40, 33} },
        { S_LINE, 0, 3, 2, 3, {13, 20, 13, 15, 19, 15} },
        { S_LINE, 0, 3, 2, 3, {35, 24, 35, 29, 29, 29} },
        { S_RECT, 9, 9, 1, 0, {18, 38, 30, 41} }, END },
    [OP_ICON_PLAYLIST] = {
        { S_RECT, 3, 9, 1, 0, {8, 6, 40, 42} },
        { S_LINE, 0, 10, 2, 2, {14, 16, 34, 16} },
        { S_LINE, 0, 10, 2, 2, {14, 24, 34, 24} },
        { S_LINE, 0, 10, 2, 2, {14, 32, 28, 32} }, END },
    [OP_ICON_INFO] = {
        { S_CIRCLE, 6, 7, 1, 0, {24, 24, 20} },
        { S_CIRCLE, 3, 3, 1, 0, {24, 14, 3} },
        { S_RECT, 3, 3, 1, 0, {21, 20, 27, 35} }, END },
    [OP_ICON_CONVERT] = {
        { S_POLY, 11, 12, 1, 5, {6, 5, 38, 5, 43, 10, 43, 43, 6, 43} },
        { S_RECT, 3, 12, 1, 0, {14, 5, 34, 17} },
        { S_RECT, 12, 12, 1, 0, {27, 8, 31, 14} },
        { S_RECT, 3, 12, 1, 0, {12, 26, 37, 43} },
        { S_LINE, 0, 11, 1, 2, {16, 33, 33, 33} }, END },
    [OP_ICON_SLIDESHOW] = {
        { S_RECT, 3, 9, 1, 0, {13, 4, 44, 30} },
        { S_RECT, 8, 9, 1, 0, {4, 13, 35, 41} },
        { S_POLY, 1, 2, 1, 5, {6, 39, 15, 27, 21, 33, 26, 29, 33, 39} },
        { S_CIRCLE, 13, 14, 1, 0, {27, 20, 3} }, END },
    [OP_ICON_SOUND] = {
        { S_POLY, 11, 12, 1, 4, {16, 10, 40, 4, 40, 12, 20, 18} },
        { S_LINE, 0, 12, 3, 2, {18, 14, 18, 36} },
        { S_LINE, 0, 12, 3, 2, {38, 8, 38, 30} },
        { S_CIRCLE, 11, 12, 1, 0, {13, 37, 6} },
        { S_CIRCLE, 11, 12, 1, 0, {33, 31, 6} }, END },
    [OP_ICON_FILM] = {
        { S_RECT, 6, 7, 1, 0, {4, 9, 44, 39} },
        { S_RECT, 3, 3, 1, 0, {7, 12, 10, 15} },
        { S_RECT, 3, 3, 1, 0, {7, 33, 10, 36} },
        { S_RECT, 3, 3, 1, 0, {38, 12, 41, 15} },
        { S_POLY, 3, 7, 1, 3, {19, 16, 31, 24, 19, 32} }, END },
    [OP_ICON_FILE] = {
        { S_POLY, 3, 9, 1, 5, {10, 4, 30, 4, 38, 12, 38, 44, 10, 44} },
        { S_LINE, 0, 9, 1, 3, {30, 4, 30, 12, 38, 12} },
        { S_LINE, 0, 8, 1, 2, {16, 20, 32, 20} },
        { S_LINE, 0, 8, 1, 2, {16, 27, 32, 27} }, END },
};

static int sc(int v, int size) { return (v * size + 24) / 48; }

static void line(struct RastPort *rp, LONG pen, const int *xy, int n, int thick)
{
    int t, i;
    ogt_set_apen(rp, pen);
    for (t = 0; t < thick; t++) {
        int ox = t & 1, oy = (t >> 1) & 1;
        Move(rp, xy[0] + ox, xy[1] + oy);
        for (i = 1; i < n; i++) Draw(rp, xy[2 * i] + ox, xy[2 * i + 1] + oy);
    }
}

void op_icon_draw(ogt_ctx *c, struct RastPort *rp, int icon, int x, int y, int size, int disabled)
{
    const shape *s;
    LONG faded_fill = 0, faded_line = 0;
    if (icon <= OP_ICON_NONE || icon >= OP_ICON_COUNT) return;
    if (disabled) {
        faded_fill = ogt_pen(c, "track");
        faded_line = ogt_pen(c, "muted");
    }
    for (s = icons[icon]; s->kind != S_END; s++) {
        LONG fill = disabled ? faded_fill : (s->fill ? ogt_pen_rgb(c, colours[s->fill]) : -1);
        LONG edge = disabled ? faded_line : ogt_pen_rgb(c, colours[s->line]);
        int pts[24], i, thick = s->thick * size / 24;
        if (thick < 1) thick = 1;
        if (thick > 3) thick = 3;
        switch (s->kind) {
        case S_POLY:
            for (i = 0; i < s->n; i++) { pts[2 * i] = x + sc(s->p[2 * i], size); pts[2 * i + 1] = y + sc(s->p[2 * i + 1], size); }
            if (fill >= 0) ogt_fill_poly(rp, fill, pts, s->n);
            ogt_poly_outline(rp, edge, pts, s->n);
            break;
        case S_LINE:
            for (i = 0; i < s->n; i++) { pts[2 * i] = x + sc(s->p[2 * i], size); pts[2 * i + 1] = y + sc(s->p[2 * i + 1], size); }
            line(rp, edge, pts, s->n, thick);
            break;
        case S_RECT: {
            int x0 = x + sc(s->p[0], size), y0 = y + sc(s->p[1], size), x1 = x + sc(s->p[2], size), y1 = y + sc(s->p[3], size);
            if (fill >= 0) ogt_box(rp, fill, x0, y0, x1 - x0 + 1, y1 - y0 + 1);
            ogt_frame(rp, edge, x0, y0, x1 - x0 + 1, y1 - y0 + 1);
            break;
        }
        case S_CIRCLE: {
            int cx = x + sc(s->p[0], size), cy = y + sc(s->p[1], size), r = sc(s->p[2], size);
            ogt_fill_circle(rp, edge, cx, cy, r);
            if (fill >= 0 && r > 1) ogt_fill_circle(rp, fill, cx, cy, r - 1);
            break;
        }
        }
    }
}
