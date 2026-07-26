#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <qqmlregistration.h>

// BiosZipExtractor — extract .rom/.sys/.bin files uit een ZIP naar een dest-dir.
//
// Bedoeld als koppel-stuk vóór BiosManager::addFromLocal(...): UI selecteert
// een .zip, deze klasse pakt de relevante BIOS-files uit naar de storage-dir,
// en Main.qml roept per geslaagde extractie BiosManager::addFromLocal(absPath)
// aan.
//
// Design-keuzes:
//  - Whitelist extensies: .rom, .sys, .bin (lowercase). Alle andere files
//    worden via skipped(...) gemeld maar leiden niet tot een parse-fout.
//  - Per-file cap 1 MiB (consistent met BiosManager::kBiosMaxBytes).
//  - Hard cap 64 files per zip (anti zip-bomb).
//  - Path-traversal-defense: weiger entries met "..", absolute paden of
//    leading separators. Schrijf altijd alleen de gesanitiseerde basenname
//    in destDir.
//  - Atomic write via .part + rename (zelfde patroon als FileDownloader).
//  - Geen externe deps: gebruikt Qt6 private QZipReader (zie .cc).
class BiosZipExtractor : public QObject {
    Q_OBJECT
    QML_ELEMENT
public:
    explicit BiosZipExtractor(QObject *parent = nullptr);

    // Per BIOS-file size cap (consistent met BiosManager::kBiosMaxBytes = 1 MiB).
    static constexpr qint64 kPerFileMaxBytes = 1 * 1024 * 1024;
    // Hard cap totaal aantal files dat we accepteren uit één ZIP (anti-zip-bomb).
    static constexpr int kMaxFilesPerZip = 64;

    // v0.4.1 (BUG-028): dezelfde extractie-motor dient nu ook de download-flow
    // van spellen. De whitelist en de per-file-cap zijn daarom instelbaar;
    // de defaults blijven exact het BIOS-gedrag van v0.2.0.
    void setAllowedExtensions(const QStringList &lowercaseExts) { m_exts = lowercaseExts; }
    void setPerFileMaxBytes(qint64 cap) { m_perFileMax = cap; }

public slots:
    // Extract alle valide files uit zip naar destDir. Return aantal geslaagd.
    // -1 bij parse-fout. fileNamesOut wordt gevuld met basenames van succesvolle extracties.
    Q_INVOKABLE int extractTo(const QString &zipPath, const QString &destDir, QStringList *fileNamesOut = nullptr);

signals:
    void fileExtracted(const QString &fileName);
    void skipped(const QString &fileName, const QString &reason);
    void parseError(const QString &reason);

private:
    QStringList m_exts{QStringLiteral("rom"), QStringLiteral("sys"), QStringLiteral("bin")};
    qint64 m_perFileMax{kPerFileMaxBytes};
};
