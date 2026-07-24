// CMAKE-TODO (handled by integrator): linkken naar Qt6::GuiPrivate omdat
// qzipreader_p.h een private header is. In src/CMakeLists.txt:
//   target_link_libraries(steamdeckmsx_core PUBLIC Qt6::GuiPrivate)
//
// Trade-off private API: QZipReader leeft sinds Qt5 in QtGui/private en is
// in de praktijk stabiel (gebruikt door QTextDocumentWriter/odf-export).
// Geen ABI-garanties tussen minor-versies, maar de signatuur die we hier
// gebruiken (FileInfo, fileInfoList(), fileData(QString)) is al jaren
// ongewijzigd. Alternatief (QuaZip / minizip) zou een externe dep
// introduceren — bewust vermeden voor v0.1.0.

#include "BiosZipExtractor.h"

// Qt 6.11 verplaatste qzipreader_p.h van QtGui naar QtCore (BUG-009).
#if __has_include(<QtCore/private/qzipreader_p.h>)
#include <QtCore/private/qzipreader_p.h>
#else
#include <QtGui/private/qzipreader_p.h>
#endif

#include <QByteArray>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
#include <QSet>
#include <QString>

BiosZipExtractor::BiosZipExtractor(QObject *parent)
    : QObject(parent)
{
}

namespace {

// Lowercase extensie uit pad halen (zonder de '.'); leeg als geen extensie.
QString lcExt(const QString &filePath)
{
    const int slash = std::max(filePath.lastIndexOf(u'/'), filePath.lastIndexOf(u'\\'));
    const QString base = (slash >= 0) ? filePath.mid(slash + 1) : filePath;
    const int dot = base.lastIndexOf(u'.');
    if (dot <= 0 || dot == base.size() - 1) return {};
    return base.mid(dot + 1).toLower();
}

// Sanitize: vervang separators door '_', strip leading dots, voorkom lege string.
QString sanitizeBasename(const QString &filePath)
{
    const int slash = std::max(filePath.lastIndexOf(u'/'), filePath.lastIndexOf(u'\\'));
    QString base = (slash >= 0) ? filePath.mid(slash + 1) : filePath;
    base.replace(u'/', u'_');
    base.replace(u'\\', u'_');
    // Strip leading dots (voorkomt hidden files + ".." overblijfsels).
    while (!base.isEmpty() && base.startsWith(u'.')) {
        base.remove(0, 1);
    }
    return base;
}

// Detecteer onveilige paden in het ZIP-entry: ".." segment, absolute pad of
// leading separator. We weigeren deze zonder ze te (proberen te) normaliseren.
bool isUnsafePath(const QString &filePath)
{
    if (filePath.isEmpty()) return true;
    if (filePath.startsWith(u'/') || filePath.startsWith(u'\\')) return true;
    // Drive-letter (Windows): "C:..." — niet relevant op Linux/Mac maar
    // ZIP-payloads kunnen alles bevatten.
    if (filePath.size() >= 2 && filePath[1] == u':') return true;

    // Splits op zowel '/' als '\'.
    const auto parts = filePath.split(QRegularExpression(QStringLiteral("[/\\\\]")),
                                      Qt::SkipEmptyParts);
    for (const QString &p : parts) {
        if (p == QStringLiteral("..")) return true;
    }
    return false;
}

} // namespace

int BiosZipExtractor::extractTo(const QString &zipPath, const QString &destDir, QStringList *fileNamesOut)
{
    if (zipPath.isEmpty()) {
        emit parseError(QStringLiteral("ZIP-pad leeg"));
        return -1;
    }
    if (destDir.isEmpty()) {
        emit parseError(QStringLiteral("Dest-dir leeg"));
        return -1;
    }

    QDir d(destDir);
    if (!d.exists() && !d.mkpath(QStringLiteral("."))) {
        emit parseError(QStringLiteral("Kan dest-dir niet aanmaken: ") + destDir);
        return -1;
    }

    QZipReader reader(zipPath, QIODevice::ReadOnly);
    if (!reader.isReadable()) {
        emit parseError(QStringLiteral("Kan ZIP niet openen: ") + zipPath);
        return -1;
    }

    const auto entries = reader.fileInfoList();
    int extracted = 0;
    QSet<QString> usedBasenames;

    for (const QZipReader::FileInfo &info : entries) {
        if (extracted >= kMaxFilesPerZip) {
            emit skipped(info.filePath, QStringLiteral("limiet bereikt"));
            // Eén notificatie volstaat; doorloop afbreken bespaart werk.
            break;
        }

        if (info.isDir) {
            continue;
        }
        if (!info.isFile) {
            // Symlinks of andere zip-entry-types: weigeren.
            emit skipped(info.filePath, QStringLiteral("geen regulier bestand"));
            continue;
        }

        const QString ext = lcExt(info.filePath);
        if (ext != QStringLiteral("rom")
            && ext != QStringLiteral("sys")
            && ext != QStringLiteral("bin")) {
            emit skipped(info.filePath, QStringLiteral("extensie niet ondersteund"));
            continue;
        }

        if (info.size > kPerFileMaxBytes) {
            emit skipped(info.filePath, QStringLiteral("te groot"));
            continue;
        }

        if (isUnsafePath(info.filePath)) {
            emit skipped(info.filePath, QStringLiteral("onveilige naam"));
            continue;
        }

        QString basename = sanitizeBasename(info.filePath);
        if (basename.isEmpty()) {
            emit skipped(info.filePath, QStringLiteral("onveilige naam"));
            continue;
        }

        // Dedupe binnen één ZIP-extractie: voorkom dat twee entries elkaars
        // .part overschrijven. Bij collisie: suffix _2, _3, ...
        if (usedBasenames.contains(basename)) {
            const QFileInfo bfi(basename);
            const QString stem = bfi.completeBaseName();
            const QString suffix = bfi.suffix();
            int n = 2;
            QString candidate;
            do {
                candidate = suffix.isEmpty()
                    ? QStringLiteral("%1_%2").arg(stem).arg(n)
                    : QStringLiteral("%1_%2.%3").arg(stem).arg(n).arg(suffix);
                ++n;
            } while (usedBasenames.contains(candidate) && n < 1000);
            basename = candidate;
        }

        const QByteArray data = reader.fileData(info.filePath);
        // Defensief: na decompressie nogmaals cappen (header kan liegen).
        if (data.size() > kPerFileMaxBytes) {
            emit skipped(info.filePath, QStringLiteral("te groot"));
            continue;
        }
        if (data.isEmpty() && info.size > 0) {
            emit skipped(info.filePath, QStringLiteral("schrijf-fout"));
            continue;
        }

        const QString destPath = d.filePath(basename);
        const QString partPath = destPath + QStringLiteral(".part");

        QFile out(partPath);
        if (!out.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            emit skipped(info.filePath, QStringLiteral("schrijf-fout"));
            continue;
        }
        const qint64 wrote = out.write(data);
        out.close();
        if (wrote != data.size()) {
            QFile::remove(partPath);
            emit skipped(info.filePath, QStringLiteral("schrijf-fout"));
            continue;
        }

        // Atomisch hernoemen (zelfde patroon als FileDownloader::onFinished).
        if (QFile::exists(destPath)) {
            QFile::remove(destPath);
        }
        if (!QFile::rename(partPath, destPath)) {
            QFile::remove(partPath);
            emit skipped(info.filePath, QStringLiteral("schrijf-fout"));
            continue;
        }

        usedBasenames.insert(basename);
        if (fileNamesOut) {
            fileNamesOut->append(basename);
        }
        ++extracted;
        emit fileExtracted(basename);
    }

    reader.close();

    qInfo().noquote() << "[BiosZipExtractor]" << QFileInfo(zipPath).fileName()
                      << "extracted=" << extracted
                      << "of entries=" << entries.size();

    return extracted;
}
