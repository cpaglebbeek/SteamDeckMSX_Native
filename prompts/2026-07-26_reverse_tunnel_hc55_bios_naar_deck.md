---
date: 2026-07-26
repo: SteamDeckMSX_Native
status: open
resume: "verder met steamdeck tunnel — BIOS-roms naar /home/deck/BIOS via reverse tunnel HC55:2223. Infra staat (decktunnel-user + icthorse.nl/steam/dev/tunnel), maar de Deck heeft HC55 nog NOOIT bereikt: script strandt op de Deck of netwerk blokkeert uitgaand 22. Eerste stap: output van `systemctl --user status decktunnel.service` op de Deck bekijken. Daarna: 4 roms uit ~/Downloads (staan ook op HC55:/root/bios) scp'en via ProxyJump + md5-check"
---

# Reverse tunnel Deck ↔ HC55 + BIOS-roms naar de Deck

## Trigger

"kopieer alle *.rom bestanden naar hc55/bios" (klaar: `/root/bios`, md5-geverifieerd, privé)
→ "gebruik ssh tunnel naar steamdeck om bios roms te plaatsen op /home/deck/BIOS".

## Wat er staat (werkend gevalideerd vanaf de Mac)

- **HC55:** user `decktunnel` (nologin, `restrict,port-forwarding,permitlisten="2223"`) —
  key kan alléén de reverse-forward op loopback:2223 openen; shell-toegang gemeten geweigerd.
- **Deck-script** `deploy/dev-tunnel-setup.sh` → gehost als
  `https://icthorse.nl/steam/dev/tunnel` (zelfde route als `/deploy`). Zet systemd --user
  service `decktunnel.service`: `ssh -N -R 127.0.0.1:2223:localhost:22` naar 157.180.29.184,
  `Restart=always`. Repo-versie bevat een placeholder; de private key wordt alleen bij
  rsync-deploy geïnjecteerd (publieke repo blijft schoon; afweging gedocumenteerd in het script).
- **Mac-route straks:** `ssh -o ProxyJump=horsecloud55 -p 2223 -i ~/.ssh/steamdeck deck@localhost`.

## Twee eigen fouten, beide hersteld

1. **Dubbele key-injectie:** placeholder stond óók in een commentaarregel → key twee keer in het
   gehoste script, deels buiten commentaar (runtime zou stranden). Bron-comment herschreven,
   herdeployed, gehost bestand geverifieerd (1× BEGIN-key, `bash -n` groen).
2. **Vals "tunnel actief":** de validatie-testtunnel vanaf de Mac werd niet gekilld
   (pkill-pattern matchte de argumentvolgorde niet) en hield 2223 bezet; de wachter zag dat aan
   voor de Deck en één scp-poging liep tegen de Mac zelf. Alle auth.log-entries voor
   `decktunnel` bleken van de Mac (zelfde key-fingerprint czYty6…, bron-IP = Mac-IPv4).
   Wachter draait nu met identiteitscheck: hostkey-fingerprint tunneleinde moet ≠ Mac zijn.

## Openstaand (blokkerend)

**De Deck heeft HC55 nog nooit bereikt** — nul verbindingspogingen in auth.log, ook na twee
runs van de one-liner door de gebruiker. Dus: script strandt op de Deck zelf, of het netwerk
van de Deck blokkeert uitgaand ssh. Diagnose nodig van de Deck-kant:
`systemctl --user status decktunnel.service` (verwacht: timeout/unreachable/niet-bestaand).

## Afstemming parallelle sessie (zelfde dag)

Zie `2026-07-26_v0.4.1_v0.5.0_deck_feedback_ronde.md`: die sessie zette v0.5.0-Goonies op de
Deck (e2e over hotspot, LAN-route) en noteerde "reverse tunnel via HC55 = later actiepunt" —
dit is dat actiepunt. Skill `/Deploy2SteamDeck` bijgewerkt met de tunnel-route
(ClaudeSkills-commit met valkuil-les). Haar open resume (gebruikerstest v0.5.0 op de Deck)
staat los van deze tunnel-debug en blijft gelden.

## BIOS-roms (gereed voor de kopieerslag)

`MSX.rom`, `MSX2J.rom`, `nms8245_disk_1.06.rom`, `vg8020_basic-bios1.rom` — in `~/Downloads`
(Mac) én `/root/bios` (HC55, md5-geverifieerd). Doel: `/home/deck/BIOS`.
