#!/usr/bin/env bash
#
# install-on-deck.sh — installeert de gehoste Flatpak-bundle op de Steam Deck.
#
# Draaien in Desktop Mode → Konsole. Er is geen SSH naar de Deck, dus dit blok is
# bedoeld om te plakken (het staat daarom ook als één subshell in het clipboard:
# een `exit` sluit dan de subshell en niet de Konsole).
#
# Twee dingen die eerder misgingen en hier zijn afgevangen:
#   - BUG-015: bundle zonder runtime-herkomst → nu ingebakken --runtime-repo;
#     de expliciete flathub-remote-workaround is niet meer nodig.
#   - "already installed": flatpak weigert over een bestaande install heen te
#     installeren, waardoor je stilzwijgend de OUDE (kapotte) versie blijft
#     testen. Daarom altijd eerst uninstall.
#
# De groottecheck vangt het geval af waarin de download een nginx-foutpagina is:
# `flatpak install` faalt daar cryptisch op.

BUNDLE=SteamDeckMSX-v0.3.0-MazeOfGalious.flatpak
EXPECTED_SIZE=26561312
URL="https://horsecloud55.ddns.net/steam/flatpak/$BUNDLE"

cd ~/Downloads || exit 1

echo "== 1/4 downloaden =="
rm -f "$BUNDLE"
curl -fLO "$URL" || { echo "!! DOWNLOAD MISLUKT"; exit 1; }
sz=$(stat -c%s "$BUNDLE")
[ "$sz" -eq "$EXPECTED_SIZE" ] || { echo "!! VERKEERDE GROOTTE: $sz (verwacht $EXPECTED_SIZE)"; exit 1; }
echo "   ok — $sz bytes"

echo "== 2/4 oude install weg (anders test je de OUDE, kapotte versie) =="
flatpak uninstall --user -y nl.icthorse.SteamDeckMSX 2>/dev/null \
    && echo "   oude verwijderd" || echo "   geen oude install"

echo "== 3/4 installeren =="
flatpak install --user -y "$BUNDLE" || { echo "!! INSTALL MISLUKT"; exit 1; }

echo "== 4/4 starten =="
flatpak run nl.icthorse.SteamDeckMSX
