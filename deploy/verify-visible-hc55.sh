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

# Een venster kan schermvullend én zwart zijn; tel wat er werkelijk oplicht.
bright_pixels() {
    ffmpeg -loglevel error -i "$1" \
        -vf "format=gray,geq=lum='if(gt(lum(X,Y),40),255,0)'" \
        -f rawvideo - 2>/dev/null | tr -d '\0' | wc -c | tr -d ' '
}

# Geometrie + map-state van het grootste venster waarvan de naam matcht.
# Grootste, niet eerste: zowel Qt als openbox houden 1x1 hulpvensters aan met
# dezelfde naam, en die rapporteren altijd IsUnMapped.
win_state() {
    local pattern="$1" id best_w=0 best_h=0 best_st="AFWEZIG"
    # Via xdotool en niet via `xwininfo -root -children`: een window manager
    # hangt het app-venster in een naamloos frame, waardoor het geen directe
    # child van de root meer is en die lijst het niet vindt.
    for id in $(timeout 10 env DISPLAY="$DISPLAY_NR" xdotool search --name "$pattern" 2>/dev/null); do
        local info; info=$(timeout 5 env DISPLAY="$DISPLAY_NR" xwininfo -id "$id" 2>/dev/null)
        [[ -z "$info" ]] && continue
        local w h st
        w=$(awk '/Width:/ {print $2}' <<<"$info")
        h=$(awk '/Height:/ {print $2}' <<<"$info")
        st=$(awk '/Map State:/ {print $3}' <<<"$info")
        [[ -z "$w" || -z "$h" ]] && continue
        if (( w * h > best_w * best_h )); then
            best_w=$w; best_h=$h; best_st=$st
        fi
    done
    [[ "$best_st" == "AFWEZIG" ]] && { echo "- - AFWEZIG"; return 0; }
    echo "$best_w $best_h $best_st"
}

# QT_QUICK_BACKEND=software: Xvfb heeft geen GPU, en zonder deze regel tekent
# Qt Quick een volledig zwart venster. Dat is een eigenschap van deze
# testomgeving, niet van de app — de Deck heeft wel een GPU.
echo "== app starten =="
DISPLAY="$DISPLAY_NR" flatpak run --user --env=QT_QUICK_BACKEND=software \
    --env=DISPLAY="$DISPLAY_NR" "$APP_ID" >"$OUTDIR/app.log" 2>&1 &
APP_PID=$!
sleep 30                      # ruim: eerste start scant de ROM-mappen
shoot 01-galerij.png
read -r gw gh gs <<<"$(win_state 'SteamDeckMSX')"
echo "   galerij: ${gw}x${gh} $gs"
[[ "$gs" == "IsViewable" ]] || fail "galerij niet zichtbaar bij start ($gs)"

# Zonder de galerij zelf te zien weet je niet of er tegels staan; een zwart
# venster levert straks een Enter die nergens op slaat.
gal_light=$(bright_pixels "$OUTDIR/01-galerij.png")
echo "   galerij-inhoud: $gal_light heldere pixels"
[[ "$gal_light" -gt 1000 ]] || fail "galerij is leeg/zwart ($gal_light) — Enter zou nergens op slaan"

# Geen --sync: dat blokkeert onbeperkt als het venster niet activeerbaar is.
echo "== spel starten (Enter, zoals de speler doet) =="
timeout 10 env DISPLAY="$DISPLAY_NR" xdotool search --name "SteamDeckMSX" windowactivate 2>/dev/null
sleep 2
timeout 10 env DISPLAY="$DISPLAY_NR" xdotool key --clearmodifiers Return
sleep 30                      # boot + C-BIOS + titelscherm
shoot 02-spel.png

read -r gw2 gh2 gs2 <<<"$(win_state 'SteamDeckMSX')"
echo "   galerij:  ${gw2}x${gh2} $gs2"
[[ "$gs2" != "IsViewable" ]] || fail "galerij hangt nog vóór de emulator — BUG-022"

# Bewust géén assert op een venster met de naam "openMSX": versie 21 tekent via
# SDL/ImGui en dat venster is niet op naam terug te vinden in de vensterboom.
# Wat telt is toch wat de speler ziet, en dat is het scherm zelf — de galerij is
# hierboven al aantoonbaar weg, dus alles wat oplicht komt van de emulator.
nonblack=$(bright_pixels "$OUTDIR/02-spel.png")
echo "   beeldvulling: $nonblack heldere pixels"
[[ "$nonblack" -gt 20000 ]] || fail "scherm is (vrijwel) zwart ($nonblack heldere pixels) — er draait wel iets, maar er is niets te zien"

echo "== terug naar de galerij (F12) =="
# Naar het actieve venster: dat is de emulator, want de galerij is unmapped.
timeout 10 env DISPLAY="$DISPLAY_NR" xdotool key --clearmodifiers F12
sleep 8
shoot 03-terug.png
read -r gw3 gh3 gs3 <<<"$(win_state 'SteamDeckMSX')"
echo "   galerij: ${gw3}x${gh3} $gs3"
[[ "$gs3" == "IsViewable" ]] || fail "galerij komt niet terug na F12 — speler zit vast in de emulator"

# BUG-024: openMSX bewaart SRAM, settings en save-states in zijn user-dir. De
# home is read-only, dus wijst de app OPENMSX_HOME naar de eigen app-map. Dit
# hoort hier en niet in een los script: alleen ná een echte spelsessie via de
# app staat vast dat de app die variabele ook werkelijk meegeeft. Een losse
# openmsx-aanroep zet hem niet en bewijst dus niets (zie BUG-022, dezelfde val).
echo "== openMSX kon schrijven =="
if grep -qi "read-only file system" "$OUTDIR/app.log"; then
    grep -i "read-only" "$OUTDIR/app.log" | head -2
    fail "openMSX kon niet schrijven — SRAM en save-states gaan verloren (BUG-024)"
fi
userdir=$(find "$HOME/.var/app/$APP_ID" -type d -name openmsx 2>/dev/null | head -1)
[[ -n "$userdir" ]] || fail "openMSX kreeg geen eigen map onder ~/.var/app/$APP_ID"
written=$(find "$userdir" -type f 2>/dev/null | wc -l | tr -d ' ')
echo "   $userdir ($written bestanden)"
[[ "$written" -gt 0 ]] || fail "map bestaat maar openMSX schreef er niets in"

echo
echo "ZICHTBAARHEIDS-GATE GROEN"
echo "  01-galerij.png  — galerij bij start"
echo "  02-spel.png     — spel schermvullend, galerij weg"
echo "  03-terug.png    — galerij terug na F12"
echo "Bekijk de drie screenshots in $OUTDIR ook met eigen ogen."
