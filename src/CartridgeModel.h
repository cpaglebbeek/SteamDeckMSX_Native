#pragma once

#include <QAbstractListModel>
#include <QString>
#include <QVector>
#include <qqmlregistration.h>

struct CartridgeEntry {
    QString title;
    QString year;
    QString publisher;
    QString machine;       // MSX1 / MSX2 / MSX2+ / TurboR
    QString romPath;       // leeg in v0.0.3 — dummy data
};

class CartridgeModel : public QAbstractListModel {
    Q_OBJECT
    QML_ELEMENT

public:
    enum Roles {
        TitleRole = Qt::UserRole + 1,
        YearRole,
        PublisherRole,
        MachineRole,
        RomPathRole
    };

    explicit CartridgeModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = {}) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

private:
    QVector<CartridgeEntry> m_entries;
};
