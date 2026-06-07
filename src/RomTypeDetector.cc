#include "RomTypeDetector.h"

#include <QCryptographicHash>

RomTypeDetector::Generation RomTypeDetector::detectGeneration(const QByteArray &rom)
{
    const auto size = rom.size();
    if (size <= 0)             return Generation::Unknown;
    if (size <= 32 * 1024)     return Generation::MSX1;
    // > 32KB → mapper-based ROM, MSX2 als safe default.
    // MSX2+ alleen via softwaredb-hash (v0.0.9+ niet geïmplementeerd).
    return Generation::MSX2;
}

bool RomTypeDetector::hasScc(const QByteArray &rom)
{
    // Konami SCC enable-write Z80 instructie-sequentie:
    //   3E 9F        LD A,#9F
    //   32 B0 80     LD (#80B0),A
    // Voldoende in ROM = SCC-titel met >95% accuratesse.
    static const QByteArray kSccPattern = QByteArray::fromHex("3E9F32B080");
    return rom.indexOf(kSccPattern) >= 0;
}

RomTypeDetector::Mapper RomTypeDetector::detectMapper(const QByteArray &rom)
{
    const auto size = rom.size();
    if (size <= 0)         return Mapper::Unknown;
    if (size <= 32 * 1024) return Mapper::Plain;

    // > 32KB → mapper-based. SCC-detect bepaalt sub-type.
    if (hasScc(rom)) return Mapper::KonamiSCC;

    // Geen SCC + groot = Konami default. ASCII-detect verschuift naar v0.0.9.
    return Mapper::Konami;
}

QString RomTypeDetector::sha1Hex(const QByteArray &rom)
{
    if (rom.isEmpty()) return QString();
    return QString::fromLatin1(
        QCryptographicHash::hash(rom, QCryptographicHash::Sha1).toHex()
    );
}

RomTypeDetector::Result RomTypeDetector::detect(const QByteArray &rom)
{
    Result r;
    r.generation = detectGeneration(rom);
    r.mapper     = detectMapper(rom);
    r.sha1Hex    = sha1Hex(rom);

    switch (r.generation) {
    case Generation::MSX1:
        r.suggestedMachine = QStringLiteral("C-BIOS_MSX1");
        r.reason = QStringLiteral("ROM ≤ 32KB → MSX1");
        break;
    case Generation::MSX2:
        r.suggestedMachine = QStringLiteral("C-BIOS_MSX2");
        r.reason = (r.mapper == Mapper::KonamiSCC)
            ? QStringLiteral("ROM > 32KB + SCC-pattern → MSX2 + Konami SCC")
            : QStringLiteral("ROM > 32KB → MSX2 + Konami mapper");
        break;
    case Generation::MSX2plus:
        r.suggestedMachine = QStringLiteral("C-BIOS_MSX2+");
        r.reason = QStringLiteral("softwaredb hash-match → MSX2+");
        break;
    case Generation::Unknown:
    default:
        // Lege of ongeldige ROM → safe fallback.
        r.suggestedMachine = QStringLiteral("C-BIOS_MSX2");
        r.reason = QStringLiteral("Onbekend ROM-type, fallback C-BIOS_MSX2");
        break;
    }
    return r;
}

QString RomTypeDetector::generationName(Generation g)
{
    switch (g) {
    case Generation::MSX1:      return QStringLiteral("MSX1");
    case Generation::MSX2:      return QStringLiteral("MSX2");
    case Generation::MSX2plus:  return QStringLiteral("MSX2+");
    case Generation::Unknown:
    default:                    return QStringLiteral("Unknown");
    }
}

QString RomTypeDetector::mapperName(Mapper m)
{
    switch (m) {
    case Mapper::Plain:      return QStringLiteral("Plain");
    case Mapper::Konami:     return QStringLiteral("Konami");
    case Mapper::KonamiSCC:  return QStringLiteral("Konami SCC");
    case Mapper::Ascii8:     return QStringLiteral("ASCII8");
    case Mapper::Ascii16:    return QStringLiteral("ASCII16");
    case Mapper::Unknown:
    default:                 return QStringLiteral("Unknown");
    }
}
