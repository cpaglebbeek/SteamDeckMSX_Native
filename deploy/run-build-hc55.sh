#!/bin/bash
cd /root/sdmsx_build
flatpak-builder --force-clean --disable-rofiles-fuse --default-branch=stable build-flatpak nl.icthorse.SteamDeckMSX.yaml > flatpak-build.log 2>&1
ec=$?
echo "BUILD_EXIT=$ec" >> flatpak-build.log
if [ $ec -eq 0 ]; then
  flatpak build-export repo build-flatpak stable >> flatpak-build.log 2>&1 && \
  flatpak build-bundle --runtime-repo=https://dl.flathub.org/repo/flathub.flatpakrepo repo SteamDeckMSX-v0.2.1-KingsValley.flatpak nl.icthorse.SteamDeckMSX stable >> flatpak-build.log 2>&1 && \
  echo "BUNDLE_OK $(ls -la SteamDeckMSX-v0.2.1-KingsValley.flatpak)" >> flatpak-build.log
fi
touch /root/sdmsx_build/BUILD_DONE
