#include "RexelPdfParser.h"
#include "PopplerPdfExtractor.h"
#include <fstream>
#include <sstream>
#include <regex>
#include <algorithm>
#include <filesystem>
#include <windows.h>

// ===== FONCTIONS HELPERS =====

// Fonction helper pour normaliser un nombre français (virgule → point)
// Ex: "0,68000" → 0.68 ou "544,00" → 544.00
static double normalizeNumber(std::string str)
{
    // Enlever les espaces
    str.erase(std::remove_if(str.begin(), str.end(),
        [](unsigned char c) { return std::isspace(c); }),
        str.end());

    // Remplacer virgule par point
    std::replace(str.begin(), str.end(), ',', '.');

    try {
        return std::stod(str);
    }
    catch (...) {
        return 0.0;
    }
}

// Fonction helper pour trim les espaces
static std::string trim(const std::string& s)
{
    auto start = s.find_first_not_of(" \t\r\n");
    auto end = s.find_last_not_of(" \t\r\n");
    if (start == std::string::npos) return std::string();
    return s.substr(start, end - start + 1);
}

// ===== METHODES DE LA CLASSE =====

std::string RexelPdfParser::extractText(const std::string& filePath)
{
    // Utiliser PopplerPdfExtractor avec -layout pour préserver la structure
    std::string text = PopplerPdfExtractor::extractTextFromPdf(filePath, true);

    // Debug: sauvegarder le texte extrait
    try
    {
        std::filesystem::path pdfPathObj(filePath);
        std::filesystem::path debugPath = pdfPathObj.parent_path() / (pdfPathObj.stem().string() + "_rexel_extracted.txt");
        std::ofstream debugFile(debugPath);
        if (debugFile.is_open())
        {
            debugFile << text;
            debugFile.close();
            std::string debugMsg = "[Rexel] Texte extrait sauvegardé dans: " + debugPath.string() + "\n";
            OutputDebugStringA(debugMsg.c_str());
        }
    }
    catch (...) {}

    // Debug: afficher dans la console
    OutputDebugStringA("=== DEBUG EXTRACTION PDF REXEL ===\n");
    std::string debug = "Poppler disponible: " + std::string(PopplerPdfExtractor::isPopplerAvailable() ? "OUI" : "NON") + "\n";
    debug += "Texte extrait (" + std::to_string(text.length()) + " caractères):\n";
    size_t previewLength = text.length() < 1500 ? text.length() : 1500;
    debug += text.substr(0, previewLength) + "\n";
    debug += "=== FIN DEBUG REXEL ===\n";
    OutputDebugStringA(debug.c_str());

    return text;
}

std::vector<PdfLine> RexelPdfParser::parseTextContent(const std::string& text)
{
    std::vector<PdfLine> lines;

    OutputDebugStringA("=== DEBUT PARSING REXEL ===\n");

    // Vérifier si c'est un message d'erreur
    if (text.find("ERREUR:") == 0)
    {
        OutputDebugStringA("[Rexel] Texte contient ERREUR, abandon\n");
        return lines;
    }

    if (text.empty())
    {
        OutputDebugStringA("[Rexel] Texte vide, abandon\n");
        return lines;
    }

    OutputDebugStringA("[Rexel] Parsing ligne par ligne avec state machine...\n");

    // Séparer le texte en lignes
    std::istringstream stream(text);
    std::string line;
    std::vector<std::string> textLines;

    while (std::getline(stream, line))
    {
        textLines.push_back(line);
    }

    std::string lineCountMsg = "[Rexel] Nombre de lignes: " + std::to_string(textLines.size()) + "\n";
    OutputDebugStringA(lineCountMsg.c_str());

    int extractedCount = 0;

    // State machine pour parser les blocs Rexel (référence, désignation, quantité, prix)
    // Format observé :
    // Ligne N : "001    NDX402010                                ..."
    // Ligne N+1 : "       ECLISSE RAPIDTOL ER 48 SZ"
    // Ligne N+2 : "       800"
    // Ligne N+3 : "       8      P        00,68000"

    enum class State { SEARCHING, FOUND_REF, FOUND_DESC, FOUND_QTE };
    State state = State::SEARCHING;
    PdfLine currentProduct;

    // Regex simples pour identifier chaque type de ligne
    std::regex refRegex(R"(^\s*\d+\s+([A-Z0-9]{5,})\s*)"); // Ligne avec numéro + référence
    std::regex qteRegex(R"(^\s*(\d+)\s*$)"); // Ligne avec juste un nombre (quantité)
    std::regex prixRegex(R"(^\s*\d+\s+P\s+([\d,]+))"); // Ligne avec "P" et prix

    for (size_t i = 0; i < textLines.size(); ++i)
    {
        std::string currentLine = trim(textLines[i]);

        if (currentLine.empty())
            continue;

        std::smatch match;

        switch (state)
        {
        case State::SEARCHING:
            // Chercher une ligne avec référence
            if (std::regex_search(currentLine, match, refRegex))
            {
                currentProduct = PdfLine();
                currentProduct.reference = trim(match[1].str());

                std::string debugMsg = "[Rexel] Ref trouvee: " + currentProduct.reference + "\n";
                OutputDebugStringA(debugMsg.c_str());

                state = State::FOUND_REF;
            }
            break;

        case State::FOUND_REF:
            // La ligne suivante est la désignation (commence par des espaces, contient des lettres)
            if (currentLine.find_first_not_of(' ') != std::string::npos &&
                std::isalpha(static_cast<unsigned char>(currentLine[currentLine.find_first_not_of(' ')])))
            {
                currentProduct.designation = trim(currentLine);

                std::string debugMsg = "[Rexel] Desc trouvee: " + currentProduct.designation + "\n";
                OutputDebugStringA(debugMsg.c_str());

                state = State::FOUND_DESC;
            }
            else
            {
                // Pas de désignation, revenir à la recherche
                state = State::SEARCHING;
            }
            break;

        case State::FOUND_DESC:
            // Chercher la quantité (ligne avec juste un nombre)
            if (std::regex_match(currentLine, match, qteRegex))
            {
                currentProduct.quantite = normalizeNumber(match[1].str());

                std::string debugMsg = "[Rexel] Qte trouvee: " + std::to_string(currentProduct.quantite) + "\n";
                OutputDebugStringA(debugMsg.c_str());

                state = State::FOUND_QTE;
            }
            else
            {
                // Pas de quantité, revenir à la recherche
                state = State::SEARCHING;
            }
            break;

        case State::FOUND_QTE:
            // Chercher le prix (ligne avec "P" et un nombre)
            if (std::regex_search(currentLine, match, prixRegex))
            {
                currentProduct.prixHT = normalizeNumber(match[1].str());

                std::string debugMsg = "[Rexel] Prix trouve: " + std::to_string(currentProduct.prixHT) + "\n";
                OutputDebugStringA(debugMsg.c_str());

                // Produit complet, l'ajouter
                lines.push_back(currentProduct);
                extractedCount++;

                std::string productMsg = "Rexel produit #" + std::to_string(extractedCount) + ": " +
                    currentProduct.reference + " | " + currentProduct.designation + " | " +
                    std::to_string(currentProduct.quantite) + " | " + std::to_string(currentProduct.prixHT) + "\n";
                OutputDebugStringA(productMsg.c_str());

                state = State::SEARCHING;
            }
            else
            {
                // Pas de prix trouvé, continuer à chercher sur la ligne suivante
                // (le prix peut être sur la ligne suivante)
            }
            break;
        }
    }

    // Log final
    std::string finalMsg = "=== RESUME PARSING REXEL ===\n";
    finalMsg += "Produits extraits : " + std::to_string(extractedCount) + "\n";
    finalMsg += "==============================\n";
    OutputDebugStringA(finalMsg.c_str());

    return lines;
}

std::vector<PdfLine> RexelPdfParser::parse(const std::string& filePath)
{
    std::string text = extractText(filePath);
    return parseTextContent(text);
}
