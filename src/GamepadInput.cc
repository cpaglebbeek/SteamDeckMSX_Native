#include "GamepadInput.h"

#include <QDateTime>
#include <QDebug>
#include <QGuiApplication>
#include <QWindow>

#ifdef STEAMDECKMSX_HAVE_SDL2
#include <SDL.h>
// QPA-injectie: het enige publieke pad dat óók de QShortcutMap passeert —
// postEvent() op het venster slaat Shortcuts over (gemeten familielid van
// BUG-027: alles leek te werken behalve elke QML Shortcut).
#include <qpa/qwindowsysteminterface.h>

namespace {
// SDL-knop → Qt-toets. Zelfde toewijzing als de footer belooft:
// A start · B terug · X saves · Y stop · Select pauze · L1 scan · R1 BIOS.
struct BtnMap { int sdlButton; int qtKey; };
constexpr BtnMap kButtons[] = {
    {SDL_CONTROLLER_BUTTON_A,             Qt::Key_Return},
    {SDL_CONTROLLER_BUTTON_B,             Qt::Key_Escape},
    {SDL_CONTROLLER_BUTTON_X,             Qt::Key_X},
    {SDL_CONTROLLER_BUTTON_Y,             Qt::Key_Y},
    {SDL_CONTROLLER_BUTTON_START,         Qt::Key_Return},
    {SDL_CONTROLLER_BUTTON_BACK,          Qt::Key_F12},
    {SDL_CONTROLLER_BUTTON_LEFTSHOULDER,  Qt::Key_R},
    {SDL_CONTROLLER_BUTTON_RIGHTSHOULDER, Qt::Key_I},
};
// Hysterese op de linker stick: aan boven 50%, pas weer uit onder 35% —
// anders klappert de richting rond de drempel.
constexpr Sint16 kAxisOn  = 16384;
constexpr Sint16 kAxisOff = 11469;
constexpr qint64 kRepeatDelayMs    = 400;
constexpr qint64 kRepeatIntervalMs = 140;
} // namespace

GamepadInput::GamepadInput(QObject *parent)
    : QObject(parent)
{
    SDL_SetHint(SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS, "1");
    if (SDL_InitSubSystem(SDL_INIT_GAMECONTROLLER) != 0) {
        qWarning() << "[Gamepad] SDL-init faalde:" << SDL_GetError();
        return;
    }
    m_timer.setInterval(16);
    connect(&m_timer, &QTimer::timeout, this, &GamepadInput::poll);
    m_timer.start();
}

GamepadInput::~GamepadInput()
{
    if (m_controller) SDL_GameControllerClose(static_cast<SDL_GameController *>(m_controller));
    SDL_QuitSubSystem(SDL_INIT_GAMECONTROLLER);
}

void GamepadInput::setConnected(bool on, const QString &name)
{
    if (on == m_connected && name == m_controllerName) return;
    m_connected = on;
    m_controllerName = name;
    emit connectedChanged();
    qWarning() << "[Gamepad]" << (on ? "verbonden:" : "losgekoppeld") << name;
}

void GamepadInput::sendKey(int qtKey, bool press, bool autoRepeat)
{
    QWindow *w = QGuiApplication::focusWindow();
    // Geen focus = het spel draait (openMSX-venster) of we zijn geminimaliseerd;
    // dan niets injecteren — openMSX leest de controller zelf.
    if (!w) return;
    QWindowSystemInterface::handleKeyEvent(
        w,
        press ? QEvent::KeyPress : QEvent::KeyRelease,
        qtKey, Qt::NoModifier, QString(), autoRepeat);
}

void GamepadInput::updateDirection(int qtKey, bool active, qint64 nowMs)
{
    DirState &st = m_dirState[qtKey];
    if (active && !st.active) {
        st.active = true;
        st.pressedAt = nowMs;
        st.lastRepeat = nowMs;
        sendKey(qtKey, true);
    } else if (!active && st.active) {
        st.active = false;
        sendKey(qtKey, false);
    } else if (active && nowMs - st.pressedAt >= kRepeatDelayMs
                      && nowMs - st.lastRepeat >= kRepeatIntervalMs) {
        // Synthetische toetsen krijgen geen OS-auto-repeat; navigatie door een
        // lange galerij zonder repeat is per tegel één klik — dus zelf herhalen.
        st.lastRepeat = nowMs;
        sendKey(qtKey, true, true);
    }
}

void GamepadInput::poll()
{
    SDL_Event ev;
    while (SDL_PollEvent(&ev)) {
        if (ev.type == SDL_CONTROLLERDEVICEADDED && !m_controller) {
            m_controller = SDL_GameControllerOpen(ev.cdevice.which);
            if (m_controller) {
                const char *n = SDL_GameControllerName(
                    static_cast<SDL_GameController *>(m_controller));
                setConnected(true, n ? QString::fromUtf8(n) : QStringLiteral("controller"));
            }
        } else if (ev.type == SDL_CONTROLLERDEVICEREMOVED && m_controller) {
            SDL_GameControllerClose(static_cast<SDL_GameController *>(m_controller));
            m_controller = nullptr;
            setConnected(false, QString());
        }
    }
    if (!m_controller) return;

    auto *gc = static_cast<SDL_GameController *>(m_controller);
    const qint64 now = QDateTime::currentMSecsSinceEpoch();

    for (const auto &m : kButtons) {
        const bool down = SDL_GameControllerGetButton(
            gc, static_cast<SDL_GameControllerButton>(m.sdlButton)) != 0;
        bool &prev = m_buttonState[m.sdlButton];
        if (down != prev) {
            prev = down;
            sendKey(m.qtKey, down);
        }
    }

    // Richtingen: D-pad ∪ linker stick, per richting één toets-status.
    const Sint16 ax = SDL_GameControllerGetAxis(gc, SDL_CONTROLLER_AXIS_LEFTX);
    const Sint16 ay = SDL_GameControllerGetAxis(gc, SDL_CONTROLLER_AXIS_LEFTY);
    const bool dpadU = SDL_GameControllerGetButton(gc, SDL_CONTROLLER_BUTTON_DPAD_UP);
    const bool dpadD = SDL_GameControllerGetButton(gc, SDL_CONTROLLER_BUTTON_DPAD_DOWN);
    const bool dpadL = SDL_GameControllerGetButton(gc, SDL_CONTROLLER_BUTTON_DPAD_LEFT);
    const bool dpadR = SDL_GameControllerGetButton(gc, SDL_CONTROLLER_BUTTON_DPAD_RIGHT);

    const bool wasL = m_dirState[Qt::Key_Left].active;
    const bool wasR = m_dirState[Qt::Key_Right].active;
    const bool wasU = m_dirState[Qt::Key_Up].active;
    const bool wasD = m_dirState[Qt::Key_Down].active;

    updateDirection(Qt::Key_Left,  dpadL || ax < -(wasL ? kAxisOff : kAxisOn), now);
    updateDirection(Qt::Key_Right, dpadR || ax >  (wasR ? kAxisOff : kAxisOn), now);
    updateDirection(Qt::Key_Up,    dpadU || ay < -(wasU ? kAxisOff : kAxisOn), now);
    updateDirection(Qt::Key_Down,  dpadD || ay >  (wasD ? kAxisOff : kAxisOn), now);
}

#else // !STEAMDECKMSX_HAVE_SDL2 — no-op zodat een platform zonder SDL2 blijft bouwen.

GamepadInput::GamepadInput(QObject *parent) : QObject(parent)
{
    qWarning() << "[Gamepad] gebouwd zonder SDL2 — controller-invoer uit";
}
GamepadInput::~GamepadInput() = default;
void GamepadInput::poll() {}
void GamepadInput::setConnected(bool, const QString &) {}
void GamepadInput::sendKey(int, bool, bool) {}
void GamepadInput::updateDirection(int, bool, qint64) {}

#endif
