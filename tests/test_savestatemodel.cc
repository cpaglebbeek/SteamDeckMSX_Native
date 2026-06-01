// Unit test voor SaveStateModel — initial state, save → occupied, persist roundtrip.

#include "../src/SaveStateModel.h"
#include "../src/MsxCore.h"

#include <QCoreApplication>
#include <QSettings>
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
    app.setOrganizationName(QStringLiteral("iCt-Horse"));
    app.setApplicationName(QStringLiteral("SteamDeckMSX-test"));

    // Schoon eerst — verwijder eventuele state van vorige run
    {
        QSettings s;
        s.remove(QStringLiteral("savestates"));
    }

    // T1: Initial state — 10 slots, alle empty
    {
        SaveStateModel m;
        EXPECT(m.rowCount() == SaveStateModel::kSlotCount);
        EXPECT(m.rowCount() == 10);
        for (int i = 0; i < 10; ++i) {
            const auto idx = m.index(i);
            EXPECT(m.data(idx, SaveStateModel::OccupiedRole).toBool() == false);
            EXPECT(m.data(idx, SaveStateModel::SlotRole).toInt() == i);
        }
    }

    // T2: saveTo zonder core attached → return -1
    {
        SaveStateModel m;
        m.setCurrentRomStem(QStringLiteral("MetalGear"));
        const int id = m.saveTo(3);
        EXPECT(id == -1);  // no core
    }

    // T3: saveTo + persistence roundtrip
    {
        SaveStateModel m;
        m.setCurrentRomStem(QStringLiteral("BubbleBobble"));

        // saveTo zonder core schrijft toch QSettings (model-state) maar geen Tcl-cmd
        m.saveTo(5);
        const auto idx = m.index(5);
        EXPECT(m.data(idx, SaveStateModel::OccupiedRole).toBool() == true);
        EXPECT(m.data(idx, SaveStateModel::RomStemRole).toString() == QStringLiteral("BubbleBobble"));
        EXPECT(m.data(idx, SaveStateModel::NameRole).toString() == QStringLiteral("slot_5_BubbleBobble"));

        // Nieuwe instance leest persisted state
        SaveStateModel m2;
        const auto idx2 = m2.index(5);
        EXPECT(m2.data(idx2, SaveStateModel::OccupiedRole).toBool() == true);
        EXPECT(m2.data(idx2, SaveStateModel::RomStemRole).toString() == QStringLiteral("BubbleBobble"));
    }

    // T4: clear()
    {
        SaveStateModel m;
        // Slot 5 is occupied vanuit T3
        EXPECT(m.data(m.index(5), SaveStateModel::OccupiedRole).toBool() == true);
        m.clear(5);
        EXPECT(m.data(m.index(5), SaveStateModel::OccupiedRole).toBool() == false);
        EXPECT(m.data(m.index(5), SaveStateModel::RomStemRole).toString().isEmpty());

        // Roundtrip nieuwe instance
        SaveStateModel m2;
        EXPECT(m2.data(m2.index(5), SaveStateModel::OccupiedRole).toBool() == false);
    }

    // T5: LabelRole formatting
    {
        SaveStateModel m;
        m.setCurrentRomStem(QStringLiteral("Nemesis"));
        m.saveTo(2);
        const QString lbl = m.data(m.index(2), SaveStateModel::LabelRole).toString();
        EXPECT(lbl.contains(QStringLiteral("Slot 2")));
        EXPECT(lbl.contains(QStringLiteral("Nemesis")));
        const QString lblEmpty = m.data(m.index(7), SaveStateModel::LabelRole).toString();
        EXPECT(lblEmpty.contains(QStringLiteral("empty")));

        // Cleanup
        m.clear(2);
    }

    // T6: loadFrom van empty slot → -1
    {
        SaveStateModel m;
        const int id = m.loadFrom(9);  // empty
        EXPECT(id == -1);
    }

    // T7: out-of-range slot
    {
        SaveStateModel m;
        EXPECT(m.saveTo(-1) == -1);
        EXPECT(m.saveTo(10) == -1);
        EXPECT(m.loadFrom(-1) == -1);
        m.clear(99);  // no-op, no crash
    }

    qInfo() << "All SaveStateModel tests OK (T1-T7)";
    return 0;
}
