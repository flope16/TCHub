#pragma once
#include <string>
#include <vector>
#include <functional>

/**
 * @brief Classe pour forcer le mot de passe d'un fichier Excel crypté
 *
 * Cette classe permet de tester différentes combinaisons de mots de passe
 * pour ouvrir un fichier Excel protégé par mot de passe.
 *
 * NOTE: Le brute-force peut prendre beaucoup de temps selon la longueur
 * et la complexité du mot de passe.
 */
class ExcelBruteForce
{
public:
    /**
     * @brief Callback pour rapporter les progrès
     * @param attempts Nombre de tentatives effectuées
     * @param currentPassword Mot de passe actuellement testé
     */
    using ProgressCallback = std::function<void(int attempts, const std::string& currentPassword)>;

    /**
     * @brief Structure de configuration pour le brute-force
     */
    struct Config
    {
        int minLength = 1;          // Longueur minimale du mot de passe
        int maxLength = 4;          // Longueur maximale du mot de passe
        std::string charset = "abcdefghijklmnopqrstuvwxyz"; // Jeu de caractères
        int progressInterval = 1000; // Intervalle de rapport de progrès
    };

    /**
     * @brief Tente de forcer le mot de passe d'un fichier Excel
     *
     * @param filePath Chemin du fichier Excel crypté
     * @param config Configuration du brute-force
     * @param progressCallback Callback pour rapporter les progrès (optionnel)
     * @return Mot de passe trouvé (vide si non trouvé)
     */
    static std::string bruteForce(
        const std::string& filePath,
        const Config& config = Config(),
        ProgressCallback progressCallback = nullptr
    );

    /**
     * @brief Teste un mot de passe spécifique sur un fichier Excel
     *
     * @param filePath Chemin du fichier Excel crypté
     * @param password Mot de passe à tester
     * @return true si le mot de passe est correct
     */
    static bool testPassword(
        const std::string& filePath,
        const std::string& password
    );

    /**
     * @brief Obtient le dernier message d'erreur
     * @return Message d'erreur
     */
    static std::string getLastError();

    /**
     * @brief Arrête le processus de brute-force en cours
     */
    static void stop();

private:
    static std::string lastError;
    static bool stopRequested;

    /**
     * @brief Génère toutes les combinaisons possibles pour une longueur donnée
     * @param charset Jeu de caractères
     * @param length Longueur des mots de passe
     * @param callback Fonction appelée pour chaque combinaison
     */
    static void generateCombinations(
        const std::string& charset,
        int length,
        const std::function<bool(const std::string&)>& callback
    );

    /**
     * @brief Fonction récursive pour générer les combinaisons
     */
    static bool generateCombinationsRecursive(
        const std::string& charset,
        int length,
        std::string current,
        const std::function<bool(const std::string&)>& callback
    );

    /**
     * @brief Vérifie si un fichier est crypté
     * @param filePath Chemin du fichier
     * @return true si le fichier est crypté
     */
    static bool isEncrypted(const std::string& filePath);
};
