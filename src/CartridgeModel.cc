#include "CartridgeModel.h"
#include "BiosZipExtractor.h"
#include "FileDownloader.h"
#include "RomTypeDetector.h"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSettings>
#include <QStandardPaths>

#include <algorithm>

namespace {
constexpr auto kSettingsKey = "recents/cartridges";

}  // end anon namespace

// v0.2.0: media-type op basis van filename-extensie.
QString CartridgeModel::mediaTypeFor(const QString &path)
{
    const QString lower = path.toLower();
    if (lower.endsWith(QStringLiteral(".dsk"))) return QStringLiteral("dsk");
    if (lower.endsWith(QStringLiteral(".cas"))) return QStringLiteral("cas");
    if (lower.endsWith(QStringLiteral(".rom"))) return QStringLiteral("rom");
    if (lower.endsWith(QStringLiteral(".zip"))) return QStringLiteral("zip");
    return QStringLiteral("rom");  // default fallback
}

namespace {
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
    , m_downloader(new FileDownloader(this))
{
    connect(m_downloader, &FileDownloader::finished, this, &CartridgeModel::onDownloadFinished);
    connect(m_downloader, &FileDownloader::failed,   this, &CartridgeModel::onDownloadFailed);
    connect(m_downloader, &FileDownloader::progress, this, &CartridgeModel::onDownloadProgress);
    load();
    migrateZipRoms();
}

QString CartridgeModel::storageDir() const
{
    const QString base = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    const QString dir  = base + QStringLiteral("/roms");
    QDir().mkpath(dir);
    return dir;
}

QString CartridgeModel::resolveDestPath(const QString &preferredName, const QString &fallbackBasename) const
{
    QString name = preferredName.isEmpty() ? fallbackBasename : preferredName;
    name = name.replace(QChar('/'), QChar('_')).replace(QChar('\\'), QChar('_'));
    if (name.isEmpty()) name = QStringLiteral("rom.rom");
    // BUG-028: hier werd blind ".rom" achter élke naam geplakt. Een download
    // "spel.zip" werd zo "spel.zip.rom": een archief vermomd als cartridge,
    // dat de galerij-headercheck terecht wegfiltert en openMSX niet boot.
    // Bekende extensies blijven wat ze zijn; alleen extensieloos wordt .rom.
    const QString lower = name.toLower();
    const bool known = lower.endsWith(QStringLiteral(".rom"))
        || lower.endsWith(QStringLiteral(".zip"))
        || lower.endsWith(QStringLiteral(".dsk"))
        || lower.endsWith(QStringLiteral(".cas"));
    if (!known) {
        name += QStringLiteral(".rom");
    }
    return storageDir() + QChar('/') + name;
}

bool CartridgeModel::addFromLocal(const QString &path, bool copyIntoStorage)
{
    if (path.isEmpty() || !QFileInfo::exists(path)) {
        emit downloadFailed(QStringLiteral("Bron-bestand niet leesbaar: ") + path);
        return false;
    }
    QFileInfo fi(path);
    if (fi.size() > kRomMaxBytes) {
        emit downloadFailed(QStringLiteral("ROM > %1 bytes (cap)").arg(kRomMaxBytes));
        return false;
    }
    QString finalPath = path;
    if (copyIntoStorage) {
        const QString dest = resolveDestPath(QString(), fi.fileName());
        if (QFile::exists(dest)) QFile::remove(dest);
        if (!QFile::copy(path, dest)) {
            emit downloadFailed(QStringLiteral("Kan ROM niet kopiëren naar storage: ") + dest);
            return false;
        }
        finalPath = dest;
    }
    registerLocal(finalPath, QStringLiteral("local:") + path);
    return true;
}

void CartridgeModel::setDownloadCredentials(const QString &user, const QString &password)
{
    if (user.isEmpty()) {
        m_downloader->clearCredentials();
    } else {
        m_downloader->setCredentials(user, password);
    }
}

void CartridgeModel::addFromUrl(const QUrl &url, const QString &preferredName)
{
    if (m_busy) {
        emit downloadFailed(QStringLiteral("CartridgeModel bezig"));
        return;
    }
    QString fallback = url.fileName();
    if (fallback.isEmpty()) fallback = QStringLiteral("rom.rom");
    const QString dest = resolveDestPath(preferredName, fallback);
    m_pendingDestPath = dest;
    m_pendingSource   = QStringLiteral("url:") + url.toString();
    setBusy(true);
    if (!m_downloader->start(url, dest, kRomMaxBytes)) {
        // failed-signal triggert onDownloadFailed
    }
}

void CartridgeModel::onDownloadFinished(const QString &destPath, const QString &sha1Hex)
{
    Q_UNUSED(sha1Hex);
    // BUG-028: online bronnen leveren vooral zips. Het archief zelf is geen
    // spel — uitpakken naar storage en de inhoud registreren, zodat de
    // galerij-scan echte .rom/.dsk/.cas-bestanden ziet.
    if (destPath.endsWith(QStringLiteral(".zip"), Qt::CaseInsensitive)) {
        QStringList names;
        const int n = extractZipToStorage(destPath, &names);
        QFile::remove(destPath);
        setBusy(false);
        if (n <= 0) {
            emit downloadFailed(QStringLiteral("ZIP bevatte geen speelbaar bestand (.rom/.dsk/.cas)"));
            return;
        }
        const QString first = storageDir() + QChar('/') + names.first();
        emit downloadFinished(QFileInfo(first).completeBaseName(), first);
        return;
    }
    registerLocal(destPath, m_pendingSource);
    setBusy(false);
    QFileInfo fi(destPath);
    emit downloadFinished(fi.completeBaseName(), destPath);
}

int CartridgeModel::extractZipToStorage(const QString &zipPath, QStringList *namesOut)
{
    BiosZipExtractor zx;
    zx.setAllowedExtensions({QStringLiteral("rom"), QStringLiteral("dsk"), QStringLiteral("cas")});
    zx.setPerFileMaxBytes(kRomMaxBytes);
    QStringList names;
    const int n = zx.extractTo(zipPath, storageDir(), &names);
    for (const QString &name : std::as_const(names)) {
        registerLocal(storageDir() + QChar('/') + name, m_pendingSource);
    }
    if (namesOut) *namesOut = names;
    return n;
}

void CartridgeModel::migrateZipRoms()
{
    // v0.4.0 sloeg online downloads op als "<naam>.zip.rom" (BUG-028): een
    // archief met cartridge-extensie dat nergens werkte maar wel bleef staan.
    // Eenmalig herstellen: terug-hernoemen naar .zip, uitpakken, archief weg.
    const QDir dir(storageDir());
    const QStringList ghosts = dir.entryList({QStringLiteral("*.zip.rom")}, QDir::Files);
    for (const QString &ghost : ghosts) {
        const QString oldPath = dir.filePath(ghost);
        QString zipName = ghost;
        zipName.chop(4);                    // ".rom" eraf → "<naam>.zip"
        const QString zipPath = dir.filePath(zipName);
        if (QFile::exists(zipPath)) QFile::remove(zipPath);
        if (!QFile::rename(oldPath, zipPath)) continue;
        extractZipToStorage(zipPath);
        QFile::remove(zipPath);
        // Het spookbestand kan als recent geregistreerd staan; opruimen.
        const bool hadGhost = std::any_of(m_entries.cbegin(), m_entries.cend(),
            [&](const CartridgeEntry &e) { return e.romPath == oldPath; });
        if (hadGhost) {
            beginResetModel();
            m_entries.removeIf([&](const CartridgeEntry &e) { return e.romPath == oldPath; });
            endResetModel();
            persist();
        }
    }
}

void CartridgeModel::onDownloadFailed(const QString &reason)
{
    if (!m_pendingDestPath.isEmpty()) {
        QFile::remove(m_pendingDestPath + QStringLiteral(".part"));
        QFile::remove(m_pendingDestPath);
    }
    setBusy(false);
    emit downloadFailed(reason);
}

void CartridgeModel::onDownloadProgress(qint64 received, qint64 total)
{
    emit downloadProgress(received, total);
}

void CartridgeModel::registerLocal(const QString &absPath, const QString &source)
{
    QFileInfo fi(absPath);
    // SHA-1 fingerprint (best-effort; skip bij failure).
    QString sha;
    {
        QFile f(absPath);
        if (f.open(QIODevice::ReadOnly)) {
            // Cap read tot 8 MiB.
            sha = RomTypeDetector::sha1Hex(f.read(kRomMaxBytes));
        }
    }
    beginResetModel();
    m_entries.removeIf([&](const CartridgeEntry &e) { return e.romPath == absPath; });
    CartridgeEntry e;
    e.romPath      = fi.absoluteFilePath();
    e.title        = fi.completeBaseName();
    e.machine      = machineHeuristic(fi.fileName());
    e.lastUsedUnix = QDateTime::currentSecsSinceEpoch();
    e.sha1Hex      = sha;
    e.source       = source;
    int sentinelIdx = -1;
    for (int i = 0; i < m_entries.size(); ++i) {
        if (m_entries[i].romPath.isEmpty()) { sentinelIdx = i; break; }
    }
    if (sentinelIdx < 0) m_entries.prepend(e);
    else                 m_entries.insert(sentinelIdx, e);
    int recents = 0;
    for (auto it = m_entries.begin(); it != m_entries.end();) {
        if (!it->romPath.isEmpty()) {
            if (++recents > kMaxRecents) { it = m_entries.erase(it); continue; }
        }
        ++it;
    }
    endResetModel();
    persist();
    emit recentsChanged();
}

void CartridgeModel::setBusy(bool b)
{
    if (m_busy == b) return;
    m_busy = b;
    emit busyChanged();
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
        e.sha1Hex      = s.value(QStringLiteral("sha1")).toString();      // v0.1.0
        e.source       = s.value(QStringLiteral("source")).toString();    // v0.1.0
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
        s.setValue(QStringLiteral("sha1"),     e.sha1Hex);     // v0.1.0
        s.setValue(QStringLiteral("source"),   e.source);      // v0.1.0
    }
    s.endArray();
    s.sync();
}

void CartridgeModel::addRom(const QString &path)
{
    // v0.0.x compat: alias voor addFromLocal zonder kopiëren naar storage.
    // Behoud bestaande gedrag voor SAF-picker via AddRomCard.
    addFromLocal(path, /*copyIntoStorage=*/false);
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
        case Sha1Role:       return e.sha1Hex;       // v0.1.0
        case SourceRole:     return e.source;        // v0.1.0
        case MediaTypeRole:  return mediaTypeFor(e.romPath);  // v0.2.0
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
        {Sha1Role,       "sha1Hex"},      // v0.1.0
        {SourceRole,     "source"},       // v0.1.0
        {MediaTypeRole,  "mediaType"},    // v0.2.0
    };
}
