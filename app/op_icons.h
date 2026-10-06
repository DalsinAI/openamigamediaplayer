/* OpenPlay's player icons, drawn from shapes on a 48 x 48 grid in the
 * GlowIcons style of OpenGadTools' own (ogt_icons). They stay here until
 * OpenGadTools has player icons; then OpenPlay uses those.
 * MIT, Copyright (c) 2026 Dalsin Limited. */
#ifndef OP_ICONS_H
#define OP_ICONS_H

#include "ogt_draw.h"

enum {
    OP_ICON_NONE = 0,
    OP_ICON_OPEN, OP_ICON_PLAY, OP_ICON_PAUSE, OP_ICON_STOP, OP_ICON_PREV, OP_ICON_NEXT, OP_ICON_REPEAT,
    OP_ICON_FULLSCREEN, OP_ICON_PLAYLIST, OP_ICON_INFO, OP_ICON_CONVERT, OP_ICON_SLIDESHOW,
    OP_ICON_SOUND, OP_ICON_FILM, OP_ICON_FILE,
    OP_ICON_COUNT
};

/* Draws an icon in a size x size box at x,y. disabled: drawn faded. */
void op_icon_draw(ogt_ctx *c, struct RastPort *rp, int icon, int x, int y, int size, int disabled);

#endif
