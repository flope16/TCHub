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

    // Vérifier si c'est un message d'erreur
    if (text.find("ERREUR:") == 0)
    {
        return lines;
    }

    // Parser multi-lignes adapté à la structure Fischer
    // Format observé :
    //   Ligne Pos : "10         Collier FRSR 20-25 M8/M10 -100/bte                                                   6   BTE           0,01           1   BTE                                                            145,14"
    //   Ligne GTIN : "4048962230956               534135                                                                                       Éco-contribution                                        0,01              0,06"
    //   Ligne Total : "Total éco-contribution comprise                                      22,99          137,94"

    std::istringstream stream(text);
    std::string line;
    int totalLineCount = 0;
    int extractedCount = 0;

    std::string currentReference;
    std::string currentDesignation;
    double currentQuantite = 0.0;
    double currentPrix = 0.0;
    bool inProduct = false;
    int stateInProduct = 0; // 0 = cherche pos, 1 = a trouvé pos, 2 = a trouvé GTIN, 3 = cherche total

    // Regex pour détecter une ligne de position (commence par un nombre suivi d'espaces)
    std::regex posLineRegex(R"(^(\d+)\s+(.+?)\s+(\d+)\s+BTE\s+[\d,]+\s+.*?([\d,]+)\s*$)");

    // Regex pour le code GTIN (13 chiffres)
    std::regex gtinRegex(R"(^(\d{13})\s+)");

    // Regex pour la ligne "Total éco-contribution comprise"
    std::regex totalRegex(R"(Total\s+[ée]co-contribution\s+comprise\s+.*?([\d,]+)\s*$)");

    while (std::getline(stream, line))
    {
        totalLineCount++;

        // Debug: afficher les lignes
        if (totalLineCount <= 100)
        {
            std::string debugLine = "Fischer L" + std::to_string(totalLineCount) + ": " + line + "\n";
            OutputDebugStringA(debugLine.c_str());
        }

        std::smatch match;

        // État 0 ou après avoir fini un produit: chercher une ligne de position
        if (stateInProduct == 0 || stateInProduct == 3)
        {
            if (std::regex_search(line, match, posLineRegex))
            {
                // Nouvelle ligne de produit trouvée
                if (stateInProduct == 3 && !currentReference.empty())
                {
                    // Sauvegarder le produit précédent
                    PdfLine pdfLine;
                    pdfLine.reference = currentReference;
                    pdfLine.designation = currentDesignation;
                    pdfLine.quantite = currentQuantite;
                    pdfLine.prixHT = currentPrix;
                    lines.push_back(pdfLine);
                    extractedCount++;

                    std::string debugMsg = "Fischer produit #" + std::to_string(extractedCount) + ": " +
                        currentReference + " | " + currentDesignation + " | " +
                        std::to_string(currentQuantite) + " | " + std::to_string(currentPrix) + "\n";
                    OutputDebugStringA(debugMsg.c_str());
                }

                // Commencer un nouveau produit
                currentReference = "";
                currentDesignation = match[2].str();
                currentQuantite = parseFrenchNumber(match[3].str());
                currentPrix = 0.0;
                stateInProduct = 1;

                std::string debugMsg = "[Fischer] Ligne Pos trouvée : " + currentDesignation +
                    " | Qté: " + std::to_string(currentQuantite) + "\n";
                OutputDebugStringA(debugMsg.c_str());
            }
        }
        // État 1: chercher le code GTIN sur la ligne suivante
        else if (stateInProduct == 1)
        {
            if (std::regex_search(line, match, gtinRegex))
            {
                currentReference = match[1].str();
                stateInProduct = 2;

                std::string debugMsg = "[Fischer] GTIN trouvé : " + currentReference + "\n";
                OutputDebugStringA(debugMsg.c_str());
            }
        }
        // État 2: chercher la ligne "Total éco-contribution comprise"
        else if (stateInProduct == 2)
        {
            if (std::regex_search(line, match, totalRegex))
            {
                currentPrix = parseFrenchNumber(match[1].str());
                stateInProduct = 3;

                std::string debugMsg = "[Fischer] Prix total trouvé : " + std::to_string(currentPrix) + "\n";
                OutputDebugStringA(debugMsg.c_str());
            }
        }
    }

    // Sauvegarder le dernier produit si disponible
    if (stateInProduct == 3 && !currentReference.empty())
    {
        PdfLine pdfLine;
        pdfLine.reference = currentReference;
        pdfLine.designation = currentDesignation;
        pdfLine.quantite = currentQuantite;
        pdfLine.prixHT = currentPrix;
        lines.push_back(pdfLine);
        extractedCount++;

        std::string debugMsg = "Fischer produit #" + std::to_string(extractedCount) + ": " +
            currentReference + " | " + currentDesignation + " | " +
            std::to_string(currentQuantite) + " | " + std::to_string(currentPrix) + "\n";
        OutputDebugStringA(debugMsg.c_str());
    }

    // Log final
    std::string finalMsg = "=== RESUME PARSING FISCHER ===\n";
    finalMsg += "Lignes totales texte : " + std::to_string(totalLineCount) + "\n";
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
