#!/usr/bin/env bash
#
# wait-for-build.sh — wacht op de HC55-build en meldt het zodra er iets mis is.
#
# Aanleiding: twee keer bleef een wachtlus minutenlang stil hangen. Eén keer
# terecht (de build compileerde openMSX), één keer omdat het buildscript nooit
# was gestart — verkeerd pad, dus BUILD_DONE kwam nooit. Beide zien er van
# buiten identiek uit: stilte. Dat verschil hoort de wachtlus zelf te maken.
#
# Deze wacht controleert daarom niet alleen op "klaar", maar ook op "leeft het
# nog": draait er een flatpak-builder, en groeit het logbestand? Zo niet, dan
# stopt hij met een foutmelding in plaats van door te wachten.
#
# Usage: ./wait-for-build.sh [max-minuten]   (default 30)

set -uo pipefail

HOST="${SDMSX_HOST:-horsecloud55}"
BUILD_DIR=/root/sdmsx_build
MAX_MIN="${1:-30}"
deadline=$(( $(date +%s) + MAX_MIN * 60 ))
last_size=0
stale_ticks=0

echo "Wachten op build (max ${MAX_MIN} min)…"
while :; do
    if ssh "$HOST" "test -f $BUILD_DIR/BUILD_DONE" 2>/dev/null; then
        ssh "$HOST" "grep -E 'BUILD_EXIT|BUNDLE_OK|PUBLISHED' $BUILD_DIR/flatpak-build.log"
        exit 0
    fi

    now=$(date +%s)
    if (( now > deadline )); then
        echo "AFGEBROKEN: nog niet klaar na ${MAX_MIN} min — kijk zelf in $BUILD_DIR/flatpak-build.log" >&2
        exit 2
    fi

    # Leeft de build nog? Twee onafhankelijke signalen, want een proces kan
    # blijven staan zonder voortgang.
    running=$(ssh "$HOST" "pgrep -c -f 'flatpak-builder --force-clean' 2>/dev/null || echo 0")
    size=$(ssh "$HOST" "stat -c %s $BUILD_DIR/flatpak-build.log 2>/dev/null || echo 0")

    if [[ "$running" == "0" ]]; then
        echo "AFGEBROKEN: geen flatpak-builder-proces én geen BUILD_DONE." >&2
        echo "  Vrijwel altijd: het buildscript is nooit gestart (pad!) of het is gecrasht." >&2
        ssh "$HOST" "tail -5 $BUILD_DIR/nohup.log 2>/dev/null" >&2
        exit 3
    fi

    if (( size == last_size )); then
        stale_ticks=$(( stale_ticks + 1 ))
        if (( stale_ticks >= 10 )); then   # ~5 minuten zonder logregel
            echo "AFGEBROKEN: proces draait, maar het log groeit al 5 min niet — vastgelopen." >&2
            exit 4
        fi
    else
        stale_ticks=0
        last_size=$size
    fi

    sleep 30
done
