#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <qqmlregistration.h>

// Finds the openmsx binary AND the share-data directory (machines/, skins/, etc.).
//
// Lookup order for the binary:
//   1. user-configured path from QSettings
//   2. $PATH lookup
//   3. Flatpak-bundled path /app/bin/openmsx
//   4. dev fallback: ../externals/openmsx/derived/*-opt/bin/openmsx
//
// Once a binary is found, the dataPath is derived:
//   - Mac dev bindist: <prefix>/bindist/openMSX.app/Contents/Resources/share
//   - Linux Flatpak:   /app/share/openmsx
//   - Linux dev:       <binDir>/../share  OR  same-dir-as-bin/../share
//   - Fallback:        $OPENMSX_SYSTEM_DATA env (already-set by user)
//
// Required because the standalone bin/openmsx searches a hard-coded set of
// paths (~/.openMSX/share + <bin>/../share) that fail in our layout. See BUG-004.
class OpenmsxLocator : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON
    Q_PROPERTY(QString found READ found NOTIFY foundChanged)
    Q_PROPERTY(QString dataPath READ dataPath NOTIFY dataPathChanged)
    Q_PROPERTY(QStringList searched READ searched CONSTANT)
    Q_PROPERTY(QStringList dataSearched READ dataSearched NOTIFY dataPathChanged)

public:
    explicit OpenmsxLocator(QObject *parent = nullptr);

    QString found() const { return m_found; }
    QString dataPath() const { return m_dataPath; }
    QStringList searched() const { return m_searched; }
    QStringList dataSearched() const { return m_dataSearched; }

public slots:
    void refresh();
    void setUserPath(const QString &p);
    QString userPath() const;

signals:
    void foundChanged();
    void dataPathChanged();

private:
    void discoverDataPath();

    QString m_found{};
    QString m_dataPath{};
    QStringList m_searched{};
    QStringList m_dataSearched{};
};
