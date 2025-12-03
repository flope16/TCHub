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

    // Regex plus simples et robustes
    // Ligne Pos: commence par 1-3 chiffres
    std::regex posLineRegex(R"(^(\d{1,3})\s+)");

    // Regex pour extraire quantité + BTE
    std::regex qtyRegex(R"((\d+)\s+BTE)");

    // Regex pour le code GTIN (13 chiffres au début de ligne)
    std::regex gtinRegex(R"(^(\d{13}))");

    // Regex pour la ligne "Total éco-contribution comprise" - capture tous les nombres
    std::regex totalRegex(R"(Total\s+[ée]co-contribution\s+comprise)");

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
                // Vérifier que la ligne contient aussi BTE (pour confirmer que c'est une ligne produit)
                std::smatch qtyMatch;
                if (std::regex_search(line, qtyMatch, qtyRegex))
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

                    // Extraire la désignation: tout ce qui est entre le numéro de position et la quantité BTE
                    std::string posNum = match[1].str();
                    size_t posNumEnd = line.find(posNum) + posNum.length();
                    size_t qtyStart = line.find(qtyMatch[0].str());

                    if (qtyStart != std::string::npos && posNumEnd < qtyStart)
                    {
                        currentDesignation = line.substr(posNumEnd, qtyStart - posNumEnd);
                        // Supprimer les espaces en début et fin
                        currentDesignation.erase(0, currentDesignation.find_first_not_of(" \t"));
                        currentDesignation.erase(currentDesignation.find_last_not_of(" \t") + 1);
                    }
                    else
                    {
                        currentDesignation = "Produit " + posNum;
                    }

                    // Commencer un nouveau produit
                    currentReference = "";
                    currentQuantite = parseFrenchNumber(qtyMatch[1].str());
                    currentPrix = 0.0;
                    stateInProduct = 1;

                    std::string debugMsg = "[Fischer] Ligne Pos trouvée : " + currentDesignation +
                        " | Qté: " + std::to_string(currentQuantite) + "\n";
                    OutputDebugStringA(debugMsg.c_str());
                }
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
                // Extraire le dernier nombre de la ligne (le prix total)
                std::regex numberRegex(R"(([\d,]+))");
                std::vector<std::string> numbers;
                auto words_begin = std::sregex_iterator(line.begin(), line.end(), numberRegex);
                auto words_end = std::sregex_iterator();

                for (std::sregex_iterator i = words_begin; i != words_end; ++i)
                {
                    numbers.push_back((*i).str());
                }

                // Le dernier nombre est le prix total
                if (!numbers.empty())
                {
                    currentPrix = parseFrenchNumber(numbers.back());
                    stateInProduct = 3;

                    std::string debugMsg = "[Fischer] Prix total trouvé : " + std::to_string(currentPrix) + "\n";
                    OutputDebugStringA(debugMsg.c_str());
                }
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
