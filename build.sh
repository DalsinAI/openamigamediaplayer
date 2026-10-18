#!/bin/sh
# OpenPlay for OS 3.2 with the os32-gcc16 stove (m68k-amigaos-gcc 16, NDK 3.2), and
# the OpenGadTools library copied into third_party/opengadtools (its FROM
# says which commit). Built for the 68040, integer maths only.
#   ./build.sh [OUT_DIR]           (default build/os3)
# MIT, Copyright (c) 2026 AmigaChrome Team.
# The compiler: OS32_GCC16, else STOVE, else ~/AmigaChrome/stoves/os32-gcc16. It stops when given an older GCC
# (the os32 stove's GCC 6.5.0b merges a word clear and the long clear after it into one clr.l at -O2) and builds
# -fno-tree-loop-distribute-patterns (GCC 16.2.0b's loop-shift trap).
set -eu
HERE=$(cd "$(dirname "$0")" && pwd)
STOVE=${OS32_GCC16:-${STOVE:-$HOME/AmigaChrome/stoves/os32-gcc16}}
OGT="$HERE/third_party/opengadtools"
CC="$STOVE/prefix/bin/m68k-amigaos-gcc"
GCCV=$("$CC" -dumpversion 2>/dev/null || echo 0)
[ "${GCCV%%.*}" -ge 16 ] 2>/dev/null || { echo "$CC is GCC $GCCV: this needs GCC 16 (the os32-gcc16 stove; set OS32_GCC16)"; exit 2; }
OUT=${1:-$HERE/build/os3}
mkdir -p "$OUT"
# Soft float: -m68040 links libnix's soft libm020, not libm881
# (specs/m68k-soft-m68040.specs: -m68040 alone would link the FPU libm).
SOFT="-specs=$HERE/specs/m68k-soft-m68040.specs"
FLAGS="-noixemul -m68040 $SOFT -std=gnu99 -Wall -Werror -O2 -fno-tree-loop-distribute-patterns -fno-delete-null-pointer-checks -fno-common -I$HERE/app -I$OGT"
# shellcheck disable=SC2086
"$CC" $FLAGS -o "$OUT/OpenPlay" \
    "$HERE/app/openplay.c" "$HERE/app/op_icons.c" "$HERE/app/op_stack.c" \
    "$OGT/ogt_theme.c" "$OGT/ogt_draw.c" "$OGT/ogt_icons.c" "$OGT/ogt_list.c" \
    "$OGT/ogt_toolbar.c" "$OGT/ogt_font.c"
echo "$OUT/OpenPlay ($(wc -c < "$OUT/OpenPlay") bytes)"
