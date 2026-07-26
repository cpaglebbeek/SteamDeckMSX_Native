#include "MsxCore.h"
#include "RomTypeDetector.h"

#include <QDir>
#include <QFile>
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

    // BUG-022: fullscreen staat aan tenzij de omgeving het uitzet. Headless
    // gates (SDL_VIDEODRIVER=offscreen) draaien met STEAMDECKMSX_FULLSCREEN=0.
    const QByteArray fs = qgetenv("STEAMDECKMSX_FULLSCREEN");
    if (fs == "0" || fs.toLower() == "false") {
        m_fullscreen = false;
    }
}

QString MsxCore::userDataDir()
{
    // BUG-024: openMSX bewaart SRAM, settings.xml en save-states in ~/.openMSX.
    // In de Flatpak is de home read-only (BUG-017-fix), dus faalde dat stil —
    // met een foutmelding over het spel heen en verloren save-states. openMSX
    // kent `OPENMSX_HOME` als override; die wijst hier naar de eigen
    // schrijfbare app-map. `--persist=.openMSX` in het manifest hielp niet: de
    // read-only home-mount wint van die bind, gemeten op HC55.
    const QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
                        + QStringLiteral("/openmsx");
    QDir().mkpath(dir);
    return dir;
}

void MsxCore::setFullscreen(bool on)
{
    if (on == m_fullscreen) return;
    m_fullscreen = on;
    emit fullscreenChanged();
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
    // BUG-022: met `-control stdio` laat openMSX het opzetten van de weergave
    // aan de aansturende partij over. Het start dan zonder renderer (geen
    // venster) én met de machine uit (geen emulatie) — de gebruiker ziet niets
    // terwijl het proces gewoon draait en netjes antwoordt. Beide moeten dus
    // expliciet aan. Faalt SDLGL-PP, dan zoekt openMSX zelf een andere renderer.
    QStringList startup{QStringLiteral("set renderer SDLGL-PP"),
                        QStringLiteral("set power on")};
    if (m_fullscreen) {
        startup << QStringLiteral("set fullscreen on");
        // De galerij is tijdens het spelen verborgen en vangt geen toetsen meer.
        // openMSX bindt daarom zelf de menu-toets, en meldt het indrukken via
        // zijn eigen `message` — dat komt hier binnen als <log>-event, het enige
        // kanaal dat een draaiende emulator terug naar de app heeft.
        // F12 gaf eerder direct `quit`; dat gaf geen keuze en sloot ook af als
        // je alleen even wilde pauzeren.
        startup << QStringLiteral("bind F12 {message \"%1\"}").arg(kMenuSignal);
    }
    args << QStringLiteral("-command") << startup.join(QStringLiteral(" ; "));
    m_lastStartArgs = args;

    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    // BUG-004 fix: set OPENMSX_SYSTEM_DATA env so the bin finds machines/skins.
    if (!m_dataPath.isEmpty()) {
        env.insert(QStringLiteral("OPENMSX_SYSTEM_DATA"), m_dataPath);
    }
    env.insert(QStringLiteral("OPENMSX_HOME"), userDataDir());
    m_process.setProcessEnvironment(env);

    setState(Booting);
    m_xmlBuffer.clear();
    m_xmlRootOpen = false;
    m_nextCommandId = 1;
    m_xml.clear();
    m_process.start(m_openmsxPath, args);
}

void MsxCore::setPaused(bool on)
{
    if (m_state != Running) return;
    sendCommand(QStringLiteral("set pause %1").arg(on ? QStringLiteral("on")
                                                      : QStringLiteral("off")));
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
    // Compat: v0.1.0-Xanadu — loadRom is alias voor loadRomSlotA (primaire slot).
    loadRomSlotA(path);
}

int MsxCore::loadRomSlotA(const QString &path)
{
    // BIOS-detect-heuristic log-only (sinds v0.0.8-Snatcher).
    {
        QFile f(path);
        if (f.open(QIODevice::ReadOnly)) {
            const QByteArray bytes = f.read(512 * 1024);
            const auto r = RomTypeDetector::detect(bytes);
            qInfo().noquote() << "[RomTypeDetector]" << QFileInfo(path).fileName()
                              << "sha1=" << r.sha1Hex.left(12) + QStringLiteral("…")
                              << "→" << RomTypeDetector::generationName(r.generation)
                              << "/" << RomTypeDetector::mapperName(r.mapper)
                              << "→ suggest" << r.suggestedMachine
                              << "(" << r.reason << ")";
        } else {
            qWarning() << "[RomTypeDetector] kon ROM niet openen:" << path;
        }
    }

    if (m_state == Running) {
        m_slotARom = path;
        m_currentRom = path;          // compat-alias.
        emit slotARomChanged();
        emit currentRomChanged();
        return sendCommand(QStringLiteral("carta \"%1\"").arg(path));
    } else {
        // Niet running → start met deze ROM in slot A.
        m_slotARom = path;
        m_currentRom = path;
        emit slotARomChanged();
        emit currentRomChanged();
        start(path);
        return 0;  // start() laadt via -cart bij spawn.
    }
}

int MsxCore::loadRomSlotB(const QString &path)
{
    // Slot B alleen mogelijk als emulator al draait — openMSX heeft anders
    // geen idee waar in te steken.
    if (m_state != Running) {
        qWarning() << "[MsxCore] loadRomSlotB vereist Running state; huidige:" << m_state;
        emit logMessage(QStringLiteral("warning"),
                        QStringLiteral("Slot B kan alleen tijdens draaiend spel — start eerst slot A"));
        return -1;
    }
    {
        QFile f(path);
        if (f.open(QIODevice::ReadOnly)) {
            const QByteArray bytes = f.read(512 * 1024);
            const auto r = RomTypeDetector::detect(bytes);
            qInfo().noquote() << "[RomTypeDetector/slotB]" << QFileInfo(path).fileName()
                              << "sha1=" << r.sha1Hex.left(12) + QStringLiteral("…")
                              << "→" << RomTypeDetector::generationName(r.generation);
        }
    }
    m_slotBRom = path;
    emit slotBRomChanged();
    return sendCommand(QStringLiteral("cartb \"%1\"").arg(path));
}

int MsxCore::removeRomSlotA()
{
    m_slotARom.clear();
    m_currentRom.clear();
    emit slotARomChanged();
    emit currentRomChanged();
    if (m_state != Running) return 0;
    // openMSX: `carta eject` of `carta -` om uit te werpen.
    return sendCommand(QStringLiteral("carta eject"));
}

int MsxCore::removeRomSlotB()
{
    m_slotBRom.clear();
    emit slotBRomChanged();
    if (m_state != Running) return 0;
    return sendCommand(QStringLiteral("cartb eject"));
}

// v0.2.0-TreasureOfUsas — Disk + Tape media.
//
// openMSX Tcl: `diska <pad>` / `diskb <pad>` voor floppy-images, `cassetteplayer
// insert <pad>` of `casa <pad>` voor tape. Voor v0.2.0 gebruiken we `diska/diskb`
// en `cassetteplayer insert` (officiële Tcl-naam in openMSX 21.0).
int MsxCore::loadDsk(const QString &path, int drive)
{
    if (m_state != Running) {
        qWarning() << "[MsxCore] loadDsk vereist Running state";
        emit logMessage(QStringLiteral("warning"),
                        QStringLiteral("Floppy alleen tijdens draaiend spel — start eerst een ROM of leeg-boot"));
        return -1;
    }
    const QString cmd = (drive == 1)
        ? QStringLiteral("diskb \"%1\"").arg(path)
        : QStringLiteral("diska \"%1\"").arg(path);
    qInfo().noquote() << "[MsxCore] loadDsk drive=" << drive << QFileInfo(path).fileName();
    return sendCommand(cmd);
}

int MsxCore::loadCas(const QString &path)
{
    if (m_state != Running) {
        emit logMessage(QStringLiteral("warning"),
                        QStringLiteral("Cassette alleen tijdens draaiend spel"));
        return -1;
    }
    qInfo().noquote() << "[MsxCore] loadCas" << QFileInfo(path).fileName();
    return sendCommand(QStringLiteral("cassetteplayer insert \"%1\"").arg(path));
}

int MsxCore::ejectDsk(int drive)
{
    if (m_state != Running) return 0;
    return sendCommand((drive == 1)
        ? QStringLiteral("diskb eject")
        : QStringLiteral("diska eject"));
}

int MsxCore::ejectCas()
{
    if (m_state != Running) return 0;
    return sendCommand(QStringLiteral("cassetteplayer eject"));
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
        const QString text = m_curText.trimmed();
        // De emulator heeft geen andere weg terug naar de app dan zijn eigen
        // log; de menu-toets komt daarom als afgesproken tekst binnen.
        if (text == kMenuSignal) {
            emit menuRequested();
        } else {
            emit logMessage(m_curLogLevel, text);
        }
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
