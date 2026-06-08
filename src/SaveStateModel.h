#pragma once

#include <QAbstractListModel>
#include <QString>
#include <QVector>
#include <QDateTime>
#include <qqmlregistration.h>

#include "MsxCore.h"

struct SaveStateSlot {
    int slot;                  // 0..9
    QString romStem;           // filename-stem of current ROM (empty == never used)
    QString name;              // openMSX savestate-name = "slot_<N>_<stem>"
    QDateTime lastUsed;        // QSettings timestamp
    bool occupied;             // from QSettings, no FS-verify in v0.0.6
    QString thumbnailPath;     // v0.2.0
};

// 10-slot save-state grid (MSX-traditie). Per slot:
//  - QSettings entry: savestates/slot_<N>/{rom, name, timestamp, occupied}
//  - openMSX-naam:    slot_<N>_<rom_stem>
//
// v0.0.6 doet GEEN FS-check op `~/.openMSX/savestates/<name>.oms` (afhankelijk
// van Flatpak/Mac paths). Vertrouwen op QSettings-state. v0.0.7+ kan dit
// uitbreiden zodra Deck-paths bekend.
class SaveStateModel : public QAbstractListModel {
    Q_OBJECT
    QML_ELEMENT

public:
    enum Roles {
        SlotRole = Qt::UserRole + 1,
        NameRole,
        RomStemRole,
        LastUsedRole,
        OccupiedRole,
        LabelRole,      // formatted "Slot N · <stem> · <ts>" (or "Slot N · empty")
        ThumbnailPathRole,    // v0.2.0: absoluut pad naar slot thumbnail PNG, lege string als geen
    };

    static constexpr int kSlotCount = 10;

    explicit SaveStateModel(QObject *parent = nullptr);

    Q_PROPERTY(MsxCore *core READ core WRITE setCore NOTIFY coreChanged)
    Q_PROPERTY(QString currentRomStem READ currentRomStem WRITE setCurrentRomStem NOTIFY currentRomStemChanged)

    MsxCore *core() const { return m_core; }
    void setCore(MsxCore *c);
    QString currentRomStem() const { return m_currentRomStem; }
    void setCurrentRomStem(const QString &stem);

    int rowCount(const QModelIndex &parent = {}) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

public slots:
    // save naar slot N. Markeert in QSettings + stuurt savestate-cmd via core.
    Q_INVOKABLE int saveTo(int slot);
    // load van slot N. Stuurt loadstate-cmd via core (alleen als occupied).
    Q_INVOKABLE int loadFrom(int slot);

    // v0.2.0: vraag openMSX om screenshot voor slot N. Stuurt Tcl `screenshot -prefix slot_N`.
    // Update thumbnailPath in model + persist. Geen-op als core niet attached/running.
    Q_INVOKABLE int requestThumbnail(int slot);

    // v0.2.0: helper voor QML — return absolute thumbnail-pad ("" als geen).
    Q_INVOKABLE QString thumbnailFor(int slot) const;

    // Reset slot N (clear QSettings entry — geen FS-delete v0.0.6).
    Q_INVOKABLE void clear(int slot);

signals:
    void coreChanged();
    void currentRomStemChanged();
    void slotChanged(int slot);
    void saveRequested(int slot, int commandId);
    void loadRequested(int slot, int commandId);

private:
    void load();
    void persistSlot(int slot);
    void emitSlotDataChanged(int slot);

    // v0.2.0: bepaal storage-dir voor thumbnails (lazy create).
    QString thumbnailDir() const;

    MsxCore *m_core{nullptr};
    QString m_currentRomStem{};
    QVector<SaveStateSlot> m_slots;
};
