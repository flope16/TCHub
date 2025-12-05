#pragma once
#include <string>

/**
 * @brief Classe pour supprimer la protection des feuilles Excel
 *
 * Cette classe permet de supprimer la protection des feuilles d'un fichier Excel (.xlsx)
 * en manipulant directement le XML à l'intérieur du fichier ZIP.
 *
 * ATTENTION: Cette classe ne fonctionne que sur des fichiers Excel NON CRYPTÉS.
 * Pour les fichiers cryptés, utilisez ExcelBruteForce.
 */
class ExcelProtectionRemover
{
public:
    /**
     * @brief Supprime la protection des feuilles d'un fichier Excel
     *
     * @param filePath Chemin complet du fichier Excel (.xlsx)
     * @param outputPath Chemin du fichier de sortie (optionnel, par défaut: fichier_unprotected.xlsx)
     * @return true si succès, false sinon
     */
    static bool removeProtection(
        const std::string& filePath,
        const std::string& outputPath = ""
    );

    /**
     * @brief Obtient le dernier message d'erreur
     * @return Message d'erreur
     */
    static std::string getLastError();

private:
    static std::string lastError;

    /**
     * @brief Extrait le fichier ZIP dans un dossier temporaire
     * @param zipPath Chemin du fichier ZIP
     * @param extractDir Dossier de destination
     * @return true si succès
     */
    static bool extractZip(const std::string& zipPath, const std::string& extractDir);

    /**
     * @brief Supprime la balise sheetProtection d'un fichier XML
     * @param xmlPath Chemin du fichier XML
     * @return true si une protection a été trouvée et supprimée
     */
    static bool removeSheetProtectionTag(const std::string& xmlPath);

    /**
     * @brief Crée un fichier ZIP à partir d'un dossier
     * @param zipPath Chemin du fichier ZIP à créer
     * @param sourceDir Dossier source
     * @return true si succès
     */
    static bool createZip(const std::string& zipPath, const std::string& sourceDir);

    /**
     * @brief Génère le nom du fichier de sortie par défaut
     * @param inputPath Chemin du fichier d'entrée
     * @return Chemin du fichier de sortie
     */
    static std::string generateOutputPath(const std::string& inputPath);
};
