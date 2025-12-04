#include "SiehrPdfParser.h"
#include "PopplerPdfExtractor.h"
#include <fstream>
#include <sstream>
#include <regex>
#include <algorithm>
#include <filesystem>
#include <windows.h>

std::string SiehrPdfParser::extractText(const std::string& filePath)
{
    // Utiliser PopplerPdfExtractor
    std::string text = PopplerPdfExtractor::extractTextFromPdf(filePath);

    // Debug: sauvegarder le texte extrait
    try
    {
        std::filesystem::path pdfPathObj(filePath);
        std::filesystem::path debugPath = pdfPathObj.parent_path() / (pdfPathObj.stem().string() + "_siehr_extracted.txt");
        std::ofstream debugFile(debugPath);
        if (debugFile.is_open())
        {
            debugFile << text;
            debugFile.close();
            std::string debugMsg = "[Siehr] Texte extrait sauvegardé dans: " + debugPath.string() + "\n";
            OutputDebugStringA(debugMsg.c_str());
        }
    }
    catch (...) {}

    // Debug: afficher dans la console
    OutputDebugStringA("=== DEBUG EXTRACTION PDF SIEHR ===\n");
    std::string debug = "Poppler disponible: " + std::string(PopplerPdfExtractor::isPopplerAvailable() ? "OUI" : "NON") + "\n";
    debug += "Texte extrait (" + std::to_string(text.length()) + " caractères):\n";
    size_t previewLength = text.length() < 1500 ? text.length() : 1500;
    debug += text.substr(0, previewLength) + "\n";
    debug += "=== FIN DEBUG SIEHR ===\n";
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

// Fonction helper pour trim les espaces
static std::string trim(const std::string& s)
{
    auto start = s.find_first_not_of(" \t\r\n");
    auto end = s.find_last_not_of(" \t\r\n");
    if (start == std::string::npos) return std::string();
    return s.substr(start, end - start + 1);
}

std::vector<PdfLine> SiehrPdfParser::parseTextContent(const std::string& text)
{
    std::vector<PdfLine> lines;

    OutputDebugStringA("=== DEBUT PARSING SIEHR ===\n");

    // Vérifier si c'est un message d'erreur
    if (text.find("ERREUR:") == 0)
    {
        OutputDebugStringA("[Siehr] Texte contient ERREUR, abandon\n");
        return lines;
    }

    if (text.empty())
    {
        OutputDebugStringA("[Siehr] Texte vide, abandon\n");
        return lines;
    }

    // Parser ligne par ligne adapté à la structure Siehr
    // Format observé :
    //   Ligne 1 : "SB2050  AS  PAROI FIXE LINEAIRE  600 HT 2000        DIVERA                2,000 PIEC    174,00 PIEC        348,00"
    //   Ligne 2 : "POLI BRILLANT - VERRE TRANSPARENT" (optionnelle, description complémentaire)

    int extractedCount = 0;

    try
    {
        OutputDebugStringA("[Siehr] Creation du regex ligne article...\n");

        // Regex pour la ligne principale (approche GPT)
        // Groupe 1 : Référence (optionnelle)
        // Groupe 2 : Désignation (première partie)
        // Groupe 3 : Quantité (format X,XXX)
        // Groupe 4 : Prix unitaire (format XX,XX)
        // Groupe 5 : Montant total (format XX,XX)
        std::regex lineRegex(
            R"(^(?:(\S+)\s+)?(.*?)\s+(\d+,\d{3})\s+PIEC\s+(\d+,\d{2})\s+PIEC\s+(\d+,\d{2})\s*$)",
            std::regex::ECMAScript
        );

        OutputDebugStringA("[Siehr] Regex cree, debut parsing ligne par ligne...\n");

        // Séparer le texte en lignes
        std::istringstream stream(text);
        std::string line;
        std::vector<std::string> textLines;

        while (std::getline(stream, line))
        {
            textLines.push_back(line);
        }

        std::string lineCountMsg = "[Siehr] Nombre de lignes: " + std::to_string(textLines.size()) + "\n";
        OutputDebugStringA(lineCountMsg.c_str());

        // Parser chaque ligne
        for (size_t i = 0; i < textLines.size(); ++i)
        {
            std::string currentLine = trim(textLines[i]);

            // Pré-filtre rapide : sauter les lignes qui ne peuvent pas être des produits
            // Une ligne produit Siehr contient toujours "PIEC"
            if (currentLine.find("PIEC") == std::string::npos)
            {
                continue; // Ligne ignorée, pas besoin de regex coûteux
            }

            std::smatch match;

            if (std::regex_match(currentLine, match, lineRegex))
            {
                PdfLine product;

                // Extraire les captures
                std::string rawRef = match[1].matched ? match[1].str() : "";
                std::string desc = trim(match[2].str());
                std::string qteStr = match[3].str();
                std::string prixStr = match[4].str();
                // Montant total dans groupe 5 (non utilisé)

                // Filtrer les références : si pas de chiffre, c'est partie de la désignation
                auto hasDigit = [](const std::string& s) {
                    return std::any_of(s.begin(), s.end(),
                        [](unsigned char c) { return std::isdigit(c); });
                };

                if (!rawRef.empty() && hasDigit(rawRef))
                {
                    // Cas normal : vraie référence type SB2050, S00286...
                    product.reference = rawRef;
                }
                else
                {
                    // Pas de vraie ref -> le premier mot fait partie de la désignation
                    if (!rawRef.empty())
                    {
                        if (!desc.empty())
                            desc = rawRef + " " + desc;
                        else
                            desc = rawRef;
                    }
                    product.reference = "VIDE";

                    OutputDebugStringA("[Siehr] Reference vide detectee (pas de chiffre dans le premier mot)\n");
                }

                // Désignation
                product.designation = desc;

                // Quantité
                product.quantite = parseFrenchNumber(qteStr);

                // Prix unitaire
                product.prixHT = parseFrenchNumber(prixStr);

                std::string debugMsg = "[Siehr] Ligne article trouvee: Ref=" + product.reference +
                    " | Qte=" + std::to_string(product.quantite) +
                    " | Prix=" + std::to_string(product.prixHT) + "\n";
                OutputDebugStringA(debugMsg.c_str());

                // Regarder la ligne suivante pour compléter la désignation
                if (i + 1 < textLines.size())
                {
                    std::string nextLine = trim(textLines[i + 1]);

                    // Vérifier si c'est une ligne descriptive complémentaire
                    bool isExtraDesc =
                        !nextLine.empty() &&
                        nextLine.find("Ecopart.") != 0 &&
                        nextLine.find("PU") != 0 &&
                        nextLine.find("PIEC") == std::string::npos;

                    if (isExtraDesc)
                    {
                        product.designation += " " + nextLine;
                        ++i; // Sauter cette ligne car elle appartient à cet article

                        std::string debugExtra = "[Siehr] Ligne complementaire ajoutee: " + nextLine + "\n";
                        OutputDebugStringA(debugExtra.c_str());
                    }
                }

                // Ajouter le produit à la liste
                lines.push_back(product);
                extractedCount++;

                std::string productMsg = "Siehr produit #" + std::to_string(extractedCount) + ": " +
                    product.reference + " | " + product.designation + " | " +
                    std::to_string(product.quantite) + " | " + std::to_string(product.prixHT) + "\n";
                OutputDebugStringA(productMsg.c_str());
            }
        }
    }
    catch (const std::regex_error& e)
    {
        std::string errorMsg = "[Siehr] ERREUR REGEX: " + std::string(e.what()) + "\n";
        OutputDebugStringA(errorMsg.c_str());
    }
    catch (const std::exception& e)
    {
        std::string errorMsg = "[Siehr] EXCEPTION: " + std::string(e.what()) + "\n";
        OutputDebugStringA(errorMsg.c_str());
    }
    catch (...)
    {
        OutputDebugStringA("[Siehr] EXCEPTION INCONNUE\n");
    }

    // Log final
    std::string finalMsg = "=== RESUME PARSING SIEHR ===\n";
    finalMsg += "Produits extraits : " + std::to_string(extractedCount) + "\n";
    finalMsg += "==============================\n";
    OutputDebugStringA(finalMsg.c_str());

    return lines;
}

std::vector<PdfLine> SiehrPdfParser::parse(const std::string& filePath)
{
    std::string text = extractText(filePath);
    return parseTextContent(text);
}
