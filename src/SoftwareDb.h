#pragma once

#include <QHash>
#include <QObject>
#include <QString>
#include <qqmlregistration.h>

// SoftwareDb — SHA-1 → machine lookup uit openMSX softwaredb.xml.
//
// v0.0.9-YS opvolger: aanvulling op RomTypeDetector heuristiek. Waar de
// detector op grootte + SCC-pattern raadt, geeft SoftwareDb een
// deterministisch antwoord per ROM-hash. Pure data-class (geen openMSX-state),
// QML_ELEMENT zodat QML instances kan maken voor lookups in UI-flows.
//
// Bewust geen QAbstractListModel: in v0.0.9+ alleen lookup-functies nodig,
// geen ranglijst of grid. Bij entry-mutaties één `changed()` signal — geen
// `entryCountChanged()` om binding-storm te voorkomen tijdens bulk-load.
//
// SHA-1 lookup is case-insensitive (intern lowercase). openMSX softwaredb.xml
// hashes zijn lowercase 40-char hex; QML/UI moet er niet over hoeven nadenken.
class SoftwareDb : public QObject {
    Q_OBJECT
    QML_ELEMENT
public:
    explicit SoftwareDb(QObject *parent = nullptr);

    Q_PROPERTY(int entryCount READ entryCount NOTIFY changed)
    int entryCount() const { return m_machines.size(); }

    // Laad openMSX softwaredb.xml. Return aantal entries geladen, -1 bij fout.
    Q_INVOKABLE int loadFromXmlFile(const QString &path);
    // In-memory entry toevoegen (voor tests + bootstrap-data).
    Q_INVOKABLE void addEntry(const QString &sha1Hex, const QString &machine, const QString &title);
    // SHA-1 lowercase 40-char → machine ("C-BIOS_MSX1" etc.). Lege string als niet gevonden.
    Q_INVOKABLE QString lookupMachine(const QString &sha1Hex) const;
    Q_INVOKABLE QString lookupTitle(const QString &sha1Hex) const;
    Q_INVOKABLE void clearAll();
    // Bootstrap-set: ~10 bekende SHA-1 hashes uit openMSX softwaredb. Roept addEntry meerdere keren.
    Q_INVOKABLE void loadBootstrapData();

signals:
    void changed();

private:
    QHash<QString, QString> m_machines;
    QHash<QString, QString> m_titles;
};
