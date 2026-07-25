#!/usr/bin/env bash
#
# verify-visible-hc55.sh — release-gate die aantoont dat de speler het spel ZIET.
#
# Aanleiding: BUG-022. De vier gates in verify-flatpak-hc55.sh draaiden headless
# en toetsten of openMSX een screenshot produceerde — precies de route die ook
# werkt als er geen zichtbaar venster is. Ze bewezen dus dat de emulator draait,
# niet dat de gebruiker hem ziet. Op de Deck bleef het beeld daardoor zwart.
#
# Deze gate draait daarom mét een window manager: zonder WM honoreert niemand
# het fullscreen-verzoek van SDL en meet je een omgeving die de gebruiker nooit
# krijgt (zelfde fout als de handmatige --filesystem-rechten in BUG-017).
#
# Vereist op de host: flatpak, Xvfb, een window manager (openbox), xdotool,
# ffmpeg. Ontbreekt er een, dan faalt de gate — overslaan is hoe BUG-022 door
# vier gates heen kwam.
#
# Usage:
#   ./verify-visible-hc55.sh [bundle.flatpak]
#
# Zonder bundle-argument wordt de reeds geïnstalleerde app getest.

set -uo pipefail

BUNDLE="${1:-}"
APP_ID="nl.icthorse.SteamDeckMSX"
DISPLAY_NR=":98"          # :0 = Sunshine, :99 = de headless gate
SCREEN_W=1280
SCREEN_H=800
OUTDIR="$(pwd)/verify-out-visible"

fail() { echo "GATE FAILED: $*" >&2; cleanup; exit 1; }

cleanup() {
    [[ -n "${APP_PID:-}" ]] && kill "$APP_PID" 2>/dev/null
    pkill -f "openmsx" 2>/dev/null
    pkill -f "openbox --replace" 2>/dev/null
    pkill -f "Xvfb $DISPLAY_NR" 2>/dev/null
    return 0
}
trap cleanup EXIT

for tool in flatpak Xvfb openbox xdotool ffmpeg xwininfo; do
    command -v "$tool" >/dev/null || fail "$tool ontbreekt — zie kop van dit script"
done

mkdir -p "$OUTDIR"
rm -f "$OUTDIR"/*.png

if [[ -n "$BUNDLE" ]]; then
    [[ -f "$BUNDLE" ]] || fail "bundle niet gevonden: $BUNDLE"
    echo "== install =="
    flatpak list --user --columns=application 2>/dev/null | grep -qx "$APP_ID" && \
        flatpak uninstall --user -y "$APP_ID" >/dev/null 2>&1
    flatpak install --user -y --noninteractive "$BUNDLE" >/dev/null 2>&1 \
        || fail "flatpak install"
    echo "   ok"
fi

echo "== scherm + window manager =="
pkill -f "Xvfb $DISPLAY_NR" 2>/dev/null; sleep 1
Xvfb "$DISPLAY_NR" -screen 0 "${SCREEN_W}x${SCREEN_H}x24" -nolisten tcp >/dev/null 2>&1 &
for i in $(seq 20); do
    DISPLAY="$DISPLAY_NR" xdpyinfo >/dev/null 2>&1 && break
    sleep 0.5
done
DISPLAY="$DISPLAY_NR" xdpyinfo >/dev/null 2>&1 || fail "Xvfb kwam niet op"
DISPLAY="$DISPLAY_NR" openbox >/dev/null 2>&1 &
sleep 2
echo "   ok (${SCREEN_W}x${SCREEN_H} + openbox)"

shoot() { ffmpeg -loglevel error -f x11grab -video_size "${SCREEN_W}x${SCREEN_H}" \
              -i "$DISPLAY_NR" -frames:v 1 -y "$OUTDIR/$1" 2>/dev/null; }

# Geometrie + map-state van het eerste venster waarvan de naam matcht.
win_state() {
    local pattern="$1" id
    for id in $(DISPLAY="$DISPLAY_NR" xwininfo -root -children 2>/dev/null | awk '/0x/ {print $1}'); do
        local info; info=$(DISPLAY="$DISPLAY_NR" xwininfo -id "$id" 2>/dev/null)
        if grep -q "$pattern" <<<"$info"; then
            local w h st
            w=$(awk '/Width:/ {print $2}' <<<"$info")
            h=$(awk '/Height:/ {print $2}' <<<"$info")
            st=$(awk '/Map State:/ {print $3}' <<<"$info")
            echo "$w $h $st"
            return 0
        fi
    done
    echo "- - AFWEZIG"
}

echo "== app starten =="
DISPLAY="$DISPLAY_NR" flatpak run --user "$APP_ID" >"$OUTDIR/app.log" 2>&1 &
APP_PID=$!
sleep 25                      # ruim: eerste start scant de ROM-mappen
shoot 01-galerij.png
read -r gw gh gs <<<"$(win_state 'SteamDeckMSX')"
echo "   galerij: ${gw}x${gh} $gs"
[[ "$gs" == "IsViewable" ]] || fail "galerij niet zichtbaar bij start ($gs)"

echo "== spel starten (Enter, zoals de speler doet) =="
DISPLAY="$DISPLAY_NR" xdotool search --name "SteamDeckMSX" windowactivate --sync 2>/dev/null
sleep 1
DISPLAY="$DISPLAY_NR" xdotool key Return
sleep 25                      # boot + C-BIOS + titelscherm
shoot 02-spel.png

read -r ew eh es <<<"$(win_state 'openMSX')"
read -r gw2 gh2 gs2 <<<"$(win_state 'SteamDeckMSX')"
echo "   emulator: ${ew}x${eh} $es"
echo "   galerij:  ${gw2}x${gh2} $gs2"

# Dit is de hele kwestie: de emulator moet het scherm vullen én de galerij mag
# er niet meer voor hangen.
[[ "$es" == "IsViewable" ]] || fail "emulatorvenster niet zichtbaar ($es)"
[[ "$ew" == "$SCREEN_W" && "$eh" == "$SCREEN_H" ]] \
    || fail "emulator vult het scherm niet (${ew}x${eh}, verwacht ${SCREEN_W}x${SCREEN_H})"
[[ "$gs2" != "IsViewable" ]] || fail "galerij hangt nog vóór de emulator — BUG-022"

# Een schermvullend venster kan nog steeds zwart zijn; tel de niet-zwarte pixels.
nonblack=$(ffmpeg -loglevel error -i "$OUTDIR/02-spel.png" -vf "format=gray,geq=lum='if(gt(lum(X,Y),40),255,0)'" \
    -f rawvideo - 2>/dev/null | tr -d '\0' | wc -c)
echo "   beeldvulling: $nonblack heldere pixels"
[[ "$nonblack" -gt 1000 ]] || fail "scherm is zwart ($nonblack heldere pixels) — er draait wel iets, maar er is niets te zien"

echo "== terug naar de galerij (F12) =="
DISPLAY="$DISPLAY_NR" xdotool search --name "openMSX" windowactivate --sync 2>/dev/null
sleep 1
DISPLAY="$DISPLAY_NR" xdotool key F12
sleep 8
shoot 03-terug.png
read -r gw3 gh3 gs3 <<<"$(win_state 'SteamDeckMSX')"
echo "   galerij: ${gw3}x${gh3} $gs3"
[[ "$gs3" == "IsViewable" ]] || fail "galerij komt niet terug na F12 — speler zit vast in de emulator"

echo
echo "ZICHTBAARHEIDS-GATE GROEN"
echo "  01-galerij.png  — galerij bij start"
echo "  02-spel.png     — spel schermvullend, galerij weg"
echo "  03-terug.png    — galerij terug na F12"
echo "Bekijk de drie screenshots in $OUTDIR ook met eigen ogen."
