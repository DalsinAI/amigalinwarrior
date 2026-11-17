#!/bin/sh
# The release archive and the GFOpenUp part, for Cradle's stove_build
# (target package):
#  1. upstream LinWarrior at the commit package/linwarrior.lock names (not
#     kept in this repository), with amiga/patches applied; Expat from the
#     release GFOpenXML pins, built with GFOpenXML's 68k config;
#  2. the game built by amiga/Makefile.amigaos;
#  3. the drawer, its icon and the Installer script beside it packed as
#     NAME-VERSION-68040.lha (tools/pack_dist.py), with its .sha256 and a
#     short .readme;
#  4. OUT/Parts/LinWarrior, a GFOpenUp part: its Part file and the drawer's
#     files for SYS:Games/LinWarrior, installed with
#       GFOpenUpTool INSTALL Parts/LinWarrior
#     It needs the GFOpenGPU part (SDL2.module, GL.module).
# Nothing is published from here.
# MIT, Copyright (c) 2026 AmiCradle Team.
set -eu

HERE=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
ROOT=$(CDPATH= cd -- "$HERE/.." && pwd)
. "$HERE/package.env"
. "$HERE/linwarrior.lock"
OUT=${OUT:-"$ROOT/build-amiga-package"}
mkdir -p "$OUT"
OUT=$(CDPATH= cd -- "$OUT" && pwd)
STOVE=${STOVE:-"$HOME/AmiCradle/stoves/os32-gcc16-open6"}
# outside the checkout: Cradle cleans it (git clean -ffdx) before every job
WORK=${WORK:-"$HOME/.cache/amigalinwarrior"}
mkdir -p "$WORK"
cd "$ROOT"

fetch() { # dir url commit
    if [ "$(git -C "$1" rev-parse HEAD 2>/dev/null)" != "$3" ]; then
        rm -rf "$1"; mkdir -p "$1"
        git -C "$1" init -q
        git -C "$1" fetch -q --depth 1 "$2" "$3"
        git -C "$1" checkout -q FETCH_HEAD
    fi
    git -C "$1" checkout -q -- . && git -C "$1" clean -qfdx
}

# a stove's compiler is in its prefix/, or STOVE is the prefix
if [ -x "$STOVE/prefix/bin/m68k-amigaos-gcc" ]; then PREFIX="$STOVE/prefix"; else PREFIX="$STOVE"; fi

# 1. upstream, patched afresh every time; GFOpenXML's Expat
L="$WORK/linwarrior"; X="$WORK/gfopenxml"
fetch "$L" "$LW_URL" "$LW_COMMIT"
for p in "$ROOT"/amiga/patches/*.patch; do
    git -C "$L" apply "$p"
done
fetch "$X" "$GFOPENXML_URL" "$GFOPENXML_COMMIT"
T="$WORK/$(basename "$EXPAT_URL")"
if ! echo "$EXPAT_SHA256  $T" | sha256sum -c - >/dev/null 2>&1; then
    curl -sSfL -o "$T" "$EXPAT_URL"
    echo "$EXPAT_SHA256  $T" | sha256sum -c - >/dev/null || { echo "Expat: SHA-256 mismatch"; exit 1; }
fi
E="$WORK/expat"
rm -rf "$E"; mkdir -p "$E"
tar xjf "$T" -C "$E" --strip-components=1

# 2. the game
make -f amiga/Makefile.amigaos LW="$L" EXPAT="$E" EXPATCFG="$X/config/expat" STOVE="$STOVE" PREFIX="$PREFIX" \
    -j4 $TARGET > "$OUT/build-output.txt" 2>&1 || { tail -n 60 "$OUT/build-output.txt"; exit 1; }

# 3. the archive
python3 "$ROOT/tools/pack_dist.py" --dist "$DIST" --name "$NAME" --out "$OUT"

# 4. the GFOpenUp part
sh "$ROOT/package/write-part.sh" "$DIST/$NAME" "$OUT/Parts/$NAME" "$NAME" "$PORT_VERSION" \
    "LinWarrior 3D: mech combat across a procedural landscape and city"
echo "part: $(grep -c '^copy' "$OUT/Parts/$NAME/Part") files, $(du -sk "$OUT/Parts/$NAME" | cut -f1) KB"
ls -l "$DIST/$NAME/$NAME"
