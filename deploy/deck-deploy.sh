( # SteamDeckMSX — volledige deploy op de Steam Deck: app + spellen + Steam-item
# Eén commando, van niets naar speelklaar. Idempotent: opnieuw draaien werkt.
#
# Waarom apart van install.sh: dat script eindigt met `flatpak run` en blijft
# dus hangen, waardoor de stappen erna (Steam-item, controle) nooit liepen.
# Dit script start de app niet zelf en sluit af met een controle.
set -u
BASE="https://horsecloud55.ddns.net/steam/flatpak"
APP=nl.icthorse.SteamDeckMSX
B=SteamDeckMSX-latest.flatpak
FOUT=0

cd ~/Downloads 2>/dev/null || cd /tmp || exit 1

echo "== 1/6 app downloaden =="
rm -f "$B"
code=$(curl -L -o "$B" -w '%{http_code}' --connect-timeout 20 --retry 2 "$BASE/$B"); rc=$?
if [ "$rc" -ne 0 ] || [ "$code" != "200" ]; then
  echo "!! DOWNLOAD MISLUKT — curl-code $rc, HTTP $code"; exit 1; fi
sz=$(stat -c%s "$B" 2>/dev/null || echo 0)
# Een HTML-foutpagina begint met '<' en is klein; die niet als bundle aanbieden.
if [ "${sz:-0}" -lt 10000000 ] || [ "$(head -c1 "$B")" = "<" ]; then
  echo "!! GEEN GELDIGE BUNDLE ($sz bytes)"; exit 1; fi
echo "   ok — $sz bytes"

echo "== 2/6 oude versie verwijderen =="
# Zonder dit weigert flatpak met 'already installed' en test je de oude versie.
flatpak uninstall --user -y "$APP" >/dev/null 2>&1 && echo "   verwijderd" || echo "   geen oude install"

echo "== 3/6 app installeren =="
flatpak install --user -y "$B" || { echo "!! INSTALL MISLUKT"; exit 1; }
echo "   ok"

echo "== 4/6 spellen in ~/ROMs =="
mkdir -p ~/ROMs
Z=/tmp/msx-startpakket.zip
if curl -fsSL -o "$Z" "$BASE/msx-startpakket.zip"; then
  if command -v unzip >/dev/null 2>&1; then unzip -o -q "$Z" -d ~/ROMs
  elif command -v bsdtar >/dev/null 2>&1; then bsdtar -xf "$Z" -C ~/ROMs
  else python3 -m zipfile -e "$Z" ~/ROMs; fi
  rm -f "$Z"
  echo "   $(ls -1 ~/ROMs/*.rom 2>/dev/null | wc -l) spellen"
else
  echo "   !! startpakket niet opgehaald (app werkt, galerij begint leeg)"
fi

echo "== 5/6 in Steam zetten =="
# De Flatpak exporteert zelf geen .desktop-bestand, dus Steam heeft niets om
# aan toe te voegen; daarom maken we het hier.
SRC=~/.local/share/flatpak/app/$APP/current/active/files/share/openmsx/icons/openMSX-logo-128.png
mkdir -p ~/.local/share/icons ~/.local/share/applications
[ -f "$SRC" ] && cp -f "$SRC" ~/.local/share/icons/steamdeckmsx.png
D=~/.local/share/applications/SteamDeckMSX.desktop
cat > "$D" <<EOF
[Desktop Entry]
Type=Application
Name=SteamDeckMSX
Comment=MSX-emulator met spelgalerij
Exec=flatpak run $APP
Icon=$HOME/.local/share/icons/steamdeckmsx.png
Terminal=false
Categories=Game;Emulator;
StartupNotify=false
EOF
chmod +x "$D"
update-desktop-database ~/.local/share/applications 2>/dev/null
if command -v steamos-add-to-steam >/dev/null 2>&1; then
  steamos-add-to-steam "$D" >/dev/null 2>&1 && echo "   toegevoegd aan Steam"
else
  echo "   handmatig: Steam > Spellen > Voeg een niet-Steam-spel toe > $D"
fi

echo "== 6/6 controle =="
# `flatpak info` toont geen Version-veld voor deze app, alleen een commit-hash.
# Aanwezigheid toetsen we dus op de exit-code, en het versienummer halen we uit
# het bestand dat naast de bundle gepubliceerd staat.
if flatpak info --user "$APP" >/dev/null 2>&1; then
  V=$(curl -fsSL "$BASE/VERSION" 2>/dev/null | tr -d '[:space:]')
  echo "   app:      geïnstalleerd${V:+ (v$V)}"
else
  echo "   !! app niet gevonden na install"; FOUT=1
fi
N=$(ls -1 ~/ROMs/*.rom 2>/dev/null | wc -l)
[ "$N" -gt 0 ] && echo "   spellen:  $N in ~/ROMs" || { echo "   !! geen spellen in ~/ROMs"; FOUT=1; }
[ -f "$D" ] && echo "   Steam:    menu-item aanwezig" || { echo "   !! geen .desktop"; FOUT=1; }
# De emulator moet ergens kunnen schrijven, anders gaan save-states verloren
# (BUG-024). De map ontstaat pas bij de eerste start, dus dit is een melding.
P=~/.var/app/$APP/data
[ -d "$P" ] && echo "   opslag:   $P" || echo "   opslag:   ontstaat bij eerste start"

echo
if [ "$FOUT" -eq 0 ]; then
  echo "KLAAR — start via Steam, of nu meteen met:"
  echo "  flatpak run $APP"
  echo "In de galerij: A/Enter = starten · R = opnieuw scannen · M = map aanwijzen · F12 = terug uit spel"
else
  echo "DEPLOY NIET COMPLEET — zie de !!-regels hierboven"
  exit 1
fi
)
