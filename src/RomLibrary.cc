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

    // Downloads: hier belanden ROMs in de praktijk.
    // Documenten staat er bewust NIET bij: die map is bij de meeste mensen vol
    // persoonlijke bestanden, en scannen levert vooral valse treffers op in
    // plaats van spellen. Wie zijn collectie daar heeft staan, voegt de map
    // zelf toe via addScanRoot().
    addIfExists(QStandardPaths::writableLocation(QStandardPaths::DownloadLocation));

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

bool RomLibrary::isSupported(const QString &path)
{
    const QString l = path.toLower();
    if (l.endsWith(QStringLiteral(".rom")) || l.endsWith(QStringLiteral(".dsk"))
        || l.endsWith(QStringLiteral(".cas"))) {
        return true;
    }
    // ZIP is bewust géén algemeen ondersteunde extensie. Een ZIP kán een ROM
    // bevatten, maar in Downloads staan vooral archieven die niets met MSX te
    // maken hebben — die vulden de galerij met documenten en dossiers (gemeten:
    // 159 valse treffers op één machine). Alleen meenemen als de map zichzelf
    // als ROM-map aankondigt.
    if (!l.endsWith(QStringLiteral(".zip"))) return false;
    // Uitsluitend de map waarin de zip *direct* staat telt. Hoger in het pad
    // kijken is te grof: één bovenliggende map die toevallig "msx" bevat
    // (bijvoorbeeld een tijdelijke map met de applicatienaam erin) zou dan élke
    // zip eronder accepteren. Wie een verzameling dieper heeft staan, voegt die
    // map zelf toe als scanroot.
    const QStringList parts = path.left(path.lastIndexOf(QChar('/'))).split(QChar('/'),
                                                                           Qt::SkipEmptyParts);
    if (parts.isEmpty()) return false;
    const QString parent = parts.last().toLower();
    return parent.contains(QStringLiteral("rom")) || parent.contains(QStringLiteral("msx"))
        || parent == QStringLiteral("games");
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

bool RomLibrary::shouldSkipDir(const QString &dirName)
{
    if (dirName.startsWith(QChar('.'))) return true;   // .git, .cache, .steam …
    static const QStringList kSkip = {
        QStringLiteral("node_modules"), QStringLiteral("Library"),
        QStringLiteral("System"),       QStringLiteral("Applications"),
        QStringLiteral("proc"),         QStringLiteral("sys"),
    };
    return kSkip.contains(dirName);
}

void RomLibrary::beginScan()
{
    m_scanResult.clear();
    m_seenPaths.clear();
    m_dirStack.clear();
    m_scannedFiles = 0;
    m_addedThisScan = 0;

    // Bestaande entries als cache aanbieden, zodat ongewijzigde bestanden niet
    // opnieuw gehasht worden en gevonden thumbnails behouden blijven.
    m_byPath.clear();
    for (const RomEntry &e : std::as_const(m_entries)) m_byPath.insert(e.romPath, e);

    // Alleen de roots op de stack: de boom wordt pas tijdens de ticks afgelopen.
    for (const QString &root : std::as_const(m_scanRoots)) {
        if (root.isEmpty() || !QFileInfo::exists(root)) continue;
        m_dirStack.append({root, 0});
    }

    m_scanning = true;
    emit scanningChanged();
    m_tick->start();
}

void RomLibrary::processFile(const QString &path)
{
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

void RomLibrary::scanTick()
{
    // Eén map per tick uitlezen, met een budget aan verwerkte bestanden. Zo
    // blijft ook een boom met tienduizenden bestanden responsief: de scan
    // vordert zichtbaar in plaats van de UI seconden vast te zetten.
    int budget = kFilesPerTick;
    while (budget > 0 && !m_dirStack.isEmpty()) {
        const auto [dirPath, depth] = m_dirStack.takeLast();
        QDir dir(dirPath);
        const auto entries = dir.entryInfoList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot
                                                   | QDir::Readable | QDir::NoSymLinks,
                                               QDir::Name);
        for (const QFileInfo &fi : entries) {
            if (fi.isDir()) {
                if (depth >= kMaxDepth) continue;
                if (shouldSkipDir(fi.fileName())) continue;
                m_dirStack.append({fi.absoluteFilePath(), depth + 1});
                continue;
            }
            const QString p = fi.absoluteFilePath();
            if (!isSupported(p)) continue;   // volledig pad: .zip telt alleen in ROM-mappen
            if (m_seenPaths.contains(p)) continue;   // roots kunnen overlappen
            m_seenPaths.insert(p);
            processFile(p);
            --budget;
        }
    }

    emit progressChanged();
    if (m_dirStack.isEmpty()) finishScan();
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
