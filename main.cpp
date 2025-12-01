#include "MainWindow.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    // Configuration de l'application
    app.setApplicationName("TC Hub");
    app.setOrganizationName("TC");
    app.setApplicationVersion("1.0.0");

    // Créer et afficher la fenêtre principale
    MainWindow mainWindow;
    mainWindow.show();

    return app.exec();
}
