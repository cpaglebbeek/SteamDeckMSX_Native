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

    connect(&m_process, &QProcess::readyReadStandardOutput, this, &MsxCore::onReadyRead);
    connect(&m_process, &QProcess::readyReadStandardError, this, [this]() {
        const QByteArray e = m_process.readAllStandardError();
        if (!e.isEmpty()) {
            qWarning() << "openmsx stderr:" << QString::fromUtf8(e).trimmed();
        }
    });
    connect(&m_process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &MsxCore::onProcessFinished);
    connect(&m_process, &QProcess::errorOccurred, this, &MsxCore::onProcessError);
}

QString MsxCore::stateLabel() const
{
    switch (m_state) {
        case Idle:     return QStringLiteral("idle");
        case Probing:  return QStringLiteral("probing");
        case Probed:   return QStringLiteral("probed");
        case Booting:  return QStringLiteral("booting");
        case Running:  return QStringLiteral("running");
        case Quitting: return QStringLiteral("quitting");
        case Failed:   return QStringLiteral("failed");
    }
    return QStringLiteral("unknown");
}

void MsxCore::setOpenmsxPath(const QString &p)
{
    if (p == m_openmsxPath) return;
    m_openmsxPath = p;
    emit openmsxPathChanged();
}

void MsxCore::setState(State s)
{
    if (s == m_state) return;
    m_state = s;
    emit stateChanged();
}

void MsxCore::setError(const QString &msg)
{
    m_errorMessage = msg;
    emit errorMessageChanged();
    setState(Failed);
}

void MsxCore::probeVersion()
{
    if (m_openmsxPath.isEmpty()) {
        setError(QStringLiteral("openmsx binary not found in PATH"));
        return;
    }
    if (m_process.state() != QProcess::NotRunning) {
        return;
    }
    m_probeMode = true;
    setState(Probing);
    m_readBuffer.clear();
    m_process.start(m_openmsxPath, {QStringLiteral("--version")});
}

void MsxCore::start(const QString &romPath)
{
    if (m_openmsxPath.isEmpty()) {
        setError(QStringLiteral("openmsx binary not found"));
        return;
    }
    if (m_state == Booting || m_state == Running) {
        // already running — load ROM via command instead
        if (!romPath.isEmpty()) {
            loadRom(romPath);
        }
        return;
    }
    m_probeMode = false;
    m_currentRom = romPath;
    emit currentRomChanged();

    QStringList args = {QStringLiteral("-control"), QStringLiteral("stdio")};
    if (!romPath.isEmpty()) {
        args << QStringLiteral("-carta") << romPath;
    }
    setState(Booting);
    m_readBuffer.clear();
    m_process.start(m_openmsxPath, args);
}

void MsxCore::stop()
{
    if (m_process.state() == QProcess::NotRunning) {
        setState(Idle);
        return;
    }
    setState(Quitting);
    sendCommand(QStringLiteral("quit"));
    if (!m_process.waitForFinished(2000)) {
        m_process.terminate();
        if (!m_process.waitForFinished(1000)) {
            m_process.kill();
        }
    }
}

void MsxCore::loadRom(const QString &path)
{
    if (m_state == Running) {
        m_currentRom = path;
        emit currentRomChanged();
        sendCommand(QStringLiteral("carta \"%1\"").arg(path));
    } else {
        start(path);
    }
}

void MsxCore::sendCommand(const QString &cmd)
{
    if (m_process.state() != QProcess::Running) {
        qWarning() << "sendCommand: process not running, dropped:" << cmd;
        return;
    }
    const QString wrapped = QStringLiteral("<command>%1</command>\n").arg(cmd);
    m_process.write(wrapped.toUtf8());
}

void MsxCore::onReadyRead()
{
    m_readBuffer.append(m_process.readAllStandardOutput());

    int pos = 0;
    while ((pos = m_readBuffer.indexOf('\n')) != -1) {
        const QString line = QString::fromUtf8(m_readBuffer.left(pos)).trimmed();
        m_readBuffer.remove(0, pos + 1);
        if (!line.isEmpty()) {
            parseLine(line);
        }
    }
}

void MsxCore::parseLine(const QString &line)
{
    if (m_probeMode) {
        if (m_version.isEmpty()) {
            m_version = line;
            emit versionChanged();
        }
        return;
    }

    // v0.0.4 minimaal: line-based scan; XML-aware parser komt v0.0.5
    // openMSX -control stdio output is XML-wrapped:
    //   <openmsx-output>
    //   <reply result="ok" command-id="N">...</reply>
    //   <update type="..." name="...">...</update>
    if (line.contains(QStringLiteral("<openmsx-output>"))) {
        // start of stream
        if (m_state == Booting) {
            setState(Running);
        }
        emit eventReceived(line);
        return;
    }
    if (line.contains(QStringLiteral("</openmsx-output>"))) {
        // end of stream — openMSX exiting
        setState(Quitting);
        emit eventReceived(line);
        return;
    }
    if (line.contains(QStringLiteral("<reply"))) {
        emit eventReceived(line);
        return;
    }
    if (line.contains(QStringLiteral("<update"))) {
        emit eventReceived(line);
        return;
    }
    // Onbekende output — door-emit voor debugging
    emit eventReceived(line);
}

void MsxCore::onProcessFinished(int code, QProcess::ExitStatus status)
{
    if (m_probeMode) {
        m_probeMode = false;
        if (status == QProcess::NormalExit && code == 0) {
            setState(Probed);
        } else {
            setError(QStringLiteral("probe failed (exit %1)").arg(code));
        }
        return;
    }
    if (status == QProcess::NormalExit && code == 0) {
        setState(Idle);
    } else if (m_state == Quitting) {
        setState(Idle);
    } else {
        setError(QStringLiteral("openmsx exited with code %1").arg(code));
    }
}

void MsxCore::onProcessError(QProcess::ProcessError err)
{
    Q_UNUSED(err);
    if (m_state == Quitting || m_state == Idle) return;
    setError(QStringLiteral("process error: %1").arg(m_process.errorString()));
}
