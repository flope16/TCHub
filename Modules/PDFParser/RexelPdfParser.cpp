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

    // Format Rexel avec -layout : chercher les lignes contenant "NDX" (références)
    // et extraire les données depuis ces lignes
    std::regex refLineRegex(R"(^\s*(\d{3})\s+(NDX\d+)\s+(.*))"); // Capte "001    NDX402010   ..."

    for (size_t i = 0; i < textLines.size(); ++i)
    {
        std::smatch match;

        // Chercher une ligne commençant par un numéro et contenant NDX
        if (std::regex_search(textLines[i], match, refLineRegex))
        {
            PdfLine product;
            product.reference = trim(match[2].str()); // NDX402010
            std::string restOfLine = match[3].str(); // Le reste de la ligne

            std::string debugMsg = "[Rexel] ✓ MATCH REF à ligne " + std::to_string(i) + ": \"" + product.reference + "\" | Reste: \"" + restOfLine + "\"\n";
            OutputDebugStringA(debugMsg.c_str());

            // Chercher la désignation sur la ligne suivante
            if (i + 1 < textLines.size())
            {
                std::string nextLine = trim(textLines[i + 1]);
                // La désignation est souvent sur la ligne suivante si elle commence par une lettre
                if (!nextLine.empty() && std::isalpha(static_cast<unsigned char>(nextLine[0])))
                {
                    product.designation = nextLine;
                    std::string debugDesc = "[Rexel] Desc trouvée (ligne " + std::to_string(i + 1) + "): \"" + product.designation + "\"\n";
                    OutputDebugStringA(debugDesc.c_str());
                }
            }

            // Parser le reste de la ligne pour extraire quantité et prix
            // Format typique : "001 NDX402010   800   P   0,68000   ...   544,00  2"
            std::regex dataRegex(R"((\d+)\s+P\s+[\d,]+\s+([\d,]+))"); // Quantité + P + ... + Prix
            std::smatch dataMatch;
            if (std::regex_search(restOfLine, dataMatch, dataRegex))
            {
                product.quantite = std::stod(dataMatch[1].str());
                product.prixHT = parseFrenchNumber(dataMatch[2].str());

                std::string debugData = "[Rexel] Qte=" + std::to_string(product.quantite) +
                                       " | PU=" + std::to_string(product.prixHT) + "\n";
                OutputDebugStringA(debugData.c_str());
            }
            else
            {
                // Si le regex ne match pas, chercher manuellement les nombres
                OutputDebugStringA("[Rexel] Regex data ne matche pas, extraction manuelle...\n");

                // Avec -layout, les données sont souvent éclatées sur plusieurs lignes
                // Chercher " P " dans cette ligne et les 5 lignes suivantes
                std::string searchText = restOfLine;
                size_t searchLineStart = i;

                for (size_t j = i + 1; j < textLines.size() && j < i + 6; ++j)
                {
                    searchText += " " + textLines[j];
                }

                size_t endLine = (i + 5 < textLines.size()) ? (i + 5) : (textLines.size() - 1);
                std::string debugSearch = "[Rexel] Texte de recherche (lignes " + std::to_string(i) + " à " + std::to_string(endLine) + "): \"" + searchText.substr(0, 200) + "...\"\n";
                OutputDebugStringA(debugSearch.c_str());

                // Chercher " P " dans le texte combiné
                size_t pPos = searchText.find(" P ");
                if (pPos != std::string::npos)
                {
                    std::string debugPFound = "[Rexel] ' P ' trouvé à position " + std::to_string(pPos) + "\n";
                    OutputDebugStringA(debugPFound.c_str());

                    // Extraire le nombre avant " P "
                    size_t startQte = searchText.rfind(' ', pPos - 1);
                    if (startQte != std::string::npos)
                    {
                        std::string qteStr = trim(searchText.substr(startQte, pPos - startQte));

                        try {
                            product.quantite = std::stod(qteStr);
                            std::string debugQte = "[Rexel] Qte extraite: \"" + qteStr + "\" = " + std::to_string(product.quantite) + "\n";
                            OutputDebugStringA(debugQte.c_str());
                        }
                        catch (...) {
                            OutputDebugStringA("[Rexel] Erreur conversion qte\n");
                        }
                    }

                    // Chercher le prix : premier nombre avec virgule et 5 décimales après " P "
                    if (pPos + 3 < searchText.length())
                    {
                        std::regex priceRegex(R"(\b(\d+,\d{5})\b)");
                        std::smatch priceMatch;
                        std::string afterP = searchText.substr(pPos + 3);

                        if (std::regex_search(afterP, priceMatch, priceRegex))
                        {
                            product.prixHT = parseFrenchNumber(priceMatch[1].str());

                            std::string debugPrix = "[Rexel] Prix extrait: \"" + priceMatch[1].str() + "\" = " + std::to_string(product.prixHT) + "\n";
                            OutputDebugStringA(debugPrix.c_str());
                        }
                        else
                        {
                            OutputDebugStringA("[Rexel] Prix non trouvé avec regex\n");
                        }
                    }
                }
                else
                {
                    OutputDebugStringA("[Rexel] ' P ' non trouvé dans les lignes suivantes\n");
                }
            }

            // Ajouter le produit si on a au moins une référence et une quantité
            if (!product.reference.empty() && product.quantite > 0)
            {
                lines.push_back(product);
                extractedCount++;

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
