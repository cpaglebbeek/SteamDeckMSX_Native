// test_romlibrary — v0.3.0-MazeOfGalious.
//
// De scanner draait incrementeel via een 0ms-timer, dus elke test moet de
// event-loop laten lopen tot scanFinished. Dat is bewust: het bewijst meteen
// dat de scan de UI-thread niet blokkeert en netjes afrondt.

#include "RomLibrary.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QSignalSpy>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QtTest>

class TestRomLibrary : public QObject {
    Q_OBJECT

private:
    QTemporaryDir m_appData;   // isoleert cache + thumbs van de echte installatie

    static void writeRom(const QString &path, const QByteArray &content)
    {
        QDir().mkpath(QFileInfo(path).absolutePath());
        QFile f(path);
        QVERIFY(f.open(QIODevice::WriteOnly));
        f.write(content);
    }

    // Laat de scan lopen tot hij klaar is. Faalt hard bij een hang, zodat een
    // kapotte tick-lus niet als time-out van de hele suite eindigt.
    static bool waitForScan(RomLibrary &lib)
    {
        QSignalSpy spy(&lib, &RomLibrary::scanFinished);
        lib.rescan();
        return spy.wait(15000);
    }

private slots:
    void initTestCase()
    {
        QVERIFY(m_appData.isValid());
        QStandardPaths::setTestModeEnabled(true);
        QCoreApplication::setOrganizationName(QStringLiteral("iCtHorseTest"));
        QCoreApplication::setApplicationName(QStringLiteral("SteamDeckMSXTest"));
    }

    void scansSupportedExtensionsOnly()
    {
        QTemporaryDir roms;
        QVERIFY(roms.isValid());
        writeRom(roms.filePath("game1.rom"), QByteArray(16 * 1024, 'A'));
        writeRom(roms.filePath("disk1.dsk"), QByteArray(1024, 'B'));
        writeRom(roms.filePath("tape1.cas"), QByteArray(1024, 'C'));
        writeRom(roms.filePath("readme.txt"), QByteArray("niet meenemen"));
        writeRom(roms.filePath("cover.png"), QByteArray(64, 'D'));

        RomLibrary lib;
        lib.setScanRoots({roms.path()});
        QVERIFY(waitForScan(lib));

        QCOMPARE(lib.rowCount(), 3);   // txt + png overgeslagen
    }

    // Regressie: een eerste versie nam .zip overal mee en scande Documenten.
    // Op een echte machine leverde dat 159 valse treffers op (dossiers,
    // WeTransfer-bundels, screenshots) die als "spel" in de galerij stonden.
    void ignoresZipOutsideRomFolders()
    {
        QTemporaryDir base;
        QVERIFY(base.isValid());
        // Expliciet in een submap met een neutrale naam: de tijdelijke map
        // zelf draagt de applicatienaam, en die mag geen invloed hebben.
        writeRom(base.filePath("Documenten/dossier.zip"), QByteArray(2048, 'Z'));
        writeRom(base.filePath("Documenten/vakantiefotos.zip"), QByteArray(2048, 'Y'));
        writeRom(base.filePath("Documenten/echtspel.rom"), QByteArray(2048, 'R'));

        RomLibrary lib;
        lib.setScanRoots({base.filePath("Documenten")});
        QVERIFY(waitForScan(lib));

        QCOMPARE(lib.rowCount(), 1);
        QCOMPARE(lib.entryAt(0).value("title").toString(), QStringLiteral("echtspel"));
    }

    void acceptsZipInsideRomFolder()
    {
        QTemporaryDir base;
        QVERIFY(base.isValid());
        writeRom(base.filePath("MSX-roms/verzameling.zip"), QByteArray(2048, 'Z'));

        RomLibrary lib;
        lib.setScanRoots({base.path()});
        QVERIFY(waitForScan(lib));
        QCOMPARE(lib.rowCount(), 1);
    }

    void skipsHiddenAndHeavyDirectories()
    {
        QTemporaryDir base;
        QVERIFY(base.isValid());
        writeRom(base.filePath("zichtbaar.rom"), QByteArray(1024, 'A'));
        writeRom(base.filePath(".git/objects/verstopt.rom"), QByteArray(1024, 'B'));
        writeRom(base.filePath("node_modules/pakket/mee.rom"), QByteArray(1024, 'C'));

        RomLibrary lib;
        lib.setScanRoots({base.path()});
        QVERIFY(waitForScan(lib));

        // Alleen het zichtbare bestand; .git en node_modules worden overgeslagen
        // omdat een scan daar anders in vastloopt.
        QCOMPARE(lib.rowCount(), 1);
        QCOMPARE(lib.entryAt(0).value("title").toString(), QStringLiteral("zichtbaar"));
    }

    void findsRomsInSubdirectories()
    {
        QTemporaryDir roms;
        QVERIFY(roms.isValid());
        writeRom(roms.filePath("top.rom"), QByteArray(1024, 'A'));
        writeRom(roms.filePath("konami/nemesis.rom"), QByteArray(1024, 'B'));
        writeRom(roms.filePath("konami/scc/salamander.rom"), QByteArray(1024, 'C'));

        RomLibrary lib;
        lib.setScanRoots({roms.path()});
        QVERIFY(waitForScan(lib));

        QCOMPARE(lib.rowCount(), 3);
    }

    void deduplicatesIdenticalContent()
    {
        QTemporaryDir roms;
        QVERIFY(roms.isValid());
        const QByteArray same(2048, 'X');
        writeRom(roms.filePath("nemesis.rom"), same);
        writeRom(roms.filePath("backup/nemesis-copy.rom"), same);
        writeRom(roms.filePath("ander.rom"), QByteArray(2048, 'Y'));

        RomLibrary lib;
        lib.setScanRoots({roms.path()});
        QVERIFY(waitForScan(lib));

        // Dezelfde dump twee keer op schijf is één spel in de galerij.
        QCOMPARE(lib.rowCount(), 2);
    }

    void stripsBracketedNoiseFromTitle()
    {
        QTemporaryDir roms;
        QVERIFY(roms.isValid());
        writeRom(roms.filePath("Nemesis 2 (1987)(Konami)[SCC].rom"), QByteArray(1024, 'A'));

        RomLibrary lib;
        lib.setScanRoots({roms.path()});
        QVERIFY(waitForScan(lib));
        QCOMPARE(lib.rowCount(), 1);

        const QVariantMap e = lib.entryAt(0);
        QCOMPARE(e.value("title").toString(), QStringLiteral("Nemesis 2"));
    }

    void detectsMediaTypePerExtension()
    {
        QTemporaryDir roms;
        QVERIFY(roms.isValid());
        writeRom(roms.filePath("a.dsk"), QByteArray(1024, 'A'));

        RomLibrary lib;
        lib.setScanRoots({roms.path()});
        QVERIFY(waitForScan(lib));
        QCOMPARE(lib.entryAt(0).value("mediaType").toString(), QStringLiteral("dsk"));
    }

    void thumbnailQueueSkipsEntriesThatAlreadyHaveOne()
    {
        QTemporaryDir roms;
        QVERIFY(roms.isValid());
        writeRom(roms.filePath("een.rom"), QByteArray(1024, 'A'));
        writeRom(roms.filePath("twee.rom"), QByteArray(1024, 'B'));

        RomLibrary lib;
        lib.setScanRoots({roms.path()});
        QVERIFY(waitForScan(lib));
        QCOMPARE(lib.entriesWithoutThumbnail().size(), 2);

        const QString sha1 = lib.entryAt(0).value("sha1").toString();
        QVERIFY(!sha1.isEmpty());
        lib.setThumbnail(sha1, QStringLiteral("/tmp/nep-thumb.png"));

        QCOMPARE(lib.entriesWithoutThumbnail().size(), 1);
    }

    void rescanKeepsThumbnailsForUnchangedFiles()
    {
        QTemporaryDir roms;
        QVERIFY(roms.isValid());
        writeRom(roms.filePath("stabiel.rom"), QByteArray(1024, 'A'));

        RomLibrary lib;
        lib.setScanRoots({roms.path()});
        QVERIFY(waitForScan(lib));

        const QString sha1 = lib.entryAt(0).value("sha1").toString();
        lib.setThumbnail(sha1, QStringLiteral("/tmp/nep-thumb.png"));
        QCOMPARE(lib.entryAt(0).value("thumbPath").toString(), QStringLiteral("/tmp/nep-thumb.png"));

        // Een tweede scan mag het werk van de generator niet weggooien —
        // anders bouwt elke start de hele galerij opnieuw op.
        QVERIFY(waitForScan(lib));
        QCOMPARE(lib.entryAt(0).value("thumbPath").toString(), QStringLiteral("/tmp/nep-thumb.png"));
    }

    // v0.3.1: entries verschijnen tijdens de scan. Een scan van een hele
    // home-map duurt tientallen seconden; alles ophouden tot het einde liet de
    // galerij al die tijd leeg — voor de gebruiker niet te onderscheiden van
    // "er is niets gevonden".
    void insertsEntriesDuringScanNotOnlyAtTheEnd()
    {
        // Ruim boven het tick-budget, én allemaal in ÉÉN map: een MSX-collectie
        // is vaak precies dat. Een eerdere versie hield het budget alleen
        // tússen mappen aan en hashte zo een hele map in één tick — dan
        // blokkeert de UI alsnog en verschijnt alles pas aan het eind.
        QTemporaryDir roms;
        QVERIFY(roms.isValid());
        for (int i = 0; i < 80; ++i) {
            writeRom(roms.filePath(QStringLiteral("spel%1.rom").arg(i, 3, 10, QChar('0'))),
                     QByteArray(512, static_cast<char>('A' + (i % 26))) + QByteArray::number(i));
        }

        RomLibrary lib;
        lib.setScanRoots({roms.path()});

        QSignalSpy inserted(&lib, &RomLibrary::rowsInserted);
        QSignalSpy finished(&lib, &RomLibrary::scanFinished);
        lib.rescan();
        // Rijen moeten al binnenkomen vóórdat de scan klaar is.
        QVERIFY(inserted.wait(5000));
        QCOMPARE(finished.count(), 0);
        QVERIFY(lib.rowCount() > 0);

        QVERIFY(finished.wait(15000));
        QCOMPARE(lib.rowCount(), 80);
    }

    void rescanRemovesEntriesWhoseFileDisappeared()
    {
        QTemporaryDir roms;
        QVERIFY(roms.isValid());
        writeRom(roms.filePath("blijft.rom"), QByteArray(1024, 'A'));
        writeRom(roms.filePath("verdwijnt.rom"), QByteArray(1024, 'B'));

        RomLibrary lib;
        lib.setScanRoots({roms.path()});
        QVERIFY(waitForScan(lib));
        QCOMPARE(lib.rowCount(), 2);

        QVERIFY(QFile::remove(roms.filePath("verdwijnt.rom")));
        QVERIFY(waitForScan(lib));

        QCOMPARE(lib.rowCount(), 1);
        QCOMPARE(lib.entryAt(0).value("title").toString(), QStringLiteral("blijft"));
    }

    void emptyRootYieldsEmptyModelWithoutHanging()
    {
        QTemporaryDir empty;
        QVERIFY(empty.isValid());

        RomLibrary lib;
        lib.setScanRoots({empty.path()});
        QVERIFY(waitForScan(lib));
        QCOMPARE(lib.rowCount(), 0);
    }

    void nonExistentRootIsIgnored()
    {
        RomLibrary lib;
        lib.setScanRoots({QStringLiteral("/pad/dat/echt/niet/bestaat")});
        QVERIFY(waitForScan(lib));
        QCOMPARE(lib.rowCount(), 0);
    }
};

QTEST_MAIN(TestRomLibrary)
#include "test_romlibrary.moc"
