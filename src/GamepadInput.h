#pragma once

#include <QObject>
#include <QString>
#include <QTimer>
#include <QHash>
#include <qqmlregistration.h>

// BUG-032: de app leunde voor controller-bediening volledig op een Steam
// Input-layout die Steam op de Deck aantoonbaar niet toepaste — de speler
// hield alleen trackpad-muis over. Deze component leest de gamepad ZELF
// (SDL2, zit in de KDE-runtime; --device=all staat al in de manifest) en
// vertaalt knoppen naar exact de toetsen waar de app al op luistert (DD-010).
// Eén vertaalpunt: alle bestaande Shortcut/Keys-paden — galerij, panelen,
// pauzemenu, overlays — werken daardoor zonder eigen gamepad-code.
//
// Injectie loopt via QWindowSystemInterface (QPA), niet via postEvent: alleen
// dat pad passeert de QShortcutMap, en de galerij bestaat grotendeels uit
// QML Shortcut-elementen.
//
// In-game is deze klasse bewust passief: tijdens het spelen heeft het
// openMSX-venster de focus (focusWindow() == nullptr voor deze app) en
// leest openMSX de controller via zijn eigen SDL — dubbelinvoer kan dus niet.
//
// Zonder SDL2 op het buildplatform compileert een no-op variant
// (STEAMDECKMSX_HAVE_SDL2), zodat de Mac-smokebuild nooit blokkeert.
class GamepadInput : public QObject {
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(bool connected READ connected NOTIFY connectedChanged)
    Q_PROPERTY(QString controllerName READ controllerName NOTIFY connectedChanged)

public:
    explicit GamepadInput(QObject *parent = nullptr);
    ~GamepadInput() override;

    bool connected() const { return m_connected; }
    QString controllerName() const { return m_controllerName; }

signals:
    void connectedChanged();

private:
    void poll();
    void setConnected(bool on, const QString &name);
    // press/release van een Qt-toets via het QPA-pad (incl. auto-repeat).
    void sendKey(int qtKey, bool press, bool autoRepeat = false);
    // Richtingen (D-pad ∪ linker stick) met eigen repeat-administratie.
    void updateDirection(int qtKey, bool active, qint64 nowMs);

    QTimer m_timer;
    bool m_connected{false};
    QString m_controllerName{};
    void *m_controller{nullptr};       // SDL_GameController* (void* houdt SDL uit deze header)

    QHash<int, bool> m_buttonState;    // SDL-button → ingedrukt
    struct DirState { bool active{false}; qint64 pressedAt{0}; qint64 lastRepeat{0}; };
    QHash<int, DirState> m_dirState;   // Qt::Key (pijl) → repeat-status
};
