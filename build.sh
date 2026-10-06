#!/bin/sh
# OpenPlay for OS 3.2 with the os32 stove (m68k-amigaos-gcc, NDK 3.2), and
# the OpenGadTools library copied into third_party/opengadtools (its FROM
# says which commit). 68020 and up, integer maths only.
#   ./build.sh [OUT_DIR]           (default build/os3)
# MIT, Copyright (c) 2026 Dalsin Limited.
set -eu
HERE=$(cd "$(dirname "$0")" && pwd)
STOVE=${STOVE:-$HOME/AmigaChrome/stoves/os32}
OGT="$HERE/third_party/opengadtools"
CC="$STOVE/prefix/bin/m68k-amigaos-gcc"
OUT=${1:-$HERE/build/os3}
mkdir -p "$OUT"
FLAGS="-noixemul -m68020 -std=gnu99 -Wall -Werror -O2 -fno-common -I$HERE/app -I$OGT"
# shellcheck disable=SC2086
"$CC" $FLAGS -o "$OUT/OpenPlay" \
    "$HERE/app/openplay.c" "$HERE/app/op_icons.c" "$HERE/app/op_stack.c" \
    "$OGT/ogt_theme.c" "$OGT/ogt_draw.c" "$OGT/ogt_icons.c" "$OGT/ogt_list.c" \
    "$OGT/ogt_toolbar.c" "$OGT/ogt_font.c"
echo "$OUT/OpenPlay ($(wc -c < "$OUT/OpenPlay") bytes)"
