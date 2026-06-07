#pragma once

#include <QObject>
#include <QString>
#include <QUrl>
#include <QByteArray>
#include <qqmlregistration.h>

class QNetworkAccessManager;
class QNetworkReply;

// FileDownloader — async HTTPS-only file fetcher met progress + cancel + SHA-1.
//
// Gebruikt door BiosManager en CartridgeModel om ROMs/BIOS-files te downloaden.
// Eén instance kan meerdere downloads sequentieel doen (geen parallel).
//
// Design-keuzes (zie docs/DESIGN_DECISIONS_v0.1.0.md DD-001 t/m DD-004):
//  - HTTPS-only (HTTP geweigerd) — security
//  - Redirects toegestaan binnen HTTPS-domein
//  - Max-size cap (per call configureerbaar, default 8 MiB)
//  - Timeout 30s (Qt6 QNetworkRequest::TransferTimeoutPreference)
class FileDownloader : public QObject {
    Q_OBJECT
    QML_ELEMENT

public:
    Q_PROPERTY(bool active READ active NOTIFY activeChanged)
    Q_PROPERTY(qint64 bytesReceived READ bytesReceived NOTIFY progress)
    Q_PROPERTY(qint64 bytesTotal READ bytesTotal NOTIFY progress)

    explicit FileDownloader(QObject *parent = nullptr);
    ~FileDownloader();

    bool active() const { return m_reply != nullptr; }
    qint64 bytesReceived() const { return m_bytesReceived; }
    qint64 bytesTotal() const { return m_bytesTotal; }

    static constexpr qint64 kDefaultMaxBytes = 8 * 1024 * 1024;  // 8 MiB

public slots:
    // Start een download. Returnt false als URL ongeldig is of er al een download loopt.
    // destPath = absoluut pad waar bytes naartoe worden geschreven bij succes.
    // maxBytes = harde cap; >cap → cancel + error.
    Q_INVOKABLE bool start(const QUrl &url, const QString &destPath, qint64 maxBytes = kDefaultMaxBytes);
    Q_INVOKABLE void cancel();

signals:
    void activeChanged();
    void progress(qint64 received, qint64 total);
    // Succes: destPath geschreven, sha1Hex berekend. Pad + sha1 in signal.
    void finished(const QString &destPath, const QString &sha1Hex);
    // Mislukt: error code (HTTP / network / cap / cancel) + leesbare message.
    void failed(const QString &reason);

private slots:
    void onReadyRead();
    void onFinished();
    void onProgress(qint64 received, qint64 total);

private:
    QNetworkAccessManager *m_nam{nullptr};
    QNetworkReply *m_reply{nullptr};
    QString m_destPath;
    qint64  m_maxBytes{kDefaultMaxBytes};
    qint64  m_bytesReceived{0};
    qint64  m_bytesTotal{0};
    QByteArray m_buffer;  // accumulating tot finished; write atomisch aan eind

    void cleanup();
    void emitFailure(const QString &reason);
};
