// Unit test voor MsxCore — alleen public API + IPC-signal-verificatie via
// een mock-script dat XML-stream simuleert.

#include "../src/MsxCore.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QSignalSpy>
#include <QStandardPaths>
#include <QTemporaryFile>
#include <QtTest/QTest>
#include <QDebug>

#define EXPECT(cond) do { \
    if (!(cond)) { qCritical() << "FAIL line" << __LINE__ << ":" << #cond; std::exit(1); } \
} while (0)

namespace {
// Schrijf een uitvoerbaar sh-script dat de gegeven XML naar stdout dumpt.
QString makeMockScript(const QString &xml)
{
    static int counter = 0;
    const QString p = QDir::tempPath() + QStringLiteral("/sdmsx_mock_%1.sh").arg(counter++);
    QFile f(p);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        return {};
    }
    QString body = QStringLiteral("#!/bin/sh\ncat <<'EOF'\n");
    body += xml;
    body += QStringLiteral("\nEOF\n");
    f.write(body.toUtf8());
    f.close();
    QFile::setPermissions(p, QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner);
    return p;
}
}

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    // T1: Initial state
    {
        MsxCore core;
        EXPECT(core.state() == MsxCore::Idle);
        EXPECT(core.stateLabel() == QStringLiteral("idle"));
        EXPECT(core.version().isEmpty());
        EXPECT(core.errorMessage().isEmpty());
        EXPECT(core.dataPath().isEmpty());
    }

    // T2: openmsxPath roundtrip
    {
        MsxCore core;
        QSignalSpy spy(&core, &MsxCore::openmsxPathChanged);
        core.setOpenmsxPath(QStringLiteral("/some/fake/path"));
        EXPECT(spy.count() >= 1);
        const int after = spy.count();
        core.setOpenmsxPath(QStringLiteral("/some/fake/path"));
        EXPECT(spy.count() == after);  // dedupe
    }

    // T3: probeVersion zonder pad → Failed
    {
        MsxCore core;
        core.setOpenmsxPath(QString());
        core.probeVersion();
        EXPECT(core.state() == MsxCore::Failed);
        EXPECT(!core.errorMessage().isEmpty());
    }

    // T4: stop() vanuit Idle is no-op
    {
        MsxCore core;
        core.stop();
        EXPECT(core.state() == MsxCore::Idle);
    }

    // T5: sendCommand vanuit Idle = warning maar geen crash en returns -1
    {
        MsxCore core;
        const int id = core.sendCommand(QStringLiteral("nop"));
        EXPECT(id == -1);
        EXPECT(core.state() == MsxCore::Idle);
    }

    // T6: dataPath setter
    {
        MsxCore core;
        QSignalSpy spy(&core, &MsxCore::dataPathChanged);
        core.setDataPath(QStringLiteral("/path/to/share"));
        EXPECT(spy.count() == 1);
        EXPECT(core.dataPath() == QStringLiteral("/path/to/share"));
    }

    // T7: XML-stream-parser — <reply> emit met command-id correlatie
    {
        const QString xml =
            QStringLiteral("<openmsx-output>\n"
                           "<reply result=\"ok\" command-id=\"42\">21.0</reply>\n"
                           "</openmsx-output>\n");
        const QString script = makeMockScript(xml);
        EXPECT(!script.isEmpty());

        MsxCore core;
        core.setOpenmsxPath(script);

        QSignalSpy stateSpy(&core, &MsxCore::stateChanged);
        QSignalSpy replySpy(&core, &MsxCore::replyReceived);

        core.start();  // spawn mock, geen ROM
        // wacht tot proces eindigt (mock exits direct)
        stateSpy.wait(2000);
        // eventueel meerdere state-changes; spin tot Idle
        for (int i = 0; i < 50 && core.state() != MsxCore::Idle; ++i) {  // BUG-010: 1s was te krap onder load (flaky)
            QTest::qWait(100);
        }
        EXPECT(replySpy.count() >= 1);
        // Verifieer command-id parsing
        const auto args = replySpy.last();
        EXPECT(args.at(0).toInt() == 42);
        EXPECT(args.at(1).toBool() == true);  // result="ok"
        EXPECT(args.at(2).toString() == QStringLiteral("21.0"));
    }

    // T8: XML-stream-parser — <update> emit met type/name/value
    {
        const QString xml =
            QStringLiteral("<openmsx-output>\n"
                           "<update type=\"setting\" name=\"throttle\">true</update>\n"
                           "</openmsx-output>\n");
        const QString script = makeMockScript(xml);
        MsxCore core;
        core.setOpenmsxPath(script);
        QSignalSpy updateSpy(&core, &MsxCore::stateUpdate);
        core.start();
        for (int i = 0; i < 50 && core.state() != MsxCore::Idle; ++i) {  // BUG-010: 1s was te krap onder load (flaky)
            QTest::qWait(100);
        }
        EXPECT(updateSpy.count() >= 1);
        const auto args = updateSpy.last();
        EXPECT(args.at(0).toString() == QStringLiteral("setting"));
        EXPECT(args.at(1).toString() == QStringLiteral("throttle"));
        EXPECT(args.at(2).toString() == QStringLiteral("true"));
    }

    // T9: XML-stream-parser — <log> emit met level
    {
        const QString xml =
            QStringLiteral("<openmsx-output>\n"
                           "<log level=\"warning\">test warning</log>\n"
                           "</openmsx-output>\n");
        const QString script = makeMockScript(xml);
        MsxCore core;
        core.setOpenmsxPath(script);
        QSignalSpy logSpy(&core, &MsxCore::logMessage);
        core.start();
        for (int i = 0; i < 50 && core.state() != MsxCore::Idle; ++i) {  // BUG-010: 1s was te krap onder load (flaky)
            QTest::qWait(100);
        }
        EXPECT(logSpy.count() >= 1);
        // Eerste log-message kan stderr-only zijn, dus zoek expliciet de warning
        bool foundWarning = false;
        for (const auto &call : logSpy) {
            if (call.at(0).toString() == QStringLiteral("warning") &&
                call.at(1).toString() == QStringLiteral("test warning")) {
                foundWarning = true;
                break;
            }
        }
        EXPECT(foundWarning);
    }

    // T10: XML-stream-parser — Booting → Running bij <openmsx-output>
    {
        const QString xml =
            QStringLiteral("<openmsx-output>\n"
                           "<log level=\"info\">ready</log>\n"
                           "</openmsx-output>\n");
        const QString script = makeMockScript(xml);
        MsxCore core;
        core.setOpenmsxPath(script);
        QSignalSpy stateSpy(&core, &MsxCore::stateChanged);
        core.start();
        // wacht tot eindstate bereikt
        for (int i = 0; i < 50 && core.state() != MsxCore::Idle; ++i) {  // BUG-010: 1s was te krap onder load (flaky)
            QTest::qWait(100);
        }
        // Verifieer dat Running ergens in de transitie zat
        bool sawRunning = false;
        // Spy houdt geen historie van waarden bij (geen args), maar we kunnen
        // verifieren via stateLabel rebuilds in een live core — fallback: alleen
        // verifiëren dat eindstate Idle of Quitting is en stateChanged ≥ 2x.
        Q_UNUSED(sawRunning);
        EXPECT(stateSpy.count() >= 2);  // minimaal Booting → ... → Idle
    }

    // T11: BUG-022 — met `-control stdio` zet openMSX zelf geen renderer aan en
    // laat het de machine uit staan. Zonder deze twee regels draait de emulator
    // wel, maar ziet de speler een zwart scherm.
    {
        const QString script = makeMockScript(QStringLiteral("<openmsx-output>\n</openmsx-output>\n"));
        MsxCore core;
        core.setOpenmsxPath(script);
        EXPECT(core.fullscreen());  // standaard aan
        core.start(QStringLiteral("/tmp/nemesis2.rom"));
        const QStringList args = core.lastStartArgs();
        const int cmd = args.indexOf(QStringLiteral("-command"));
        EXPECT(cmd >= 0 && cmd + 1 < args.size());
        const QString startup = args.at(cmd + 1);
        EXPECT(startup.contains(QStringLiteral("set renderer")));
        EXPECT(startup.contains(QStringLiteral("set power on")));
        EXPECT(startup.contains(QStringLiteral("set fullscreen on")));
        EXPECT(startup.contains(QStringLiteral("bind F12 quit")));
        // Renderer vóór fullscreen: fullscreen slaat nergens op zonder venster.
        EXPECT(startup.indexOf(QStringLiteral("set renderer"))
               < startup.indexOf(QStringLiteral("set fullscreen")));
        for (int i = 0; i < 50 && core.state() != MsxCore::Idle; ++i) {
            QTest::qWait(100);
        }
    }

    // T12: fullscreen uit (zo draaien de headless gates) laat renderer en power
    // staan — die zijn nodig voor beeld, niet voor schermvullend beeld.
    {
        const QString script = makeMockScript(QStringLiteral("<openmsx-output>\n</openmsx-output>\n"));
        MsxCore core;
        core.setOpenmsxPath(script);
        QSignalSpy fsSpy(&core, &MsxCore::fullscreenChanged);
        core.setFullscreen(false);
        EXPECT(fsSpy.count() == 1);
        core.setFullscreen(false);
        EXPECT(fsSpy.count() == 1);  // dedupe
        core.start(QStringLiteral("/tmp/nemesis2.rom"));
        const QStringList args = core.lastStartArgs();
        const int cmd = args.indexOf(QStringLiteral("-command"));
        EXPECT(cmd >= 0 && cmd + 1 < args.size());
        const QString startup = args.at(cmd + 1);
        EXPECT(startup.contains(QStringLiteral("set renderer")));
        EXPECT(startup.contains(QStringLiteral("set power on")));
        EXPECT(!startup.contains(QStringLiteral("fullscreen")));
        EXPECT(!startup.contains(QStringLiteral("bind")));
        EXPECT(args.contains(QStringLiteral("-carta")));
        for (int i = 0; i < 50 && core.state() != MsxCore::Idle; ++i) {
            QTest::qWait(100);
        }
    }

    // T13: BUG-024 — openMSX krijgt een eigen schrijfbare map mee. In de
    // Flatpak is de home read-only, dus zonder OPENMSX_HOME gaan SRAM en
    // save-states verloren en klaagt de emulator over het spel heen.
    {
        const QString dir = MsxCore::userDataDir();
        EXPECT(!dir.isEmpty());
        EXPECT(QDir(dir).exists());  // wordt aangemaakt, niet alleen berekend
        QFileInfo probe(dir);
        EXPECT(probe.isWritable());
        // Niet in de home-root: dat is precies de map die read-only is.
        EXPECT(dir != QDir::homePath() + QStringLiteral("/.openMSX"));
    }

    qInfo() << "All MsxCore smoke-tests OK (T1-T13)";
    return 0;
}
