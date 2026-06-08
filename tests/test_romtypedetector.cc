// Unit-tests voor RomTypeDetector — heuristische BIOS-detect per ROM-bytes.
//
// Test-corpus:
//   T1: lege ROM → Unknown + fallback C-BIOS_MSX2
//   T2: 8KB plain → MSX1 + Plain mapper + C-BIOS_MSX1
//   T3: 32KB plain → MSX1 (grenscase)
//   T4: 33KB → MSX2 (grenscase aan andere kant)
//   T5: 64KB zonder SCC-pattern → MSX2 + Konami
//   T6: 64KB met SCC-pattern → MSX2 + KonamiSCC
//   T7: 256KB met SCC-pattern op offset → MSX2 + KonamiSCC (offset-detect)
//   T8: helpers generationName/mapperName

#include "../src/RomTypeDetector.h"

#include <QByteArray>
#include <QCoreApplication>
#include <QDebug>

#define EXPECT(cond) do { \
    if (!(cond)) { qCritical() << "FAIL line" << __LINE__ << ":" << #cond; std::exit(1); } \
} while (0)

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    using Gen = RomTypeDetector::Generation;
    using Map = RomTypeDetector::Mapper;

    // ----- T1: lege ROM -----
    {
        QByteArray empty;
        const auto r = RomTypeDetector::detect(empty);
        EXPECT(r.generation == Gen::Unknown);
        EXPECT(r.mapper == Map::Unknown);
        EXPECT(r.suggestedMachine == QStringLiteral("C-BIOS_MSX2"));
        EXPECT(!r.reason.isEmpty());
    }

    // ----- T2: 8KB plain -----
    {
        QByteArray rom(8 * 1024, char(0x00));
        const auto r = RomTypeDetector::detect(rom);
        EXPECT(r.generation == Gen::MSX1);
        EXPECT(r.mapper == Map::Plain);
        EXPECT(r.suggestedMachine == QStringLiteral("C-BIOS_MSX1"));
    }

    // ----- T3: 32KB plain (grenscase MSX1) -----
    {
        QByteArray rom(32 * 1024, char(0xFF));
        const auto r = RomTypeDetector::detect(rom);
        EXPECT(r.generation == Gen::MSX1);
        EXPECT(r.mapper == Map::Plain);
    }

    // ----- T4: 33KB (grenscase MSX2) -----
    {
        QByteArray rom(33 * 1024, char(0x00));
        const auto r = RomTypeDetector::detect(rom);
        EXPECT(r.generation == Gen::MSX2);
        // Geen SCC-pattern in nullen → default Konami mapper.
        EXPECT(r.mapper == Map::Konami);
        EXPECT(r.suggestedMachine == QStringLiteral("C-BIOS_MSX2"));
    }

    // ----- T5: 64KB zonder SCC-pattern -----
    {
        QByteArray rom(64 * 1024, char(0xAA));  // pattern dat NIET overeenkomt
        const auto r = RomTypeDetector::detect(rom);
        EXPECT(r.generation == Gen::MSX2);
        EXPECT(r.mapper == Map::Konami);
        EXPECT(!RomTypeDetector::hasScc(rom));
    }

    // ----- T6: 64KB met SCC-pattern -----
    {
        // SCC enable-write: 3E 9F 32 B0 80
        QByteArray rom(64 * 1024, char(0x00));
        const QByteArray sccPattern = QByteArray::fromHex("3E9F32B080");
        // Plak pattern op offset 0x100 (typische cartridge-init code-locatie)
        rom.replace(0x100, sccPattern.size(), sccPattern);
        EXPECT(RomTypeDetector::hasScc(rom));
        const auto r = RomTypeDetector::detect(rom);
        EXPECT(r.generation == Gen::MSX2);
        EXPECT(r.mapper == Map::KonamiSCC);
        EXPECT(r.suggestedMachine == QStringLiteral("C-BIOS_MSX2"));
        EXPECT(r.reason.contains(QStringLiteral("SCC")));
    }

    // ----- T7: 256KB met SCC op offset 0x40000 -----
    {
        QByteArray rom(256 * 1024, char(0x00));
        const QByteArray sccPattern = QByteArray::fromHex("3E9F32B080");
        rom.replace(0x40000, sccPattern.size(), sccPattern);
        EXPECT(RomTypeDetector::hasScc(rom));
        const auto r = RomTypeDetector::detect(rom);
        EXPECT(r.mapper == Map::KonamiSCC);
    }

    // ----- T8: helpers -----
    {
        EXPECT(RomTypeDetector::generationName(Gen::MSX1) == QStringLiteral("MSX1"));
        EXPECT(RomTypeDetector::generationName(Gen::MSX2) == QStringLiteral("MSX2"));
        EXPECT(RomTypeDetector::generationName(Gen::MSX2plus) == QStringLiteral("MSX2+"));
        EXPECT(RomTypeDetector::generationName(Gen::Unknown) == QStringLiteral("Unknown"));
        EXPECT(RomTypeDetector::mapperName(Map::Plain) == QStringLiteral("Plain"));
        EXPECT(RomTypeDetector::mapperName(Map::Konami) == QStringLiteral("Konami"));
        EXPECT(RomTypeDetector::mapperName(Map::KonamiSCC) == QStringLiteral("Konami SCC"));
        EXPECT(RomTypeDetector::mapperName(Map::Unknown) == QStringLiteral("Unknown"));
    }

    // ----- T9: SHA-1 helper deterministisch + lege string voor lege input -----
    // v0.0.9-YS: opbouw naar softwaredb-hash-match (v0.0.10+).
    {
        // Lege input → lege string (geen SHA1 van niets).
        EXPECT(RomTypeDetector::sha1Hex(QByteArray()).isEmpty());

        // Bekend SHA-1: "abc" → "a9993e364706816aba3e25717850c26c9cd0d89d"
        const QByteArray abc("abc");
        const QString sha = RomTypeDetector::sha1Hex(abc);
        EXPECT(sha.length() == 40);
        EXPECT(sha == QStringLiteral("a9993e364706816aba3e25717850c26c9cd0d89d"));

        // SHA-1 van lege QByteArray van 1 byte (#0x00) ≠ leeg.
        const QByteArray oneZero(1, char(0x00));
        const QString shaZero = RomTypeDetector::sha1Hex(oneZero);
        EXPECT(shaZero.length() == 40);
        EXPECT(shaZero != sha);

        // Bekend SHA-1: 64 nulbytes → fixed hash (deterministisch).
        const QByteArray sixtyFourZeros(64, char(0x00));
        const QString sha64 = RomTypeDetector::sha1Hex(sixtyFourZeros);
        EXPECT(sha64.length() == 40);
        EXPECT(sha64 == QStringLiteral("c8d7d0ef0eedfa82d2ea1aa592845b9a6d4b02b7"));
    }

    // ----- T10: Result.sha1Hex wordt ingevuld in detect() -----
    {
        QByteArray rom(8 * 1024, char(0x42));
        const auto r = RomTypeDetector::detect(rom);
        EXPECT(!r.sha1Hex.isEmpty());
        EXPECT(r.sha1Hex.length() == 40);
        // Determinisme: zelfde input → zelfde sha1.
        const auto r2 = RomTypeDetector::detect(rom);
        EXPECT(r.sha1Hex == r2.sha1Hex);
        // Verschillende input → andere sha1.
        QByteArray rom2(8 * 1024, char(0x43));
        const auto r3 = RomTypeDetector::detect(rom2);
        EXPECT(r3.sha1Hex != r.sha1Hex);
    }

    // ----- T11: lege ROM → r.sha1Hex leeg (detect-pad) -----
    {
        const auto r = RomTypeDetector::detect(QByteArray());
        EXPECT(r.sha1Hex.isEmpty());
    }

    // ----- T12: ASCII8-mapper detect -----
    {
        QByteArray rom(64 * 1024, char(0x00));
        rom.replace(0x200, 3, QByteArray::fromHex("320068"));
        rom.replace(0x400, 3, QByteArray::fromHex("320070"));
        rom.replace(0x600, 3, QByteArray::fromHex("320078"));
        EXPECT(RomTypeDetector::hasAscii8(rom));
        const auto r = RomTypeDetector::detect(rom);
        EXPECT(r.generation == Gen::MSX2);
        EXPECT(r.mapper == Map::Ascii8);
        EXPECT(r.reason.contains(QStringLiteral("ASCII8")));
    }

    // ----- T13: ASCII16-mapper detect (heeft voorrang boven ASCII8 als allebei aanwezig) -----
    {
        QByteArray rom(64 * 1024, char(0x00));
        rom.replace(0x100, 3, QByteArray::fromHex("320060"));  // 0x6000 write → ASCII16-signaal
        rom.replace(0x300, 3, QByteArray::fromHex("320070"));  // 0x7000 write
        EXPECT(RomTypeDetector::hasAscii16(rom));
        const auto r = RomTypeDetector::detect(rom);
        EXPECT(r.mapper == Map::Ascii16);
    }

    qInfo() << "test_romtypedetector: 13/13 cases PASS";
    return 0;
}
