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
    // Utiliser PopplerPdfExtractor
    std::string text = PopplerPdfExtractor::extractTextFromPdf(filePath);

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

// Fonction helper pour normaliser un nombre français (virgule, espaces des milliers)
static double normalizeNumber(std::string str)
{
    // Enlever les espaces (y compris espaces insécables U+00A0)
    str.erase(std::remove_if(str.begin(), str.end(),
        [](unsigned char c) { return c == ' ' || c == '\xA0'; }),
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

// Fonction pour détecter et corriger les espaces entre caractères
static std::string fixInterleavedSpaces(const std::string& text)
{
    if (text.empty()) return text;

    // Compter le ratio d'espaces pour détecter le problème
    size_t spaceCount = std::count(text.begin(), text.end(), ' ');
    double spaceRatio = static_cast<double>(spaceCount) / text.length();

    // Si le ratio d'espaces est supérieur à 30%, on considère qu'il y a des espaces intercalés
    if (spaceRatio < 0.3)
    {
        OutputDebugStringA("[CGR] Pas d'espaces intercales detectes\n");
        return text; // Pas de problème détecté
    }

    std::string debugMsg = "[CGR] Espaces intercales detectes (ratio: " +
        std::to_string(static_cast<int>(spaceRatio * 100)) + "%)\n";
    OutputDebugStringA(debugMsg.c_str());

    // Stratégie : remplacer les espaces multiples par un marqueur, supprimer les espaces simples,
    // puis restaurer les espaces entre mots
    std::string result = text;

    // Marqueur temporaire unique
    const std::string MARKER = "\x01\x02\x03";

    // 1. Remplacer les espaces multiples (2 ou plus) par le marqueur
    size_t pos = 0;
    while ((pos = result.find("  ", pos)) != std::string::npos)
    {
        // Compter le nombre d'espaces consécutifs
        size_t endPos = pos;
        while (endPos < result.length() && result[endPos] == ' ')
            endPos++;

        // Remplacer par le marqueur
        result.replace(pos, endPos - pos, MARKER);
        pos += MARKER.length();
    }

    // 2. Supprimer tous les espaces simples restants
    result.erase(std::remove(result.begin(), result.end(), ' '), result.end());

    // 3. Remplacer le marqueur par un espace unique
    pos = 0;
    while ((pos = result.find(MARKER, pos)) != std::string::npos)
    {
        result.replace(pos, MARKER.length(), " ");
        pos += 1;
    }

    OutputDebugStringA("[CGR] Espaces intercales corriges\n");

    return result;
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

    // Corriger les espaces intercalés si nécessaire
    std::string cleanedText = fixInterleavedSpaces(text);

    // Debug: sauvegarder le texte nettoyé si différent
    if (cleanedText != text)
    {
        OutputDebugStringA("[CGR] Texte nettoye different de l'original\n");
        std::string debugPreview = cleanedText.length() > 500 ? cleanedText.substr(0, 500) + "..." : cleanedText;
        std::string debugMsg = "[CGR] Preview texte nettoye: " + debugPreview + "\n";
        OutputDebugStringA(debugMsg.c_str());
    }

    // Parser ligne par ligne adapté à la structure CGR
    // Format observé (très propre) :
    //   "1    RSAU 50        RACCORD RSAU 50X4                    390    19,02 €    7 417,80 €   0764"
    // Colonnes : Numéro | Référence | Désignation | Quantité | Prix unitaire | Total | Code catalogue

    int extractedCount = 0;

    try
    {
        OutputDebugStringA("[CGR] Creation du regex ligne article...\n");

        // Regex pour ligne article CGR
        // Groupe 1 : Référence (peut contenir espaces, ex: "RSAU 50")
        // Groupe 2 : Désignation
        // Groupe 3 : Quantité (entier)
        // Groupe 4 : Prix unitaire (format XX,XX)
        // Groupe 5 : Total ligne (format X XXX,XX avec espaces des milliers)
        std::regex lineRegex(
            R"(^\s*\d+\s+(.+?)\s{2,}(.+?)\s+(\d+)\s+(\d+,\d{2})\s*€?\s+([\d\s]+,\d{2})\s*€?.*$)",
            std::regex::ECMAScript
        );

        OutputDebugStringA("[CGR] Regex cree, debut parsing ligne par ligne...\n");

        // Séparer le texte en lignes (utiliser le texte nettoyé)
        std::istringstream stream(cleanedText);
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

            std::smatch match;

            if (!std::regex_match(currentLine, match, lineRegex))
                continue; // Pas une ligne article

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

            // Total dans groupe 5 (non utilisé pour l'instant)
            // std::string totalStr = match[5].str();

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
