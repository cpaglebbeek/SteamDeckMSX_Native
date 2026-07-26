#include "OnlineIndex.h"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QSet>
#include <QStandardPaths>
#include <QUrl>
#include <QDebug>

namespace {
// Alleen bestandstypen die de emulator ook echt kan laden; de index bevat
// verder documentatie, torrents en installers die hier niets te zoeken hebben.
bool isPlayable(const QString &name)
{
    static const QStringList exts{
        QStringLiteral(".rom"), QStringLiteral(".dsk"), QStringLiteral(".cas"),
        QStringLiteral(".zip"), QStringLiteral(".mx1"), QStringLiteral(".mx2")
    };
    for (const auto &e : exts) {
        if (name.endsWith(e, Qt::CaseInsensitive)) return true;
    }
    return false;
}
}

OnlineIndex::OnlineIndex(QObject *parent)
    : QAbstractListModel(parent)
{
}

int OnlineIndex::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) return 0;
    return m_view.size();
}

QVariant OnlineIndex::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_view.size())
        return {};
    const Entry &e = m_all.at(m_view.at(index.row()));
    switch (role) {
        case NameRole:   return e.name;
        case PathRole:   return e.path;
        case FolderRole: return e.folder;
        case UrlRole:    return m_baseUrl + QString::fromLatin1(QUrl::toPercentEncoding(e.path, "/"));
        default:         return {};
    }
}

QHash<int, QByteArray> OnlineIndex::roleNames() const
{
    return {
        {NameRole,   "name"},
        {PathRole,   "path"},
        {UrlRole,    "url"},
        {FolderRole, "folder"},
    };
}

QVariantMap OnlineIndex::entryAt(int row) const
{
    QVariantMap m;
    if (row < 0 || row >= m_view.size()) return m;
    const Entry &e = m_all.at(m_view.at(row));
    m.insert(QStringLiteral("name"), e.name);
    m.insert(QStringLiteral("path"), e.path);
    m.insert(QStringLiteral("folder"), e.folder);
    m.insert(QStringLiteral("url"),
             m_baseUrl + QString::fromLatin1(QUrl::toPercentEncoding(e.path, "/")));
    return m;
}

void OnlineIndex::setIndexUrl(const QString &u)
{
    if (u == m_indexUrl) return;
    m_indexUrl = u;
    emit indexUrlChanged();
}

void OnlineIndex::setBaseUrl(const QString &u)
{
    if (u == m_baseUrl) return;
    m_baseUrl = u;
    emit baseUrlChanged();
}

void OnlineIndex::setQuery(const QString &q)
{
    if (q == m_query) return;
    m_query = q;
    emit queryChanged();
    applyFilter();
}

void OnlineIndex::setFolder(const QString &f)
{
    if (f == m_folder) return;
    m_folder = f;
    emit folderChanged();
    applyFilter();
}

void OnlineIndex::setLoading(bool on)
{
    if (on == m_loading) return;
    m_loading = on;
    emit loadingChanged();
}

void OnlineIndex::setStatus(const QString &s)
{
    if (s == m_status) return;
    m_status = s;
    emit statusChanged();
}

QString OnlineIndex::cachePath() const
{
    const QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
                        + QStringLiteral("/online");
    QDir().mkpath(dir);
    return dir + QStringLiteral("/index.txt");
}

bool OnlineIndex::loadFromCache()
{
    QFile f(cachePath());
    if (!f.open(QIODevice::ReadOnly)) return false;
    const QByteArray raw = f.readAll();
    f.close();
    if (raw.isEmpty()) return false;
    parseIndex(raw);
    return true;
}

void OnlineIndex::refresh(int maxAgeHours)
{
    // Een index van tienduizenden regels hoeft niet bij elke start opnieuw
    // opgehaald te worden; hij verandert hooguit dagelijks.
    const QFileInfo fi(cachePath());
    if (fi.exists() && fi.size() > 0
        && fi.lastModified().secsTo(QDateTime::currentDateTime()) < maxAgeHours * 3600) {
        if (loadFromCache()) {
            setStatus(tr("%1 bestanden (uit cache)").arg(m_all.size()));
            emit refreshed(m_all.size());
            return;
        }
    }
    forceRefresh();
}

void OnlineIndex::forceRefresh()
{
    if (m_loading) return;
    if (m_indexUrl.isEmpty()) {
        emit failed(tr("geen index-URL ingesteld"));
        return;
    }
    const QUrl url(m_indexUrl);
    // Zelfde regel als bij ROM-downloads (DD-001): niets over plain HTTP.
    if (url.scheme().compare(QStringLiteral("https"), Qt::CaseInsensitive) != 0) {
        emit failed(tr("alleen https is toegestaan"));
        return;
    }

    setLoading(true);
    setStatus(tr("lijst ophalen…"));

    QNetworkRequest req(url);
    req.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                     QNetworkRequest::NoLessSafeRedirectPolicy);
    // Bewust een vaste naam: STEAMDECKMSX_VERSION is alleen op het app-target
    // gedefinieerd, niet op de core-lib waar dit bestand in zit.
    req.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("SteamDeckMSX"));
    m_reply = m_net.get(req);

    connect(m_reply, &QNetworkReply::finished, this, [this]() {
        QNetworkReply *r = m_reply;
        m_reply = nullptr;
        setLoading(false);
        if (!r) return;
        r->deleteLater();

        if (r->error() != QNetworkReply::NoError) {
            setStatus(tr("ophalen mislukt"));
            emit failed(r->errorString());
            // Terugvallen op een oudere kopie is beter dan een leeg scherm.
            if (loadFromCache()) {
                setStatus(tr("%1 bestanden (oudere kopie)").arg(m_all.size()));
            }
            return;
        }

        const QByteArray raw = r->readAll();
        QFile f(cachePath());
        if (f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            f.write(raw);
            f.close();
        }
        parseIndex(raw);
        setStatus(tr("%1 bestanden").arg(m_all.size()));
        emit refreshed(m_all.size());
    });
}

void OnlineIndex::parseIndex(const QByteArray &raw)
{
    beginResetModel();
    m_all.clear();
    m_view.clear();

    QSet<QString> folders;
    const QList<QByteArray> lines = raw.split('\n');
    m_all.reserve(lines.size());
    for (const QByteArray &lineRaw : lines) {
        QString line = QString::fromUtf8(lineRaw).trimmed();
        if (line.isEmpty()) continue;
        // Byte-order-mark van de bron aan het begin van het bestand.
        if (line.startsWith(QChar(0xFEFF))) line.remove(0, 1);
        if (!isPlayable(line)) continue;

        Entry e;
        e.path = line;
        const int slash = line.lastIndexOf(QChar('/'));
        e.name = (slash >= 0) ? line.mid(slash + 1) : line;
        e.folder = (slash >= 0) ? line.left(slash) : QString();
        if (!e.folder.isEmpty()) folders.insert(e.folder);
        m_all.append(e);
    }
    endResetModel();

    m_folders = folders.values();
    m_folders.sort(Qt::CaseInsensitive);
    emit foldersChanged();
    emit totalChanged();
    applyFilter();
}

void OnlineIndex::applyFilter()
{
    beginResetModel();
    m_view.clear();
    const QString q = m_query.trimmed();
    for (int i = 0; i < m_all.size(); ++i) {
        const Entry &e = m_all.at(i);
        if (!m_folder.isEmpty() && !e.folder.startsWith(m_folder, Qt::CaseInsensitive))
            continue;
        if (!q.isEmpty() && !e.name.contains(q, Qt::CaseInsensitive))
            continue;
        m_view.append(i);
        // Een handheld toont er hooguit een paar tientallen; verder vullen kost
        // alleen geheugen en maakt het typen traag.
        if (m_view.size() >= 400) break;
    }
    endResetModel();
}
