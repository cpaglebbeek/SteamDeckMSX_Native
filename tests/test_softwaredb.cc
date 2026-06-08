// Unit-tests voor SoftwareDb — SHA-1 → machine lookup uit openMSX softwaredb.xml.
//
// Test-corpus:
//   T1: leeg DB → entryCount==0, lookup geeft ""
//   T2: addEntry + lookupMachine == machine
//   T3: case-insensitive lookup (HOOFDLETTERS-input → lookup via lowercase + andersom)
//   T4: loadBootstrapData → entryCount >= 5
//   T5: loadFromXmlFile met QTemporaryFile (2 software-entries, parse + verifieer)
//   T6: clearAll → entryCount==0

#include "../src/SoftwareDb.h"

#include <QByteArray>
#include <QCoreApplication>
#include <QDebug>
#include <QTemporaryFile>

#define EXPECT(cond) do { \
    if (!(cond)) { qCritical() << "FAIL line" << __LINE__ << ":" << #cond; std::exit(1); } \
} while (0)

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    // ----- T1: leeg DB -----
    {
        SoftwareDb db;
        EXPECT(db.entryCount() == 0);
        EXPECT(db.lookupMachine(QStringLiteral("aabbccddeeff00112233445566778899aabbccdd")).isEmpty());
        EXPECT(db.lookupTitle(QStringLiteral("aabbccddeeff00112233445566778899aabbccdd")).isEmpty());
    }

    // ----- T2: addEntry + lookup -----
    {
        SoftwareDb db;
        db.addEntry(QStringLiteral("aabbccddeeff00112233445566778899aabbccdd"),
                    QStringLiteral("C-BIOS_MSX1"),
                    QStringLiteral("Bubble Bobble"));
        EXPECT(db.entryCount() == 1);
        EXPECT(db.lookupMachine(QStringLiteral("aabbccddeeff00112233445566778899aabbccdd"))
               == QStringLiteral("C-BIOS_MSX1"));
        EXPECT(db.lookupTitle(QStringLiteral("aabbccddeeff00112233445566778899aabbccdd"))
               == QStringLiteral("Bubble Bobble"));
    }

    // ----- T3: case-insensitive lookup -----
    {
        SoftwareDb db;
        // Toevoegen met HOOFDLETTERS — moet intern lowercase worden opgeslagen.
        db.addEntry(QStringLiteral("AABBCCDDEEFF00112233445566778899AABBCCDD"),
                    QStringLiteral("C-BIOS_MSX2"),
                    QStringLiteral("Metal Gear"));
        // Lookup met lowercase → moet werken (intern == lowercase).
        EXPECT(db.lookupMachine(QStringLiteral("aabbccddeeff00112233445566778899aabbccdd"))
               == QStringLiteral("C-BIOS_MSX2"));
        // Lookup met HOOFDLETTERS → ook werken (lookup-pad lowercase't input).
        EXPECT(db.lookupMachine(QStringLiteral("AABBCCDDEEFF00112233445566778899AABBCCDD"))
               == QStringLiteral("C-BIOS_MSX2"));
        // Mixed-case → ook werken.
        EXPECT(db.lookupMachine(QStringLiteral("AaBbCcDdEeFf00112233445566778899aAbBcCdD"))
               == QStringLiteral("C-BIOS_MSX2"));
    }

    // ----- T4: loadBootstrapData -----
    {
        SoftwareDb db;
        db.loadBootstrapData();
        EXPECT(db.entryCount() >= 5);
        // Specifiek de Salamander-entry uit bootstrap moet vindbaar zijn.
        EXPECT(db.lookupMachine(QStringLiteral("c8d7d0ef0eedfa82d2ea1aa592845b9a6d4b02b7"))
               == QStringLiteral("C-BIOS_MSX2"));
        EXPECT(db.lookupTitle(QStringLiteral("c8d7d0ef0eedfa82d2ea1aa592845b9a6d4b02b7"))
               .contains(QStringLiteral("Salamander")));
    }

    // ----- T5: loadFromXmlFile met inline temp XML -----
    {
        QTemporaryFile tmp;
        tmp.setAutoRemove(true);
        EXPECT(tmp.open());

        // Twee software-entries: MSX1 (rom/Plain) + MSX2 (megarom/KonamiSCC).
        // Hashes lowercase 40-char. openMSX-formaat met <hash algo="sha1">.
        const QByteArray xml =
            "<?xml version=\"1.0\"?>\n"
            "<softwaredb>\n"
            "  <software>\n"
            "    <title>Bubble Bobble</title>\n"
            "    <dump>\n"
            "      <rom>\n"
            "        <hash algo=\"sha1\">8de10b9d1b97e9c6c2c1ec7b6b6f5b0d3a5e4c1b</hash>\n"
            "        <type>Plain</type>\n"
            "      </rom>\n"
            "    </dump>\n"
            "    <system>MSX</system>\n"
            "  </software>\n"
            "  <software>\n"
            "    <title>Nemesis 2</title>\n"
            "    <dump>\n"
            "      <megarom>\n"
            "        <hash algo=\"sha1\">ffeeddccbbaa99887766554433221100ffeeddcc</hash>\n"
            "        <type>KonamiSCC</type>\n"
            "      </megarom>\n"
            "    </dump>\n"
            "    <system>MSX2</system>\n"
            "  </software>\n"
            "</softwaredb>\n";

        tmp.write(xml);
        tmp.flush();
        const QString path = tmp.fileName();

        SoftwareDb db;
        const int loaded = db.loadFromXmlFile(path);
        EXPECT(loaded == 2);
        EXPECT(db.entryCount() == 2);

        // Bubble Bobble → C-BIOS_MSX1 (system=MSX).
        EXPECT(db.lookupMachine(QStringLiteral("8de10b9d1b97e9c6c2c1ec7b6b6f5b0d3a5e4c1b"))
               == QStringLiteral("C-BIOS_MSX1"));
        EXPECT(db.lookupTitle(QStringLiteral("8de10b9d1b97e9c6c2c1ec7b6b6f5b0d3a5e4c1b"))
               == QStringLiteral("Bubble Bobble"));

        // Nemesis 2 → C-BIOS_MSX2 (system=MSX2).
        EXPECT(db.lookupMachine(QStringLiteral("ffeeddccbbaa99887766554433221100ffeeddcc"))
               == QStringLiteral("C-BIOS_MSX2"));
        EXPECT(db.lookupTitle(QStringLiteral("ffeeddccbbaa99887766554433221100ffeeddcc"))
               == QStringLiteral("Nemesis 2"));

        // Niet-bestaande hash → lege string.
        EXPECT(db.lookupMachine(QStringLiteral("0000000000000000000000000000000000000000")).isEmpty());
    }

    // ----- T6: clearAll -----
    {
        SoftwareDb db;
        db.loadBootstrapData();
        EXPECT(db.entryCount() > 0);
        db.clearAll();
        EXPECT(db.entryCount() == 0);
        EXPECT(db.lookupMachine(QStringLiteral("c8d7d0ef0eedfa82d2ea1aa592845b9a6d4b02b7")).isEmpty());
    }

    qInfo() << "test_softwaredb: 6/6 cases PASS";
    return 0;
}
