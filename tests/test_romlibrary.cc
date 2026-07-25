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
