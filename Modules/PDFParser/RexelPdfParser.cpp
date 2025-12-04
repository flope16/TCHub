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

// Fonction helper pour détecter une ligne de désignation Rexel
static bool isDesignationRexel(const std::string& line)
{
    std::string s = trim(line);

    // Doit être non vide et commencer par une lettre
    if (s.empty() || !std::isalpha(static_cast<unsigned char>(s[0])))
        return false;

    // Rejeter les lignes d'en-tête/pied de page
    if (s.find("Tél.") != std::string::npos ||
        s.find("Fax.") != std::string::npos ||
        s.find("TCI :") != std::string::npos ||
        s.find("REXEL") != std::string::npos)
        return false;

    return true;
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

    OutputDebugStringA("[Rexel] Parsing ligne par ligne avec nouvelles regex...\n");

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

    // Regex précises pour le format Rexel
    // Format observé dans le PDF :
    // 001    NDX402010
    // 0,68000           <- prix unitaire
    // 800    P          <- quantité
    // 544,00   2        <- total HT
    // ECLISSE RAPIDTOL ER 48 SZ  <- désignation

    std::regex refRegex(R"(^\s*(\d{3})\s+([A-Z0-9]{5,})\s*$)");        // 3 chiffres + référence
    std::regex puRegex(R"(^\s*(\d+,\d{5})\s*$)");                      // Prix unitaire (5 décimales)
    std::regex qteRegex(R"(^\s*(\d+)\s+P\s*$)");                       // Quantité + P
    std::regex totalRegex(R"(^\s*([\d\s]+,\d{2})\s+2\s*$)");          // Total + code TVA "2"

    int extractedCount = 0;

    // Variables pour le produit en cours
    std::string currentRef;
    std::string currentDesc;
    std::string puStr, qteStr, totalStr;
    bool inArticle = false;

    for (size_t i = 0; i < textLines.size(); ++i)
    {
        std::string currentLine = textLines[i];
        std::smatch match;

        // 1) Détection d'une nouvelle référence
        if (std::regex_match(currentLine, match, refRegex))
        {
            // Si on avait déjà un article en cours et complet, on le sauvegarde
            if (inArticle && !currentRef.empty() && !qteStr.empty() && !puStr.empty() && !totalStr.empty())
            {
                PdfLine product;
                product.reference = currentRef;
                product.designation = trim(currentDesc);
                product.quantite = normalizeNumber(qteStr);
                product.prixHT = normalizeNumber(puStr);
                // Note: totalStr est disponible si besoin pour validation

                lines.push_back(product);
                extractedCount++;

                std::string productMsg = "[Rexel] Produit #" + std::to_string(extractedCount) + ": " +
                    product.reference + " | Qte=" + std::to_string(product.quantite) +
                    " | PU=" + std::to_string(product.prixHT) +
                    " | Desc=\"" + product.designation + "\"\n";
                OutputDebugStringA(productMsg.c_str());
            }

            // Démarrer un nouvel article
            inArticle = true;
            currentRef = match[2].str();  // Groupe 2 = référence
            currentDesc.clear();
            qteStr.clear();
            puStr.clear();
            totalStr.clear();

            std::string debugMsg = "[Rexel] Nouvelle ref: " + currentRef + "\n";
            OutputDebugStringA(debugMsg.c_str());
            continue;
        }

        if (!inArticle)
            continue; // Ignorer tout avant la première référence

        // 2) Prix unitaire
        if (puStr.empty() && std::regex_match(currentLine, match, puRegex))
        {
            puStr = match[1].str();
            std::string debugMsg = "[Rexel] PU trouve: " + puStr + "\n";
            OutputDebugStringA(debugMsg.c_str());
            continue;
        }

        // 3) Quantité
        if (qteStr.empty() && std::regex_match(currentLine, match, qteRegex))
        {
            qteStr = match[1].str();
            std::string debugMsg = "[Rexel] Qte trouvee: " + qteStr + "\n";
            OutputDebugStringA(debugMsg.c_str());
            continue;
        }

        // 4) Montant total
        if (totalStr.empty() && std::regex_match(currentLine, match, totalRegex))
        {
            totalStr = match[1].str();
            std::string debugMsg = "[Rexel] Total trouve: " + totalStr + "\n";
            OutputDebugStringA(debugMsg.c_str());
            continue;
        }

        // 5) Désignation (après qu'on ait PU, Qte, Total)
        if (!puStr.empty() && !qteStr.empty() && !totalStr.empty() && isDesignationRexel(currentLine))
        {
            if (!currentDesc.empty())
                currentDesc += " ";
            currentDesc += trim(currentLine);

            std::string debugMsg = "[Rexel] Desc ajoutee: " + trim(currentLine) + "\n";
            OutputDebugStringA(debugMsg.c_str());
            continue;
        }
    }

    // Fin de fichier : sauvegarder le dernier article s'il est complet
    if (inArticle && !currentRef.empty() && !qteStr.empty() && !puStr.empty() && !totalStr.empty())
    {
        PdfLine product;
        product.reference = currentRef;
        product.designation = trim(currentDesc);
        product.quantite = normalizeNumber(qteStr);
        product.prixHT = normalizeNumber(puStr);

        lines.push_back(product);
        extractedCount++;

        std::string productMsg = "[Rexel] Produit #" + std::to_string(extractedCount) + ": " +
            product.reference + " | Qte=" + std::to_string(product.quantite) +
            " | PU=" + std::to_string(product.prixHT) +
            " | Desc=\"" + product.designation + "\"\n";
        OutputDebugStringA(productMsg.c_str());
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
