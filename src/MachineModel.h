#pragma once

#include <QAbstractListModel>
#include <QString>
#include <QStringList>
#include <qqmlregistration.h>

#include "MsxCore.h"

// Holds the list of openMSX machine-configs (e.g. "C-BIOS_MSX1", "C-BIOS_MSX2",
// "C-BIOS_MSX2+", and user-imported machines). Populated by sending the Tcl
// command `machine_info machines` to a running MsxCore instance and parsing
// the reply.
//
// Persists the user-selected machine in QSettings (key: machine/current) so
// next launch defaults to the same choice.
class MachineModel : public QAbstractListModel {
    Q_OBJECT
    QML_ELEMENT

public:
    enum Roles {
        NameRole = Qt::UserRole + 1,
        IsCurrentRole
    };

    explicit MachineModel(QObject *parent = nullptr);

    Q_PROPERTY(MsxCore *core READ core WRITE setCore NOTIFY coreChanged)
    Q_PROPERTY(QString currentMachine READ currentMachine WRITE setCurrentMachine NOTIFY currentMachineChanged)
    Q_PROPERTY(bool loaded READ loaded NOTIFY loadedChanged)

    MsxCore *core() const { return m_core; }
    void setCore(MsxCore *c);
    QString currentMachine() const { return m_currentMachine; }
    void setCurrentMachine(const QString &m);
    bool loaded() const { return !m_machines.isEmpty(); }

    int rowCount(const QModelIndex &parent = {}) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

public slots:
    void refresh();

signals:
    void coreChanged();
    void currentMachineChanged();
    void loadedChanged();

private:
    void loadPersisted();
    void persist();
    void onReply(int commandId, bool ok, const QString &body);

    MsxCore *m_core{nullptr};
    QStringList m_machines{};
    QString m_currentMachine{};
    int m_pendingCmdId{-1};
};
