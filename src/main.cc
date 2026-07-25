#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QDebug>

#include "MsxCore.h"
#include "CartridgeModel.h"

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);
    app.setApplicationName("SteamDeckMSX");
    app.setOrganizationName("iCt Horse");
    app.setOrganizationDomain("icthorse.nl");
    app.setApplicationVersion(STEAMDECKMSX_VERSION);

    // BUG-022: de galerij verbergt zichzelf terwijl de emulator speelt. Met de
    // standaardinstelling zou dat het laatste venster sluiten en de app — en
    // daarmee het openMSX-kindproces — meteen beëindigen.
    app.setQuitOnLastWindowClosed(false);

    qInfo() << "SteamDeckMSX" << STEAMDECKMSX_VERSION
            << "target=" << STEAMDECKMSX_TARGET;

    QQmlApplicationEngine engine;
    engine.loadFromModule("SteamDeckMSX", "Main");

    if (engine.rootObjects().isEmpty()) {
        qCritical() << "Failed to load QML root";
        return -1;
    }

    return app.exec();
}
