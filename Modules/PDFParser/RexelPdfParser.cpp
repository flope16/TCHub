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
    // Utiliser PopplerPdfExtractor SANS -layout pour obtenir le format éclaté ligne par ligne
    std::string text = PopplerPdfExtractor::extractTextFromPdf(filePath, false);

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

    OutputDebugStringA("=== DEBUT PARSING REXEL (format éclaté) ===\n");

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

    // Format Rexel sans layout - structure fixe par produit :
    // L0  : 001    NDX402010          (numéro + référence)
    // L1  : 00086, 0                  (prix de base, ignoré)
    // L2  : 0,68000                   (prix unitaire net)
    // L3  : 008                       (remise, ignoré)
    // L4  : 800    P                  (quantité)
    // L5  : 00, 445                   (ignoré)
    // L6  : 544,00   2                (montant HT + code TVA)
    // L7  : 6                         (ignoré)
    // L8  : 636     1   j             (dispo, ignoré)
    // L9  : ECLISSE RAPIDTOL ER 48 SZ (désignation)
    // L10 : 1                         (ignoré)
    // L11 : 164     5   j             (délai, ignoré)
    // L12 : 002    NDX421955          (produit suivant)

    std::regex refRegex(R"(^\s*(\d{3})\s+(\S+)\s*$)"); // Détecte "001    NDX402010"
    std::regex qteRegex(R"(^\s*(\d+)\s+P\b)");          // Détecte "800    P"

    int extractedCount = 0;

    for (size_t i = 0; i < textLines.size(); ++i)
    {
        std::smatch match;
        if (!std::regex_match(textLines[i], match, refRegex))
            continue; // Pas un début de produit

        PdfLine product;
        product.reference = match[2].str();

        std::string debugMsg = "[Rexel] Ref trouvée à ligne " + std::to_string(i) + ": " + product.reference + "\n";
        OutputDebugStringA(debugMsg.c_str());

        // Prix unitaire net : ligne i+2
        if (i + 2 < textLines.size())
        {
            std::string prixStr = trim(textLines[i + 2]);
            product.prixHT = parseFrenchNumber(prixStr);

            std::string debugPrix = "[Rexel] PU (ligne " + std::to_string(i + 2) + "): " + prixStr + " = " + std::to_string(product.prixHT) + "\n";
            OutputDebugStringA(debugPrix.c_str());
        }

        // Quantité : ligne i+4 (format "800    P")
        if (i + 4 < textLines.size())
        {
            std::smatch matchQte;
            if (std::regex_search(textLines[i + 4], matchQte, qteRegex))
            {
                product.quantite = std::stod(matchQte[1].str());

                std::string debugQte = "[Rexel] Qte (ligne " + std::to_string(i + 4) + "): " + matchQte[1].str() + "\n";
                OutputDebugStringA(debugQte.c_str());
            }
        }

        // Montant HT : ligne i+6 (format "544,00   2")
        // On ne l'utilise pas pour le produit mais on peut le logger pour validation
        if (i + 6 < textLines.size())
        {
            std::string totalStr = trim(textLines[i + 6]);
            std::string debugTotal = "[Rexel] Total HT (ligne " + std::to_string(i + 6) + "): " + totalStr + "\n";
            OutputDebugStringA(debugTotal.c_str());
        }

        // Désignation : ligne i+9
        if (i + 9 < textLines.size())
        {
            product.designation = trim(textLines[i + 9]);

            std::string debugDesc = "[Rexel] Desc (ligne " + std::to_string(i + 9) + "): " + product.designation + "\n";
            OutputDebugStringA(debugDesc.c_str());
        }

        // Ajouter le produit si on a au moins une référence et une quantité
        if (!product.reference.empty() && product.quantite > 0)
        {
            lines.push_back(product);
            extractedCount++;

            std::string productMsg = "[Rexel] Produit #" + std::to_string(extractedCount) + ": " +
                product.reference + " | Qte=" + std::to_string(product.quantite) +
                " | PU=" + std::to_string(product.prixHT) +
                " | Desc=\"" + product.designation + "\"\n";
            OutputDebugStringA(productMsg.c_str());
        }

        // Avancer de 11 lignes (i sera incrémenté à la boucle suivante)
        // Structure : 12 lignes par produit (0 à 11), donc on avance à i+11
        i += 11;
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
