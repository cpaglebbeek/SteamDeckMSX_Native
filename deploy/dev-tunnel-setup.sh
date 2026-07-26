#!/usr/bin/env bash
#
# Deploy2SteamDeck — reverse tunnel Deck → HC55 (buitenshuis-route).
#
# Gehost als https://icthorse.nl/steam/dev/tunnel (zelfde route als /deploy;
# icthorse.nl is Hostinger-DNS en heeft geen last van het No-IP-verval van
# horsecloud55.ddns.net). Draaien in Konsole (Desktop Mode):
#
#   curl -fsSL https://icthorse.nl/steam/dev/tunnel | bash
#
# Wat het doet (idempotent, opnieuw draaien kan altijd):
#   1. Controleert dat sshd op de Deck draait (anders eerst /steam/dev/deploy).
#   2. Schrijft de tunnel-sleutel (alleen bruikbaar voor: reverse forward van
#      poort 2223 op HC55-loopback; geen shell, geen andere poorten — op HC55
#      afgedwongen met restrict,port-forwarding,permitlisten="2223").
#   3. Zet een systemd --user service die de tunnel opent en openhoudt:
#      HC55:127.0.0.1:2223 → Deck:22. De ontwikkel-Mac bereikt de Deck dan
#      via ProxyJump over HC55, waar de Deck ook is (LTE, hotel-wifi, ...).
#
# LET OP (bewuste afweging, 2026-07-26): de gehoste variant van dit script
# bevat de PRIVATE tunnel-sleutel. Die sleutel kan uitsluitend de reverse
# forward op HC55:2223 openen — geen shell, geen bestanden, geen andere
# poorten. Wie hem misbruikt kan hooguit poort 2223 bezet houden. De sleutel
# in dít repo-bestand is een placeholder; injectie gebeurt bij deploy.
#
# In de repo staat een placeholder in het heredoc hieronder; de deploy-stap
# (rsync naar Hostinger) vervangt die door de echte sleutel.

set -euo pipefail

HC55_IP="157.180.29.184"
TUNNEL_PORT="2223"
KEYFILE="$HOME/.ssh/decktunnel_hc55"
UNIT_DIR="$HOME/.config/systemd/user"
UNIT="decktunnel.service"

echo "== Deploy2SteamDeck reverse-tunnel setup =="

echo "== 1/3 sshd-check =="
if systemctl is-active -q sshd; then
    echo "   sshd draait"
else
    echo "!! sshd draait niet op deze Deck — de tunnel zou nergens heen wijzen."
    echo "   Draai eerst:  curl -fsSL https://icthorse.nl/steam/dev/deploy | bash"
    exit 1
fi

echo "== 2/3 tunnel-sleutel plaatsen =="
mkdir -p "$HOME/.ssh" && chmod 700 "$HOME/.ssh"
cat > "$KEYFILE" <<'DECKTUNNEL_KEY_EOF'
__DECKTUNNEL_PRIVATE_KEY__
DECKTUNNEL_KEY_EOF
chmod 600 "$KEYFILE"
if grep -q "__DECKTUNNEL" "$KEYFILE"; then
    echo "!! Dit is de repo-versie zonder sleutel — gebruik de gehoste variant:"
    echo "   curl -fsSL https://icthorse.nl/steam/dev/tunnel | bash"
    rm -f "$KEYFILE"
    exit 1
fi
echo "   sleutel staat in $KEYFILE"

echo "== 3/3 tunnel-service aanzetten =="
mkdir -p "$UNIT_DIR"
cat > "$UNIT_DIR/$UNIT" <<EOF
[Unit]
Description=Reverse ssh-tunnel Deck -> HC55 (HC55:${TUNNEL_PORT} -> Deck:22)
After=network-online.target

[Service]
ExecStart=/usr/bin/ssh -N \\
    -R 127.0.0.1:${TUNNEL_PORT}:localhost:22 \\
    -i %h/.ssh/decktunnel_hc55 \\
    -o ServerAliveInterval=15 -o ServerAliveCountMax=3 \\
    -o ExitOnForwardFailure=yes \\
    -o StrictHostKeyChecking=accept-new \\
    -o BatchMode=yes \\
    decktunnel@${HC55_IP}
Restart=always
RestartSec=10

[Install]
WantedBy=default.target
EOF
systemctl --user daemon-reload
systemctl --user enable --now "$UNIT"
sleep 3
if systemctl --user is-active -q "$UNIT"; then
    echo "   tunnel-service draait"
else
    echo "!! service start niet — status:"
    systemctl --user status "$UNIT" --no-pager 2>&1 | head -10
    exit 1
fi

echo
echo "== Klaar. De ontwikkel-Mac kan nu bij deze Deck via HC55:${TUNNEL_PORT}. =="
echo "   De tunnel herstelt zichzelf (Restart=always) en start mee bij inloggen."
echo "   Uitzetten:  systemctl --user disable --now ${UNIT}"
