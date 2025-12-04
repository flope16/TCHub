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

// Fonction pour détecter et corriger les espaces entre caractères (CGR uniquement)
static std::string fixInterleavedSpaces(const std::string& text)
{
    if (text.empty()) return text;

    // Analyser un échantillon pour détecter le pattern "X Y Z" (espaces entre chaque caractère)
    // On prend les 500 premiers caractères pour l'analyse
    size_t sampleSize = std::min<size_t>(500, text.length());
    std::string sample = text.substr(0, sampleSize);

    // Compter le pattern "caractère + espace + caractère" (pas newline)
    int charSpaceCharCount = 0;
    for (size_t i = 0; i + 2 < sample.length(); i++)
    {
        if (sample[i] != ' ' && sample[i] != '\n' && sample[i] != '\r' &&
            sample[i + 1] == ' ' &&
            sample[i + 2] != ' ' && sample[i + 2] != '\n' && sample[i + 2] != '\r')
        {
            charSpaceCharCount++;
        }
    }

    double charSpaceCharRatio = static_cast<double>(charSpaceCharCount) / sampleSize;

    std::string debugMsg = "[CGR] Detection espaces intercales - Ratio pattern 'X Y': " +
        std::to_string(static_cast<int>(charSpaceCharRatio * 100)) + "%\n";
    OutputDebugStringA(debugMsg.c_str());

    // Si moins de 20% de patterns "X Y", pas de problème
    if (charSpaceCharRatio < 0.2)
    {
        OutputDebugStringA("[CGR] Pas d'espaces intercales detectes\n");
        return text;
    }

    OutputDebugStringA("[CGR] Espaces intercales DETECTES, correction en cours...\n");

    // Stratégie améliorée :
    // - Conserver les newlines et tabs
    // - Remplacer 3+ espaces par un marqueur (séparateurs de colonnes)
    // - Remplacer 2 espaces par un autre marqueur (séparateurs de mots)
    // - Supprimer tous les espaces simples (entre caractères)
    // - Restaurer les marqueurs

    std::string result = text;

    const std::string MARKER_MULTI = "\x01\x02\x03";  // Pour 3+ espaces
    const std::string MARKER_DOUBLE = "\x04\x05\x06"; // Pour 2 espaces

    // 1. Remplacer les séquences de 3+ espaces
    size_t pos = 0;
    while ((pos = result.find("   ", pos)) != std::string::npos)
    {
        size_t endPos = pos;
        while (endPos < result.length() && result[endPos] == ' ')
            endPos++;

        result.replace(pos, endPos - pos, MARKER_MULTI);
        pos += MARKER_MULTI.length();
    }

    // 2. Remplacer les double espaces
    pos = 0;
    while ((pos = result.find("  ", pos)) != std::string::npos)
    {
        result.replace(pos, 2, MARKER_DOUBLE);
        pos += MARKER_DOUBLE.length();
    }

    // 3. Supprimer tous les espaces simples
    result.erase(std::remove(result.begin(), result.end(), ' '), result.end());

    // 4. Restaurer les marqueurs par des espaces
    pos = 0;
    while ((pos = result.find(MARKER_MULTI, pos)) != std::string::npos)
    {
        result.replace(pos, MARKER_MULTI.length(), "  ");  // 2 espaces pour colonnes
        pos += 2;
    }

    pos = 0;
    while ((pos = result.find(MARKER_DOUBLE, pos)) != std::string::npos)
    {
        result.replace(pos, MARKER_DOUBLE.length(), " ");  // 1 espace pour mots
        pos += 1;
    }

    OutputDebugStringA("[CGR] Espaces intercales corriges avec succes\n");

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

        // Sauvegarder le texte nettoyé dans un fichier pour inspection
        try
        {
            // Le fichier source devrait être accessible via la fonction appelante
            // Pour l'instant on sauvegarde dans un fichier temporaire
            std::ofstream debugFile("CGR_cleaned_text.txt");
            if (debugFile.is_open())
            {
                debugFile << cleanedText;
                debugFile.close();
                OutputDebugStringA("[CGR] Texte nettoye sauvegarde dans: CGR_cleaned_text.txt\n");
            }
        }
        catch (...) {}

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
