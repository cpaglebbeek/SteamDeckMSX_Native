#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <qqmlregistration.h>

// Finds the openmsx binary, in order:
//   1. user-configured path from QSettings
//   2. $PATH lookup
//   3. Flatpak-bundled path /app/bin/openmsx
//   4. fallback to ../externals/openmsx/derived/*-opt/bin/openmsx (dev)
class OpenmsxLocator : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON
    Q_PROPERTY(QString found READ found NOTIFY foundChanged)
    Q_PROPERTY(QStringList searched READ searched CONSTANT)

public:
    explicit OpenmsxLocator(QObject *parent = nullptr);

    QString found() const { return m_found; }
    QStringList searched() const { return m_searched; }

public slots:
    void refresh();
    void setUserPath(const QString &p);
    QString userPath() const;

signals:
    void foundChanged();

private:
    QString m_found{};
    QStringList m_searched{};
};
