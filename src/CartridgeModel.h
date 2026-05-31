#pragma once

#include <QAbstractListModel>
#include <QString>
#include <QVector>
#include <qqmlregistration.h>

struct CartridgeEntry {
    QString title;          // display title (filename stem)
    QString romPath;        // absolute path; empty == sentinel
    qint64  lastUsedUnix;   // sort key for recents
    QString machine;        // MSX1/MSX2/MSX2+/TurboR — heuristic, default MSX2
};

class CartridgeModel : public QAbstractListModel {
    Q_OBJECT
    QML_ELEMENT

public:
    enum Roles {
        TitleRole = Qt::UserRole + 1,
        RomPathRole,
        MachineRole,
        IsSentinelRole,
        LastUsedRole
    };

    explicit CartridgeModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = {}) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

public slots:
    void addRom(const QString &path);
    void clearRecents();

signals:
    void recentsChanged();

private:
    void load();
    void persist();

    static constexpr int kMaxRecents = 8;
    QVector<CartridgeEntry> m_entries;
};
