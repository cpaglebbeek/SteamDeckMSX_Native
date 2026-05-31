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

    if (check(userPath())) { emit foundChanged(); return; }

    if (check(QStandardPaths::findExecutable(QStringLiteral("openmsx")))) {
        emit foundChanged();
        return;
    }

    if (check(QStringLiteral("/app/bin/openmsx"))) { emit foundChanged(); return; }

    const QString appDir = QCoreApplication::applicationDirPath();
    const QStringList devCandidates = {
        appDir + QStringLiteral("/../../externals/openmsx/derived/x86_64-darwin-opt/bin/openmsx"),
        appDir + QStringLiteral("/../../externals/openmsx/derived/aarch64-linux-opt/bin/openmsx"),
        appDir + QStringLiteral("/../../externals/openmsx/derived/x86_64-linux-opt/bin/openmsx"),
        appDir + QStringLiteral("/../externals/openmsx/derived/x86_64-darwin-opt/bin/openmsx"),
    };
    for (const auto &c : devCandidates) {
        const QString abs = QDir(c).absolutePath();
        if (check(abs)) { emit foundChanged(); return; }
    }

    emit foundChanged();
}
