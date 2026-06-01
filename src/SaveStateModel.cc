#include "SaveStateModel.h"

#include <QSettings>
#include <QDebug>

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
    m_core = c;
    emit coreChanged();
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
    st.sync();
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
    emit loadRequested(slot, id);
    return id;
}

void SaveStateModel::clear(int slot)
{
    if (slot < 0 || slot >= kSlotCount) return;
    SaveStateSlot &s = m_slots[slot];
    s.occupied = false;
    s.romStem.clear();
    s.name.clear();
    s.lastUsed = QDateTime();
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
    };
}
