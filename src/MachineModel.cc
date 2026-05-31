#include "MachineModel.h"
#include "MsxCore.h"

#include <QSettings>
#include <QDebug>

namespace {
constexpr auto kCurrentKey = "machine/current";
// Fallback hardcoded C-BIOS-machines if dynamic load fails or is not yet attempted.
const QStringList kFallback = {
    QStringLiteral("C-BIOS_MSX1"),
    QStringLiteral("C-BIOS_MSX2"),
    QStringLiteral("C-BIOS_MSX2+"),
};
}

MachineModel::MachineModel(QObject *parent)
    : QAbstractListModel(parent)
{
    loadPersisted();
    // Begin with fallback list so UI is non-empty immediately.
    m_machines = kFallback;
}

void MachineModel::setCore(MsxCore *c)
{
    if (c == m_core) return;
    if (m_core) disconnect(m_core, nullptr, this, nullptr);
    m_core = c;
    if (m_core) {
        connect(m_core, &MsxCore::replyReceived, this, &MachineModel::onReply);
    }
    emit coreChanged();
}

void MachineModel::loadPersisted()
{
    QSettings s;
    m_currentMachine = s.value(kCurrentKey).toString();
}

void MachineModel::persist()
{
    QSettings s;
    s.setValue(kCurrentKey, m_currentMachine);
    s.sync();
}

void MachineModel::setCurrentMachine(const QString &m)
{
    if (m == m_currentMachine) return;
    m_currentMachine = m;
    persist();
    emit currentMachineChanged();
    // Push through to MsxCore (will only send command if Running)
    if (m_core) m_core->setCurrentMachine(m);
    // Refresh list to update isCurrent role
    if (!m_machines.isEmpty()) {
        emit dataChanged(index(0), index(m_machines.size() - 1));
    }
}

int MachineModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) return 0;
    return m_machines.size();
}

QVariant MachineModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_machines.size())
        return {};

    const QString &name = m_machines[index.row()];
    switch (role) {
        case NameRole:      return name;
        case IsCurrentRole: return name == m_currentMachine;
        default:            return {};
    }
}

QHash<int, QByteArray> MachineModel::roleNames() const
{
    return {
        {NameRole,      "name"},
        {IsCurrentRole, "isCurrent"},
    };
}

void MachineModel::refresh()
{
    if (!m_core) return;
    m_pendingCmdId = m_core->sendCommand(QStringLiteral("machine_info machines"));
}

void MachineModel::onReply(int commandId, bool ok, const QString &body)
{
    if (commandId != m_pendingCmdId || m_pendingCmdId < 0) return;
    m_pendingCmdId = -1;

    if (!ok) {
        qWarning() << "machine_info reply not-ok:" << body;
        return;
    }
    // openMSX Tcl-list output: space-separated machine names (with optional braces)
    QStringList parsed;
    QString token;
    int depth = 0;
    for (QChar c : body) {
        if (c == '{') { depth++; continue; }
        if (c == '}') { depth--; if (!token.isEmpty()) { parsed << token; token.clear(); } continue; }
        if (c.isSpace() && depth == 0) {
            if (!token.isEmpty()) { parsed << token; token.clear(); }
            continue;
        }
        token.append(c);
    }
    if (!token.isEmpty()) parsed << token;

    if (parsed.isEmpty()) {
        qWarning() << "machine_info parsed empty:" << body;
        return;
    }
    beginResetModel();
    m_machines = parsed;
    endResetModel();
    emit loadedChanged();
}
