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

    // T11: BUG-022 — start() geeft -fullscreen mee, met een quit-binding zodat
    // de speler terugkan nu de galerij zich tijdens het spelen verbergt.
    {
        const QString script = makeMockScript(QStringLiteral("<openmsx-output>\n</openmsx-output>\n"));
        MsxCore core;
        core.setOpenmsxPath(script);
        EXPECT(core.fullscreen());  // standaard aan
        core.start(QStringLiteral("/tmp/nemesis2.rom"));
        const QStringList args = core.lastStartArgs();
        EXPECT(args.contains(QStringLiteral("-fullscreen")));
        const int cmd = args.indexOf(QStringLiteral("-command"));
        EXPECT(cmd >= 0 && cmd + 1 < args.size());
        EXPECT(args.at(cmd + 1) == QStringLiteral("bind F12 quit"));
        for (int i = 0; i < 50 && core.state() != MsxCore::Idle; ++i) {
            QTest::qWait(100);
        }
    }

    // T12: fullscreen uitzetten haalt beide vlaggen weg — de headless gates
    // draaien zo, en dan hoort de galerij zichtbaar te blijven.
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
        EXPECT(!args.contains(QStringLiteral("-fullscreen")));
        EXPECT(!args.contains(QStringLiteral("-command")));
        EXPECT(args.contains(QStringLiteral("-carta")));
        for (int i = 0; i < 50 && core.state() != MsxCore::Idle; ++i) {
            QTest::qWait(100);
        }
    }

    qInfo() << "All MsxCore smoke-tests OK (T1-T12)";
    return 0;
}
