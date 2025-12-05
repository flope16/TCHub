#include "ExcelProtectionRemover.h"
#include <filesystem>
#include <fstream>
#include <sstream>
#include <vector>
#include <regex>
#include <windows.h>

// Inclure minizip
#ifdef USE_MINIZIP
#include <minizip/unzip.h>
#include <minizip/zip.h>
#endif

std::string ExcelProtectionRemover::lastError = "";

std::string ExcelProtectionRemover::getLastError()
{
    return lastError;
}

std::string ExcelProtectionRemover::generateOutputPath(const std::string& inputPath)
{
    std::filesystem::path p(inputPath);
    std::string stem = p.stem().string();
    std::string extension = p.extension().string();
    std::filesystem::path parent = p.parent_path();

    return (parent / (stem + "_unprotected" + extension)).string();
}

#ifdef USE_MINIZIP
bool ExcelProtectionRemover::extractZip(const std::string& zipPath, const std::string& extractDir)
{
    unzFile uf = unzOpen(zipPath.c_str());
    if (uf == nullptr)
    {
        lastError = "Impossible d'ouvrir le fichier ZIP: " + zipPath;
        OutputDebugStringA(("[ExcelProtectionRemover] " + lastError + "\n").c_str());
        return false;
    }

    unz_global_info gi;
    if (unzGetGlobalInfo(uf, &gi) != UNZ_OK)
    {
        lastError = "Erreur lors de la lecture des informations du ZIP";
        unzClose(uf);
        return false;
    }

    // Créer le dossier d'extraction
    std::filesystem::create_directories(extractDir);

    // Extraire tous les fichiers
    for (uLong i = 0; i < gi.number_entry; i++)
    {
        char filename[256];
        unz_file_info file_info;

        if (unzGetCurrentFileInfo(uf, &file_info, filename, sizeof(filename), nullptr, 0, nullptr, 0) != UNZ_OK)
        {
            lastError = "Erreur lors de la lecture des informations du fichier";
            unzClose(uf);
            return false;
        }

        // Construire le chemin complet
        std::filesystem::path filePath = std::filesystem::path(extractDir) / filename;

        // Si c'est un dossier (se termine par /)
        if (filename[strlen(filename) - 1] == '/')
        {
            std::filesystem::create_directories(filePath);
        }
        else
        {
            // Créer les dossiers parents si nécessaire
            if (filePath.has_parent_path())
            {
                std::filesystem::create_directories(filePath.parent_path());
            }

            // Ouvrir le fichier dans le ZIP
            if (unzOpenCurrentFile(uf) != UNZ_OK)
            {
                lastError = "Impossible d'ouvrir le fichier: " + std::string(filename);
                unzClose(uf);
                return false;
            }

            // Lire et écrire le fichier
            std::ofstream outFile(filePath, std::ios::binary);
            if (!outFile.is_open())
            {
                lastError = "Impossible de créer le fichier: " + filePath.string();
                unzCloseCurrentFile(uf);
                unzClose(uf);
                return false;
            }

            char buffer[4096];
            int bytesRead;
            while ((bytesRead = unzReadCurrentFile(uf, buffer, sizeof(buffer))) > 0)
            {
                outFile.write(buffer, bytesRead);
            }

            outFile.close();
            unzCloseCurrentFile(uf);
        }

        // Passer au fichier suivant
        if (i + 1 < gi.number_entry)
        {
            if (unzGoToNextFile(uf) != UNZ_OK)
            {
                lastError = "Erreur lors du passage au fichier suivant";
                unzClose(uf);
                return false;
            }
        }
    }

    unzClose(uf);
    return true;
}

bool ExcelProtectionRemover::createZip(const std::string& zipPath, const std::string& sourceDir)
{
    zipFile zf = zipOpen(zipPath.c_str(), APPEND_STATUS_CREATE);
    if (zf == nullptr)
    {
        lastError = "Impossible de créer le fichier ZIP: " + zipPath;
        OutputDebugStringA(("[ExcelProtectionRemover] " + lastError + "\n").c_str());
        return false;
    }

    try
    {
        // Parcourir récursivement tous les fichiers du dossier source
        for (const auto& entry : std::filesystem::recursive_directory_iterator(sourceDir))
        {
            if (entry.is_regular_file())
            {
                // Chemin relatif dans le ZIP
                std::filesystem::path relativePath = std::filesystem::relative(entry.path(), sourceDir);
                std::string relativePathStr = relativePath.string();

                // Remplacer les backslashes par des slashes pour le ZIP
                std::replace(relativePathStr.begin(), relativePathStr.end(), '\\', '/');

                // Ouvrir le fichier dans le ZIP
                zip_fileinfo zi = {};
                if (zipOpenNewFileInZip(zf, relativePathStr.c_str(), &zi,
                    nullptr, 0, nullptr, 0, nullptr, Z_DEFLATED, Z_DEFAULT_COMPRESSION) != ZIP_OK)
                {
                    lastError = "Erreur lors de la création du fichier dans le ZIP: " + relativePathStr;
                    zipClose(zf, nullptr);
                    return false;
                }

                // Lire et écrire le contenu du fichier
                std::ifstream file(entry.path(), std::ios::binary);
                if (file.is_open())
                {
                    std::vector<char> buffer((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
                    file.close();

                    if (zipWriteInFileInZip(zf, buffer.data(), static_cast<unsigned int>(buffer.size())) != ZIP_OK)
                    {
                        lastError = "Erreur lors de l'écriture dans le ZIP";
                        zipCloseFileInZip(zf);
                        zipClose(zf, nullptr);
                        return false;
                    }
                }
                else
                {
                    lastError = "Impossible de lire le fichier: " + entry.path().string();
                    zipCloseFileInZip(zf);
                    zipClose(zf, nullptr);
                    return false;
                }

                zipCloseFileInZip(zf);
            }
        }

        zipClose(zf, nullptr);
        return true;
    }
    catch (const std::exception& e)
    {
        lastError = "Exception lors de la création du ZIP: " + std::string(e.what());
        zipClose(zf, nullptr);
        return false;
    }
}
#endif

bool ExcelProtectionRemover::removeSheetProtectionTag(const std::string& xmlPath)
{
    // Lire le fichier XML
    std::ifstream file(xmlPath);
    if (!file.is_open())
    {
        lastError = "Impossible de lire le fichier: " + xmlPath;
        return false;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string content = buffer.str();
    file.close();

    // Chercher la balise <sheetProtection>
    // Elle peut être auto-fermante <sheetProtection .../> ou avec une balise de fermeture <sheetProtection ...></sheetProtection>

    // Expression régulière pour trouver <sheetProtection...> ou <sheetProtection.../>
    std::regex protectionRegex(R"(<sheetProtection[^>]*?(?:/>|>[^<]*</sheetProtection>))");

    std::string newContent = std::regex_replace(content, protectionRegex, "");

    // Vérifier si une protection a été trouvée
    bool protectionFound = (newContent != content);

    if (protectionFound)
    {
        // Écrire le nouveau contenu
        std::ofstream outFile(xmlPath);
        if (!outFile.is_open())
        {
            lastError = "Impossible d'écrire le fichier: " + xmlPath;
            return false;
        }

        outFile << newContent;
        outFile.close();

        OutputDebugStringA(("[ExcelProtectionRemover] Protection supprimée dans: " + xmlPath + "\n").c_str());
    }

    return protectionFound;
}

bool ExcelProtectionRemover::removeProtection(const std::string& filePath, const std::string& outputPath)
{
#ifndef USE_MINIZIP
    lastError = "MINIZIP n'est pas activé. Veuillez recompiler avec USE_MINIZIP défini.";
    OutputDebugStringA(("[ExcelProtectionRemover] " + lastError + "\n").c_str());
    return false;
#else
    try
    {
        OutputDebugStringA(("[ExcelProtectionRemover] Début de suppression de protection: " + filePath + "\n").c_str());

        // Vérifier que le fichier existe
        if (!std::filesystem::exists(filePath))
        {
            lastError = "Le fichier n'existe pas: " + filePath;
            OutputDebugStringA(("[ExcelProtectionRemover] " + lastError + "\n").c_str());
            return false;
        }

        // Créer un dossier temporaire
        std::filesystem::path tempDir = std::filesystem::temp_directory_path() /
            ("excel_unprotect_" + std::to_string(std::time(nullptr)));

        OutputDebugStringA(("[ExcelProtectionRemover] Dossier temporaire: " + tempDir.string() + "\n").c_str());

        // Extraire le ZIP
        if (!extractZip(filePath, tempDir.string()))
        {
            OutputDebugStringA(("[ExcelProtectionRemover] Échec de l'extraction\n").c_str());
            return false;
        }

        OutputDebugStringA("[ExcelProtectionRemover] Extraction réussie\n");

        // Parcourir les fichiers sheet*.xml dans xl/worksheets/
        std::filesystem::path worksheetsPath = tempDir / "xl" / "worksheets";

        if (!std::filesystem::exists(worksheetsPath))
        {
            lastError = "Le dossier xl/worksheets n'existe pas. Le fichier n'est peut-être pas un fichier Excel valide.";
            OutputDebugStringA(("[ExcelProtectionRemover] " + lastError + "\n").c_str());
            std::filesystem::remove_all(tempDir);
            return false;
        }

        bool protectionFound = false;
        int sheetCount = 0;

        for (const auto& entry : std::filesystem::directory_iterator(worksheetsPath))
        {
            if (entry.is_regular_file())
            {
                std::string filename = entry.path().filename().string();

                // Vérifier si c'est un fichier sheet*.xml
                if (filename.find("sheet") == 0 && filename.find(".xml") != std::string::npos)
                {
                    sheetCount++;
                    if (removeSheetProtectionTag(entry.path().string()))
                    {
                        protectionFound = true;
                        OutputDebugStringA(("[ExcelProtectionRemover] Protection supprimée dans: " + filename + "\n").c_str());
                    }
                }
            }
        }

        if (sheetCount == 0)
        {
            lastError = "Aucune feuille trouvée dans le fichier Excel";
            OutputDebugStringA(("[ExcelProtectionRemover] " + lastError + "\n").c_str());
            std::filesystem::remove_all(tempDir);
            return false;
        }

        if (!protectionFound)
        {
            lastError = "Aucune protection trouvée dans les feuilles";
            OutputDebugStringA(("[ExcelProtectionRemover] " + lastError + "\n").c_str());
            std::filesystem::remove_all(tempDir);
            return false;
        }

        // Créer une sauvegarde
        std::string backupPath = filePath + ".backup";
        std::filesystem::copy_file(filePath, backupPath, std::filesystem::copy_options::overwrite_existing);
        OutputDebugStringA(("[ExcelProtectionRemover] Sauvegarde créée: " + backupPath + "\n").c_str());

        // Générer le nom du fichier de sortie
        std::string finalOutputPath = outputPath.empty() ? generateOutputPath(filePath) : outputPath;

        // Supprimer le fichier de sortie s'il existe déjà
        if (std::filesystem::exists(finalOutputPath))
        {
            std::filesystem::remove(finalOutputPath);
        }

        // Créer le nouveau fichier ZIP
        if (!createZip(finalOutputPath, tempDir.string()))
        {
            OutputDebugStringA("[ExcelProtectionRemover] Échec de la création du ZIP\n");
            std::filesystem::remove_all(tempDir);
            return false;
        }

        OutputDebugStringA(("[ExcelProtectionRemover] Fichier créé avec succès: " + finalOutputPath + "\n").c_str());

        // Nettoyer le dossier temporaire
        try
        {
            std::filesystem::remove_all(tempDir);
            OutputDebugStringA("[ExcelProtectionRemover] Dossier temporaire supprimé\n");
        }
        catch (...)
        {
            OutputDebugStringA("[ExcelProtectionRemover] ATTENTION: Échec suppression dossier temporaire\n");
        }

        lastError = ""; // Pas d'erreur
        return true;
    }
    catch (const std::exception& e)
    {
        lastError = "Exception: " + std::string(e.what());
        OutputDebugStringA(("[ExcelProtectionRemover] EXCEPTION: " + lastError + "\n").c_str());
        return false;
    }
#endif
}
