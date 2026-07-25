#!/usr/bin/env bash
#
# verify-flatpak-hc55.sh — post-build release-gate voor de Flatpak-bundle.
#
# Draait ON HC55 (of elke Linux-host met flatpak + Xvfb) en verifieert het
# gebouwde distributie-artefact zelf, niet de dev-build. Aanleiding: BUG-016 —
# "Mac smoke groen" was tests-only, de app zelf startte sinds v0.2.0 nooit; dat
# kwam pas op de Deck aan het licht. Sindsdien is een app-launch onderdeel van
# de gate (BUGLIST BUG-016, architectonische les).
#
# Usage:
#   ./verify-flatpak-hc55.sh <pad-naar-bundle.flatpak> [rom-pad]
#
# Vier stappen, elk fataal:
#   1. install   — bundle installeert user-scope (vangt runtime-metadata, BUG-015)
#   2. launch    — Qt/QML-app start offscreen zonder output (vangt BUG-016)
#   3. cbios     — openMSX boot C-BIOS in de sandbox (vangt BUG-012/BUG-004)
#   4. cartridge — echte ROM start tot Konami-logo (end-to-end emulatie)
#
# Screenshots landen in ./verify-out/ voor visuele controle.

set -uo pipefail

BUNDLE="${1:-}"
ROM="${2:-/var/lib/steamdeckmsx/roms/nemesis2.rom}"
APP_ID="nl.icthorse.SteamDeckMSX"
DISPLAY_NR=":99"
OUTDIR="$(pwd)/verify-out"

if [[ -z "$BUNDLE" || ! -f "$BUNDLE" ]]; then
    echo "Usage: $0 <bundle.flatpak> [rom]" >&2
    exit 64
fi

mkdir -p "$OUTDIR"
fail() { echo "GATE FAILED: $*" >&2; exit 1; }

# Altijd eerst verwijderen: flatpak weigert over een bestaande install heen te
# installeren ("already installed"), en dan zou de gate stilzwijgend het VORIGE
# artefact testen in plaats van het zojuist gebouwde.
echo "== 1/4 install =="
if flatpak list --user --columns=application 2>/dev/null | grep -qx "$APP_ID"; then
    echo "   bestaande install gevonden -> verwijderen"
    flatpak uninstall --user -y "$APP_ID" >/dev/null 2>&1 \
        || fail "kon bestaande install niet verwijderen"
fi
flatpak install --user -y --noninteractive "$BUNDLE" >/dev/null 2>&1 \
    || fail "flatpak install"
echo "   ok"

# Offscreen: geen display nodig, en een kapotte QML-boom logt hier hard.
# Timeout 20 => exit 124 is het GOEDE resultaat (app bleef draaien).
echo "== 2/4 app-launch (offscreen) =="
launch_log=$(mktemp)
timeout 20 flatpak run --user --env=QT_QPA_PLATFORM=offscreen "$APP_ID" \
    >"$launch_log" 2>&1
ec=$?
[[ $ec -eq 124 ]] || fail "app stopte vroegtijdig (exit $ec): $(head -5 "$launch_log")"
[[ -s "$launch_log" ]] && fail "app logde warnings/errors: $(head -5 "$launch_log")"
echo "   ok (20s stabiel, geen output)"
rm -f "$launch_log"

# Vanaf hier een echt X-display: openMSX rendert via SDL.
# :0 is van Sunshine (Stream_Server) — bewust een eigen display.
pgrep -f "Xvfb $DISPLAY_NR" >/dev/null || {
    Xvfb "$DISPLAY_NR" -screen 0 640x480x24 -nolisten tcp >/dev/null 2>&1 &
    sleep 2
}

echo "== 3/4 C-BIOS boot in sandbox =="
rm -f /tmp/vfy_cbios*.png
DISPLAY="$DISPLAY_NR" timeout 40 flatpak run --user --command=openmsx \
    --filesystem=/tmp --env=DISPLAY="$DISPLAY_NR" --env=SDL_AUDIODRIVER=dummy \
    "$APP_ID" -machine C-BIOS_MSX2+ \
    -command "after time 6 {screenshot /tmp/vfy_cbios.png ; quit}" >/dev/null 2>&1
[[ -f /tmp/vfy_cbios.png ]] || fail "C-BIOS boot leverde geen screenshot"
cp /tmp/vfy_cbios.png "$OUTDIR/01-cbios-boot.png"
echo "   ok -> $OUTDIR/01-cbios-boot.png"

echo "== 4/4 cartridge end-to-end =="
if [[ ! -f "$ROM" ]]; then
    echo "   OVERGESLAAN — geen ROM op $ROM (niet in repo: P-SDM-05)"
    exit 0
fi
rm -f /tmp/vfy_cart*.png
romdir="$(dirname "$ROM")"
DISPLAY="$DISPLAY_NR" timeout 60 flatpak run --user --command=openmsx \
    --filesystem=/tmp --filesystem="$romdir:ro" --env=DISPLAY="$DISPLAY_NR" \
    --env=SDL_AUDIODRIVER=dummy "$APP_ID" -machine C-BIOS_MSX2+ -carta "$ROM" \
    -command "after time 7 {screenshot /tmp/vfy_cart_logo.png}; after time 18 {screenshot /tmp/vfy_cart_run.png}; after time 20 quit" \
    >/dev/null 2>&1
[[ -f /tmp/vfy_cart_logo.png ]] || fail "cartridge leverde geen screenshot"
cp /tmp/vfy_cart_logo.png "$OUTDIR/02-cartridge-logo.png"
[[ -f /tmp/vfy_cart_run.png ]] && cp /tmp/vfy_cart_run.png "$OUTDIR/03-cartridge-running.png"
echo "   ok -> $OUTDIR/02-cartridge-logo.png"

echo
echo "ALLE GATES GROEN — bundle is release-waardig."
echo "Controleer de screenshots in $OUTDIR visueel (logo/beeld daadwerkelijk zichtbaar)."
