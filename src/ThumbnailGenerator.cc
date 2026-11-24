#include "ThumbnailGenerator.h"
#include "MsxCore.h"
#include "RomLibrary.h"

#include <QDir>
#include <QFileInfo>
#include <QProcessEnvironment>
#include <QTimer>
#include <QVariantMap>

ThumbnailGenerator::ThumbnailGenerator(QObject *parent)
    : QObject(parent)
    , m_proc(new QProcess(this))
    , m_killTimer(new QTimer(this))
{
    m_killTimer->setSingleShot(true);
    connect(m_killTimer, &QTimer::timeout, this, &ThumbnailGenerator::onTimeout);
    connect(m_proc, &QProcess::finished, this, &ThumbnailGenerator::onFinished);
}

ThumbnailGenerator::~ThumbnailGenerator()
{
    if (m_proc->state() != QProcess::NotRunning) {
        m_proc->kill();
        m_proc->waitForFinished(2000);
    }
}

QString ThumbnailGenerator::thumbnailPathFor(const QString &sha1Hex)
{
    return RomLibrary::thumbnailDir() + QChar('/') + sha1Hex.toLower() + QStringLiteral(".png");
}

QStringList ThumbnailGenerator::framesFor(const QString &sha1Hex)
{
    const QString base = thumbnailPathFor(sha1Hex);
    QStringList out;
    // Ruim doorzoeken: het aantal frames is een instelling die kan wijzigen,
    // en oude reeksen mogen niet ineens half verdwijnen uit de galerij.
    for (int i = 0; i < 64; ++i) {
        const QFileInfo f(framePath(base, i));
        if (f.exists() && f.size() > 0) out << f.absoluteFilePath();
    }
    return out;
}

void ThumbnailGenerator::repairBase(const QString &sha1Hex)
{
    const QString base = thumbnailPathFor(sha1Hex);
    const QStringList fr = framesFor(sha1Hex);
    if (fr.size() < 2 || !QFileInfo::exists(base)) return;
    qint64 bestSize = -1;
    QString best;
    for (const QString &f : fr) {
        const QFileInfo fi(f);
        if (fi.size() > bestSize) { bestSize = fi.size(); best = fi.absoluteFilePath(); }
    }
    const QFileInfo b(base);
    const QFileInfo f0(fr.first());
    // Alleen ingrijpen op het oude patroon (basis == frame 0) en alleen als
    // er echt een beter frame bestaat — anders bestanden met rust laten.
    if (best.isEmpty() || b.size() != f0.size() || b.size() == bestSize) return;
    QFile::remove(base);
    QFile::copy(best, base);
}

QString ThumbnailGenerator::framePath(const QString &basePath, int index)
{
    // …/<sha1>.png → …/<sha1>_00.png. Frame 0 houdt bewust óók een eigen naam:
    // dan is aan het bestandspatroon te zien dat het om een reeks gaat, en kan
    // de galerij het aantal frames tellen zonder aparte administratie.
    QString base = basePath;
    if (base.endsWith(QStringLiteral(".png"), Qt::CaseInsensitive)) {
        base.chop(4);
    }
    return QStringLiteral("%1_%2.png").arg(base).arg(index, 2, 10, QChar('0'));
}

void ThumbnailGenerator::setOpenmsxPath(const QString &p)
{
    if (p == m_openmsxPath) return;
    m_openmsxPath = p;
    emit openmsxPathChanged();
}

void ThumbnailGenerator::setDataPath(const QString &p)
{
    if (p == m_dataPath) return;
    m_dataPath = p;
    emit dataPathChanged();
}

void ThumbnailGenerator::setCaptureSeconds(int s)
{
    const int clamped = qBound(2, s, 60);
    if (clamped == m_captureSeconds) return;
    m_captureSeconds = clamped;
    emit captureSecondsChanged();
}

void ThumbnailGenerator::setBusy(bool b)
{
    if (b == m_busy) return;
    m_busy = b;
    emit busyChanged();
}

void ThumbnailGenerator::enqueue(const QString &sha1Hex, const QString &romPath,
                                 const QString &mediaType, const QString &machine)
{
    if (sha1Hex.isEmpty() || romPath.isEmpty()) return;
    if (!QFileInfo::exists(romPath)) return;
    // Al gemaakt in een eerdere sessie? Meteen melden, niet opnieuw booten.
    // BUG-026: een thumbnail uit v0.3.x is één los beeld. Wie daarna een
    // versie met animaties draait, hield dat ene beeld voor altijd — de
    // cache-check keek naar het oude artefact, niet naar wat er nu nodig is.
    // Bestaat de basis maar ontbreekt de frame-reeks: alsnog genereren.
    const QString existing = thumbnailPathFor(sha1Hex);
    if (QFileInfo::exists(existing)
        && (m_frameCount <= 1 || framesFor(sha1Hex).size() >= 2)) {
        repairBase(sha1Hex);
        emit thumbnailReady(sha1Hex, existing);
        return;
    }
    for (const Job &j : std::as_const(m_queue)) {
        if (j.sha1.compare(sha1Hex, Qt::CaseInsensitive) == 0) return;
    }
    m_queue.enqueue(Job{sha1Hex.toLower(), romPath, mediaType, machine});
    emit pendingChanged();
    if (!m_busy) startNext();
}

void ThumbnailGenerator::enqueueAll(const QVariantList &entries)
{
    for (const QVariant &v : entries) {
        const QVariantMap m = v.toMap();
        // machineId is de openMSX-machine ("C-BIOS_MSX1"); `machine` is het
        // label voor op de tegel ("MSX1") en zou als -machine niet werken.
        QString machine = m.value(QStringLiteral("machineId")).toString();
        if (machine.isEmpty()) machine = QStringLiteral("C-BIOS_MSX2+");
        enqueue(m.value(QStringLiteral("sha1")).toString(),
                m.value(QStringLiteral("romPath")).toString(),
                m.value(QStringLiteral("mediaType")).toString(),
                machine);
    }
}

void ThumbnailGenerator::cancelAll()
{
    m_queue.clear();
    emit pendingChanged();
    if (m_proc->state() != QProcess::NotRunning) {
        m_killTimer->stop();
        m_proc->kill();
    }
    setBusy(false);
}

QStringList ThumbnailGenerator::argsFor(const Job &job, const QString &outPath) const
{
    QStringList args;
    // v0.3.2: de machine komt uit RomTypeDetector (grootte + mapper-signatuur),
    // niet meer altijd C-BIOS_MSX2+. Een MSX1-titel op een MSX2+-machine geeft
    // regelmatig een zwart of afwijkend beeld — precies wat je niet als tegel
    // wilt. Valt terug op MSX2+ als er geen keuze bekend is (P-SDM-05: alleen
    // C-BIOS mag mee).
    args << QStringLiteral("-machine")
         << (job.machine.isEmpty() ? QStringLiteral("C-BIOS_MSX2+") : job.machine);
    if (job.mediaType == QStringLiteral("dsk")) {
        args << QStringLiteral("-diska") << job.romPath;
    } else if (job.mediaType == QStringLiteral("cas")) {
        args << QStringLiteral("-casa") << job.romPath;
    } else {
        args << QStringLiteral("-carta") << job.romPath;
    }
    // v0.4.0: niet één plaatje maar een reeks over de eerste minuut, zodat de
    // tegel laat zien wat een spel dóét in plaats van alleen het titelscherm —
    // en zodat een spel dat traag opstart niet als zwart vlak in de galerij komt.
    //
    // `set throttle off` is hier het verschil tussen bruikbaar en onbruikbaar:
    // zonder dat kost één minuut speeltijd ook echt een minuut, en een collectie
    // van twintig spellen dus twintig minuten. openMSX rekent dan zo snel als de
    // machine toelaat.
    //
    // Pad tussen accolades zodat spaties in het pad de Tcl-parser niet breken.
    QStringList cmds;
    cmds << QStringLiteral("set throttle off");
    const int n = qMax(1, m_frameCount);
    for (int i = 0; i < n; ++i) {
        // Eerste frame op captureSeconds (na de boot), daarna gespreid tot
        // frameSpanSeconds. Bij n == 1 blijft het gedrag van v0.3.x: één plaatje.
        const int t = (n == 1)
            ? m_captureSeconds
            : m_captureSeconds + i * (m_frameSpanSeconds - m_captureSeconds) / (n - 1);
        cmds << QStringLiteral("after time %1 {screenshot {%2}}")
                    .arg(t)
                    .arg(framePath(outPath, i));
    }
    const int last = (n == 1) ? m_captureSeconds : m_frameSpanSeconds;
    cmds << QStringLiteral("after time %1 {quit}").arg(last + 1);
    args << QStringLiteral("-command") << cmds.join(QStringLiteral(" ; "));
    return args;
}

void ThumbnailGenerator::startNext()
{
    if (m_queue.isEmpty()) {
        setBusy(false);
        emit queueDrained();
        return;
    }
    if (m_openmsxPath.isEmpty() || !QFileInfo::exists(m_openmsxPath)) {
        const Job j = m_queue.dequeue();
        emit pendingChanged();
        emit thumbnailFailed(j.sha1, QStringLiteral("openMSX-pad onbekend"));
        startNext();
        return;
    }

    m_current    = m_queue.dequeue();
    m_currentOut = thumbnailPathFor(m_current.sha1);
    emit pendingChanged();
    QDir().mkpath(QFileInfo(m_currentOut).absolutePath());

    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    // Kern van de onzichtbaarheid: renderen zonder venster (zie header).
    env.insert(QStringLiteral("SDL_VIDEODRIVER"), QStringLiteral("offscreen"));
    env.insert(QStringLiteral("SDL_AUDIODRIVER"), QStringLiteral("dummy"));
    env.remove(QStringLiteral("DISPLAY"));
    env.remove(QStringLiteral("WAYLAND_DISPLAY"));
    if (!m_dataPath.isEmpty()) {
        env.insert(QStringLiteral("OPENMSX_SYSTEM_DATA"), m_dataPath);
    }
    // BUG-024: ook hier moet openMSX ergens kunnen schrijven; de home is
    // read-only, en een emulator die zijn eigen map niet kan aanmaken kost
    // per tegel een foutmelding.
    env.insert(QStringLiteral("OPENMSX_HOME"), MsxCore::userDataDir());
    m_proc->setProcessEnvironment(env);
    m_proc->setProcessChannelMode(QProcess::MergedChannels);

    setBusy(true);
    m_killTimer->start(killTimeoutMs());
    m_proc->start(m_openmsxPath, argsFor(m_current, m_currentOut));
}

void ThumbnailGenerator::onTimeout()
{
    if (m_proc->state() != QProcess::NotRunning) m_proc->kill();
    // onFinished() volgt op de kill en handelt de rest af.
}

void ThumbnailGenerator::onFinished(int exitCode, QProcess::ExitStatus status)
{
    Q_UNUSED(exitCode);
    Q_UNUSED(status);
    m_killTimer->stop();

    // Enige waarheid is of er een bruikbaar bestand staat: openMSX kan met
    // exitcode 0 eindigen zonder screenshot (ROM die niet boot) én met een
    // niet-nul code nádat de PNG al geschreven is.
    //
    // v0.4.0: de frames heten <sha1>_00.png … Eén frame wordt óók onder de
    // oude naam <sha1>.png weggeschreven, zodat alles wat een enkele tegel
    // verwacht blijft werken.
    //
    // BUG-034: dat was eerst het EERSTE bruikbare frame — maar frame 0 is het
    // bootmoment en dus vrijwel altijd een zwart scherm (op HC55 gemeten:
    // basis-PNG 0 heldere pixels, frame 05 een vol beeld). Een niet-gefocuste
    // tegel toont alleen de basis, dus de hele galerij oogde dood. Nu wordt
    // het GROOTSTE frame de basis: een zwart PNG comprimeert naar bijna niets,
    // een titelscherm niet — bestandsgrootte is hier een betrouwbare
    // helderheids-proxy zonder het beeld te hoeven decoderen.
    int frames = 0;
    qint64 bestSize = -1;
    QString bestFrame;
    for (int i = 0; i < qMax(1, m_frameCount); ++i) {
        const QFileInfo f(framePath(m_currentOut, i));
        if (!f.exists() || f.size() == 0) continue;
        ++frames;
        if (f.size() > bestSize) {
            bestSize = f.size();
            bestFrame = f.absoluteFilePath();
        }
    }
    if (!bestFrame.isEmpty()) {
        QFile::remove(m_currentOut);
        QFile::copy(bestFrame, m_currentOut);
    }

    const QFileInfo fi(m_currentOut);
    if (fi.exists() && fi.size() > 0) {
        ++m_generated;
        emit generatedChanged();
        emit thumbnailReady(m_current.sha1, m_currentOut);
    } else {
        emit thumbnailFailed(m_current.sha1, QStringLiteral("geen screenshot geproduceerd"));
    }
    startNext();
}
