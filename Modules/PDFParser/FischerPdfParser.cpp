#include "FischerPdfParser.h"
#include "PopplerPdfExtractor.h"
#include <fstream>
#include <sstream>
#include <regex>
#include <algorithm>
#include <filesystem>
#include <windows.h>

std::string FischerPdfParser::extractText(const std::string& filePath)
{
    // Utiliser PopplerPdfExtractor
    std::string text = PopplerPdfExtractor::extractTextFromPdf(filePath);

    // Debug: sauvegarder le texte extrait
    try
    {
        std::filesystem::path pdfPathObj(filePath);
        std::filesystem::path debugPath = pdfPathObj.parent_path() / (pdfPathObj.stem().string() + "_fischer_extracted.txt");
        std::ofstream debugFile(debugPath);
        if (debugFile.is_open())
        {
            debugFile << text;
            debugFile.close();
            std::string debugMsg = "[Fischer] Texte extrait sauvegardé dans: " + debugPath.string() + "\n";
            OutputDebugStringA(debugMsg.c_str());
        }
    }
    catch (...) {}

    // Debug: afficher dans la console
    OutputDebugStringA("=== DEBUG EXTRACTION PDF FISCHER ===\n");
    std::string debug = "Poppler disponible: " + std::string(PopplerPdfExtractor::isPopplerAvailable() ? "OUI" : "NON") + "\n";
    debug += "Texte extrait (" + std::to_string(text.length()) + " caractères):\n";
    size_t previewLength = text.length() < 1500 ? text.length() : 1500;
    debug += text.substr(0, previewLength) + "\n";
    debug += "=== FIN DEBUG FISCHER ===\n";
    OutputDebugStringA(debug.c_str());

    return text;
}

// Fonction helper pour parser un nombre au format français (virgule décimale)
static double parseFrenchNumber(const std::string& str)
{
    std::string cleaned = str;
    // Supprimer les espaces
    cleaned.erase(std::remove(cleaned.begin(), cleaned.end(), ' '), cleaned.end());
    // Remplacer virgule par point
    std::replace(cleaned.begin(), cleaned.end(), ',', '.');
    try {
        return std::stod(cleaned);
    }
    catch (...) {
        return 0.0;
    }
}

std::vector<PdfLine> FischerPdfParser::parseTextContent(const std::string& text)
{
    std::vector<PdfLine> lines;

    OutputDebugStringA("=== DEBUT PARSING FISCHER ===\n");

    // Vérifier si c'est un message d'erreur
    if (text.find("ERREUR:") == 0)
    {
        OutputDebugStringA("[Fischer] Texte contient ERREUR, abandon\n");
        return lines;
    }

    if (text.empty())
    {
        OutputDebugStringA("[Fischer] Texte vide, abandon\n");
        return lines;
    }

    // Parser multi-lignes adapté à la structure Fischer
    // Format observé sur 3 lignes :
    //   Ligne 1 : "10         Collier FRSR 20-25 M8/M10 -100/bte           6   BTE    0,01    1   BTE    145,14"
    //   Ligne 2 : "4048962230956               534135                      Éco-contribution    0,01    0,06"
    //   Ligne 3 : "Total éco-contribution comprise                         22,99    137,94"

    int extractedCount = 0;

    try
    {
        OutputDebugStringA("[Fischer] Creation du regex...\n");

        // Regex plus permissif:
        // - Utiliser \s+ au lieu de simples espaces pour gérer la mise en page avec beaucoup d'espaces
        // - Rendre la désignation plus flexible avec .*? pour capturer tout jusqu'aux unités
        std::regex productRegex(
            R"((\d+)\s+(.+?)\s+(\d+)\s+(?:BTE|PCE)\s+[0-9.,]+\s+1\s+(?:BTE|PCE)\s+[0-9.,]+)"  // Ligne produit (sans \n obligatoire)
            R"(.*?)"  // Espaces et retour à la ligne flexibles
            R"((\d{13}).*?[ÉEé]co-contribution.*?)"  // Ligne GTIN + éco (flexible)
            R"(Total\s+[ée]co-contribution\s+comprise\s+(?:[0-9.,]+\s+)?([0-9.,]+))"  // Ligne Total
        );

        OutputDebugStringA("[Fischer] Regex cree, debut recherche...\n");

        // Utiliser regex_iterator pour trouver tous les matches dans le texte
        auto matches_begin = std::sregex_iterator(text.begin(), text.end(), productRegex);
        auto matches_end = std::sregex_iterator();

        std::string countMsg = "[Fischer] Nombre de matches trouves: " +
            std::to_string(std::distance(matches_begin, matches_end)) + "\n";
        OutputDebugStringA(countMsg.c_str());

        // Re-créer l'itérateur car distance() l'a consommé
        matches_begin = std::sregex_iterator(text.begin(), text.end(), productRegex);

        for (std::sregex_iterator i = matches_begin; i != matches_end; ++i)
        {
            std::smatch match = *i;

            OutputDebugStringA("[Fischer] Match trouve, extraction...\n");

            PdfLine pdfLine;
            pdfLine.reference = match[4].str();  // GTIN (13 chiffres)
            pdfLine.designation = match[2].str(); // Désignation

            // Nettoyer la désignation (supprimer espaces superflus)
            pdfLine.designation.erase(0, pdfLine.designation.find_first_not_of(" \t"));
            pdfLine.designation.erase(pdfLine.designation.find_last_not_of(" \t") + 1);

            pdfLine.quantite = parseFrenchNumber(match[3].str());   // Quantité
            pdfLine.prixHT = parseFrenchNumber(match[5].str());     // Prix total

            lines.push_back(pdfLine);
            extractedCount++;

            std::string debugMsg = "Fischer produit #" + std::to_string(extractedCount) + ": " +
                pdfLine.reference + " | " + pdfLine.designation + " | " +
                std::to_string(pdfLine.quantite) + " | " + std::to_string(pdfLine.prixHT) + "\n";
            OutputDebugStringA(debugMsg.c_str());
        }
    }
    catch (const std::regex_error& e)
    {
        std::string errorMsg = "[Fischer] ERREUR REGEX: " + std::string(e.what()) + "\n";
        OutputDebugStringA(errorMsg.c_str());
    }
    catch (const std::exception& e)
    {
        std::string errorMsg = "[Fischer] EXCEPTION: " + std::string(e.what()) + "\n";
        OutputDebugStringA(errorMsg.c_str());
    }
    catch (...)
    {
        OutputDebugStringA("[Fischer] EXCEPTION INCONNUE\n");
    }

    // Log final
    std::string finalMsg = "=== RESUME PARSING FISCHER ===\n";
    finalMsg += "Produits extraits : " + std::to_string(extractedCount) + "\n";
    finalMsg += "==============================\n";
    OutputDebugStringA(finalMsg.c_str());

    return lines;
}

std::vector<PdfLine> FischerPdfParser::parse(const std::string& filePath)
{
    std::string text = extractText(filePath);
    return parseTextContent(text);
}
