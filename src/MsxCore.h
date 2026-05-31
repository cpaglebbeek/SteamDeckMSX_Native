#pragma once

#include <QObject>
#include <QProcess>
#include <QString>
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
    Q_PROPERTY(QString currentRom READ currentRom NOTIFY currentRomChanged)

    explicit MsxCore(QObject *parent = nullptr);

    State state() const { return m_state; }
    QString stateLabel() const;
    QString version() const { return m_version; }
    QString errorMessage() const { return m_errorMessage; }
    QString openmsxPath() const { return m_openmsxPath; }
    void setOpenmsxPath(const QString &p);
    QString currentRom() const { return m_currentRom; }

public slots:
    void probeVersion();
    void start(const QString &romPath = QString());
    void stop();
    void loadRom(const QString &path);
    void sendCommand(const QString &cmd);

signals:
    void stateChanged();
    void versionChanged();
    void errorMessageChanged();
    void openmsxPathChanged();
    void currentRomChanged();
    void eventReceived(const QString &line);

private:
    void setState(State s);
    void setError(const QString &msg);
    void onReadyRead();
    void onProcessFinished(int code, QProcess::ExitStatus status);
    void onProcessError(QProcess::ProcessError err);
    void parseLine(const QString &line);

    QString m_openmsxPath{};
    QString m_version{};
    QString m_errorMessage{};
    QString m_currentRom{};
    State m_state{Idle};

    QProcess m_process;
    QByteArray m_readBuffer;
    bool m_probeMode{false};
};
