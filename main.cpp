#include "MainWindow.h"
#include "UpdateChecker.h"
#include "UpdateDialog.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    // Configuration de l'application
    app.setApplicationName("TCHub");
    app.setOrganizationName("TC");
    app.setApplicationVersion("1.0.0");

    // Vérification des mises à jour au démarrage
    UpdateChecker updateChecker;
    updateChecker.setUpdatePath("Y:\\Florian\\TCHub");

    if (updateChecker.checkForUpdates())
    {
        // Une mise à jour est disponible
        UpdateDialog updateDialog(
            updateChecker.getCurrentVersion(),
            updateChecker.getNetworkVersion()
        );
        updateDialog.setUpdateChecker(&updateChecker);

        // Afficher le dialogue de mise à jour
        updateDialog.exec();
        // Si l'utilisateur a choisi de mettre à jour, l'application se fermera
        // après la mise à jour
    }

    // Créer et afficher la fenêtre principale
    MainWindow mainWindow;
    mainWindow.show();

    return app.exec();
}

