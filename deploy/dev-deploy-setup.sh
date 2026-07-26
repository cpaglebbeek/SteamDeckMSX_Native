#!/usr/bin/env bash
#
# Deploy2SteamDeck — eenmalige setup op de Steam Deck.
#
# Gehost als https://icthorse.nl/steam/dev/deploy (bewust icthorse.nl: dat
# domein is Hostinger-DNS en heeft geen last van het No-IP-verval van
# horsecloud55.ddns.net). Draaien in Konsole (Desktop Mode):
#
#   curl -fsSL https://icthorse.nl/steam/dev/deploy | bash
#
# Wat het doet (idempotent, opnieuw draaien kan altijd):
#   1. sshd aanzetten — SteamOS levert die mee maar heeft hem standaard uit.
#      Dit is de "app die luistert": inkomende deploys zijn gewoon ssh/scp.
#   2. De publieke sleutel van de ontwikkel-Mac autoriseren. Alleen die Mac
#      kan inloggen; het script bevat uitsluitend een PUBLIEKE sleutel en is
#      daarom veilig om openbaar te hosten.
#   3. Melden onder welke naam de Deck bereikbaar is (mDNS: steamdeck.local).
#
# Daarna deployt de Mac met de skill /Deploy2SteamDeck — zonder dat er op de
# Deck nog iets getypt hoeft te worden.

set -euo pipefail

PUBKEY='ssh-ed25519 AAAAC3NzaC1lZDI1NTE5AAAAIEd7ST22ld/FeI3DRpoqLI2E/HdVSvVuonJBXS2zpqE2 claude-code@macbook-christian → steamdeck'

echo "== Deploy2SteamDeck setup =="

echo "== 1/3 ssh-server aanzetten =="
if systemctl is-active -q sshd 2>/dev/null; then
    echo "   sshd draait al"
else
    echo "   sudo-wachtwoord van de Deck nodig (eenmalig):"
    sudo systemctl enable --now sshd
    echo "   sshd aan + start voortaan mee"
fi

echo "== 2/3 ontwikkel-Mac autoriseren =="
mkdir -p "$HOME/.ssh" && chmod 700 "$HOME/.ssh"
touch "$HOME/.ssh/authorized_keys" && chmod 600 "$HOME/.ssh/authorized_keys"
if grep -qF "$PUBKEY" "$HOME/.ssh/authorized_keys"; then
    echo "   sleutel stond er al"
else
    printf '%s\n' "$PUBKEY" >> "$HOME/.ssh/authorized_keys"
    echo "   sleutel toegevoegd"
fi

echo "== 3/3 bereikbaarheid =="
HOST_LOCAL="$(hostname).local"
DECK_IP=$(ip -4 -o addr show scope global 2>/dev/null | awk '{print $4}' | cut -d/ -f1 | head -1)
echo
echo "KLAAR — de Deck luistert nu naar deploys."
echo "   naam:  $USER@$HOST_LOCAL"
echo "   ip:    ${DECK_IP:-onbekend} (wisselt; de naam hierboven is leidend)"
echo
echo "Vanaf nu op de Mac: /Deploy2SteamDeck — hier hoeft niets meer."
