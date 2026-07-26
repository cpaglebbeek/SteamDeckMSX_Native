---
date: 2026-07-26
repo: SteamDeckMSX_Native
status: done
resume: ""
---

# Sessie: eigen BIOS-dumps naar HC55 /root/bios

## Prompt

"kopieer alle *.rom bestanden naar hc55/bios" → daarna "oeu".

## Acties

- 4 eigen BIOS-dumps gevonden in `~/Downloads`: `MSX.rom`, `MSX2J.rom`,
  `nms8245_disk_1.06.rom`, `vg8020_basic-bios1.rom`.
- Op HC55 bestond nog geen `bios`-map; `/root/bios` aangemaakt (naast bestaand
  `/root/ROMs` met Nemesis 1+2) en de 4 bestanden via scp gekopieerd.
- Integriteit geverifieerd: md5 lokaal == md5 op HC55 voor alle 4.

## Keuzes / kaders

- **Privé root-pad, NIET in een webroot.** Dit respecteert het eerdere besluit
  (v0.4.0/v0.5.0-sessie) om BIOS-ROM's niet publiek of achter een ingebakken
  wachtwoord te hosten (P-SDM-05). `/root/bios` is alleen via ssh bereikbaar en
  kan later dienen als "eigen bron met inloggegevens die de gebruiker zelf
  invult" voor de v0.5.0 URL-import.
- Geen wijzigingen aan repo-code, nginx of services; alleen bestandsplaatsing
  op de server.
