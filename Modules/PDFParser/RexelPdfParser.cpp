#include "RexelPdfParser.h"
#include "PopplerPdfExtractor.h"
#include <fstream>
#include <sstream>
#include <regex>
#include <algorithm>
#include <cctype>
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
    // Utiliser PopplerPdfExtractor AVEC -layout pour préserver la structure en colonnes
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

// Fonction helper pour parser un nombre français avec virgule
static double parseFrenchNumber(const std::string& s)
{
    std::string cleaned;
    cleaned.reserve(s.size());

    for (char c : s)
    {
        if (c == ' ' || c == '\t')
            continue; // Ignorer les espaces
        if (c == ',')
            cleaned.push_back('.'); // Remplacer virgule par point
        else if ((c >= '0' && c <= '9') || c == '.')
            cleaned.push_back(c);
    }

    if (cleaned.empty())
        return 0.0;

    try {
        return std::stod(cleaned);
    }
    catch (...) {
        return 0.0;
    }
}

std::vector<PdfLine> RexelPdfParser::parseTextContent(const std::string& text)
{
    std::vector<PdfLine> lines;

    OutputDebugStringA("=== DEBUT PARSING REXEL (format avec -layout) ===\n");

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

    // LOG: afficher les 100 premières lignes pour debug
    OutputDebugStringA("[Rexel] === AFFICHAGE DES 100 PREMIERES LIGNES (avec -layout) ===\n");
    for (size_t i = 0; i < textLines.size() && i < 100; ++i)
    {
        std::string logLine = "[Rexel] Ligne " + std::to_string(i) + ": \"" + textLines[i] + "\"\n";
        OutputDebugStringA(logLine.c_str());
    }
    OutputDebugStringA("[Rexel] === FIN AFFICHAGE ===\n");

    int extractedCount = 0;

    // Nouveau parsing : on traite bloc par bloc entre deux références "NDX"
    std::regex refPattern(R"(NDX\d+)");

    for (size_t i = 0; i < textLines.size(); ++i)
    {
        std::smatch refMatch;
        if (!std::regex_search(textLines[i], refMatch, refPattern))
            continue;

        PdfLine product;
        product.reference = trim(refMatch.str());

        // Déterminer la fin du bloc (prochaine référence ou fin du fichier)
        size_t blockEnd = i + 1;
        for (; blockEnd < textLines.size(); ++blockEnd)
        {
            if (std::regex_search(textLines[blockEnd], refMatch, refPattern))
                break;
        }

        // Agréger les lignes du bloc pour faciliter le parsing
        std::string blockText;
        for (size_t j = i; j < blockEnd; ++j)
        {
            if (!blockText.empty()) blockText += ' ';
            blockText += trim(textLines[j]);
        }

        std::string debugMsg = "[Rexel] ✓ MATCH REF à ligne " + std::to_string(i) + ": \"" + product.reference + "\" | Bloc=" + blockText.substr(0, 150) + "...\n";
        OutputDebugStringA(debugMsg.c_str());

        // Trouver la désignation : première ligne texte non vide du bloc qui contient des lettres
        for (size_t j = i; j < blockEnd; ++j)
        {
            std::string candidate = trim(textLines[j]);
            bool hasAlpha = std::any_of(candidate.begin(), candidate.end(), [](unsigned char c) { return std::isalpha(c); });
            if (hasAlpha && candidate.find("NDX") == std::string::npos)
            {
                product.designation = candidate;
                std::string debugDesc = "[Rexel] Desc trouvée (ligne " + std::to_string(j) + "): \"" + product.designation + "\"\n";
                OutputDebugStringA(debugDesc.c_str());
                break;
            }
        }

        // Collapser le bloc (suppression des espaces) pour reconstruire quantité/prix éclatés
        std::string collapsed;
        collapsed.reserve(blockText.size());
        for (char c : blockText)
        {
            if (!std::isspace(static_cast<unsigned char>(c)))
                collapsed.push_back(c);
        }

        // Chercher quantité juste après la référence
        size_t refPos = collapsed.find(product.reference);
        std::string collapsedAfterRef = (refPos != std::string::npos) ? collapsed.substr(refPos + product.reference.length()) : collapsed;

        std::regex qtyRegex(R"((\d+)P)");
        std::smatch qtyMatch;
        if (std::regex_search(collapsedAfterRef, qtyMatch, qtyRegex))
        {
            try
            {
                product.quantite = std::stod(qtyMatch[1].str());
                std::string debugQte = "[Rexel] Qte extraite: " + qtyMatch[1].str() + "\n";
                OutputDebugStringA(debugQte.c_str());
            }
            catch (...)
            {
                OutputDebugStringA("[Rexel] Erreur conversion qte\n");
            }

            // Rechercher le prix APRÈS la quantité
            std::string afterQty = qtyMatch.suffix().str();
            std::regex priceRegex(R"((\d+,\d{2,5}))");
            std::smatch priceMatch;
            if (std::regex_search(afterQty, priceMatch, priceRegex))
            {
                product.prixHT = parseFrenchNumber(priceMatch[1].str());
                std::string debugPrix = "[Rexel] Prix extrait: " + priceMatch[1].str() + " => " + std::to_string(product.prixHT) + "\n";
                OutputDebugStringA(debugPrix.c_str());
            }
            else
            {
                OutputDebugStringA("[Rexel] Prix non trouvé\n");
            }
        }
        else
        {
            OutputDebugStringA("[Rexel] Quantité non trouvée\n");
        }

        if (!product.reference.empty() && product.quantite > 0)
        {
            extractedCount++;
            lines.push_back(product);

            std::string productMsg = "[Rexel] ✓✓✓ Produit #" + std::to_string(extractedCount) + " AJOUTE: " +
                product.reference + " | Qte=" + std::to_string(product.quantite) +
                " | PU=" + std::to_string(product.prixHT) +
                " | Desc=\"" + product.designation + "\"\n";
            OutputDebugStringA(productMsg.c_str());
        }
        else
        {
            std::string debugFail = "[Rexel] ✗ Produit NON ajouté (ref=\"" + product.reference +
                "\", qte=" + std::to_string(product.quantite) + ")\n";
            OutputDebugStringA(debugFail.c_str());
        }

        // Continuer après le bloc déjà traité
        i = (blockEnd == 0) ? i : blockEnd - 1;
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
