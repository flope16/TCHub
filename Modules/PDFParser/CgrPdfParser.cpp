#include "CgrPdfParser.h"
#include "PopplerPdfExtractor.h"
#include <fstream>
#include <sstream>
#include <regex>
#include <algorithm>
#include <filesystem>
#include <windows.h>

std::string CgrPdfParser::extractText(const std::string& filePath)
{
    // Utiliser PopplerPdfExtractor SANS l'option -layout car elle cause des espaces intercalés pour CGR
    std::string text = PopplerPdfExtractor::extractTextFromPdf(filePath, false);

    // Debug: sauvegarder le texte extrait
    try
    {
        std::filesystem::path pdfPathObj(filePath);
        std::filesystem::path debugPath = pdfPathObj.parent_path() / (pdfPathObj.stem().string() + "_cgr_extracted.txt");
        std::ofstream debugFile(debugPath);
        if (debugFile.is_open())
        {
            debugFile << text;
            debugFile.close();
            std::string debugMsg = "[CGR] Texte extrait sauvegardé dans: " + debugPath.string() + "\n";
            OutputDebugStringA(debugMsg.c_str());
        }
    }
    catch (...) {}

    // Debug: afficher dans la console
    OutputDebugStringA("=== DEBUG EXTRACTION PDF CGR ===\n");
    std::string debug = "Poppler disponible: " + std::string(PopplerPdfExtractor::isPopplerAvailable() ? "OUI" : "NON") + "\n";
    debug += "Texte extrait (" + std::to_string(text.length()) + " caractères):\n";
    size_t previewLength = text.length() < 1500 ? text.length() : 1500;
    debug += text.substr(0, previewLength) + "\n";
    debug += "=== FIN DEBUG CGR ===\n";
    OutputDebugStringA(debug.c_str());

    return text;
}

// Fonction helper pour normaliser un nombre français avec espaces intercalés
// Ex: "1 9 ,0 2" → 19.02
//     "7 4 1 7 ,8 0" → 7417.80
static double normalizeNumber(std::string str)
{
    // Enlever TOUS les espaces (y compris tabs, espaces insécables U+00A0)
    str.erase(std::remove_if(str.begin(), str.end(),
        [](unsigned char c) { return std::isspace(c) || c == '\xA0'; }),
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

// Fonction pour nettoyer une référence en enlevant les espaces intercalés
// Ex: "R SA U 5 0" → "RSAU50"
static std::string cleanReference(std::string str)
{
    str.erase(std::remove_if(str.begin(), str.end(),
        [](unsigned char c) { return std::isspace(c); }),
        str.end());
    return str;
}

std::vector<PdfLine> CgrPdfParser::parseTextContent(const std::string& text)
{
    std::vector<PdfLine> lines;

    OutputDebugStringA("=== DEBUT PARSING CGR ===\n");

    // Vérifier si c'est un message d'erreur
    if (text.find("ERREUR:") == 0)
    {
        OutputDebugStringA("[CGR] Texte contient ERREUR, abandon\n");
        return lines;
    }

    if (text.empty())
    {
        OutputDebugStringA("[CGR] Texte vide, abandon\n");
        return lines;
    }

    // Parser ligne par ligne adapté à la structure CGR avec espaces intercalés
    // Format observé avec espaces entre caractères :
    //   "1   R SA U 5 0   R A C C OR D R S AU 5 0 X 4      390    1 9 ,0 2 €   7 4 1 7 ,8 0 €"
    // Colonnes : Numéro | Référence | Désignation | Quantité | Prix unitaire | Total

    int extractedCount = 0;

    try
    {
        OutputDebugStringA("[CGR] Creation du regex ligne article (avec espaces intercales)...\n");

        // Regex simplifié pour CGR avec chiffres espacés (évite error_complexity)
        // (\d(?:\s?\d)*\s*,\s*\d{2}) = nombre avec chiffres possiblement espacés
        // Ex: "1 9 ,0 2" ou "19,02" ou "7 4 1 7 ,8 0"
        std::regex lineRegex(
            R"(^\s*\d+\s+(.+?)\s{2,}(.+?)\s+(\d+)\s+(\d(?:\s?\d)*\s*,\s*\d{2})\s+(\d(?:\s?\d)*\s*,\s*\d{2}))",
            std::regex::ECMAScript
        );

        OutputDebugStringA("[CGR] Regex cree, debut parsing ligne par ligne...\n");

        // Séparer le texte en lignes
        std::istringstream stream(text);
        std::string line;
        std::vector<std::string> textLines;

        while (std::getline(stream, line))
        {
            textLines.push_back(line);
        }

        std::string lineCountMsg = "[CGR] Nombre de lignes: " + std::to_string(textLines.size()) + "\n";
        OutputDebugStringA(lineCountMsg.c_str());

        // Parser chaque ligne
        for (size_t i = 0; i < textLines.size(); ++i)
        {
            std::string currentLine = trim(textLines[i]);

            if (currentLine.empty())
                continue;

            // Filtrage pré-regex pour éviter error_complexity sur les pavés de texte
            // Une ligne article CGR doit contenir une virgule ET un symbole € (ou chiffres typiques)
            if (currentLine.find(',') == std::string::npos)
                continue;

            std::smatch match;

            try
            {
                if (!std::regex_match(currentLine, match, lineRegex))
                    continue; // Pas une ligne article
            }
            catch (const std::regex_error& e)
            {
                std::string errorMsg = "[CGR] ERREUR REGEX runtime sur ligne: " + std::string(e.what()) + "\n";
                OutputDebugStringA(errorMsg.c_str());
                continue;
            }

            PdfLine product;

            // Référence (groupe 1) - nettoyer les espaces intercalés
            std::string refRaw = trim(match[1].str());
            product.reference = cleanReference(refRaw);  // "R SA U 5 0" → "RSAU50"

            // Désignation (groupe 2) - peut avoir des espaces intercalés aussi, mais on garde tel quel
            product.designation = trim(match[2].str());
            // Optionnel : nettoyer aussi la désignation
            // product.designation = cleanReference(product.designation);

            // Quantité (groupe 3)
            std::string qteStr = match[3].str();
            product.quantite = normalizeNumber(qteStr);

            // Prix unitaire (groupe 4) - avec espaces intercalés : "1 9 ,0 2"
            std::string prixStr = match[4].str();
            product.prixHT = normalizeNumber(prixStr);

            // Total (groupe 5) - avec espaces intercalés : "7 4 1 7 ,8 0"
            // std::string totalStr = match[5].str();
            // double total = normalizeNumber(totalStr);

            std::string debugMsg = "[CGR] Ligne article trouvee: Ref=" + product.reference +
                " | Desc=" + product.designation +
                " | Qte=" + std::to_string(product.quantite) +
                " | Prix=" + std::to_string(product.prixHT) + "\n";
            OutputDebugStringA(debugMsg.c_str());

            // Ajouter le produit à la liste
            lines.push_back(product);
            extractedCount++;

            std::string productMsg = "CGR produit #" + std::to_string(extractedCount) + ": " +
                product.reference + " | " + product.designation + " | " +
                std::to_string(product.quantite) + " | " + std::to_string(product.prixHT) + "\n";
            OutputDebugStringA(productMsg.c_str());
        }
    }
    catch (const std::regex_error& e)
    {
        std::string errorMsg = "[CGR] ERREUR REGEX: " + std::string(e.what()) + "\n";
        OutputDebugStringA(errorMsg.c_str());
    }
    catch (const std::exception& e)
    {
        std::string errorMsg = "[CGR] EXCEPTION: " + std::string(e.what()) + "\n";
        OutputDebugStringA(errorMsg.c_str());
    }
    catch (...)
    {
        OutputDebugStringA("[CGR] EXCEPTION INCONNUE\n");
    }

    // Log final
    std::string finalMsg = "=== RESUME PARSING CGR ===\n";
    finalMsg += "Produits extraits : " + std::to_string(extractedCount) + "\n";
    finalMsg += "==============================\n";
    OutputDebugStringA(finalMsg.c_str());

    return lines;
}

std::vector<PdfLine> CgrPdfParser::parse(const std::string& filePath)
{
    std::string text = extractText(filePath);
    return parseTextContent(text);
}
