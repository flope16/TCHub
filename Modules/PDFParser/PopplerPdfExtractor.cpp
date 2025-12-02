#include "PopplerPdfExtractor.h"
#include <filesystem>
#include <fstream>
#include <sstream>

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
                result << text.to_latin1() << "\n";
            }
        }

        return result.str();
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

    // Essayer avec Poppler si disponible
    if (isPopplerAvailable())
    {
        std::string text = readWithPoppler(pdfPath);
        if (!text.empty())
        {
            return text;
        }
    }

    // Fallback: chercher un fichier .txt correspondant
    std::filesystem::path txtPath = std::filesystem::path(pdfPath).replace_extension(".txt");
    if (std::filesystem::exists(txtPath))
    {
        std::ifstream file(txtPath);
        if (file.is_open())
        {
            std::stringstream buffer;
            buffer << file.rdbuf();
            return buffer.str();
        }
    }

    // Fallback: essayer pdftotext en ligne de commande
    std::string tempTxt = std::filesystem::path(pdfPath).replace_extension(".temp.txt").string();
    std::string command = "pdftotext \"" + pdfPath + "\" \"" + tempTxt + "\" 2>nul";

    if (system(command.c_str()) == 0 && std::filesystem::exists(tempTxt))
    {
        std::ifstream file(tempTxt);
        if (file.is_open())
        {
            std::stringstream buffer;
            buffer << file.rdbuf();
            file.close();
            std::filesystem::remove(tempTxt);
            return buffer.str();
        }
    }

    return "";
}
