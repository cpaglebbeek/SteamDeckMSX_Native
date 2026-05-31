#include "CartridgeModel.h"

CartridgeModel::CartridgeModel(QObject *parent)
    : QAbstractListModel(parent)
{
    m_entries = {
        {QStringLiteral("Metal Gear"),         QStringLiteral("1987"), QStringLiteral("Konami"),  QStringLiteral("MSX2"),   {}},
        {QStringLiteral("Bubble Bobble"),      QStringLiteral("1987"), QStringLiteral("Taito"),   QStringLiteral("MSX1"),   {}},
        {QStringLiteral("Knightmare"),         QStringLiteral("1986"), QStringLiteral("Konami"),  QStringLiteral("MSX1"),   {}},
        {QStringLiteral("Vampire Killer"),     QStringLiteral("1986"), QStringLiteral("Konami"),  QStringLiteral("MSX2"),   {}},
        {QStringLiteral("Nemesis"),            QStringLiteral("1986"), QStringLiteral("Konami"),  QStringLiteral("MSX1"),   {}},
        {QStringLiteral("Aleste"),             QStringLiteral("1988"), QStringLiteral("Compile"), QStringLiteral("MSX2"),   {}},
        {QStringLiteral("Penguin Adventure"),  QStringLiteral("1986"), QStringLiteral("Konami"),  QStringLiteral("MSX1"),   {}},
        {QStringLiteral("Snatcher"),           QStringLiteral("1988"), QStringLiteral("Konami"),  QStringLiteral("MSX2"),   {}},
    };
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
        case TitleRole:     return e.title;
        case YearRole:      return e.year;
        case PublisherRole: return e.publisher;
        case MachineRole:   return e.machine;
        case RomPathRole:   return e.romPath;
        default:            return {};
    }
}

QHash<int, QByteArray> CartridgeModel::roleNames() const
{
    return {
        {TitleRole,     "title"},
        {YearRole,      "year"},
        {PublisherRole, "publisher"},
        {MachineRole,   "machine"},
        {RomPathRole,   "romPath"},
    };
}
