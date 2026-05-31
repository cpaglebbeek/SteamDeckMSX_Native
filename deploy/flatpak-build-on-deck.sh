#!/usr/bin/env bash
#
# flatpak-build-on-deck.sh — runt flatpak-builder op de Steam Deck.
#
# UITVOEREN OP DE STEAM DECK (Desktop Mode), niet op Mac/laptop.
# Mac kan dit niet — Flatpak is Linux-only.
#
# Vereist:
#   flatpak install -y flathub org.flatpak.Builder
#   flatpak install -y flathub org.freedesktop.Sdk.Extension.llvm18//23.08
#   flatpak install -y flathub-beta org.freedesktop.Platform//23.08 org.freedesktop.Sdk//23.08
#
# Usage (op Deck):
#   cd ~/SteamDeckMSX_Native
#   ./deploy/flatpak-build-on-deck.sh
#
# Output: flatpak-builder/build-flatpak/ + .flatpak-builder/ cache
#
# Status v0.0.3-Castlevania: nog NIET getest end-to-end (Christian heeft
# stap 21 'skip nu' gekozen). Eerste echte run = v0.0.4-fase.

set -euo pipefail

repo_root="$(cd "$(dirname "$0")/.." && pwd)"
cd "$repo_root"

if ! command -v flatpak-builder >/dev/null 2>&1; then
    if flatpak list --app | grep -q org.flatpak.Builder; then
        BUILDER="flatpak run org.flatpak.Builder"
    else
        echo "ERROR: flatpak-builder niet gevonden." >&2
        echo "Installeer: flatpak install -y flathub org.flatpak.Builder" >&2
        exit 1
    fi
else
    BUILDER="flatpak-builder"
fi

build_dir="build-flatpak"
repo_dir=".flatpak-builder/repo"
manifest="nl.icthorse.SteamDeckMSX.yaml"

if [[ ! -f "$manifest" ]]; then
    echo "ERROR: $manifest niet gevonden in $repo_root" >&2
    exit 1
fi

echo "Builder:  $BUILDER"
echo "Manifest: $manifest"
echo "Output:   $build_dir"

$BUILDER --force-clean \
    --repo="$repo_dir" \
    --install-deps-from=flathub \
    --user \
    "$build_dir" \
    "$manifest"

echo ""
echo "Build klaar."
echo "Installeer lokaal op de Deck:"
echo "  flatpak --user install -y $repo_dir nl.icthorse.SteamDeckMSX"
echo "Of bundle naar single-file:"
echo "  $BUILDER --repo=$repo_dir --bundle nl.icthorse.SteamDeckMSX SteamDeckMSX.flatpak"
