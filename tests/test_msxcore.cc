// Unit test voor MsxCore — alleen public API.
// Geen echte openMSX nodig: gebruikt /usr/bin/true als mock.

#include "../src/MsxCore.h"

#include <QCoreApplication>
#include <QSignalSpy>
#include <QStandardPaths>
#include <QtTest/QTest>
#include <QDebug>

#define EXPECT(cond) do { \
    if (!(cond)) { qCritical() << "FAIL line" << __LINE__ << ":" << #cond; std::exit(1); } \
} while (0)

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
    }

    // T2: openmsxPath roundtrip — signal alleen bij echte change
    {
        MsxCore core;
        QSignalSpy spy(&core, &MsxCore::openmsxPathChanged);
        core.setOpenmsxPath(QStringLiteral("/some/fake/path"));
        EXPECT(spy.count() >= 1);  // may be 0..N if found path already same in ctor
        const int after = spy.count();
        core.setOpenmsxPath(QStringLiteral("/some/fake/path"));
        EXPECT(spy.count() == after);  // geen extra signal
    }

    // T3: probeVersion zonder pad → Failed + errorMessage
    {
        MsxCore core;
        core.setOpenmsxPath(QString());
        core.probeVersion();
        EXPECT(core.state() == MsxCore::Failed);
        EXPECT(!core.errorMessage().isEmpty());
    }

    // T4: probeVersion met /usr/bin/true → eindigt in Probed (exit 0, geen stdout)
    {
        const QString trueBin = QStandardPaths::findExecutable(QStringLiteral("true"));
        if (!trueBin.isEmpty()) {
            MsxCore core;
            core.setOpenmsxPath(trueBin);
            QSignalSpy spy(&core, &MsxCore::stateChanged);
            core.probeVersion();
            EXPECT(core.state() == MsxCore::Probing);
            // wait for finish
            spy.wait(2000);
            // er kunnen meerdere state-changes zijn (Idle->Probing->Probed); we
            // willen alleen verifiëren dat eind-state Probed is
            EXPECT(core.state() == MsxCore::Probed);
        } else {
            qWarning() << "T4 skipped: /usr/bin/true not found";
        }
    }

    // T5: stop() vanuit Idle is no-op
    {
        MsxCore core;
        EXPECT(core.state() == MsxCore::Idle);
        core.stop();
        EXPECT(core.state() == MsxCore::Idle);
    }

    // T6: sendCommand vanuit Idle = warning maar geen crash
    {
        MsxCore core;
        core.sendCommand(QStringLiteral("nop"));
        EXPECT(core.state() == MsxCore::Idle);  // unchanged
    }

    qInfo() << "All MsxCore smoke-tests OK";
    return 0;
}
