#include "OpenmsxLocator.h"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QSettings>
#include <QStandardPaths>

OpenmsxLocator::OpenmsxLocator(QObject *parent)
    : QObject(parent)
{
    refresh();
}

QString OpenmsxLocator::userPath() const
{
    QSettings s;
    return s.value(QStringLiteral("openmsx/userPath")).toString();
}

void OpenmsxLocator::setUserPath(const QString &p)
{
    QSettings s;
    s.setValue(QStringLiteral("openmsx/userPath"), p);
    refresh();
}

void OpenmsxLocator::refresh()
{
    m_searched.clear();
    m_found.clear();
    m_dataPath.clear();
    m_dataSearched.clear();

    const auto check = [&](const QString &candidate) -> bool {
        if (candidate.isEmpty()) return false;
        m_searched << candidate;
        const QFileInfo fi(candidate);
        if (fi.exists() && fi.isExecutable()) {
            m_found = candidate;
            return true;
        }
        return false;
    };

    bool ok = check(userPath());
    if (!ok) ok = check(QStandardPaths::findExecutable(QStringLiteral("openmsx")));
    if (!ok) ok = check(QStringLiteral("/app/bin/openmsx"));

    if (!ok) {
        const QString appDir = QCoreApplication::applicationDirPath();
        // Verschillende relative-roots afhankelijk van waar de exe staat:
        //   build/native-debug/bin/steamdeckmsx → repo-root = ../../../
        //   build/steamdeck-release/bin/...     → idem
        //   Flatpak install: /app/bin/...       → wordt door /app/bin/openmsx check al gecovered
        const QStringList relativeRoots = {
            QStringLiteral("/../../../"),
            QStringLiteral("/../../"),
            QStringLiteral("/../"),
        };
        const QStringList platformPaths = {
            QStringLiteral("externals/openmsx/derived/x86_64-darwin-opt/bin/openmsx"),
            QStringLiteral("externals/openmsx/derived/aarch64-darwin-opt/bin/openmsx"),
            QStringLiteral("externals/openmsx/derived/x86_64-linux-opt/bin/openmsx"),
            QStringLiteral("externals/openmsx/derived/aarch64-linux-opt/bin/openmsx"),
        };
        for (const auto &root : relativeRoots) {
            for (const auto &p : platformPaths) {
                const QString abs = QDir(appDir + root + p).absolutePath();
                if (check(abs)) { ok = true; break; }
            }
            if (ok) break;
        }
    }

    if (ok) discoverDataPath();

    emit foundChanged();
    emit dataPathChanged();
}

void OpenmsxLocator::discoverDataPath()
{
    if (m_found.isEmpty()) return;

    const auto check = [&](const QString &candidate) -> bool {
        if (candidate.isEmpty()) return false;
        const QString abs = QDir(candidate).absolutePath();
        m_dataSearched << abs;
        // share-dir is geldig als machines/C-BIOS_MSX2+.xml of skins/ aanwezig is
        const QFileInfo machines(abs + QStringLiteral("/machines"));
        const QFileInfo skins(abs + QStringLiteral("/skins"));
        if (machines.exists() && machines.isDir()) {
            m_dataPath = abs;
            return true;
        }
        if (skins.exists() && skins.isDir()) {
            m_dataPath = abs;
            return true;
        }
        return false;
    };

    const QFileInfo binFi(m_found);
    const QString binDir = binFi.absolutePath();

    // 1. Mac dev bindist: <prefix>/bin/openmsx → <prefix>/bindist/openMSX.app/Contents/Resources/share
    //    of: <prefix>/bindist/openMSX.app/Contents/MacOS/openmsx → ./Resources/share
    if (binDir.contains(QStringLiteral("/bin")) && !binDir.contains(QStringLiteral(".app"))) {
        const QString derivedDir = QDir(binDir + QStringLiteral("/..")).absolutePath();
        if (check(derivedDir + QStringLiteral("/bindist/openMSX.app/Contents/Resources/share"))) return;
    }
    if (binDir.contains(QStringLiteral(".app/Contents/MacOS"))) {
        if (check(binDir + QStringLiteral("/../Resources/share"))) return;
    }

    // 2. Linux Flatpak: /app/bin/openmsx → /app/share/openmsx
    if (binDir == QStringLiteral("/app/bin")) {
        if (check(QStringLiteral("/app/share/openmsx"))) return;
    }

    // 3. Linux generic dev: <bin>/../share
    if (check(binDir + QStringLiteral("/../share"))) return;

    // 4. Fallback: laat openMSX zelf zoeken (geen env-var setting)
    m_dataPath.clear();
}
