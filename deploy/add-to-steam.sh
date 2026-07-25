( # SteamDeckMSX als niet-Steam-spel toevoegen (Desktop Mode)
APP=nl.icthorse.SteamDeckMSX
flatpak info --user "$APP" >/dev/null 2>&1 || { echo "!! app niet geinstalleerd — draai eerst het installblok"; exit 1; }

echo "== 1/3 icoon =="
# Pad via 'current/active' blijft geldig over updates heen; het pad met de
# commit-hash erin verandert bij elke nieuwe versie.
SRC=~/.local/share/flatpak/app/$APP/current/active/files/share/openmsx/icons/openMSX-logo-128.png
mkdir -p ~/.local/share/icons
if [ -f "$SRC" ]; then cp -f "$SRC" ~/.local/share/icons/steamdeckmsx.png && echo "   ok"; else echo "   geen icoon gevonden (niet fataal)"; fi

echo "== 2/3 menu-item =="
# De Flatpak exporteert zelf geen .desktop, dus die maken we hier. Zonder dit
# bestand heeft Steam niets om aan toe te voegen.
mkdir -p ~/.local/share/applications
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
echo "   $D"

echo "== 3/3 aan Steam toevoegen =="
if command -v steamos-add-to-steam >/dev/null 2>&1; then
  steamos-add-to-steam "$D" && echo "   toegevoegd — Steam toont een bevestiging"
else
  echo "   'steamos-add-to-steam' niet gevonden. Handmatig in Steam:"
  echo "     Spellen -> Voeg een niet-Steam-spel toe -> Bladeren"
  echo "     kies:  $D"
fi
echo
echo "Daarna: Steam herstarten (of terug naar Gaming Mode)."
echo "Het spel heet 'SteamDeckMSX' onder Bibliotheek -> Niet-Steam."
)
