#!/usr/bin/env bash
#
# install-on-deck.sh — installeert de nieuwste Flatpak-bundle op de Steam Deck.
#
# Draaien in Desktop Mode → Konsole. Er is geen SSH naar de Deck, dus dit blok is
# bedoeld om te plakken (het staat daarom ook als één subshell in het clipboard:
# een `exit` sluit dan de subshell en niet de Konsole).
#
# Vier dingen die eerder misgingen en hier zijn afgevangen:
#   - BUG-015: bundle zonder runtime-herkomst → nu ingebakken --runtime-repo;
#     de expliciete flathub-remote-workaround is niet meer nodig.
#   - "already installed": flatpak weigert over een bestaande install heen te
#     installeren, waardoor je stilzwijgend de OUDE versie blijft testen. Sinds
#     v0.3.1 extra van belang: die versie vraagt nieuwe sandbox-rechten
#     (home:ro), en die komen er alleen bij een echte herinstallatie.
#   - "download mislukt" (2026-07-25): dit blok wees naar een bestandsnaam mét
#     versienummer, en dat bestand verdween bij de volgende release → 404. Nu de
#     stabiele `-latest`-URL, die altijd naar de actuele bundle wijst.
#   - Een vaste groottecheck brak bij elke nieuwe versie. Er wordt nu
#     gecontroleerd of het resultaat plausibel is (ruim boven 10 MB, geen
#     HTML-foutpagina) in plaats van exact hoeveel bytes het moet zijn.

BUNDLE=SteamDeckMSX-latest.flatpak
URL="https://horsecloud55.ddns.net/steam/flatpak/$BUNDLE"

cd ~/Downloads 2>/dev/null || cd /tmp || exit 1

echo "== 1/4 downloaden =="
rm -f "$BUNDLE"
code=$(curl -L -o "$BUNDLE" -w '%{http_code}' --connect-timeout 20 --retry 2 "$URL"); rc=$?
if [ "$rc" -ne 0 ] || [ "$code" != "200" ]; then
    # Zonder deze uitsplitsing is "download mislukt" niet te onderscheiden van
    # een netwerkprobleem, een certificaatfout of een verdwenen bestand.
    echo "!! DOWNLOAD MISLUKT — curl-code $rc, HTTP $code"
    echo "   (6=DNS onbereikbaar, 7=geen verbinding, 60=certificaatfout, 404=bestand weg)"
    echo "   Test handmatig:  curl -I $URL"
    exit 1
fi

sz=$(stat -c%s "$BUNDLE" 2>/dev/null || echo 0)
if [ "${sz:-0}" -lt 10000000 ] || [ "$(head -c1 "$BUNDLE")" = "<" ]; then
    echo "!! GEEN GELDIGE BUNDLE ($sz bytes) — waarschijnlijk een foutpagina:"
    head -c 200 "$BUNDLE"; echo
    exit 1
fi
echo "   ok — $sz bytes"

echo "== 2/4 oude install weg =="
flatpak uninstall --user -y nl.icthorse.SteamDeckMSX 2>/dev/null \
    && echo "   oude verwijderd" || echo "   geen oude install"

echo "== 3/4 installeren =="
flatpak install --user -y "$BUNDLE" || { echo "!! INSTALL MISLUKT"; exit 1; }

echo "== 4/4 starten =="
echo "   De scan start vanzelf: hele persoonlijke map + SD-kaart."
echo "   R = opnieuw scannen   M = map aanwijzen   O = bestand openen   A = starten"
flatpak run nl.icthorse.SteamDeckMSX
