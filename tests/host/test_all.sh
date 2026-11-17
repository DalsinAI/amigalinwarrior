#!/bin/sh
# Host tests of the LinWarrior port. Needs a C compiler; for the 68k half,
# m68k-linux-gnu-gcc and qemu-m68k (skipped, and said so, without them).
#   sh tests/host/test_all.sh [WORKDIR]
# 1. amiga/lw_al_core.c (the sound under the OpenAL calls): its checks, and
#    the game's own WAVs read and mixed, on the PC and on 68k under
#    qemu-m68k -cpu m68040; the two must print the same.
# 2. tests/host/test_*.py.
# MIT, Copyright (c) 2026 AmiCradle Team.
set -eu
HERE=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
ROOT=$(CDPATH= cd -- "$HERE/../.." && pwd)
. "$ROOT/package/linwarrior.lock"
W=${1:-"${TMPDIR:-/tmp}/amigalinwarrior-test"}
mkdir -p "$W"
L="$W/linwarrior"
if [ "$(git -C "$L" rev-parse HEAD 2>/dev/null)" != "$LW_COMMIT" ]; then
    rm -rf "$L"; mkdir -p "$L"; git -C "$L" init -q
    git -C "$L" fetch -q --depth 1 "$LW_URL" "$LW_COMMIT"
    git -C "$L" checkout -q FETCH_HEAD
fi
WAVS=$(find "$L/data" -name '*.wav' | sort)

cc -O2 -Wall -I"$ROOT/amiga" -o "$W/test_lw_al-pc" "$HERE/test_lw_al.c" "$ROOT/amiga/lw_al_core.c" -lm
"$W/test_lw_al-pc" $WAVS > "$W/lw_al-pc.txt" || { cat "$W/lw_al-pc.txt"; exit 1; }
grep -E "^(PASS|FAIL):" "$W/lw_al-pc.txt"
if command -v m68k-linux-gnu-gcc >/dev/null && command -v qemu-m68k >/dev/null; then
    m68k-linux-gnu-gcc -m68040 -O2 -static -I"$ROOT/amiga" -o "$W/test_lw_al-68k" \
        "$HERE/test_lw_al.c" "$ROOT/amiga/lw_al_core.c" -lm
    qemu-m68k -cpu m68040 "$W/test_lw_al-68k" $WAVS > "$W/lw_al-68k.txt" || { cat "$W/lw_al-68k.txt"; exit 1; }
    if diff -u "$W/lw_al-pc.txt" "$W/lw_al-68k.txt"; then
        echo "PASS sound: PC and 68k the same ($(grep -c '^WAV' "$W/lw_al-pc.txt") WAVs, $(grep -c '^MIX' "$W/lw_al-pc.txt") seconds mixed)"
    else
        echo "FAIL sound: PC and 68k differ"; exit 1
    fi
else
    echo "SKIP sound on 68k: no m68k-linux-gnu-gcc or qemu-m68k"
fi

for t in "$HERE"/test_*.py; do
    python3 "$t"
done
echo "ALL PASS"
