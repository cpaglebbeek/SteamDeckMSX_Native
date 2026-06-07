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

    qInfo() << "test_romtypedetector: 8/8 cases PASS";
    return 0;
}
