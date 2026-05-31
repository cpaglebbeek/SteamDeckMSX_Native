#!/usr/bin/env bash
#
# sync-to-deck.sh — rsync source-tree naar Steam Deck Desktop Mode.
#
# Usage:
#   ./deploy/sync-to-deck.sh [user@]<host>[:<remote-path>]
#
# Voorbeelden:
#   ./deploy/sync-to-deck.sh deck@192.168.1.42
#   ./deploy/sync-to-deck.sh deck@192.168.1.42:/home/deck/SteamDeckMSX_Native
#
# Vereist:
#   - SSH-key gedeeld (ssh-copy-id) — Steam Deck Desktop Mode:
#       passwd  # eerst password instellen
#       systemctl --user start sshd  # of via Settings
#   - rsync op Deck (standaard aanwezig in SteamOS Holo)

set -euo pipefail

if [[ $# -ne 1 ]]; then
    echo "Usage: $0 [user@]<host>[:<remote-path>]" >&2
    exit 64
fi

target="$1"
if [[ "$target" != *:* ]]; then
    target="${target}:/home/deck/SteamDeckMSX_Native"
fi

repo_root="$(cd "$(dirname "$0")/.." && pwd)"

echo "rsync from: $repo_root"
echo "rsync to:   $target"

rsync -avz --delete \
    --exclude='.git/' \
    --exclude='build/' \
    --exclude='build-*/' \
    --exclude='externals/openmsx/derived/' \
    --exclude='externals/openmsx/build/' \
    --exclude='.flatpak-builder/' \
    --exclude='*.flatpak' \
    --exclude='releases/*.flatpak' \
    --exclude='roms/' \
    --exclude='bios/' \
    --exclude='*.rom' \
    --exclude='*.dsk' \
    --exclude='*.cas' \
    "$repo_root/" \
    "$target/"

echo ""
echo "Sync klaar. Volgende: ssh naar Deck en run deploy/flatpak-build-on-deck.sh"
