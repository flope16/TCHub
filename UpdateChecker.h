#pragma once

#include <QObject>
#include <QString>
#include <QVersionNumber>

/**
 * @brief Classe pour vérifier et gérer les mises à jour de l'application
 *
 * Cette classe vérifie la présence d'une nouvelle version sur un partage réseau
 * et propose à l'utilisateur de mettre à jour l'application.
 */
class UpdateChecker : public QObject
{
    Q_OBJECT

public:
    explicit UpdateChecker(QObject *parent = nullptr);

    /**
     * @brief Vérifie si une mise à jour est disponible
     * @return true si une mise à jour est disponible
     */
    bool checkForUpdates();

    /**
     * @brief Obtient la version actuelle de l'application
     * @return Version actuelle (ex: "1.0.0")
     */
    QString getCurrentVersion() const;

    /**
     * @brief Obtient la version disponible sur le réseau
     * @return Version réseau (ex: "1.1.0")
     */
    QString getNetworkVersion() const;

    /**
     * @brief Lance le processus de mise à jour
     * @return true si la mise à jour a réussi
     */
    bool performUpdate();

    /**
     * @brief Définit le chemin réseau où se trouvent les mises à jour
     * @param path Chemin réseau (ex: "Y:\\Florian\\TCHub")
     */
    void setUpdatePath(const QString& path);

    /**
     * @brief Vérifie si le chemin réseau est accessible
     * @return true si le chemin est accessible
     */
    bool isNetworkPathAccessible() const;

signals:
    /**
     * @brief Signal émis quand une mise à jour est disponible
     * @param currentVersion Version actuelle
     * @param newVersion Nouvelle version disponible
     */
    void updateAvailable(const QString& currentVersion, const QString& newVersion);

    /**
     * @brief Signal émis pendant la progression de la mise à jour
     * @param progress Progression (0-100)
     * @param message Message de statut
     */
    void updateProgress(int progress, const QString& message);

    /**
     * @brief Signal émis quand la mise à jour est terminée
     * @param success true si réussie
     */
    void updateCompleted(bool success);

private:
    QString updatePath;           // Chemin réseau vers les mises à jour
    QString currentVersion;       // Version actuelle
    QString networkVersion;       // Version disponible sur le réseau

    /**
     * @brief Lit le fichier version.txt local
     * @return Version lue
     */
    QString readLocalVersion() const;

    /**
     * @brief Lit le fichier version.txt depuis le réseau
     * @return Version lue
     */
    QString readNetworkVersion() const;

    /**
     * @brief Compare deux numéros de version
     * @param v1 Version 1
     * @param v2 Version 2
     * @return true si v2 > v1
     */
    bool isNewerVersion(const QString& v1, const QString& v2) const;

    /**
     * @brief Copie un fichier avec gestion d'erreur
     * @param source Fichier source
     * @param destination Fichier destination
     * @return true si succès
     */
    bool copyFileWithBackup(const QString& source, const QString& destination);
};
