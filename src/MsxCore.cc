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
            const QString line = QString::fromUtf8(e).trimmed();
            qWarning() << "openmsx stderr:" << line;
            emit logMessage(QStringLiteral("stderr"), line);
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

void MsxCore::setDataPath(const QString &p)
{
    if (p == m_dataPath) return;
    m_dataPath = p;
    emit dataPathChanged();
}

void MsxCore::setCurrentMachine(const QString &m)
{
    if (m == m_currentMachine) return;
    m_currentMachine = m;
    emit currentMachineChanged();
    if (m_state == Running && !m.isEmpty()) {
        sendCommand(QStringLiteral("set machine %1").arg(m));
    }
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
    m_xmlBuffer.clear();
    m_xmlRootOpen = false;
    m_process.start(m_openmsxPath, {QStringLiteral("--version")});
}

void MsxCore::start(const QString &romPath)
{
    if (m_openmsxPath.isEmpty()) {
        setError(QStringLiteral("openmsx binary not found"));
        return;
    }
    if (m_state == Booting || m_state == Running) {
        if (!romPath.isEmpty()) {
            loadRom(romPath);
        }
        return;
    }
    m_probeMode = false;
    m_currentRom = romPath;
    emit currentRomChanged();

    QStringList args = {QStringLiteral("-control"), QStringLiteral("stdio")};
    if (!m_currentMachine.isEmpty()) {
        args << QStringLiteral("-machine") << m_currentMachine;
    }
    if (!romPath.isEmpty()) {
        args << QStringLiteral("-carta") << romPath;
    }

    // BUG-004 fix: set OPENMSX_SYSTEM_DATA env so the bin finds machines/skins.
    if (!m_dataPath.isEmpty()) {
        QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
        env.insert(QStringLiteral("OPENMSX_SYSTEM_DATA"), m_dataPath);
        m_process.setProcessEnvironment(env);
    }

    setState(Booting);
    m_xmlBuffer.clear();
    m_xmlRootOpen = false;
    m_nextCommandId = 1;
    m_xml.clear();
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

int MsxCore::sendCommand(const QString &cmd)
{
    if (m_process.state() != QProcess::Running) {
        qWarning() << "sendCommand: process not running, dropped:" << cmd;
        return -1;
    }
    const int id = m_nextCommandId++;
    const QString wrapped =
        QStringLiteral("<command id=\"%1\">%2</command>\n").arg(id).arg(cmd);
    m_process.write(wrapped.toUtf8());
    return id;
}

void MsxCore::requestMachineList()
{
    sendCommand(QStringLiteral("machine_info machines"));
}

int MsxCore::savestate(const QString &name)
{
    if (name.isEmpty()) return -1;
    return sendCommand(QStringLiteral("savestate \"%1\"").arg(name));
}

int MsxCore::loadstate(const QString &name)
{
    if (name.isEmpty()) return -1;
    return sendCommand(QStringLiteral("loadstate \"%1\"").arg(name));
}

void MsxCore::onReadyRead()
{
    const QByteArray chunk = m_process.readAllStandardOutput();
    if (chunk.isEmpty()) return;

    if (m_probeMode) {
        // --version output is plain text, not XML
        const QString line = QString::fromUtf8(chunk).trimmed();
        if (!line.isEmpty() && m_version.isEmpty()) {
            // First line is "openMSX 21.0", second line is "flavour: opt"
            const auto parts = line.split('\n', Qt::SkipEmptyParts);
            if (!parts.isEmpty()) {
                m_version = parts.first().trimmed();
                emit versionChanged();
            }
        }
        return;
    }

    parseChunk(chunk);
}

void MsxCore::parseChunk(const QByteArray &chunk)
{
    // QXmlStreamReader works incremental via addData(). When NotWellFormed
    // due to premature end of buffer, we ignore — next chunk continues.
    m_xml.addData(chunk);

    while (!m_xml.atEnd()) {
        QXmlStreamReader::TokenType t = m_xml.readNext();
        if (m_xml.hasError()) {
            // PrematureEndOfDocumentError = need more data; bail and resume next chunk.
            if (m_xml.error() == QXmlStreamReader::PrematureEndOfDocumentError) {
                return;
            }
            // Anders: skip de huidige error en reset; logging-only
            qWarning() << "xml parse error:" << m_xml.errorString();
            break;
        }
        switch (t) {
            case QXmlStreamReader::StartElement: handleStartElement(); break;
            case QXmlStreamReader::EndElement:   handleEndElement(); break;
            case QXmlStreamReader::Characters:   handleCharacters(); break;
            default: break;
        }
    }
}

void MsxCore::handleStartElement()
{
    const QString name = m_xml.name().toString();
    m_curElement = name;
    m_curText.clear();

    if (name == QStringLiteral("openmsx-output")) {
        m_xmlRootOpen = true;
        if (m_state == Booting) {
            setState(Running);
        }
        return;
    }
    if (name == QStringLiteral("reply")) {
        const auto attrs = m_xml.attributes();
        m_curReplyResult = attrs.value(QStringLiteral("result")).toString();
        m_curReplyId     = attrs.value(QStringLiteral("command-id")).toInt();
        return;
    }
    if (name == QStringLiteral("update")) {
        const auto attrs = m_xml.attributes();
        m_curUpdateType = attrs.value(QStringLiteral("type")).toString();
        m_curUpdateName = attrs.value(QStringLiteral("name")).toString();
        return;
    }
    if (name == QStringLiteral("log")) {
        const auto attrs = m_xml.attributes();
        m_curLogLevel = attrs.value(QStringLiteral("level")).toString();
        return;
    }
}

void MsxCore::handleEndElement()
{
    const QString name = m_xml.name().toString();

    if (name == QStringLiteral("openmsx-output")) {
        m_xmlRootOpen = false;
        setState(Quitting);
        return;
    }
    if (name == QStringLiteral("reply")) {
        emit replyReceived(m_curReplyId,
                           m_curReplyResult == QStringLiteral("ok"),
                           m_curText.trimmed());
        m_curReplyResult.clear();
        m_curReplyId = 0;
        return;
    }
    if (name == QStringLiteral("update")) {
        emit stateUpdate(m_curUpdateType, m_curUpdateName, m_curText.trimmed());
        m_curUpdateType.clear();
        m_curUpdateName.clear();
        return;
    }
    if (name == QStringLiteral("log")) {
        emit logMessage(m_curLogLevel, m_curText.trimmed());
        m_curLogLevel.clear();
        return;
    }
}

void MsxCore::handleCharacters()
{
    m_curText.append(m_xml.text().toString());
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
