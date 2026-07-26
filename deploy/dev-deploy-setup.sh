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

# Verse Steam Decks hebben géén gebruikerswachtwoord, en sudo weigert dan
# hard. Eerst detecteren, anders strandt stap 1 met een cryptische prompt.
if passwd -S "$USER" 2>/dev/null | grep -qE ' (NP|L) '; then
    echo
    echo "!! Er is nog geen wachtwoord ingesteld op deze Deck — sudo werkt dan niet."
    echo "   Doe eerst (eenmalig):   passwd"
    echo "   Kies een wachtwoord, en draai daarna dit commando opnieuw:"
    echo "   curl -fsSL https://icthorse.nl/steam/dev/deploy | bash"
    exit 1
fi

echo "== 1/3 ssh-server aanzetten =="
# </dev/tty: bij `curl | bash` is stdin de pipe en kan sudo zijn
# wachtwoordprompt nergens kwijt. Altijd enable --now draaien (idempotent):
# een eerdere run kan half gestrand zijn terwijl is-active toen al ja zei.
echo "   sudo-wachtwoord van de Deck nodig (eenmalig):"
sudo systemctl enable --now sshd </dev/tty
if systemctl is-active -q sshd; then
    echo "   sshd draait"
else
    echo "!! sshd start niet — status:"
    systemctl status sshd --no-pager 2>&1 | head -8
    exit 1
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
# Geen `hostname` gebruiken: dat commando bestaat niet op SteamOS (gemeten
# 2026-07-26, "opdracht niet gevonden" crashte het script hier via set -e).
HOST_NAME="${HOSTNAME:-$(uname -n)}"
DECK_IP=$(ip -4 -o addr show scope global 2>/dev/null | awk '{print $4}' | cut -d/ -f1 | head -1) || true
LISTEN=$(ss -tln 2>/dev/null | grep -c ':22 ') || true
echo "   luistert op poort 22: ${LISTEN:-0} socket(s)"
echo
echo "KLAAR — de Deck luistert nu naar deploys."
echo "   naam:  $USER@${HOST_NAME}.fritz.box (of ${HOST_NAME}.local)"
echo "   ip:    ${DECK_IP:-onbekend} (wisselt; de naam hierboven is leidend)"
echo
echo "Vanaf nu op de Mac: /Deploy2SteamDeck — hier hoeft niets meer."
