#!/usr/bin/env bash
#
# build-all.sh — van broncode naar een op de Deck installeerbare bundle.
#
# Eén ingang voor de hele keten, want die bestond tot nu toe als losse stappen
# die je in de juiste volgorde moest kennen: lokaal bouwen en testen, naar HC55
# synchroniseren, Flatpak bouwen en publiceren, en de gates draaien. Een stap
# overslaan leverde eerder een release op die er groen uitzag maar het niet was.
#
# Draaien vanaf de Mac, in de repo-root:
#   ./deploy/build-all.sh            volledige keten
#   ./deploy/build-all.sh --quick    alleen lokaal bouwen en testen
#
# Faalt bij de eerste stap die niet klopt; geen stille voortgang.

set -uo pipefail

HOST="${SDMSX_HOST:-horsecloud55}"
REMOTE=/root/sdmsx_build
QUICK=0
[[ "${1:-}" == "--quick" ]] && QUICK=1

cd "$(dirname "$0")/.." || exit 1
VERSION=$(tr -d '[:space:]' < VERSION)

step() { printf '\n== %s ==\n' "$1"; }
fail() { echo "AFGEBROKEN: $*" >&2; exit 1; }

step "1 versie-consistentie"
# BUG-008/BUG-020 kwamen allebei doordat VERSION en CMakeLists uit elkaar liepen
# en de app zich anders voorstelde dan de bundelnaam beloofde.
cmake_ver=$(grep -oE 'VERSION [0-9]+\.[0-9]+\.[0-9]+' CMakeLists.txt | head -1 | awk '{print $2}')
cmake_code=$(grep -oE 'STEAMDECKMSX_VERSION_CODENAME "[^"]+"' CMakeLists.txt | head -1 | sed 's/.*"\(.*\)"/\1/')
[[ "$VERSION" == "${cmake_ver}-${cmake_code}" ]] \
    || fail "VERSION ($VERSION) wijkt af van CMakeLists (${cmake_ver}-${cmake_code})"
# BUG-025: de openmsx-module bouwt sindsdien uit de git-fork, gepind op een
# commit in het manifest. Drift tussen die pin en de submodule zou stil een
# andere emulator bundelen dan de repo laat zien — daarom hier blokkerend.
sub_head=$(git -C externals/openmsx rev-parse HEAD)
manifest_commit=$(grep -oE 'commit: [0-9a-f]{40}' nl.icthorse.SteamDeckMSX.yaml | awk '{print $2}')
[[ "$sub_head" == "$manifest_commit" ]] \
    || fail "manifest pint openmsx-commit ${manifest_commit:-<geen>} maar submodule staat op $sub_head"
[[ -z "$(git -C externals/openmsx status --porcelain)" ]] \
    || fail "externals/openmsx heeft onvastgelegde wijzigingen — commit + push de fork eerst"
echo "   $VERSION (openmsx-pin ${manifest_commit:0:9} ok)"

step "2 lokaal bouwen"
cmake --preset native-debug >/dev/null 2>&1 || fail "cmake configure"
cmake --build build/native-debug -j8 >/tmp/sdmsx_build.log 2>&1 \
    || { tail -20 /tmp/sdmsx_build.log; fail "compileren"; }
echo "   ok"

step "3 tests"
(cd build/native-debug && ctest --output-on-failure >/tmp/sdmsx_test.log 2>&1) \
    || { tail -20 /tmp/sdmsx_test.log; fail "tests"; }
grep -E "tests passed" /tmp/sdmsx_test.log | tail -1 | sed 's/^/   /'

step "4 app-launch (offscreen)"
# BUG-016: een groene testsuite zei niets over de app zelf; die startte een
# release lang niet. Sindsdien is een echte start onderdeel van de keten.
QT_QPA_PLATFORM=offscreen ./build/native-debug/bin/steamdeckmsx >/tmp/sdmsx_run.log 2>&1 &
pid=$!
sleep 12
if kill -0 $pid 2>/dev/null; then kill $pid 2>/dev/null; wait $pid 2>/dev/null; else fail "app stopte vroegtijdig: $(head -3 /tmp/sdmsx_run.log)"; fi
if grep -qiE "is not a type|Cannot assign|SyntaxError|Unable to assign|Could not attach" /tmp/sdmsx_run.log; then
    grep -iE "is not a type|Cannot assign|SyntaxError|Unable to assign|Could not attach" /tmp/sdmsx_run.log | head -3
    fail "QML-fouten bij het starten"
fi
echo "   ok (12s stabiel, geen QML-fouten)"

if [[ $QUICK -eq 1 ]]; then
    echo; echo "KLAAR (--quick: niet gebouwd voor de Deck)"; exit 0
fi

step "5 broncode naar $HOST"
# 'derived' (openMSX-buildoutput, 70MB) hoeft niet mee: de flatpak-build haalt
# de emulator sinds BUG-025 uit de git-fork, niet uit deze bestandsboom.
rsync -az --exclude '.git' --exclude 'build' --exclude 'build-flatpak' \
      --exclude 'repo' --exclude '.flatpak-builder' --exclude '*.flatpak' \
      --exclude 'derived' \
      --exclude 'verify-out*' ./ "$HOST:$REMOTE/" || fail "rsync"
echo "   ok"

step "6 Flatpak bouwen en publiceren"
ssh "$HOST" "rm -f $REMOTE/BUILD_DONE; cd $REMOTE && setsid nohup bash deploy/run-build-hc55.sh > nohup.log 2>&1 < /dev/null &" \
    || fail "build starten"
bash deploy/wait-for-build.sh 30 || fail "build"

step "7 release-gates"
ssh "$HOST" "cd $REMOTE && bash deploy/verify-flatpak-hc55.sh SteamDeckMSX-v${VERSION}.flatpak" \
    || fail "flatpak-gate"
ssh "$HOST" "cd $REMOTE && bash deploy/verify-visible-hc55.sh SteamDeckMSX-v${VERSION}.flatpak" \
    || fail "zichtbaarheids-gate"

echo
echo "KLAAR — v$VERSION staat live."
echo "Installeren op de Deck:"
echo "  curl -fsSL https://horsecloud55.ddns.net/steam/flatpak/deploy.sh | bash"
