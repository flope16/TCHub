#include "PopplerPdfExtractor.h"
#include <filesystem>
#include <fstream>
#include <sstream>
#include <vector>
#include <windows.h>

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
        // Charger le document PDF
        std::unique_ptr<poppler::document> doc(poppler::document::load_from_file(pdfPath));

        if (!doc || doc->is_locked())
        {
            return "";
        }

        std::stringstream result;

        // Extraire le texte de chaque page
        int pages = doc->pages();
        for (int i = 0; i < pages; ++i)
        {
            std::unique_ptr<poppler::page> page(doc->create_page(i));
            if (page)
            {
                poppler::ustring text = page->text();
                // Utiliser UTF-8 pour préserver tous les caractères Unicode (accents, espaces insécables, etc.)
                poppler::byte_array utf8_data = text.to_utf8();
                result << std::string(utf8_data.begin(), utf8_data.end()) << "\n";
            }
        }

        std::string extracted = result.str();

        // Si Poppler ne retourne rien, ce n'est pas une vraie erreur
        // Retourner la chaîne vide pour essayer les autres méthodes
        return extracted;
    }
    catch (...)
    {
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
        "C:\\Dev\\vcpkg\\installed\\x64-windows\\tools\\poppler\\pdftotext.exe",
        "C:\\Dev\\vcpkg\\installed\\x64-windows\\bin\\pdftotext.exe",
        "pdftotext"  // Dans le PATH
    };

    for (const auto& pdftotext_path : pdftotext_paths)
    {
        std::string command = "\"" + pdftotext_path + "\" -layout \"" + pdfPath + "\" \"" + tempTxt + "\" 2>nul";

        if (system(command.c_str()) == 0 && std::filesystem::exists(tempTxt))
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
