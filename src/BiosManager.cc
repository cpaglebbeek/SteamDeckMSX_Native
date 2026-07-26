#include "BiosManager.h"
#include "FileDownloader.h"
#include "RomTypeDetector.h"
#include "MsxCore.h"

#include <QSettings>
#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QUrl>
#include <QDebug>

BiosManager::BiosManager(QObject *parent)
    : QAbstractListModel(parent)
    , m_downloader(new FileDownloader(this))
{
    connect(m_downloader, &FileDownloader::finished, this, &BiosManager::onDownloadFinished);
    connect(m_downloader, &FileDownloader::failed,   this, &BiosManager::onDownloadFailed);
    connect(m_downloader, &FileDownloader::progress, this, &BiosManager::onDownloadProgress);
    loadFromSettings();
    // BUG-033-migratie: eerder geïmporteerde BIOS-bestanden stonden alleen in
    // de app-opslag en waren voor openMSX onzichtbaar. Idempotent bijspiegelen.
    for (const auto &e : std::as_const(m_entries)) {
        if (QFileInfo::exists(e.absPath)) mirrorToSystemroms(e.absPath);
    }
}

void BiosManager::mirrorToSystemroms(const QString &absPath)
{
    // BUG-033: openMSX zoekt machine-roms op SHA-1 in OPENMSX_HOME/share/
    // systemroms; de app-opslag onder bios/ ziet hij nooit. Zonder deze
    // spiegel was een geïmporteerde BIOS nergens "te kiezen" — echte machines
    // in de machine-kiezer bleven onbootbaar. De bestandsnaam is voor openMSX
    // irrelevant (matching op inhoud), maar blijft leesbaar voor de gebruiker.
    const QString dir = MsxCore::userDataDir() + QStringLiteral("/share/systemroms");
    QDir().mkpath(dir);
    const QString dest = dir + QChar('/') + QFileInfo(absPath).fileName();
    if (QFileInfo::exists(dest)) return;   // idempotent
    QFile::copy(absPath, dest);
}

void BiosManager::removeMirror(const QString &absPath)
{
    QFile::remove(MsxCore::userDataDir() + QStringLiteral("/share/systemroms/")
                  + QFileInfo(absPath).fileName());
}

QString BiosManager::storageDir() const
{
    const QString base = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    const QString dir  = base + QStringLiteral("/bios");
    QDir().mkpath(dir);
    return dir;
}

int BiosManager::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) return 0;
    return m_entries.size();
}

QVariant BiosManager::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_entries.size()) {
        return {};
    }
    const auto &e = m_entries[index.row()];
    switch (role) {
    case IdRole:        return e.id;
    case FileNameRole:  return e.fileName;
    case AbsPathRole:   return e.absPath;
    case Sha1Role:      return e.sha1Hex;
    case SizeRole:      return e.sizeBytes;
    case AddedAtRole:   return e.addedAt;
    case SourceRole:    return e.source;
    default: return {};
    }
}

QHash<int, QByteArray> BiosManager::roleNames() const
{
    return {
        { IdRole,       "biosId"   },
        { FileNameRole, "fileName" },
        { AbsPathRole,  "absPath"  },
        { Sha1Role,     "sha1Hex"  },
        { SizeRole,     "sizeBytes"},
        { AddedAtRole,  "addedAt"  },
        { SourceRole,   "source"   },
    };
}

void BiosManager::addFromUrl(const QUrl &url, const QString &preferredName)
{
    if (m_busy) {
        emit addFailed(QStringLiteral("BiosManager bezig — wacht tot huidige operatie klaar is"));
        return;
    }
    // Bepaal destination-naam.
    QString fallback = url.fileName();
    if (fallback.isEmpty()) fallback = QStringLiteral("downloaded.rom");
    const QString dest = resolveDestPath(preferredName, fallback);
    m_pendingDestPath = dest;
    m_pendingSource   = QStringLiteral("url:") + url.toString();
    setBusy(true);
    if (!m_downloader->start(url, dest, kBiosMaxBytes)) {
        // start() emits failed() zelf via signal — busy wordt daar weer false
    }
}

bool BiosManager::addFromLocal(const QString &localPath, const QString &preferredName)
{
    if (m_busy) {
        emit addFailed(QStringLiteral("BiosManager bezig"));
        return false;
    }
    QFileInfo fi(localPath);
    if (!fi.exists() || !fi.isReadable()) {
        emit addFailed(QStringLiteral("Bron-bestand niet leesbaar: ") + localPath);
        return false;
    }
    if (fi.size() > kBiosMaxBytes) {
        emit addFailed(QStringLiteral("Bestand > %1 bytes (BIOS-cap)").arg(kBiosMaxBytes));
        return false;
    }
    const QString destBaseName = preferredName.isEmpty() ? fi.fileName() : preferredName;
    const QString dest = resolveDestPath(preferredName, destBaseName);

    setBusy(true);
    if (!QFile::copy(localPath, dest)) {
        // Kan al bestaan; remove + retry.
        QFile::remove(dest);
        if (!QFile::copy(localPath, dest)) {
            setBusy(false);
            emit addFailed(QStringLiteral("Kopiëren mislukte: ") + dest);
            return false;
        }
    }

    // SHA-1 berekenen.
    QFile f(dest);
    QString sha;
    if (f.open(QIODevice::ReadOnly)) {
        sha = RomTypeDetector::sha1Hex(f.readAll());
        f.close();
    }

    const QString source = QStringLiteral("local:") + localPath;
    auto e = buildEntryFromFile(dest, source, sha);
    appendEntry(e);
    persistToSettings();
    mirrorToSystemroms(dest);
    setBusy(false);
    emit entryAdded(e.id, e.fileName);
    return true;
}

bool BiosManager::removeEntry(const QString &id)
{
    for (int i = 0; i < m_entries.size(); ++i) {
        if (m_entries[i].id == id) {
            removeMirror(m_entries[i].absPath);
            QFile::remove(m_entries[i].absPath);
            beginRemoveRows({}, i, i);
            m_entries.removeAt(i);
            endRemoveRows();
            persistToSettings();
            return true;
        }
    }
    return false;
}

void BiosManager::clearAll()
{
    for (const auto &e : m_entries) {
        removeMirror(e.absPath);
        QFile::remove(e.absPath);
    }
    beginResetModel();
    m_entries.clear();
    endResetModel();
    persistToSettings();
}

void BiosManager::onDownloadFinished(const QString &destPath, const QString &sha1Hex)
{
    auto e = buildEntryFromFile(destPath, m_pendingSource, sha1Hex);
    appendEntry(e);
    persistToSettings();
    mirrorToSystemroms(destPath);
    setBusy(false);
    emit entryAdded(e.id, e.fileName);
}

void BiosManager::onDownloadFailed(const QString &reason)
{
    // Cleanup partial bestand.
    if (!m_pendingDestPath.isEmpty()) {
        QFile::remove(m_pendingDestPath + QStringLiteral(".part"));
        QFile::remove(m_pendingDestPath);
    }
    setBusy(false);
    emit addFailed(reason);
}

void BiosManager::onDownloadProgress(qint64 received, qint64 total)
{
    emit downloadProgress(received, total);
}

void BiosManager::loadFromSettings()
{
    QSettings s;
    const int count = s.beginReadArray(QStringLiteral("BiosManager/entries"));
    beginResetModel();
    m_entries.clear();
    for (int i = 0; i < count; ++i) {
        s.setArrayIndex(i);
        BiosEntry e;
        e.id        = s.value(QStringLiteral("id")).toString();
        e.fileName  = s.value(QStringLiteral("fileName")).toString();
        e.absPath   = s.value(QStringLiteral("absPath")).toString();
        e.sha1Hex   = s.value(QStringLiteral("sha1")).toString();
        e.sizeBytes = s.value(QStringLiteral("size")).toLongLong();
        e.addedAt   = s.value(QStringLiteral("addedAt")).toDateTime();
        e.source    = s.value(QStringLiteral("source")).toString();
        // Verifieer dat file nog bestaat (na disk-cleanup); zo niet skip.
        if (!e.absPath.isEmpty() && QFileInfo::exists(e.absPath)) {
            m_entries.append(e);
        }
    }
    s.endArray();
    endResetModel();
}

void BiosManager::persistToSettings()
{
    QSettings s;
    s.beginWriteArray(QStringLiteral("BiosManager/entries"), m_entries.size());
    for (int i = 0; i < m_entries.size(); ++i) {
        s.setArrayIndex(i);
        const auto &e = m_entries[i];
        s.setValue(QStringLiteral("id"),       e.id);
        s.setValue(QStringLiteral("fileName"), e.fileName);
        s.setValue(QStringLiteral("absPath"),  e.absPath);
        s.setValue(QStringLiteral("sha1"),     e.sha1Hex);
        s.setValue(QStringLiteral("size"),     e.sizeBytes);
        s.setValue(QStringLiteral("addedAt"),  e.addedAt);
        s.setValue(QStringLiteral("source"),   e.source);
    }
    s.endArray();
}

QString BiosManager::resolveDestPath(const QString &preferredName, const QString &fallbackBasename) const
{
    QString name = preferredName.isEmpty() ? fallbackBasename : preferredName;
    // Strip path-separators voor veiligheid (geen ../ tricks).
    name = name.replace(QChar('/'), QChar('_')).replace(QChar('\\'), QChar('_'));
    if (name.isEmpty()) name = QStringLiteral("bios.rom");
    return storageDir() + QChar('/') + name;
}

BiosEntry BiosManager::buildEntryFromFile(const QString &absPath, const QString &source, const QString &sha1Hex)
{
    QFileInfo fi(absPath);
    BiosEntry e;
    e.id        = sha1Hex.isEmpty() ? fi.completeBaseName() : sha1Hex.left(12);
    e.fileName  = fi.fileName();
    e.absPath   = absPath;
    e.sha1Hex   = sha1Hex;
    e.sizeBytes = fi.size();
    e.addedAt   = QDateTime::currentDateTime();
    e.source    = source;
    return e;
}

void BiosManager::appendEntry(const BiosEntry &e)
{
    // Dedup: vervang als zelfde id bestaat.
    for (int i = 0; i < m_entries.size(); ++i) {
        if (m_entries[i].id == e.id) {
            m_entries[i] = e;
            const auto idx = index(i);
            emit dataChanged(idx, idx);
            return;
        }
    }
    beginInsertRows({}, m_entries.size(), m_entries.size());
    m_entries.append(e);
    endInsertRows();
}

void BiosManager::setBusy(bool b)
{
    if (m_busy == b) return;
    m_busy = b;
    emit busyChanged();
}
