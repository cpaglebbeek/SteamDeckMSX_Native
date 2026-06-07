#pragma once

#include <QAbstractListModel>
#include <QString>
#include <QVector>
#include <QDateTime>
#include <QUrl>
#include <qqmlregistration.h>

class FileDownloader;

// BiosEntry — één rij in de BIOS-bibliotheek.
struct BiosEntry {
    QString id;           // stable id = sha1Hex (12 chars) of fallback filename-stem
    QString fileName;     // basename (bv. "cbios.sys", "msx2p.rom")
    QString absPath;      // resolved pad in storage-dir
    QString sha1Hex;      // 40-char lowercase
    qint64  sizeBytes{0};
    QDateTime addedAt;
    QString source;       // "url:https://..." of "local:/abs/path"
};

// BiosManager — beheert lijst BIOS-files + add via URL of lokaal pad.
//
// Storage-locatie (DD-005 + DD-006):
//   QStandardPaths::AppDataLocation/bios/   (Mac: ~/Library/.../iCt-Horse/SteamDeckMSX/bios)
//                                            (Linux: ~/.local/share/iCt-Horse/SteamDeckMSX/bios)
//                                            (Flatpak: ~/.var/app/.../data/bios)
// Persistentie metadata: QSettings (BiosManager/entries/...).
//
// openMSX-integratie (DD-007): user wijst storage-dir aan als extra zoek-pad
// (via Settings of OPENMSX_USER_DATA). v0.1.0: alleen administratie, geen
// auto-machine.xml-generatie.
class BiosManager : public QAbstractListModel {
    Q_OBJECT
    QML_ELEMENT

public:
    enum Roles {
        IdRole = Qt::UserRole + 1,
        FileNameRole,
        AbsPathRole,
        Sha1Role,
        SizeRole,
        AddedAtRole,
        SourceRole,
    };

    // Max BIOS-file: 1 MiB (largest MSX-BIOS is 64 KB; ruime marge voor extensies).
    static constexpr qint64 kBiosMaxBytes = 1 * 1024 * 1024;

    explicit BiosManager(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = {}) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    // Absoluut pad naar BIOS-storage-dir. Wordt aangemaakt bij eerste call.
    Q_INVOKABLE QString storageDir() const;

    Q_PROPERTY(bool busy READ busy NOTIFY busyChanged)
    bool busy() const { return m_busy; }

public slots:
    // Voeg BIOS toe vanaf URL (HTTPS-only). preferredName = optionele naam (basename in storage).
    Q_INVOKABLE void addFromUrl(const QUrl &url, const QString &preferredName = QString());
    // Kopieer bestaand bestand naar storage-dir + neem op.
    Q_INVOKABLE bool addFromLocal(const QString &localPath, const QString &preferredName = QString());
    // Verwijder entry + file.
    Q_INVOKABLE bool removeEntry(const QString &id);
    Q_INVOKABLE void clearAll();

signals:
    void busyChanged();
    void entryAdded(const QString &id, const QString &fileName);
    void addFailed(const QString &reason);
    void downloadProgress(qint64 received, qint64 total);

private slots:
    void onDownloadFinished(const QString &destPath, const QString &sha1Hex);
    void onDownloadFailed(const QString &reason);
    void onDownloadProgress(qint64 received, qint64 total);

private:
    void loadFromSettings();
    void persistToSettings();
    QString resolveDestPath(const QString &preferredName, const QString &fallbackBasename) const;
    BiosEntry buildEntryFromFile(const QString &absPath, const QString &source, const QString &sha1Hex);
    void appendEntry(const BiosEntry &e);
    void setBusy(bool b);

    QVector<BiosEntry> m_entries;
    FileDownloader *m_downloader{nullptr};
    QString m_pendingDestPath;
    QString m_pendingSource;
    bool m_busy{false};
};
