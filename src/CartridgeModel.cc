#include "CartridgeModel.h"

#include <QDateTime>
#include <QFileInfo>
#include <QSettings>

namespace {
constexpr auto kSettingsKey = "recents/cartridges";

QString machineHeuristic(const QString &fileName)
{
    const QString lower = fileName.toLower();
    if (lower.contains(QStringLiteral("turbor"))) return QStringLiteral("TurboR");
    if (lower.contains(QStringLiteral("msx2+")) || lower.contains(QStringLiteral("msx2plus"))) return QStringLiteral("MSX2+");
    if (lower.contains(QStringLiteral("msx2"))) return QStringLiteral("MSX2");
    if (lower.contains(QStringLiteral("msx1"))) return QStringLiteral("MSX1");
    return QStringLiteral("MSX2");
}
}

CartridgeModel::CartridgeModel(QObject *parent)
    : QAbstractListModel(parent)
{
    load();
}

void CartridgeModel::load()
{
    beginResetModel();
    m_entries.clear();

    QSettings s;
    const int count = s.beginReadArray(kSettingsKey);
    for (int i = 0; i < count && i < kMaxRecents; ++i) {
        s.setArrayIndex(i);
        CartridgeEntry e;
        e.romPath      = s.value(QStringLiteral("path")).toString();
        e.title        = s.value(QStringLiteral("title")).toString();
        e.machine      = s.value(QStringLiteral("machine")).toString();
        e.lastUsedUnix = s.value(QStringLiteral("lastUsed"), 0).toLongLong();
        if (!e.romPath.isEmpty() && QFileInfo::exists(e.romPath)) {
            m_entries.push_back(e);
        }
    }
    s.endArray();

    // Sentinel "Add ROM..." entry — always last
    CartridgeEntry sentinel;
    sentinel.title   = QStringLiteral("+ Add ROM...");
    m_entries.push_back(sentinel);

    endResetModel();
    emit recentsChanged();
}

void CartridgeModel::persist()
{
    QSettings s;
    s.beginWriteArray(kSettingsKey);
    int idx = 0;
    for (const auto &e : m_entries) {
        if (e.romPath.isEmpty()) continue; // skip sentinel
        s.setArrayIndex(idx++);
        s.setValue(QStringLiteral("path"),     e.romPath);
        s.setValue(QStringLiteral("title"),    e.title);
        s.setValue(QStringLiteral("machine"),  e.machine);
        s.setValue(QStringLiteral("lastUsed"), e.lastUsedUnix);
    }
    s.endArray();
    s.sync();
}

void CartridgeModel::addRom(const QString &path)
{
    if (path.isEmpty() || !QFileInfo::exists(path)) return;

    beginResetModel();

    // Remove existing entry with same path
    m_entries.removeIf([&](const CartridgeEntry &e) { return e.romPath == path; });

    // Build new entry
    QFileInfo fi(path);
    CartridgeEntry e;
    e.romPath      = fi.absoluteFilePath();
    e.title        = fi.completeBaseName();
    e.machine      = machineHeuristic(fi.fileName());
    e.lastUsedUnix = QDateTime::currentSecsSinceEpoch();

    // Find sentinel index and insert before it
    int sentinelIdx = -1;
    for (int i = 0; i < m_entries.size(); ++i) {
        if (m_entries[i].romPath.isEmpty()) { sentinelIdx = i; break; }
    }
    if (sentinelIdx < 0) {
        m_entries.prepend(e);
    } else {
        m_entries.insert(sentinelIdx, e);
    }

    // Cap recents
    int recentsCount = 0;
    for (auto it = m_entries.begin(); it != m_entries.end();) {
        if (!it->romPath.isEmpty()) {
            if (++recentsCount > kMaxRecents) {
                it = m_entries.erase(it);
                continue;
            }
        }
        ++it;
    }

    endResetModel();
    persist();
    emit recentsChanged();
}

void CartridgeModel::clearRecents()
{
    beginResetModel();
    m_entries.removeIf([](const CartridgeEntry &e) { return !e.romPath.isEmpty(); });
    endResetModel();
    persist();
    emit recentsChanged();
}

int CartridgeModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) return 0;
    return static_cast<int>(m_entries.size());
}

QVariant CartridgeModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_entries.size())
        return {};

    const auto &e = m_entries[index.row()];
    switch (role) {
        case TitleRole:      return e.title;
        case RomPathRole:    return e.romPath;
        case MachineRole:    return e.machine;
        case IsSentinelRole: return e.romPath.isEmpty();
        case LastUsedRole:   return e.lastUsedUnix;
        default:             return {};
    }
}

QHash<int, QByteArray> CartridgeModel::roleNames() const
{
    return {
        {TitleRole,      "title"},
        {RomPathRole,    "romPath"},
        {MachineRole,    "machine"},
        {IsSentinelRole, "isSentinel"},
        {LastUsedRole,   "lastUsed"},
    };
}
