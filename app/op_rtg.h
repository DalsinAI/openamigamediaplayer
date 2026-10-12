/* The parts of openrtg.library OpenPlay uses for films that fit the window
 * (0.1.6): what a bitmap is (ORTG_BitMapInfo) and pixels written at another
 * size (ORTG_WritePixels with dest_width and dest_height), which openrtg.library
 * 0.16 draws with OpenGPU's COMPOSITE, on the host on AmigaChrome. The same
 * layouts and LVOs as openamigartg's include/openrtg/openrtg.h and
 * include/inline/openrtg.h.
 * MIT, Copyright (c) 2026 Dalsin Limited. */
#ifndef OP_RTG_H
#define OP_RTG_H

#include <exec/types.h>
#include <graphics/rastport.h>
#include <inline/macros.h>

#define OP_RTG_NAME "openrtg.library"
#define OP_RTG_SCALE_REVISION 16        /* 0.16: pixels at another size through OpenGPU */

struct OpenRTGPixels {
    void                *data;              /* row 0 of the memory */
    long                 x, y;              /* the rectangle's corner in it */
    long                 modulo;            /* bytes a row */
    unsigned long        format;            /* ORTG_PIX_ */
    const unsigned long *ctable;
    long                 width, height;     /* the rectangle */
    long                 dest_width, dest_height;   /* written at this size (0: the same) */
};

struct OpenRTGBitMapInfo {
    void          *memory;          /* pixel 0,0 */
    unsigned long  bytes_per_row;
    unsigned short width, height;
    unsigned char  depth;           /* 8, 16 or 32 */
    unsigned char  format;
    unsigned char  monitor;         /* 1-4: in that board's video RAM; 0: fast RAM */
    unsigned char  pad;
};

#define ORTG_PIX_RAW      5UL       /* the bitmap's own pixels */
#define ORTG_PIX_0RGB     114UL     /* 4 bytes: 0, R, G, B */

#define OP_ORTG_WritePixels(base, rp, x, y, pixels) \
    LP4(0x4e, LONG, ORTG_WritePixels, struct RastPort *, (rp), a1, LONG, (x), d0, LONG, (y), d1, \
        const struct OpenRTGPixels *, (pixels), a0, , (base))
#define OP_ORTG_BestMode(base, monitor, width, height, depth) \
    LP4(0x30, ULONG, ORTG_BestMode, ULONG, (monitor), d0, ULONG, (width), d1, ULONG, (height), d2, ULONG, (depth), d3, , (base))
#define OP_ORTG_BitMapInfo(base, bitmap, info) \
    LP2(0x66, BOOL, ORTG_BitMapInfo, struct BitMap *, (bitmap), a0, struct OpenRTGBitMapInfo *, (info), a1, , (base))

#endif
