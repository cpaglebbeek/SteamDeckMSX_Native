#include "MsxCore.h"

#include <QFileInfo>
#include <QStandardPaths>
#include <QDebug>

MsxCore::MsxCore(QObject *parent)
    : QObject(parent)
{
    const QString found = QStandardPaths::findExecutable("openmsx");
    if (!found.isEmpty()) {
        m_openmsxPath = found;
    }

    connect(&m_probe, &QProcess::readyReadStandardOutput, this, [this]() {
        const QByteArray out = m_probe.readAllStandardOutput();
        const QString line = QString::fromUtf8(out).trimmed();
        if (!line.isEmpty()) {
            m_version = line;
            emit versionChanged();
        }
    });

    connect(&m_probe, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, [this](int code, QProcess::ExitStatus status) {
        if (status == QProcess::NormalExit && code == 0) {
            setStatus(QStringLiteral("probed"));
        } else {
            setStatus(QStringLiteral("probe-failed"));
        }
    });
}

void MsxCore::setOpenmsxPath(const QString &p)
{
    if (p == m_openmsxPath) return;
    m_openmsxPath = p;
    emit openmsxPathChanged();
}

void MsxCore::probeVersion()
{
    if (m_openmsxPath.isEmpty()) {
        setStatus(QStringLiteral("no-openmsx-found"));
        return;
    }
    if (m_probe.state() != QProcess::NotRunning) {
        return;
    }
    setStatus(QStringLiteral("probing"));
    m_probe.start(m_openmsxPath, {QStringLiteral("-version")});
}

void MsxCore::setStatus(const QString &s)
{
    if (s == m_status) return;
    m_status = s;
    emit statusChanged();
}
