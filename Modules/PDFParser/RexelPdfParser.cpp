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

    // Regex Rexel pour extraire : Référence + Désignation + Quantité + Prix unitaire + Total
    // Groupe 1: Référence (ex: NDX402010)
    // Groupe 2: Désignation (ex: ECLISSE RAPIDTOL ER 48 SZ)
    // Groupe 3: Quantité (ex: 800)
    // Groupe 4: Prix unitaire (ex: 0,68000)
    // Groupe 5: Montant total (ex: 544,00)
    std::regex rexelRegex(
        R"(([A-Z0-9]{5,})\s*\r?\n([A-Z0-9 \-/]+?)\s+(?:\S+\s+){2,}?(\d+)\s+\S+\s+([\d.,]+)\s+\S+\s+([\d.,]+))",
        std::regex::ECMAScript
    );

    OutputDebugStringA("[Rexel] Regex cree, debut parsing...\n");

    int extractedCount = 0;

    try
    {
        // Chercher tous les matches dans le texte
        auto words_begin = std::sregex_iterator(text.begin(), text.end(), rexelRegex);
        auto words_end = std::sregex_iterator();

        int matchCount = std::distance(words_begin, words_end);
        std::string matchMsg = "[Rexel] Nombre de matches trouves: " + std::to_string(matchCount) + "\n";
        OutputDebugStringA(matchMsg.c_str());

        for (std::sregex_iterator i = words_begin; i != words_end; ++i)
        {
            std::smatch match = *i;

            PdfLine product;

            // Référence (groupe 1)
            product.reference = trim(match[1].str());

            // Désignation (groupe 2)
            product.designation = trim(match[2].str());

            // Quantité (groupe 3)
            std::string qteStr = match[3].str();
            product.quantite = normalizeNumber(qteStr);

            // Prix unitaire (groupe 4)
            std::string prixStr = match[4].str();
            product.prixHT = normalizeNumber(prixStr);

            // Total (groupe 5) - non utilisé pour l'instant mais disponible
            // std::string totalStr = match[5].str();
            // double total = normalizeNumber(totalStr);

            std::string debugMsg = "[Rexel] Article trouve: Ref=" + product.reference +
                " | Desc=" + product.designation +
                " | Qte=" + std::to_string(product.quantite) +
                " | Prix=" + std::to_string(product.prixHT) + "\n";
            OutputDebugStringA(debugMsg.c_str());

            // Ajouter le produit à la liste
            lines.push_back(product);
            extractedCount++;
        }
    }
    catch (const std::regex_error& e)
    {
        std::string errorMsg = "[Rexel] ERREUR REGEX: " + std::string(e.what()) + "\n";
        OutputDebugStringA(errorMsg.c_str());
    }
    catch (const std::exception& e)
    {
        std::string errorMsg = "[Rexel] EXCEPTION: " + std::string(e.what()) + "\n";
        OutputDebugStringA(errorMsg.c_str());
    }
    catch (...)
    {
        OutputDebugStringA("[Rexel] EXCEPTION INCONNUE\n");
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
