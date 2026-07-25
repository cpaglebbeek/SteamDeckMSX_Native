#include "RomLibrary.h"
#include "CartridgeModel.h"

#include <QCryptographicHash>
#include <QDateTime>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <QSet>
#include <QSettings>
#include <QStandardPaths>
#include <QTimer>
#include <QVariantMap>

#include <algorithm>
#include <utility>

namespace {
constexpr auto kSettingsKeyRoots = "library/scanRoots";

QString appDataDir()
{
    const QString base = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(base);
    return base;
}

QString sha1OfFile(const QString &path)
{
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) return {};
    QCryptographicHash h(QCryptographicHash::Sha1);
    if (!h.addData(&f)) return {};
    return QString::fromLatin1(h.result().toHex());
}
}  // namespace

QString RomLibrary::cacheFilePath()
{
    return appDataDir() + QStringLiteral("/library.json");
}

QString RomLibrary::thumbnailDir()
{
    const QString dir = appDataDir() + QStringLiteral("/thumbs");
    QDir().mkpath(dir);
    return dir;
}

RomLibrary::RomLibrary(QObject *parent)
    : QAbstractListModel(parent)
    , m_tick(new QTimer(this))
{
    m_tick->setInterval(0);
    m_tick->setSingleShot(false);
    connect(m_tick, &QTimer::timeout, this, &RomLibrary::scanTick);

    QSettings s;
    m_scanRoots = s.value(QString::fromLatin1(kSettingsKeyRoots)).toStringList();
    if (m_scanRoots.isEmpty()) m_scanRoots = defaultScanRoots();

    loadCache();
}

QStringList RomLibrary::defaultScanRoots()
{
    QStringList out;
    const auto addIfExists = [&out](const QString &p) {
        if (p.isEmpty()) return;
        if (out.contains(p)) return;
        if (QFileInfo::exists(p)) out << p;
    };

    // Eigen storage (altijd aanwezig — hier landen URL-imports).
    const QString own = appDataDir() + QStringLiteral("/roms");
    QDir().mkpath(own);
    addIfExists(own);

    // Gebruikersmappen die de Flatpak-sandbox via finish-args mag zien.
    addIfExists(QStandardPaths::writableLocation(QStandardPaths::DownloadLocation));
    addIfExists(QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation));

    const QString home = QDir::homePath();
    for (const auto &sub : {"/ROMs", "/roms", "/Games/MSX", "/MSX"}) {
        addIfExists(home + QString::fromLatin1(sub));
    }

    // Steam Deck: SD-kaart en externe media.
    QDir media(QStringLiteral("/run/media"));
    if (media.exists()) {
        const auto users = media.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
        for (const QString &u : users) {
            const QDir udir(media.filePath(u));
            const auto cards = udir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
            for (const QString &c : cards) addIfExists(udir.filePath(c));
            if (cards.isEmpty()) addIfExists(udir.absolutePath());
        }
    }
    return out;
}

void RomLibrary::setScanRoots(const QStringList &roots)
{
    if (roots == m_scanRoots) return;
    m_scanRoots = roots;
    QSettings().setValue(QString::fromLatin1(kSettingsKeyRoots), m_scanRoots);
    emit scanRootsChanged();
}

void RomLibrary::addScanRoot(const QString &dir)
{
    if (dir.isEmpty() || m_scanRoots.contains(dir)) return;
    QStringList next = m_scanRoots;
    next << dir;
    setScanRoots(next);
}

void RomLibrary::removeScanRoot(const QString &dir)
{
    QStringList next = m_scanRoots;
    if (next.removeAll(dir) > 0) setScanRoots(next);
}

int RomLibrary::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) return 0;
    return static_cast<int>(m_entries.size());
}

QVariant RomLibrary::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_entries.size()) return {};
    const RomEntry &e = m_entries.at(index.row());
    switch (role) {
    case TitleRole:     return e.title;
    case RomPathRole:   return e.romPath;
    case MachineRole:   return e.machine;
    case MediaTypeRole: return e.mediaType;
    case Sha1Role:      return e.sha1Hex;
    case ThumbPathRole: return e.thumbPath;
    case HasThumbRole:  return !e.thumbPath.isEmpty();
    case SizeBytesRole: return e.sizeBytes;
    default:            return {};
    }
}

QHash<int, QByteArray> RomLibrary::roleNames() const
{
    return {
        {TitleRole,     "title"},
        {RomPathRole,   "romPath"},
        {MachineRole,   "machine"},
        {MediaTypeRole, "mediaType"},
        {Sha1Role,      "sha1"},
        {ThumbPathRole, "thumbPath"},
        {HasThumbRole,  "hasThumb"},
        {SizeBytesRole, "sizeBytes"},
    };
}

bool RomLibrary::isSupported(const QString &fileName)
{
    const QString l = fileName.toLower();
    return l.endsWith(QStringLiteral(".rom")) || l.endsWith(QStringLiteral(".dsk"))
        || l.endsWith(QStringLiteral(".cas")) || l.endsWith(QStringLiteral(".zip"));
}

QString RomLibrary::titleFromFileName(const QString &fileName)
{
    QFileInfo fi(fileName);
    QString t = fi.completeBaseName();
    // Scene-dumps staan vol met "(1985)(Konami)[SCC]"-achtige haken; die
    // maken een tegel onleesbaar. Strip ze, maar laat de kale titel staan.
    t.remove(QRegularExpression(QStringLiteral("\\s*[\\(\\[][^\\)\\]]*[\\)\\]]")));
    t.replace(QChar('_'), QChar(' '));
    t = t.simplified();
    return t.isEmpty() ? fi.completeBaseName() : t;
}

QString RomLibrary::machineFor(const QString &fileName, qint64 sizeBytes)
{
    const QString l = fileName.toLower();
    if (l.contains(QStringLiteral("turbor"))) return QStringLiteral("TurboR");
    if (l.contains(QStringLiteral("msx2+")) || l.contains(QStringLiteral("msx2plus"))) return QStringLiteral("MSX2+");
    if (l.contains(QStringLiteral("msx2"))) return QStringLiteral("MSX2");
    if (l.contains(QStringLiteral("msx1"))) return QStringLiteral("MSX1");
    // Zonder aanwijzing in de naam: kleine cartridges zijn vrijwel altijd MSX1.
    if (sizeBytes > 0 && sizeBytes <= 32 * 1024) return QStringLiteral("MSX1");
    return QStringLiteral("MSX2");
}

void RomLibrary::rescan()
{
    if (m_scanning) {
        m_tick->stop();
        m_scanning = false;
    }
    beginScan();
}

void RomLibrary::beginScan()
{
    m_pendingFiles.clear();
    m_scanResult.clear();
    m_cursor = 0;
    m_scannedFiles = 0;
    m_addedThisScan = 0;

    // Bestaande entries als cache aanbieden, zodat ongewijzigde bestanden niet
    // opnieuw gehasht worden en gevonden thumbnails behouden blijven.
    m_byPath.clear();
    for (const RomEntry &e : std::as_const(m_entries)) m_byPath.insert(e.romPath, e);

    // QSet, geen QStringList: scanroots overlappen vaak (Downloads ligt onder
    // Documents, /run/media bevat kopieën), en een lineaire contains() maakt
    // het scannen van een grote collectie kwadratisch.
    QSet<QString> seen;
    for (const QString &root : std::as_const(m_scanRoots)) {
        if (root.isEmpty() || !QFileInfo::exists(root)) continue;
        const int rootDepth = root.count(QChar('/'));
        QDirIterator it(root, QDir::Files | QDir::Readable, QDirIterator::Subdirectories);
        while (it.hasNext()) {
            const QString p = it.next();
            if (!isSupported(p)) continue;
            if (p.count(QChar('/')) - rootDepth > kMaxDepth) continue;
            if (seen.contains(p)) continue;
            seen.insert(p);
            m_pendingFiles << p;
        }
    }

    m_scanning = true;
    emit scanningChanged();
    m_tick->start();
}

void RomLibrary::scanTick()
{
    int processed = 0;
    while (m_cursor < m_pendingFiles.size() && processed < kFilesPerTick) {
        const QString path = m_pendingFiles.at(m_cursor++);
        ++processed;
        ++m_scannedFiles;

        QFileInfo fi(path);
        const qint64 size  = fi.size();
        const qint64 mtime = fi.lastModified().toSecsSinceEpoch();

        RomEntry e;
        const auto cached = m_byPath.constFind(path);
        if (cached != m_byPath.constEnd() && cached->sizeBytes == size && cached->mtimeUnix == mtime) {
            e = *cached;   // ongewijzigd: hergebruik hash + thumbnail
        } else {
            e.romPath   = path;
            e.title     = titleFromFileName(fi.fileName());
            e.mediaType = CartridgeModel::mediaTypeFor(path);
            e.machine   = machineFor(fi.fileName(), size);
            e.sizeBytes = size;
            e.mtimeUnix = mtime;
            e.sha1Hex   = sha1OfFile(path);
            ++m_addedThisScan;
        }
        // Thumbnail kan in een eerdere sessie zijn gemaakt.
        if (e.thumbPath.isEmpty() && !e.sha1Hex.isEmpty()) {
            const QString candidate = thumbnailDir() + QChar('/') + e.sha1Hex + QStringLiteral(".png");
            if (QFileInfo::exists(candidate)) e.thumbPath = candidate;
        }
        m_scanResult << e;
    }

    emit progressChanged();
    if (m_cursor >= m_pendingFiles.size()) finishScan();
}

void RomLibrary::finishScan()
{
    m_tick->stop();

    // Dedup op SHA-1: dezelfde dump in twee mappen is één spel. Zonder hash
    // (onleesbaar bestand) valt de entry terug op zijn pad als sleutel.
    QVector<RomEntry> unique;
    QSet<QString> seenKeys;
    for (const RomEntry &e : std::as_const(m_scanResult)) {
        const QString key = e.sha1Hex.isEmpty() ? e.romPath : e.sha1Hex;
        if (seenKeys.contains(key)) continue;
        seenKeys.insert(key);
        unique << e;
    }
    std::sort(unique.begin(), unique.end(), [](const RomEntry &a, const RomEntry &b) {
        return a.title.compare(b.title, Qt::CaseInsensitive) < 0;
    });

    beginResetModel();
    m_entries = unique;
    endResetModel();

    m_scanning = false;
    emit scanningChanged();
    emit countChanged();
    saveCache();
    emit scanFinished(static_cast<int>(m_entries.size()), m_addedThisScan);
}

void RomLibrary::setThumbnail(const QString &sha1Hex, const QString &thumbPath)
{
    if (sha1Hex.isEmpty()) return;
    for (int i = 0; i < m_entries.size(); ++i) {
        if (m_entries[i].sha1Hex.compare(sha1Hex, Qt::CaseInsensitive) != 0) continue;
        m_entries[i].thumbPath = thumbPath;
        const QModelIndex idx = index(i, 0);
        emit dataChanged(idx, idx, {ThumbPathRole, HasThumbRole});
        saveCache();
        return;
    }
}

QVariantList RomLibrary::entriesWithoutThumbnail(int max) const
{
    QVariantList out;
    for (const RomEntry &e : std::as_const(m_entries)) {
        if (!e.thumbPath.isEmpty()) continue;
        if (e.sha1Hex.isEmpty()) continue;
        // ZIP's kunnen we niet direct booten; die krijgen een fallback-tegel.
        if (e.mediaType == QStringLiteral("zip")) continue;
        QVariantMap m;
        m.insert(QStringLiteral("sha1"), e.sha1Hex);
        m.insert(QStringLiteral("romPath"), e.romPath);
        m.insert(QStringLiteral("mediaType"), e.mediaType);
        m.insert(QStringLiteral("title"), e.title);
        m.insert(QStringLiteral("machine"), e.machine);
        out << m;
        if (out.size() >= max) break;
    }
    return out;
}

QVariantMap RomLibrary::entryAt(int row) const
{
    QVariantMap m;
    if (row < 0 || row >= m_entries.size()) return m;
    const RomEntry &e = m_entries.at(row);
    m.insert(QStringLiteral("title"), e.title);
    m.insert(QStringLiteral("romPath"), e.romPath);
    m.insert(QStringLiteral("machine"), e.machine);
    m.insert(QStringLiteral("mediaType"), e.mediaType);
    m.insert(QStringLiteral("sha1"), e.sha1Hex);
    m.insert(QStringLiteral("thumbPath"), e.thumbPath);
    return m;
}

void RomLibrary::loadCache()
{
    QFile f(cacheFilePath());
    if (!f.open(QIODevice::ReadOnly)) return;
    const QJsonDocument doc = QJsonDocument::fromJson(f.readAll());
    if (!doc.isArray()) return;

    QVector<RomEntry> loaded;
    const QJsonArray arr = doc.array();
    for (const QJsonValue &v : arr) {
        if (!v.isObject()) continue;
        const QJsonObject o = v.toObject();
        RomEntry e;
        e.title     = o.value(QStringLiteral("title")).toString();
        e.romPath   = o.value(QStringLiteral("romPath")).toString();
        e.machine   = o.value(QStringLiteral("machine")).toString();
        e.mediaType = o.value(QStringLiteral("mediaType")).toString();
        e.sha1Hex   = o.value(QStringLiteral("sha1")).toString();
        e.thumbPath = o.value(QStringLiteral("thumbPath")).toString();
        e.sizeBytes = static_cast<qint64>(o.value(QStringLiteral("size")).toDouble());
        e.mtimeUnix = static_cast<qint64>(o.value(QStringLiteral("mtime")).toDouble());
        if (e.romPath.isEmpty()) continue;
        // Verdwenen bestanden niet tonen: een tegel die niet start is erger
        // dan een tegel die ontbreekt.
        if (!QFileInfo::exists(e.romPath)) continue;
        if (!e.thumbPath.isEmpty() && !QFileInfo::exists(e.thumbPath)) e.thumbPath.clear();
        loaded << e;
    }
    if (loaded.isEmpty()) return;

    beginResetModel();
    m_entries = loaded;
    endResetModel();
    emit countChanged();
}

void RomLibrary::saveCache()
{
    QJsonArray arr;
    for (const RomEntry &e : std::as_const(m_entries)) {
        QJsonObject o;
        o.insert(QStringLiteral("title"), e.title);
        o.insert(QStringLiteral("romPath"), e.romPath);
        o.insert(QStringLiteral("machine"), e.machine);
        o.insert(QStringLiteral("mediaType"), e.mediaType);
        o.insert(QStringLiteral("sha1"), e.sha1Hex);
        o.insert(QStringLiteral("thumbPath"), e.thumbPath);
        o.insert(QStringLiteral("size"), static_cast<double>(e.sizeBytes));
        o.insert(QStringLiteral("mtime"), static_cast<double>(e.mtimeUnix));
        arr << o;
    }
    QFile f(cacheFilePath());
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) return;
    f.write(QJsonDocument(arr).toJson(QJsonDocument::Compact));
}
