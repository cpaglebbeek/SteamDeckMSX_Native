#pragma once

#include <QObject>
#include <QProcess>
#include <QString>
#include <QStringList>
#include <QVariant>
#include <QXmlStreamReader>
#include <qqmlregistration.h>

class MsxCore : public QObject {
    Q_OBJECT
    QML_ELEMENT

public:
    enum State {
        Idle,
        Probing,
        Probed,
        Booting,
        Running,
        Quitting,
        Failed
    };
    Q_ENUM(State)

    Q_PROPERTY(State state READ state NOTIFY stateChanged)
    Q_PROPERTY(QString stateLabel READ stateLabel NOTIFY stateChanged)
    Q_PROPERTY(QString version READ version NOTIFY versionChanged)
    Q_PROPERTY(QString errorMessage READ errorMessage NOTIFY errorMessageChanged)
    Q_PROPERTY(QString openmsxPath READ openmsxPath WRITE setOpenmsxPath NOTIFY openmsxPathChanged)
    Q_PROPERTY(QString dataPath READ dataPath WRITE setDataPath NOTIFY dataPathChanged)
    Q_PROPERTY(QString currentRom READ currentRom NOTIFY currentRomChanged)
    Q_PROPERTY(QString currentMachine READ currentMachine WRITE setCurrentMachine NOTIFY currentMachineChanged)

    explicit MsxCore(QObject *parent = nullptr);

    State state() const { return m_state; }
    QString stateLabel() const;
    QString version() const { return m_version; }
    QString errorMessage() const { return m_errorMessage; }
    QString openmsxPath() const { return m_openmsxPath; }
    void setOpenmsxPath(const QString &p);
    QString dataPath() const { return m_dataPath; }
    void setDataPath(const QString &p);
    QString currentRom() const { return m_currentRom; }
    QString currentMachine() const { return m_currentMachine; }
    void setCurrentMachine(const QString &m);

public slots:
    void probeVersion();
    void start(const QString &romPath = QString());
    void stop();
    void loadRom(const QString &path);
    // Returns the assigned command-id (>= 1). Reply will arrive on replyReceived(id, ...).
    int sendCommand(const QString &cmd);
    void requestMachineList();
    // Save-state primitives — wrappen Tcl-commands `savestate <name>` / `loadstate <name>`.
    int savestate(const QString &name);
    int loadstate(const QString &name);

signals:
    void stateChanged();
    void versionChanged();
    void errorMessageChanged();
    void openmsxPathChanged();
    void dataPathChanged();
    void currentRomChanged();
    void currentMachineChanged();
    // Generic IPC signals
    void replyReceived(int commandId, bool ok, const QString &body);
    void stateUpdate(const QString &type, const QString &name, const QString &value);
    void logMessage(const QString &level, const QString &message);
    void rawLine(const QString &line);

private:
    void setState(State s);
    void setError(const QString &msg);
    void onReadyRead();
    void onProcessFinished(int code, QProcess::ExitStatus status);
    void onProcessError(QProcess::ProcessError err);

    // Incremental XML-stream parsing
    void parseChunk(const QByteArray &chunk);
    void handleStartElement();
    void handleEndElement();
    void handleCharacters();

    QString m_openmsxPath{};
    QString m_dataPath{};
    QString m_version{};
    QString m_errorMessage{};
    QString m_currentRom{};
    QString m_currentMachine{};
    State m_state{Idle};

    QProcess m_process;
    QXmlStreamReader m_xml;
    QByteArray m_xmlBuffer;
    bool m_xmlRootOpen{false};
    bool m_probeMode{false};
    int m_nextCommandId{1};

    // Per-element accumulator
    QString m_curElement{};
    QString m_curReplyResult{};
    int m_curReplyId{0};
    QString m_curUpdateType{};
    QString m_curUpdateName{};
    QString m_curLogLevel{};
    QString m_curText{};
};
