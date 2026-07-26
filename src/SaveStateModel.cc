#include "SaveStateModel.h"

#include <QSettings>
#include <QDebug>
#include <QStandardPaths>
#include <QDir>
#include <QFileInfo>

namespace {
QString stateName(int slot, const QString &romStem) {
    return QStringLiteral("slot_%1_%2").arg(slot).arg(romStem.isEmpty() ? QStringLiteral("nopath") : romStem);
}
QString slotKey(int slot, const QString &field) {
    return QStringLiteral("savestates/slot_%1/%2").arg(slot).arg(field);
}
}

SaveStateModel::SaveStateModel(QObject *parent)
    : QAbstractListModel(parent)
{
    m_slots.reserve(kSlotCount);
    for (int i = 0; i < kSlotCount; ++i) {
        SaveStateSlot s;
        s.slot = i;
        s.occupied = false;
        m_slots.push_back(s);
    }
    load();
}

void SaveStateModel::setCore(MsxCore *c)
{
    if (c == m_core) return;
    if (m_core) disconnect(m_core, &MsxCore::replyReceived, this, &SaveStateModel::onCoreReply);
    m_core = c;
    if (m_core) connect(m_core, &MsxCore::replyReceived, this, &SaveStateModel::onCoreReply);
    emit coreChanged();
}

void SaveStateModel::onCoreReply(int commandId, bool ok, const QString &body)
{
    if (!m_pending.contains(commandId)) return;
    const auto op = m_pending.take(commandId);
    if (ok) {
        // qWarning en niet qInfo: info-berichten staan standaard uit in
        // release-omgevingen en dan ziet de gate dit bewijs nooit.
        qWarning() << "[SaveState]" << op.first << "slot" << op.second << "ok";
        emit operationFinished(op.second, op.first, true, QString());
        return;
    }
    qWarning() << "[SaveState]" << op.first << "slot" << op.second << "FAALDE:" << body;
    if (op.first == QStringLiteral("save")) {
        // saveTo had het slot al bezet gemarkeerd; zonder terugdraai wijst het
        // slot naar een state-bestand dat nooit geschreven is en faalt elke
        // latere load op een "bezet" slot.
        SaveStateSlot &s = m_slots[op.second];
        s.occupied = false;
        persistSlot(op.second);
        emitSlotDataChanged(op.second);
    }
    emit operationFinished(op.second, op.first, false, body);
}

void SaveStateModel::setCurrentRomStem(const QString &stem)
{
    if (stem == m_currentRomStem) return;
    m_currentRomStem = stem;
    emit currentRomStemChanged();
}

void SaveStateModel::load()
{
    QSettings s;
    for (int i = 0; i < kSlotCount; ++i) {
        SaveStateSlot &slot = m_slots[i];
        slot.occupied = s.value(slotKey(i, QStringLiteral("occupied")), false).toBool();
        slot.romStem  = s.value(slotKey(i, QStringLiteral("rom")), QString()).toString();
        slot.name     = s.value(slotKey(i, QStringLiteral("name")), QString()).toString();
        slot.lastUsed = s.value(slotKey(i, QStringLiteral("timestamp"))).toDateTime();
        slot.thumbnailPath = s.value(slotKey(i, QStringLiteral("thumbnail")), QString()).toString();
    }
}

void SaveStateModel::persistSlot(int slot)
{
    if (slot < 0 || slot >= kSlotCount) return;
    const auto &s = m_slots[slot];
    QSettings st;
    st.setValue(slotKey(slot, QStringLiteral("occupied")), s.occupied);
    st.setValue(slotKey(slot, QStringLiteral("rom")),      s.romStem);
    st.setValue(slotKey(slot, QStringLiteral("name")),     s.name);
    st.setValue(slotKey(slot, QStringLiteral("timestamp")),s.lastUsed);
    st.setValue(slotKey(slot, QStringLiteral("thumbnail")),s.thumbnailPath);
    st.sync();
}

QString SaveStateModel::thumbnailDir() const
{
    const QString base = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
        + QStringLiteral("/savestates/thumbs");
    QDir().mkpath(base);
    return QDir(base).absolutePath();
}

void SaveStateModel::emitSlotDataChanged(int slot)
{
    const QModelIndex idx = index(slot);
    emit dataChanged(idx, idx);
    emit slotChanged(slot);
}

int SaveStateModel::saveTo(int slot)
{
    if (slot < 0 || slot >= kSlotCount) return -1;

    // Model-state altijd updaten — semantisch "slot wordt bezet". Tcl-cmd
    // alleen als core attached. Dit ondersteunt "stage save" patroon:
    // model-state kan vóór core-attach worden klaargezet (bv. uit een test
    // of een UI-flow waarbij core nog niet runt).
    SaveStateSlot &s = m_slots[slot];
    s.romStem  = m_currentRomStem;
    s.name     = stateName(slot, m_currentRomStem);
    s.lastUsed = QDateTime::currentDateTime();
    s.occupied = true;
    persistSlot(slot);
    emitSlotDataChanged(slot);

    if (!m_core) {
        qWarning() << "saveTo: no core attached — model-state updated, geen Tcl-cmd";
        emit saveRequested(slot, -1);
        return -1;
    }
    const int id = m_core->savestate(s.name);
    if (id > 0) m_pending.insert(id, {QStringLiteral("save"), slot});
    emit saveRequested(slot, id);
    return id;
}

int SaveStateModel::loadFrom(int slot)
{
    if (slot < 0 || slot >= kSlotCount) return -1;
    if (!m_core) return -1;
    const SaveStateSlot &s = m_slots[slot];
    if (!s.occupied || s.name.isEmpty()) {
        qWarning() << "loadFrom: slot" << slot << "is empty";
        return -1;
    }
    const int id = m_core->loadstate(s.name);
    if (id > 0) m_pending.insert(id, {QStringLiteral("load"), slot});
    emit loadRequested(slot, id);
    return id;
}

int SaveStateModel::requestThumbnail(int slot)
{
    if (slot < 0 || slot >= kSlotCount) return -1;
    if (!m_core || m_core->state() != MsxCore::Running) {
        qWarning() << "requestThumbnail: core niet attached/running — geen-op voor slot" << slot;
        return -1;
    }
    const QString name = QStringLiteral("slot_%1").arg(slot);
    const QString expectedPath = thumbnailDir() + QStringLiteral("/") + name + QStringLiteral(".png");

    m_slots[slot].thumbnailPath = expectedPath;
    persistSlot(slot);

    const int id = m_core->sendCommand(
        QStringLiteral("screenshot -prefix %1").arg(thumbnailDir() + QStringLiteral("/") + name));

    const QModelIndex idx = index(slot);
    emit dataChanged(idx, idx, {ThumbnailPathRole});
    return id;
}

QString SaveStateModel::thumbnailFor(int slot) const
{
    if (slot < 0 || slot >= kSlotCount) return {};
    const QString p = m_slots[slot].thumbnailPath;
    if (p.isEmpty()) return {};
    return QFileInfo::exists(p) ? p : QString();
}

void SaveStateModel::clear(int slot)
{
    if (slot < 0 || slot >= kSlotCount) return;
    SaveStateSlot &s = m_slots[slot];
    s.occupied = false;
    s.romStem.clear();
    s.name.clear();
    s.lastUsed = QDateTime();
    s.thumbnailPath.clear();
    persistSlot(slot);
    emitSlotDataChanged(slot);
}

int SaveStateModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) return 0;
    return kSlotCount;
}

QVariant SaveStateModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= kSlotCount)
        return {};
    const SaveStateSlot &s = m_slots[index.row()];
    switch (role) {
        case SlotRole:     return s.slot;
        case NameRole:     return s.name;
        case RomStemRole:  return s.romStem;
        case LastUsedRole: return s.lastUsed;
        case OccupiedRole: return s.occupied;
        case LabelRole: {
            if (!s.occupied) return QStringLiteral("Slot %1 · empty").arg(s.slot);
            const QString ts = s.lastUsed.isValid()
                ? s.lastUsed.toString(QStringLiteral("yyyy-MM-dd HH:mm"))
                : QStringLiteral("?");
            return QStringLiteral("Slot %1 · %2 · %3")
                .arg(s.slot).arg(s.romStem.isEmpty() ? QStringLiteral("?") : s.romStem).arg(ts);
        }
        case ThumbnailPathRole: return s.thumbnailPath;
        default: return {};
    }
}

QHash<int, QByteArray> SaveStateModel::roleNames() const
{
    return {
        {SlotRole,     "slot"},
        {NameRole,     "stateName"},
        {RomStemRole,  "romStem"},
        {LastUsedRole, "lastUsed"},
        {OccupiedRole, "occupied"},
        {LabelRole,    "label"},
        {ThumbnailPathRole, "thumbnailPath"},
    };
}
