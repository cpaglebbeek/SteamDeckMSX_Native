#!/bin/bash
#
# run-build-hc55.sh — bouwt de Flatpak-bundle op HC55 en publiceert hem.
#
# Draaien in /root/sdmsx_build (daar staat de gesynchroniseerde bron).
# De bundelnaam wordt afgeleid uit VERSION: eerder stond hij hard in dit script
# en werd hij per release met sed bijgewerkt, waardoor de kopie op HC55 en de
# versie in de repo uit elkaar liepen.

cd /root/sdmsx_build || exit 1

VERSION_STR=$(cat VERSION 2>/dev/null | tr -d '[:space:]')
BUNDLE_NAME="SteamDeckMSX-v${VERSION_STR}.flatpak"
WEBDIR=/srv/steamweb/flatpak

rm -f BUILD_DONE
flatpak-builder --force-clean --disable-rofiles-fuse --default-branch=stable \
    build-flatpak nl.icthorse.SteamDeckMSX.yaml > flatpak-build.log 2>&1
ec=$?
echo "BUILD_EXIT=$ec" >> flatpak-build.log

if [ $ec -eq 0 ]; then
  flatpak build-export repo build-flatpak stable >> flatpak-build.log 2>&1 && \
  flatpak build-bundle --runtime-repo=https://dl.flathub.org/repo/flathub.flatpakrepo \
      repo "$BUNDLE_NAME" nl.icthorse.SteamDeckMSX stable >> flatpak-build.log 2>&1 && \
  echo "BUNDLE_OK $(ls -la "$BUNDLE_NAME")" >> flatpak-build.log
fi

# Publiceren + de stabiele -latest-URL meeverhuizen. Zonder die symlink breekt
# een eerder verstrekt installblok zodra de vorige versie wordt opgeruimd
# (2026-07-25: melding "download mislukt" bleek een 404 op de oude bestandsnaam).
if [ -f "$BUNDLE_NAME" ]; then
  cp "$BUNDLE_NAME" "$WEBDIR/" && \
  ln -sfn "$BUNDLE_NAME" "$WEBDIR/SteamDeckMSX-latest.flatpak" && \
  echo "PUBLISHED $BUNDLE_NAME + latest-symlink" >> flatpak-build.log
fi

touch /root/sdmsx_build/BUILD_DONE
