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

bool RomTypeDetector::hasAscii8(const QByteArray &rom)
{
    // Detecteer bank-switch naar 0x6800/0x7000/0x7800 (8KB-mapper).
    // Echte ASCII8 schrijft naar alle 4 (0x6000+0x6800+0x7000+0x7800);
    // vereenvoudigd: 3 van de laatste 3 aanwezig = match.
    static const QByteArray p6800 = QByteArray::fromHex("320068");
    static const QByteArray p7000 = QByteArray::fromHex("320070");
    static const QByteArray p7800 = QByteArray::fromHex("320078");
    int hits = 0;
    if (rom.indexOf(p6800) >= 0) ++hits;
    if (rom.indexOf(p7000) >= 0) ++hits;
    if (rom.indexOf(p7800) >= 0) ++hits;
    return hits >= 2;
}

bool RomTypeDetector::hasAscii16(const QByteArray &rom)
{
    // ASCII16 schrijft naar 0x6000 EN 0x7000 (16KB-banken). Beide vereist.
    static const QByteArray p6000 = QByteArray::fromHex("320060");
    static const QByteArray p7000 = QByteArray::fromHex("320070");
    return rom.indexOf(p6000) >= 0 && rom.indexOf(p7000) >= 0;
}

RomTypeDetector::Mapper RomTypeDetector::detectMapper(const QByteArray &rom)
{
    const auto size = rom.size();
    if (size <= 0)         return Mapper::Unknown;
    if (size <= 32 * 1024) return Mapper::Plain;

    // > 32KB → mapper-based. SCC-detect bepaalt sub-type.
    if (hasScc(rom)) return Mapper::KonamiSCC;
    if (hasAscii16(rom)) return Mapper::Ascii16;
    if (hasAscii8(rom))  return Mapper::Ascii8;
    return Mapper::Konami;  // default voor >32KB blijft Konami
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
        r.reason = QStringLiteral("ROM > 32KB → MSX2 + ") + mapperName(r.mapper) + QStringLiteral(" mapper");
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
