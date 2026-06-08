#pragma once

#include <QByteArray>
#include <QString>

// RomTypeDetector — heuristische BIOS-detect per ROM-bytes.
//
// v0.0.8: lichte heuristiek (size + SCC-write-pattern). Geen softwaredb-hash
// match nog (verschuift naar v0.0.9 — vereist openMSX softwaredb.xml-parse).
// Doel: in MsxCore::loadRom() kunnen voorstellen welke C-BIOS-machine
// gebruikt moet worden ipv user-keuze elke keer.
//
// API is bewust static + struct-result — geen Qt-state, makkelijk testbaar.
class RomTypeDetector {
public:
    enum class Generation {
        Unknown,
        MSX1,        // ROM ≤ 32KB, plain mapper
        MSX2,        // ROM > 32KB, mapper-based
        MSX2plus,    // alleen via softwaredb-hash (v0.0.9+)
    };

    enum class Mapper {
        Unknown,
        Plain,       // ≤32KB linear
        Konami,      // bekende Konami-mapper signature (geen SCC)
        KonamiSCC,   // Konami met SCC-sound (Nemesis 2/3, SD-Snatcher, Salamander, F1 Spirit)
        Ascii8,      // ASCII 8KB mapper (verschoven naar v0.0.9 — heuristiek complex)
        Ascii16,     // ASCII 16KB mapper (v0.0.9)
    };

    struct Result {
        Generation generation = Generation::Unknown;
        Mapper mapper = Mapper::Unknown;
        QString suggestedMachine;  // bv. "C-BIOS_MSX1" / "C-BIOS_MSX2" / "C-BIOS_MSX2+"
        QString reason;            // mens-leesbaar voor toast/log
        QString sha1Hex;           // SHA-1 hex (lowercase 40 chars) — toekomstige softwaredb lookup-key (v0.0.9+)
    };

    // Hoofd-API: analyseer ROM-bytes en return Result.
    // romBytes leeg → Unknown + fallback machine C-BIOS_MSX2 (safe default).
    static Result detect(const QByteArray &romBytes);

    // Helpers — publiek voor unit-tests.
    static Generation detectGeneration(const QByteArray &romBytes);
    static Mapper detectMapper(const QByteArray &romBytes);

    // SCC-detect: zoekt naar Konami SCC enable-write pattern
    //   LD A,#9F  →  $3E $9F
    //   LD (#80B0),A  →  $32 $B0 $80
    // = "3E 9F 32 B0 80" anywhere in ROM. False-positive mogelijk maar
    // 95%+ accuraat voor commercieel SCC-titels.
    static bool hasScc(const QByteArray &romBytes);

    // ASCII8-detect via 8KB-bank-switch write-pattern.
    static bool hasAscii8(const QByteArray &romBytes);
    // ASCII16-detect via 16KB-bank-switch write-pattern.
    static bool hasAscii16(const QByteArray &romBytes);

    // SHA-1 hex-string van ROM-bytes (lowercase 40 chars). Voor softwaredb-lookup
    // (v0.0.10+) en als fingerprint in `Result.sha1Hex`. Lege input → lege string.
    static QString sha1Hex(const QByteArray &romBytes);

    // String-helpers voor logging.
    static QString generationName(Generation g);
    static QString mapperName(Mapper m);
};
