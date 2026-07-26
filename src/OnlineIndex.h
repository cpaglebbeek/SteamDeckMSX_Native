#pragma once

#include <QAbstractListModel>
#include <QNetworkAccessManager>
#include <QString>
#include <QStringList>
#include <qqmlregistration.h>

class QNetworkReply;

// OnlineIndex — bladeren en zoeken in een externe MSX-bestandsindex.
//
// De bron publiceert zijn volledige inhoud als één plat tekstbestand, één pad
// per regel. Dat is bewust de bron die we gebruiken en niet de HTML-lijst: die
// is JavaScript-gehydrateerd en levert bij een gewone fetch geen bestanden op.
// Eén tekstbestand betekent ook: geen HTML-parser die breekt bij een
// opmaakwijziging.
//
// De index wordt op schijf bewaard en offline doorzocht. Op een handheld is dat
// het verschil tussen typen dat direct filtert en typen dat per letter op het
// netwerk wacht.
class OnlineIndex : public QAbstractListModel {
    Q_OBJECT
    QML_ELEMENT

public:
    enum Roles {
        NameRole = Qt::UserRole + 1,   // bestandsnaam zonder pad
        PathRole,                      // pad binnen de bron
        UrlRole,                       // volledige download-URL
        FolderRole                     // bovenliggende map, voor het filter
    };

    Q_PROPERTY(QString indexUrl READ indexUrl WRITE setIndexUrl NOTIFY indexUrlChanged)
    Q_PROPERTY(QString baseUrl READ baseUrl WRITE setBaseUrl NOTIFY baseUrlChanged)
    Q_PROPERTY(QString query READ query WRITE setQuery NOTIFY queryChanged)
    Q_PROPERTY(QString letter READ letter WRITE setLetter NOTIFY letterChanged)
    Q_PROPERTY(QString folder READ folder WRITE setFolder NOTIFY folderChanged)
    Q_PROPERTY(bool loading READ loading NOTIFY loadingChanged)
    Q_PROPERTY(int total READ total NOTIFY totalChanged)          // in de index
    Q_PROPERTY(QString status READ status NOTIFY statusChanged)
    Q_PROPERTY(QStringList folders READ folders NOTIFY foldersChanged)

    explicit OnlineIndex(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    QString indexUrl() const { return m_indexUrl; }
    void setIndexUrl(const QString &u);
    QString baseUrl() const { return m_baseUrl; }
    void setBaseUrl(const QString &u);
    QString query() const { return m_query; }
    void setQuery(const QString &q);
    QString letter() const { return m_letter; }
    void setLetter(const QString &l);
    QString folder() const { return m_folder; }
    void setFolder(const QString &f);
    bool loading() const { return m_loading; }
    int total() const { return m_all.size(); }
    QString status() const { return m_status; }
    QStringList folders() const { return m_folders; }

public slots:
    // Haalt de index op als die er niet is of ouder is dan maxAgeHours.
    void refresh(int maxAgeHours = 24);
    void forceRefresh();
    Q_INVOKABLE QVariantMap entryAt(int row) const;

signals:
    void indexUrlChanged();
    void baseUrlChanged();
    void queryChanged();
    void letterChanged();
    void folderChanged();
    void loadingChanged();
    void totalChanged();
    void statusChanged();
    void foldersChanged();
    void refreshed(int entries);
    void failed(const QString &reason);

private:
    struct Entry {
        QString name;
        QString path;
        QString folder;
    };

    void setLoading(bool on);
    void setStatus(const QString &s);
    void applyFilter();
    void parseIndex(const QByteArray &raw);
    QString cachePath() const;
    bool loadFromCache();

    QNetworkAccessManager m_net;
    QNetworkReply *m_reply{nullptr};

    QString m_indexUrl;
    QString m_baseUrl;
    QString m_query;
    QString m_letter;
    QString m_folder;
    QString m_status;
    bool m_loading{false};

    QVector<Entry> m_all;       // volledige index
    QVector<int> m_view;        // indices in m_all die aan het filter voldoen
    QStringList m_folders;
};
