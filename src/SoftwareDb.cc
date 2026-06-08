#include "SoftwareDb.h"

#include <QDebug>
#include <QFile>
#include <QXmlStreamReader>

namespace {
// Map openMSX <system>-veld naar C-BIOS machine-id.
// MSX2 als fallback voor onbekende waarden (safe default, draait de meeste
// software ook al niet 100% accuraat).
QString systemToMachine(const QString &system)
{
    const QString s = system.trimmed();
    if (s == QLatin1String("MSX"))     return QStringLiteral("C-BIOS_MSX1");
    if (s == QLatin1String("MSX2"))    return QStringLiteral("C-BIOS_MSX2");
    if (s == QLatin1String("MSX2+"))   return QStringLiteral("C-BIOS_MSX2+");
    return QStringLiteral("C-BIOS_MSX2");
}
} // namespace

SoftwareDb::SoftwareDb(QObject *parent)
    : QObject(parent)
{
}

void SoftwareDb::addEntry(const QString &sha1Hex, const QString &machine, const QString &title)
{
    const QString key = sha1Hex.toLower();
    if (key.isEmpty()) return;
    m_machines.insert(key, machine);
    m_titles.insert(key, title);
    emit changed();
}

QString SoftwareDb::lookupMachine(const QString &sha1Hex) const
{
    return m_machines.value(sha1Hex.toLower());
}

QString SoftwareDb::lookupTitle(const QString &sha1Hex) const
{
    return m_titles.value(sha1Hex.toLower());
}

void SoftwareDb::clearAll()
{
    if (m_machines.isEmpty() && m_titles.isEmpty()) return;
    m_machines.clear();
    m_titles.clear();
    emit changed();
}

int SoftwareDb::loadFromXmlFile(const QString &path)
{
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "SoftwareDb::loadFromXmlFile: kan niet openen:" << path << f.errorString();
        return -1;
    }

    QXmlStreamReader xml(&f);

    // Per-software accumulators. Eén <software>-blok kan meerdere <dump>'s
    // hebben (rom, megarom, etc.) en meerdere <hash>'s; we registreren elke
    // sha1-hash apart met dezelfde title + machine.
    QString curTitle;
    QString curSystem;
    QStringList curSha1s;
    int loaded = 0;
    bool inSoftware = false;

    auto flushSoftware = [&]() {
        if (!inSoftware) return;
        const QString machine = systemToMachine(curSystem);
        for (const QString &sha : std::as_const(curSha1s)) {
            const QString key = sha.trimmed().toLower();
            if (key.isEmpty()) continue;
            m_machines.insert(key, machine);
            m_titles.insert(key, curTitle);
            ++loaded;
        }
        curTitle.clear();
        curSystem.clear();
        curSha1s.clear();
        inSoftware = false;
    };

    while (!xml.atEnd() && !xml.hasError()) {
        const auto tt = xml.readNext();
        if (tt == QXmlStreamReader::StartElement) {
            const auto name = xml.name();
            if (name == QLatin1String("software")) {
                inSoftware = true;
                curTitle.clear();
                curSystem.clear();
                curSha1s.clear();
            } else if (inSoftware && name == QLatin1String("title")) {
                curTitle = xml.readElementText().trimmed();
            } else if (inSoftware && name == QLatin1String("system")) {
                curSystem = xml.readElementText().trimmed();
            } else if (inSoftware && name == QLatin1String("hash")) {
                // openMSX softwaredb: <hash algo="sha1">...</hash>. Alleen sha1.
                const auto attrs = xml.attributes();
                const QString algo = attrs.value(QLatin1String("algo")).toString();
                const QString val  = xml.readElementText().trimmed();
                if (algo.compare(QLatin1String("sha1"), Qt::CaseInsensitive) == 0 && !val.isEmpty()) {
                    curSha1s.append(val);
                }
            }
        } else if (tt == QXmlStreamReader::EndElement) {
            if (xml.name() == QLatin1String("software")) {
                flushSoftware();
            }
        }
    }

    if (xml.hasError()) {
        qWarning() << "SoftwareDb::loadFromXmlFile: XML parse-fout:" << xml.errorString()
                   << "@ regel" << xml.lineNumber();
        return -1;
    }

    if (loaded > 0) emit changed();
    return loaded;
}

void SoftwareDb::loadBootstrapData()
{
    // Plausibele maar fictieve hashes. Echt softwaredb-data komt via
    // loadFromXmlFile() in productie. Hier alleen format-validatie + UI-smoke.
    addEntry("aabbccddeeff00112233445566778899aabbccdd", "C-BIOS_MSX1", "Bubble Bobble (sample)");
    addEntry("1122334455667788990011223344556677889900", "C-BIOS_MSX2", "Metal Gear (sample)");
    addEntry("ffeeddccbbaa99887766554433221100ffeeddcc", "C-BIOS_MSX2", "Nemesis 2 (sample)");
    addEntry("0011223344556677889900112233445566778899", "C-BIOS_MSX1", "Penguin Adventure (sample)");
    addEntry("c8d7d0ef0eedfa82d2ea1aa592845b9a6d4b02b7", "C-BIOS_MSX2", "Salamander (sample)");
}
