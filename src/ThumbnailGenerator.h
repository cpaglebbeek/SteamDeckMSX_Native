#pragma once

#include <QObject>
#include <QProcess>
#include <QQueue>
#include <QString>
#include <QVariantList>
#include <qqmlregistration.h>

class QTimer;

// ThumbnailGenerator — v0.3.0-MazeOfGalious: echte gamebeelden als tegel.
//
// Werkwijze: start openMSX per ROM als los proces, laat de emulator een paar
// seconden lopen zodat het titel-/logoscherm staat, en laat openMSX zelf via
// een Tcl-commando een screenshot wegschrijven. Daarna quit.
//
// De truc die dit onzichtbaar maakt is SDL_VIDEODRIVER=offscreen: SDL rendert
// dan zonder venster, terwijl openMSX' screenshot-commando gewoon werkt.
// Empirisch vastgesteld (2026-07-25, HC55 Flatpak-sandbox): het beeld is
// identiek aan een run mét venster. Zonder die driver flitst er per ROM een
// venster op het scherm — onacceptabel tijdens het bladeren.
//
// Verwerking is strikt serieel: één emulator tegelijk. Op een Steam Deck is
// dat het verschil tussen "achtergrondtaak" en "alles hapert".
class ThumbnailGenerator : public QObject {
    Q_OBJECT
    QML_ELEMENT

public:
    explicit ThumbnailGenerator(QObject *parent = nullptr);
    ~ThumbnailGenerator() override;

    Q_PROPERTY(QString openmsxPath READ openmsxPath WRITE setOpenmsxPath NOTIFY openmsxPathChanged)
    Q_PROPERTY(QString dataPath READ dataPath WRITE setDataPath NOTIFY dataPathChanged)
    Q_PROPERTY(bool busy READ busy NOTIFY busyChanged)
    Q_PROPERTY(int pending READ pending NOTIFY pendingChanged)
    Q_PROPERTY(int generated READ generated NOTIFY generatedChanged)
    // Seconden emuleren vóór de opname. Te vroeg = zwart scherm, te laat =
    // voorbij het titelscherm. 7s is empirisch goed voor Konami-titels.
    Q_PROPERTY(int captureSeconds READ captureSeconds WRITE setCaptureSeconds NOTIFY captureSecondsChanged)

    QString openmsxPath() const { return m_openmsxPath; }
    void setOpenmsxPath(const QString &p);
    QString dataPath() const { return m_dataPath; }
    void setDataPath(const QString &p);
    bool busy() const { return m_busy; }
    int pending() const { return static_cast<int>(m_queue.size()); }
    int generated() const { return m_generated; }
    int captureSeconds() const { return m_captureSeconds; }
    void setCaptureSeconds(int s);

    // entries: lijst van {sha1, romPath, mediaType, machine} — precies wat
    // RomLibrary::entriesWithoutThumbnail() teruggeeft.
    Q_INVOKABLE void enqueueAll(const QVariantList &entries);
    Q_INVOKABLE void enqueue(const QString &sha1Hex, const QString &romPath,
                             const QString &mediaType, const QString &machine);
    Q_INVOKABLE void cancelAll();

    // Doelpad voor een SHA-1 (ook zonder dat er al een bestand staat).
    Q_INVOKABLE static QString thumbnailPathFor(const QString &sha1Hex);
    // Voor de galerij: hoeveel frames staan er klaar voor deze ROM, en waar.
    // Teruggeven als lijst i.p.v. een aantal, omdat een reeks gaten kan hebben
    // als het spel halverwege stopte.
    Q_INVOKABLE static QStringList framesFor(const QString &sha1Hex);
    // BUG-034-migratie: bestaande installaties hebben frame 0 (zwart
    // bootscherm) als basis-PNG. Vervangt de basis door het grootste frame
    // wanneer de basis byte-gelijk is aan frame 0 — nieuw gegenereerde
    // reeksen (basis = grootste frame) zijn dan vanzelf een no-op.
    static void repairBase(const QString &sha1Hex);

signals:
    void openmsxPathChanged();
    void dataPathChanged();
    void busyChanged();
    void pendingChanged();
    void generatedChanged();
    void captureSecondsChanged();
    void thumbnailReady(const QString &sha1Hex, const QString &thumbPath);
    void thumbnailFailed(const QString &sha1Hex, const QString &reason);
    void queueDrained();

private:
    struct Job {
        QString sha1;
        QString romPath;
        QString mediaType;
        QString machine;
    };

    void startNext();
    void onFinished(int exitCode, QProcess::ExitStatus status);
    void onTimeout();
    void setBusy(bool b);
    QStringList argsFor(const Job &job, const QString &outPath) const;

    // Harde bovengrens per ROM. Een enkele ROM die niet wil booten mag de
    // wachtrij niet blokkeren; kill en door naar de volgende.
    // Wachttijd is emulatietijd, geen wandkloktijd: met `throttle off` rekent
    // openMSX sneller dan realtime, maar hoeveel sneller hangt van de machine
    // af. Ruim bemeten, want een te krappe kill maakt halve frame-reeksen.
    int killTimeoutMs() const { return (m_frameSpanSeconds + 30) * 1000; }
    // Pad van frame i, afgeleid van het basispad (…/<sha1>.png → …/<sha1>_03.png).
    static QString framePath(const QString &basePath, int index);

    QString m_openmsxPath;
    QString m_dataPath;
    QQueue<Job> m_queue;
    Job m_current;
    QString m_currentOut;
    QProcess *m_proc{nullptr};
    QTimer *m_killTimer{nullptr};
    int m_captureSeconds{7};
    // Aantal frames en de emulatietijd waarover ze gespreid worden. 12 frames
    // over een minuut geeft een herkenbare beweging zonder de galerij vol te
    // zetten met bestanden; 1 frame schakelt terug naar het v0.3.x-gedrag.
    int m_frameCount{12};
    int m_frameSpanSeconds{60};
    int m_generated{0};
    bool m_busy{false};
};
