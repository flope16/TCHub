#include "UpdateChecker.h"
#include <QFile>
#include <QTextStream>
#include <QDir>
#include <QCoreApplication>
#include <QProcess>
#include <QMessageBox>
#include <QVersionNumber>
#include <QFileInfo>
#include <QDateTime>

UpdateChecker::UpdateChecker(QObject *parent)
    : QObject(parent)
    , updatePath("Y:\\Florian\\TCHub")  // Chemin par défaut
{
    currentVersion = readLocalVersion();
}

void UpdateChecker::setUpdatePath(const QString& path)
{
    updatePath = path;
}

bool UpdateChecker::isNetworkPathAccessible() const
{
    QDir networkDir(updatePath);
    return networkDir.exists();
}

QString UpdateChecker::readLocalVersion() const
{
    // Chercher version.txt dans le répertoire de l'application
    QString versionFilePath = QCoreApplication::applicationDirPath() + "/version.txt";

    QFile file(versionFilePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        // Si le fichier n'existe pas, retourner version par défaut
        return "1.0.0";
    }

    QTextStream in(&file);
    QString version = in.readLine().trimmed();
    file.close();

    return version.isEmpty() ? "1.0.0" : version;
}

QString UpdateChecker::readNetworkVersion() const
{
    if (!isNetworkPathAccessible())
    {
        return QString(); // Retourne chaîne vide si inaccessible
    }

    QString networkVersionFile = updatePath + "/version.txt";

    QFile file(networkVersionFile);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        return QString();
    }

    QTextStream in(&file);
    QString version = in.readLine().trimmed();
    file.close();

    return version;
}

bool UpdateChecker::isNewerVersion(const QString& v1, const QString& v2) const
{
    // Utiliser QVersionNumber pour comparer les versions
    QVersionNumber current = QVersionNumber::fromString(v1);
    QVersionNumber network = QVersionNumber::fromString(v2);

    return network > current;
}

QString UpdateChecker::getCurrentVersion() const
{
    return currentVersion;
}

QString UpdateChecker::getNetworkVersion() const
{
    return networkVersion;
}

bool UpdateChecker::checkForUpdates()
{
    // Lire la version actuelle
    currentVersion = readLocalVersion();

    // Vérifier l'accessibilité du réseau
    if (!isNetworkPathAccessible())
    {
        // Réseau non accessible, pas de mise à jour disponible
        return false;
    }

    // Lire la version réseau
    networkVersion = readNetworkVersion();

    if (networkVersion.isEmpty())
    {
        // Impossible de lire la version réseau
        return false;
    }

    // Comparer les versions
    if (isNewerVersion(currentVersion, networkVersion))
    {
        // Une mise à jour est disponible
        emit updateAvailable(currentVersion, networkVersion);
        return true;
    }

    return false;
}

bool UpdateChecker::copyFileWithBackup(const QString& source, const QString& destination)
{
    // Si le fichier de destination existe, créer une sauvegarde
    if (QFile::exists(destination))
    {
        QString backup = destination + ".backup";

        // Supprimer l'ancienne sauvegarde si elle existe
        if (QFile::exists(backup))
        {
            QFile::remove(backup);
        }

        // Créer la sauvegarde
        if (!QFile::copy(destination, backup))
        {
            return false;
        }

        // Supprimer le fichier de destination
        if (!QFile::remove(destination))
        {
            return false;
        }
    }

    // Copier le nouveau fichier
    return QFile::copy(source, destination);
}

bool UpdateChecker::performUpdate()
{
    if (!isNetworkPathAccessible())
    {
        emit updateCompleted(false);
        return false;
    }

    QString appDir = QCoreApplication::applicationDirPath();
    QString exeName = QCoreApplication::applicationName() + ".exe";

    // Liste des fichiers à mettre à jour
    QStringList filesToUpdate;
    filesToUpdate << exeName;
    filesToUpdate << "version.txt";

    // Optionnel: ajouter les DLL si nécessaire
    // filesToUpdate << "Qt6Core.dll" << "Qt6Gui.dll" << "Qt6Widgets.dll" << "Qt6Concurrent.dll";

    int totalFiles = filesToUpdate.size();
    int currentFile = 0;

    // Copier chaque fichier
    for (const QString& fileName : filesToUpdate)
    {
        QString sourceFile = updatePath + "/" + fileName;
        QString destFile = appDir + "/" + fileName;

        // Vérifier que le fichier source existe
        if (!QFile::exists(sourceFile))
        {
            continue; // Ignorer les fichiers qui n'existent pas
        }

        emit updateProgress((currentFile * 100) / totalFiles,
                           QString("Mise à jour de %1...").arg(fileName));

        // Ne pas essayer de copier l'exe en cours d'exécution
        // On va créer un script batch pour le faire après redémarrage
        if (fileName.endsWith(".exe"))
        {
            // Créer un script de mise à jour pour l'exe
            QString batchScript = appDir + "/update.bat";
            QFile batchFile(batchScript);

            if (batchFile.open(QIODevice::WriteOnly | QIODevice::Text))
            {
                QTextStream out(&batchFile);
                out << "@echo off\n";
                out << "timeout /t 2 /nobreak > nul\n";  // Attendre 2 secondes
                out << "copy /Y \"" << sourceFile << "\" \"" << destFile << "\"\n";
                out << "del \"%~f0\"\n";  // Supprimer le script
                batchFile.close();

                // Le script sera exécuté après la fermeture de l'application
            }
        }
        else
        {
            // Copier les autres fichiers directement
            if (!copyFileWithBackup(sourceFile, destFile))
            {
                emit updateCompleted(false);
                return false;
            }
        }

        currentFile++;
    }

    emit updateProgress(100, "Mise à jour terminée");
    emit updateCompleted(true);

    return true;
}
