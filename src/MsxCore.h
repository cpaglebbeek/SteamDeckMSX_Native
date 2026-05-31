#pragma once

#include <QObject>
#include <QProcess>
#include <QString>
#include <qqmlregistration.h>

class MsxCore : public QObject {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QString version READ version NOTIFY versionChanged)
    Q_PROPERTY(QString status READ status NOTIFY statusChanged)
    Q_PROPERTY(QString openmsxPath READ openmsxPath WRITE setOpenmsxPath NOTIFY openmsxPathChanged)

public:
    explicit MsxCore(QObject *parent = nullptr);

    QString version() const { return m_version; }
    QString status() const { return m_status; }
    QString openmsxPath() const { return m_openmsxPath; }
    void setOpenmsxPath(const QString &p);

public slots:
    void probeVersion();

signals:
    void versionChanged();
    void statusChanged();
    void openmsxPathChanged();

private:
    void setStatus(const QString &s);

    QString m_openmsxPath{};
    QString m_version{};
    QString m_status{"idle"};
    QProcess m_probe;
};
