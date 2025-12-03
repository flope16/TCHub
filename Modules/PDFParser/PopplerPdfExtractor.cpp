#include "PopplerPdfExtractor.h"
#include <filesystem>
#include <fstream>
#include <sstream>
#include <vector>
#include <windows.h>

// Fonction helper pour exécuter une commande sans afficher de fenêtre
static int executeCommandSilent(const std::string& command)
{
    STARTUPINFOA si;
    PROCESS_INFORMATION pi;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;  // Cacher la fenêtre
    ZeroMemory(&pi, sizeof(pi));

    // Créer une copie modifiable de la commande
    std::string cmdCopy = command;

    // Créer le processus
    if (!CreateProcessA(
        NULL,                   // Nom de l'application
        &cmdCopy[0],           // Ligne de commande (modifiable)
        NULL,                   // Attributs de sécurité du processus
        NULL,                   // Attributs de sécurité du thread
        FALSE,                  // Héritage des handles
        CREATE_NO_WINDOW,       // Drapeaux de création (pas de fenêtre)
        NULL,                   // Environnement
        NULL,                   // Répertoire courant
        &si,                    // Informations de démarrage
        &pi))                   // Informations du processus
    {
        return -1;  // Échec
    }

    // Attendre que le processus se termine
    WaitForSingleObject(pi.hProcess, INFINITE);

    // Récupérer le code de sortie
    DWORD exitCode = 0;
    GetExitCodeProcess(pi.hProcess, &exitCode);

    // Fermer les handles
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    return static_cast<int>(exitCode);
}

// Définir USE_POPPLER si Poppler est disponible
// Pour l'instant, on détecte à la compilation
#ifdef USE_POPPLER
#include <poppler/cpp/poppler-document.h>
#include <poppler/cpp/poppler-page.h>
#include <poppler/cpp/poppler-page-renderer.h>
#endif

bool PopplerPdfExtractor::isPopplerAvailable()
{
#ifdef USE_POPPLER
    return true;
#else
    return false;
#endif
}

std::string PopplerPdfExtractor::readWithPoppler(const std::string& pdfPath)
{
#ifdef USE_POPPLER
    try
    {
        // Normaliser le chemin (convertir backslashes en forward slashes)
        std::string normalizedPath = pdfPath;
        std::replace(normalizedPath.begin(), normalizedPath.end(), '\\', '/');

        std::string debugMsg = "[Poppler API] Tentative de chargement: " + normalizedPath + "\n";
        OutputDebugStringA(debugMsg.c_str());

        // Charger le document PDF
        std::unique_ptr<poppler::document> doc(poppler::document::load_from_file(normalizedPath));

        if (!doc)
        {
            OutputDebugStringA("[Poppler API] ERREUR: Document NULL - fichier introuvable ou invalide\n");
            return "";
        }

        if (doc->is_locked())
        {
            OutputDebugStringA("[Poppler API] ERREUR: Document verrouille (crypte)\n");
            return "";
        }

        std::stringstream result;

        // Extraire le texte de chaque page
        int pages = doc->pages();
        debugMsg = "[Poppler API] Document charge: " + std::to_string(pages) + " page(s)\n";
        OutputDebugStringA(debugMsg.c_str());

        for (int i = 0; i < pages; ++i)
        {
            std::unique_ptr<poppler::page> page(doc->create_page(i));
            if (!page)
            {
                debugMsg = "[Poppler API] ERREUR: Impossible de creer la page " + std::to_string(i + 1) + "\n";
                OutputDebugStringA(debugMsg.c_str());
                continue;
            }

            // Méthode 1 : Essayer d'extraire avec le layout préservé
            poppler::rectf pageRect(0, 0, page->page_rect().width(), page->page_rect().height());
            poppler::ustring text = page->text(pageRect);

            // Utiliser UTF-8 pour préserver tous les caractères Unicode (accents, espaces insécables, etc.)
            poppler::byte_array utf8_data = text.to_utf8();
            std::string pageText(utf8_data.begin(), utf8_data.end());

            // Si le texte extrait est vide, essayer la méthode simple
            if (pageText.empty())
            {
                text = page->text();
                utf8_data = text.to_utf8();
                pageText = std::string(utf8_data.begin(), utf8_data.end());
            }

            debugMsg = "[Poppler API] Page " + std::to_string(i + 1) + ": " +
                std::to_string(pageText.length()) + " caracteres extraits\n";
            OutputDebugStringA(debugMsg.c_str());

            result << pageText << "\n";
        }

        std::string extracted = result.str();

        debugMsg = "[Poppler API] Total extrait: " + std::to_string(extracted.length()) + " caracteres\n";
        OutputDebugStringA(debugMsg.c_str());

        return extracted;
    }
    catch (const std::exception& e)
    {
        std::string errorMsg = "[Poppler API] EXCEPTION: ";
        errorMsg += e.what();
        errorMsg += "\n";
        OutputDebugStringA(errorMsg.c_str());
        return "";
    }
    catch (...)
    {
        OutputDebugStringA("[Poppler API] EXCEPTION INCONNUE\n");
        return "";
    }
#else
    // Poppler non disponible
    return "";
#endif
}

std::string PopplerPdfExtractor::extractTextFromPdf(const std::string& pdfPath)
{
    if (!std::filesystem::exists(pdfPath))
    {
        return "";
    }

    // MÉTHODE 1 : Essayer pdftotext (utilitaire en ligne de commande de Poppler)
    // C'est souvent plus robuste que l'API C++ pour certains PDFs
    std::string tempTxt = std::filesystem::path(pdfPath).replace_extension(".poppler_temp.txt").string();

    // Essayer plusieurs emplacements pour pdftotext
    std::vector<std::string> pdftotext_paths = {
        "pdftotext",  // Dans le PATH (si ajouté)
        "C:\\Dev\\vcpkg\\installed\\x64-windows\\tools\\poppler\\pdftotext.exe",
        "C:\\Dev\\vcpkg\\installed\\x64-windows\\bin\\pdftotext.exe",
        ".\\pdftotext.exe",  // Dans le dossier de l'exécutable TCHub
        "C:\\poppler-utils\\pdftotext.exe"  // Emplacement alternatif
    };

    for (const auto& pdftotext_path : pdftotext_paths)
    {
        std::string command = "\"" + pdftotext_path + "\" -layout \"" + pdfPath + "\" \"" + tempTxt + "\" 2>nul";

        // Exécuter pdftotext sans afficher de fenêtre
        if (executeCommandSilent(command) == 0 && std::filesystem::exists(tempTxt))
        {
            std::ifstream file(tempTxt);
            if (file.is_open())
            {
                std::stringstream buffer;
                buffer << file.rdbuf();
                std::string text = buffer.str();
                file.close();
                std::filesystem::remove(tempTxt);

                if (!text.empty())
                {
                    std::string msg = "[PopplerPdfExtractor] Extraction reussie avec pdftotext (" + pdftotext_path + ")\n";
                    OutputDebugStringA(msg.c_str());
                    return text;
                }
            }
        }
    }

    // MÉTHODE 2 : Essayer avec l'API Poppler C++
    if (isPopplerAvailable())
    {
        std::string text = readWithPoppler(pdfPath);
        if (!text.empty())
        {
            OutputDebugStringA("[PopplerPdfExtractor] Extraction reussie avec API Poppler C++\n");
            return text;
        }
    }

    // MÉTHODE 3 : Fallback vers fichier .txt existant
    std::filesystem::path txtPath = std::filesystem::path(pdfPath).replace_extension(".txt");
    if (std::filesystem::exists(txtPath))
    {
        std::ifstream file(txtPath);
        if (file.is_open())
        {
            std::stringstream buffer;
            buffer << file.rdbuf();
            std::string text = buffer.str();
            if (!text.empty())
            {
                OutputDebugStringA("[PopplerPdfExtractor] Extraction depuis fichier .txt existant\n");
                return text;
            }
        }
    }

    OutputDebugStringA("[PopplerPdfExtractor] ECHEC: Aucune methode n'a reussi a extraire du texte\n");
    return "";
}
