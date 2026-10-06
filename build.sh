#!/bin/sh
# OpenPlay for OS 3.2 with the os32 stove (m68k-amigaos-gcc, NDK 3.2), and
# OpenGadTools' sources beside it. 68020 and up, integer maths only.
#   ./build.sh [OUT_DIR]           (default build/os3)
#   OGT=path/to/opengadtools ./build.sh
# MIT, Copyright (c) 2026 Dalsin Limited.
set -eu
HERE=$(cd "$(dirname "$0")" && pwd)
STOVE=${STOVE:-$HOME/AmigaChrome/stoves/os32}
OGT=${OGT:-$HERE/../opengadtools}
CC="$STOVE/prefix/bin/m68k-amigaos-gcc"
OUT=${1:-$HERE/build/os3}
mkdir -p "$OUT"
FLAGS="-noixemul -m68020 -std=gnu99 -Wall -Werror -O2 -fno-common -I$HERE/app -I$OGT/lib"
# shellcheck disable=SC2086
"$CC" $FLAGS -o "$OUT/OpenPlay" \
    "$HERE/app/openplay.c" "$HERE/app/op_stack.c" \
    "$OGT/lib/ogt_theme.c" "$OGT/lib/ogt_draw.c" "$OGT/lib/ogt_icons.c" "$OGT/lib/ogt_list.c" \
    "$OGT/lib/ogt_toolbar.c" "$OGT/lib/ogt_font.c"
echo "$OUT/OpenPlay ($(wc -c < "$OUT/OpenPlay") bytes)"
