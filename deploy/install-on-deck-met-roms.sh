( # SteamDeckMSX — app + startpakket met spellen
BASE="https://hc55.icthorse.nl/steam/flatpak"
B=SteamDeckMSX-latest.flatpak
cd ~/Downloads 2>/dev/null || cd /tmp || exit 1

echo "== 1/5 app downloaden =="
rm -f "$B"
code=$(curl -L -o "$B" -w '%{http_code}' --connect-timeout 20 --retry 2 "$BASE/$B"); rc=$?
if [ "$rc" -ne 0 ] || [ "$code" != "200" ]; then
  echo "!! DOWNLOAD MISLUKT — curl-code $rc, HTTP $code"; exit 1; fi
sz=$(stat -c%s "$B" 2>/dev/null || echo 0)
if [ "${sz:-0}" -lt 10000000 ] || [ "$(head -c1 "$B")" = "<" ]; then
  echo "!! GEEN GELDIGE BUNDLE ($sz bytes)"; exit 1; fi
echo "   ok — $sz bytes"

echo "== 2/5 oude install weg =="
flatpak uninstall --user -y nl.icthorse.SteamDeckMSX 2>/dev/null && echo "   verwijderd" || echo "   geen oude install"

echo "== 3/5 app installeren =="
flatpak install --user -y "$B" || { echo "!! INSTALL MISLUKT"; exit 1; }

echo "== 4/5 spellen installeren in ~/ROMs =="
# De app zoekt losse .rom/.dsk/.cas-bestanden in je persoonlijke map. Een map
# die "ROMs" heet is de meest voor de hand liggende plek en wordt gescand.
mkdir -p ~/ROMs
Z=/tmp/msx-startpakket.zip
if curl -fsSL -o "$Z" "$BASE/msx-startpakket.zip"; then
  if command -v unzip >/dev/null 2>&1; then unzip -o -q "$Z" -d ~/ROMs
  elif command -v bsdtar >/dev/null 2>&1; then bsdtar -xf "$Z" -C ~/ROMs
  else python3 -m zipfile -e "$Z" ~/ROMs; fi
  rm -f "$Z"
  echo "   spellen in ~/ROMs:"
  ls -1 ~/ROMs/*.rom 2>/dev/null | sed 's|.*/|     |'
else
  echo "   !! startpakket kon niet worden opgehaald (app werkt wel, alleen leeg)"
fi

echo "== 5/5 starten =="
echo "   R = opnieuw scannen   M = map aanwijzen   O = bestand openen   A = starten"
flatpak run nl.icthorse.SteamDeckMSX
)
