#include "FileDownloader.h"
#include "RomTypeDetector.h"

#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QTimer>
#include <QDebug>

FileDownloader::FileDownloader(QObject *parent)
    : QObject(parent)
    , m_nam(new QNetworkAccessManager(this))
{
}

FileDownloader::~FileDownloader()
{
    cleanup();
}

void FileDownloader::setCredentials(const QString &user, const QString &password)
{
    m_authUser = user;
    m_authPassword = password;
}

void FileDownloader::clearCredentials()
{
    m_authUser.clear();
    m_authPassword.clear();
}

bool FileDownloader::start(const QUrl &url, const QString &destPath, qint64 maxBytes)
{
    if (m_reply) {
        emitFailure(QStringLiteral("Andere download al actief"));
        return false;
    }
    if (!url.isValid()) {
        emitFailure(QStringLiteral("Ongeldige URL"));
        return false;
    }
    // DD-001: HTTPS-only.
    if (url.scheme().compare(QStringLiteral("https"), Qt::CaseInsensitive) != 0) {
        emitFailure(QStringLiteral("Alleen HTTPS toegestaan (HTTP geweigerd voor veiligheid)"));
        return false;
    }
    if (destPath.isEmpty()) {
        emitFailure(QStringLiteral("Bestemming-pad leeg"));
        return false;
    }

    // Maak parent-dir aan indien nodig.
    const QFileInfo fi(destPath);
    QDir d(fi.absolutePath());
    if (!d.exists() && !d.mkpath(QStringLiteral("."))) {
        emitFailure(QStringLiteral("Kan parent-directory niet aanmaken: ") + fi.absolutePath());
        return false;
    }

    m_destPath = destPath;
    m_maxBytes = (maxBytes > 0) ? maxBytes : kDefaultMaxBytes;
    m_bytesReceived = 0;
    m_bytesTotal = 0;
    m_buffer.clear();

    QNetworkRequest req(url);
    // DD-002: redirects toegestaan binnen HTTPS-domein.
    req.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                     QNetworkRequest::NoLessSafeRedirectPolicy);
    // DD-004: 30s timeout.
    req.setTransferTimeout(30 * 1000);
    req.setHeader(QNetworkRequest::UserAgentHeader,
                  QStringLiteral("SteamDeckMSX/0.1.0 FileDownloader"));
    // Basic-auth vooraf meesturen in plaats van wachten op de 401-uitdaging:
    // dat scheelt een round-trip en voorkomt dat Qt om gegevens vraagt in een
    // dialoog die op een handheld niet te bedienen is. Alleen over https, wat
    // hierboven al is afgedwongen — anders zou het wachtwoord leesbaar meegaan.
    if (!m_authUser.isEmpty()) {
        const QByteArray token = (m_authUser + QChar(':') + m_authPassword).toUtf8().toBase64();
        req.setRawHeader("Authorization", "Basic " + token);
    }

    m_reply = m_nam->get(req);
    if (!m_reply) {
        emitFailure(QStringLiteral("QNetworkAccessManager weigerde request"));
        return false;
    }
    connect(m_reply, &QNetworkReply::readyRead,         this, &FileDownloader::onReadyRead);
    connect(m_reply, &QNetworkReply::finished,          this, &FileDownloader::onFinished);
    connect(m_reply, &QNetworkReply::downloadProgress,  this, &FileDownloader::onProgress);

    emit activeChanged();
    return true;
}

void FileDownloader::cancel()
{
    if (!m_reply) return;
    m_reply->abort();  // triggert onFinished met error.
}

void FileDownloader::onReadyRead()
{
    if (!m_reply) return;
    const QByteArray chunk = m_reply->readAll();
    m_buffer.append(chunk);
    if (m_buffer.size() > m_maxBytes) {
        m_reply->abort();
        emitFailure(QStringLiteral("Bestand overschrijdt cap (%1 bytes)").arg(m_maxBytes));
    }
}

void FileDownloader::onProgress(qint64 received, qint64 total)
{
    m_bytesReceived = received;
    m_bytesTotal    = total;
    emit progress(received, total);
}

void FileDownloader::onFinished()
{
    if (!m_reply) return;
    const auto err = m_reply->error();
    if (err != QNetworkReply::NoError) {
        const QString reason = m_reply->errorString();
        cleanup();
        emit failed(reason);
        return;
    }

    // Schrijf atomisch via .part-suffix + rename.
    const QString partPath = m_destPath + QStringLiteral(".part");
    QFile out(partPath);
    if (!out.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        cleanup();
        emit failed(QStringLiteral("Kan bestemmings-bestand niet schrijven: ") + out.errorString());
        return;
    }
    const qint64 wrote = out.write(m_buffer);
    out.close();
    if (wrote != m_buffer.size()) {
        QFile::remove(partPath);
        cleanup();
        emit failed(QStringLiteral("Schrijf-fout (%1 / %2 bytes)").arg(wrote).arg(m_buffer.size()));
        return;
    }

    // Atomisch hernoemen.
    if (QFile::exists(m_destPath)) {
        QFile::remove(m_destPath);
    }
    if (!QFile::rename(partPath, m_destPath)) {
        QFile::remove(partPath);
        cleanup();
        emit failed(QStringLiteral("Rename .part → dest mislukte"));
        return;
    }

    // SHA-1 fingerprint via bestaande RomTypeDetector-helper (single-source-of-truth).
    const QString sha = RomTypeDetector::sha1Hex(m_buffer);

    qInfo().noquote() << "[FileDownloader] OK" << QFileInfo(m_destPath).fileName()
                      << "size=" << m_buffer.size()
                      << "sha1=" << sha.left(12) + QStringLiteral("…");

    const QString dest = m_destPath;
    cleanup();
    emit finished(dest, sha);
}

void FileDownloader::cleanup()
{
    if (m_reply) {
        m_reply->deleteLater();
        m_reply = nullptr;
    }
    m_buffer.clear();
    emit activeChanged();
}

void FileDownloader::emitFailure(const QString &reason)
{
    cleanup();
    emit failed(reason);
}
