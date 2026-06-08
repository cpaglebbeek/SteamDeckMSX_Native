#pragma once

#include <QAbstractListModel>
#include <QString>
#include <QUrl>
#include <QVector>
#include <qqmlregistration.h>

class FileDownloader;

struct CartridgeEntry {
    QString title;          // display title (filename stem)
    QString romPath;        // absolute path; empty == sentinel
    qint64  lastUsedUnix;   // sort key for recents
    QString machine;        // MSX1/MSX2/MSX2+/TurboR — heuristic, default MSX2
    QString sha1Hex;        // v0.1.0: fingerprint (lege string voor sentinel)
    QString source;         // v0.1.0: "local:<path>" of "url:<url>"
};

class CartridgeModel : public QAbstractListModel {
    Q_OBJECT
    QML_ELEMENT

public:
    enum Roles {
        TitleRole = Qt::UserRole + 1,
        RomPathRole,
        MachineRole,
        IsSentinelRole,
        LastUsedRole,
        Sha1Role,           // v0.1.0
        SourceRole,         // v0.1.0
        MediaTypeRole       // v0.2.0: "rom" / "dsk" / "cas" — bepaalt openMSX Tcl-cmd-route
    };

    // v0.2.0: detecteer media-type per file-extensie.
    static QString mediaTypeFor(const QString &path);

    // ROM-cap: 8 MiB (MSX2 mega-ROM max ~4MB, ruime marge).
    static constexpr qint64 kRomMaxBytes = 8 * 1024 * 1024;

    explicit CartridgeModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = {}) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    Q_PROPERTY(bool busy READ busy NOTIFY busyChanged)
    bool busy() const { return m_busy; }

    // Storage-dir voor ge-importeerde / gedownloade ROMs.
    Q_INVOKABLE QString storageDir() const;

public slots:
    // v0.0.x compat: addRom(path) = local-import-pad.
    void addRom(const QString &path);
    // v0.1.0: addFromLocal kopieert (of registreert in-place) een lokaal pad.
    // copyIntoStorage=true → kopieer naar storageDir() en registreer dat pad
    // (handig voor portability / Flatpak-sandbox);
    // copyIntoStorage=false → registreer pad direct.
    Q_INVOKABLE bool addFromLocal(const QString &path, bool copyIntoStorage = true);
    // v0.1.0: addFromUrl downloadt asynchroon naar storageDir/<preferredName>
    // en voegt toe na succes. Resultaat via downloadFinished / downloadFailed signals.
    Q_INVOKABLE void addFromUrl(const QUrl &url, const QString &preferredName = QString());
    void clearRecents();

signals:
    void recentsChanged();
    void busyChanged();
    void downloadProgress(qint64 received, qint64 total);
    void downloadFinished(const QString &title, const QString &romPath);
    void downloadFailed(const QString &reason);

private slots:
    void onDownloadFinished(const QString &destPath, const QString &sha1Hex);
    void onDownloadFailed(const QString &reason);
    void onDownloadProgress(qint64 received, qint64 total);

private:
    void load();
    void persist();
    QString resolveDestPath(const QString &preferredName, const QString &fallbackBasename) const;
    void registerLocal(const QString &absPath, const QString &source);
    void setBusy(bool b);

    static constexpr int kMaxRecents = 8;
    QVector<CartridgeEntry> m_entries;
    FileDownloader *m_downloader{nullptr};
    QString m_pendingDestPath;
    QString m_pendingSource;
    bool m_busy{false};
};
