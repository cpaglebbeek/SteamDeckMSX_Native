#pragma once

#include <QAbstractListModel>
#include <QHash>
#include <QStringList>
#include <QVector>
#include <qqmlregistration.h>

class QTimer;

// RomLibrary — v0.3.0-MazeOfGalious: volledige lokale ROM-bibliotheek.
//
// Waar CartridgeModel een korte recents-lijst is (max 8, handmatig gevuld),
// scant RomLibrary alle geconfigureerde mappen en levert *alles* wat er staat
// als grid-model voor de galerij. Beide bestaan naast elkaar: recents blijft de
// snelle route, de bibliotheek is de bladerroute.
//
// Scannen gebeurt incrementeel op de UI-thread via een 0ms-timer die per tick
// een klein aantal bestanden verwerkt. Bewust géén threads: de UI blijft
// responsief, er zijn geen races op het model, en het is deterministisch
// testbaar. SHA-1 wordt alleen berekend als (mtime, size) afwijkt van de cache,
// dus een rescan van een ongewijzigde map kost vrijwel niets.
struct RomEntry {
    QString title;       // filename-stem, opgeschoond
    QString romPath;     // absoluut pad
    QString machine;     // MSX1/MSX2/MSX2+/TurboR (heuristiek + SoftwareDb)
    QString mediaType;   // "rom" / "dsk" / "cas" / "zip"
    QString sha1Hex;     // fingerprint; leeg tot gehasht
    QString thumbPath;   // absoluut pad naar gecachete screenshot; leeg = nog geen
    qint64  sizeBytes{0};
    qint64  mtimeUnix{0};
};

class RomLibrary : public QAbstractListModel {
    Q_OBJECT
    QML_ELEMENT

public:
    enum Roles {
        TitleRole = Qt::UserRole + 1,
        RomPathRole,
        MachineRole,
        MediaTypeRole,
        Sha1Role,
        ThumbPathRole,
        HasThumbRole,
        SizeBytesRole
    };

    explicit RomLibrary(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = {}) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    Q_PROPERTY(bool scanning READ scanning NOTIFY scanningChanged)
    Q_PROPERTY(int count READ rowCount NOTIFY countChanged)
    Q_PROPERTY(int scannedFiles READ scannedFiles NOTIFY progressChanged)
    Q_PROPERTY(QStringList scanRoots READ scanRoots WRITE setScanRoots NOTIFY scanRootsChanged)

    bool scanning() const { return m_scanning; }
    int scannedFiles() const { return m_scannedFiles; }
    QStringList scanRoots() const { return m_scanRoots; }
    void setScanRoots(const QStringList &roots);

    // Mappen die standaard worden aangeboden. Alleen bestaande paden komen
    // terug — de Flatpak-sandbox ziet lang niet alles wat op de host bestaat.
    Q_INVOKABLE static QStringList defaultScanRoots();

    // Start een (her)scan. Idempotent: een lopende scan wordt afgebroken en
    // opnieuw gestart.
    Q_INVOKABLE void rescan();
    Q_INVOKABLE void addScanRoot(const QString &dir);
    Q_INVOKABLE void removeScanRoot(const QString &dir);

    // Koppelt een gegenereerde thumbnail aan de entry met deze SHA-1.
    Q_INVOKABLE void setThumbnail(const QString &sha1Hex, const QString &thumbPath);

    // Entries zonder thumbnail, oplopend — voedt de ThumbnailGenerator-queue.
    // Elk element: {sha1, romPath, mediaType, title}.
    Q_INVOKABLE QVariantList entriesWithoutThumbnail(int max = 500) const;

    Q_INVOKABLE QVariantMap entryAt(int row) const;

    // Cache-locatie (JSON). Publiek zodat tests hem kunnen inspecteren.
    static QString cacheFilePath();
    static QString thumbnailDir();

signals:
    void scanningChanged();
    void countChanged();
    void progressChanged();
    void scanRootsChanged();
    void scanFinished(int total, int added);

private:
    void loadCache();
    void saveCache();
    void beginScan();
    void scanTick();
    void finishScan();
    void processFile(const QString &path);
    static QString titleFromFileName(const QString &fileName);
    static QString machineFor(const QString &fileName, qint64 sizeBytes);
    // Neemt het volledige pad, niet alleen de bestandsnaam: of een .zip meetelt
    // hangt af van de map waarin hij staat.
    static bool isSupported(const QString &path);
    // Mappen die we nooit binnengaan: verborgen mappen, repo-interne mappen en
    // systeemmappen. Zonder deze filter loopt een scan van Documenten dood in
    // .git/node_modules-bomen.
    static bool shouldSkipDir(const QString &dirName);

    // Per tick verwerkte bestanden. Klein genoeg om frames niet te missen,
    // groot genoeg om een map met honderden ROMs snel door te komen.
    static constexpr int kFilesPerTick = 24;
    // Ondergrens tegen scannen van een hele home-map: max diepte vanaf root.
    static constexpr int kMaxDepth = 6;

    QVector<RomEntry> m_entries;
    QStringList m_scanRoots;
    QTimer *m_tick{nullptr};

    // Scan-state. De mappenboom wordt tijdens de scan afgelopen, niet vooraf:
    // vooraf enumereren blokkeert de start seconden lang zodra een scanroot een
    // grote boom is (Documenten met repo's erin).
    QVector<QPair<QString, int>> m_dirStack;   // (map, diepte)
    QStringList m_pendingFiles;                // gevonden, nog te hashen
    QSet<QString> m_seenPaths;
    // Sleutels (sha1, of pad als er geen hash is) die deze scan heeft gezien.
    // Entries die aan het eind ontbreken, zijn van schijf verdwenen.
    QSet<QString> m_seenKeys;
    // Sleutels die al in m_entries zitten — O(1)-check bij het invoegen.
    QSet<QString> m_liveKeys;
    QHash<QString, RomEntry> m_byPath;   // cache van vorige scan (pad → entry)
    int m_scannedFiles{0};
    int m_addedThisScan{0};
    bool m_scanning{false};
};
