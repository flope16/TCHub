#include "PompacPdfParser.h"
#include "PopplerPdfExtractor.h"
#include <fstream>
#include <sstream>
#include <regex>
#include <algorithm>
#include <filesystem>
#include <windows.h>

std::string PompacPdfParser::extractText(const std::string& filePath)
{
    // Utiliser PopplerPdfExtractor
    std::string text = PopplerPdfExtractor::extractTextFromPdf(filePath);

    // Debug: sauvegarder le texte extrait
    try
    {
        std::filesystem::path pdfPathObj(filePath);
        std::filesystem::path debugPath = pdfPathObj.parent_path() / (pdfPathObj.stem().string() + "_pompac_extracted.txt");
        std::ofstream debugFile(debugPath);
        if (debugFile.is_open())
        {
            debugFile << text;
            debugFile.close();
            std::string debugMsg = "[Pompac] Texte extrait sauvegardé dans: " + debugPath.string() + "\n";
            OutputDebugStringA(debugMsg.c_str());
        }
    }
    catch (...) {}

    // Debug: afficher dans la console
    OutputDebugStringA("=== DEBUG EXTRACTION PDF POMPAC ===\n");
    std::string debug = "Poppler disponible: " + std::string(PopplerPdfExtractor::isPopplerAvailable() ? "OUI" : "NON") + "\n";
    debug += "Texte extrait (" + std::to_string(text.length()) + " caractères):\n";
    size_t previewLength = text.length() < 1500 ? text.length() : 1500;
    debug += text.substr(0, previewLength) + "\n";
    debug += "=== FIN DEBUG POMPAC ===\n";
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

std::vector<PdfLine> PompacPdfParser::parseTextContent(const std::string& text)
{
    std::vector<PdfLine> lines;

    OutputDebugStringA("=== DEBUT PARSING POMPAC ===\n");

    // Vérifier si c'est un message d'erreur
    if (text.find("ERREUR:") == 0)
    {
        OutputDebugStringA("[Pompac] Texte contient ERREUR, abandon\n");
        return lines;
    }

    if (text.empty())
    {
        OutputDebugStringA("[Pompac] Texte vide, abandon\n");
        return lines;
    }

    // Parser ligne par ligne adapté à la structure Pompac (quasi-identique à Siehr)
    // Format observé :
    //   Ligne normale : "T30139  RAD. ALU KLASS. SIMPLE 22 500 1200 2196W  1,000 PIEC  257,64 PIEC  257,64"
    //   Ligne gratuite : "T28971  RAD. PIANO UNI 6 22 900 900 1880W  1,000 PIEC  PIEC  Gratuit"

    int extractedCount = 0;

    try
    {
        OutputDebugStringA("[Pompac] Creation des regex (normal + gratuit)...\n");

        // Regex pour article normal (avec prix)
        // Groupe 1 : Référence (optionnelle)
        // Groupe 2 : Désignation (première partie)
        // Groupe 3 : Quantité (format X,XXX)
        // Groupe 4 : Prix unitaire (format XX,XX ou XXX,XX)
        // Groupe 5 : Montant total (format XX,XX ou XXX,XX)
        std::regex normalRegex(
            R"(^(?:(\S+)\s+)?(.*?)\s+(\d+,\d{3})\s+PIEC\s+(\d+,\d{2})\s+PIEC\s+(\d+,\d{2})\s*$)",
            std::regex::ECMAScript
        );

        // Regex pour article GRATUIT
        // Groupe 1 : Référence (optionnelle)
        // Groupe 2 : Désignation
        // Groupe 3 : Quantité (format X,XXX)
        std::regex gratuitRegex(
            R"(^(?:(\S+)\s+)?(.*?)\s+(\d+,\d{3})\s+PIEC\s+PIEC\s+Gratuit\s*$)",
            std::regex::ECMAScript
        );

        OutputDebugStringA("[Pompac] Regex crees, debut parsing ligne par ligne...\n");

        // Séparer le texte en lignes
        std::istringstream stream(text);
        std::string line;
        std::vector<std::string> textLines;

        while (std::getline(stream, line))
        {
            textLines.push_back(line);
        }

        std::string lineCountMsg = "[Pompac] Nombre de lignes: " + std::to_string(textLines.size()) + "\n";
        OutputDebugStringA(lineCountMsg.c_str());

        // Fonction lambda pour vérifier si une chaîne contient des chiffres
        auto hasDigit = [](const std::string& s) {
            return std::any_of(s.begin(), s.end(),
                [](unsigned char c) { return std::isdigit(c); });
        };

        // Parser chaque ligne
        for (size_t i = 0; i < textLines.size(); ++i)
        {
            std::string currentLine = trim(textLines[i]);

            // Pré-filtre rapide : sauter les lignes qui ne peuvent pas être des produits
            // Une ligne produit Pompac contient toujours "PIEC"
            if (currentLine.find("PIEC") == std::string::npos)
            {
                continue; // Ligne ignorée, pas besoin de regex coûteux
            }

            std::smatch match;
            bool isFree = false;

            // Tester d'abord le regex normal, puis gratuit
            if (std::regex_match(currentLine, match, normalRegex))
            {
                isFree = false;
                OutputDebugStringA("[Pompac] Article normal detecte\n");
            }
            else if (std::regex_match(currentLine, match, gratuitRegex))
            {
                isFree = true;
                OutputDebugStringA("[Pompac] Article GRATUIT detecte\n");
            }
            else
            {
                continue; // Pas une ligne d'article
            }

            PdfLine product;

            // Extraire les captures communes
            std::string rawRef = match[1].matched ? match[1].str() : "";
            std::string desc = trim(match[2].str());
            std::string qteStr = match[3].str();

            // Filtrer les références : si pas de chiffre, c'est partie de la désignation
            if (!rawRef.empty() && hasDigit(rawRef))
            {
                // Cas normal : vraie référence type T30139, T07321...
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

                OutputDebugStringA("[Pompac] Reference vide detectee (pas de chiffre dans le premier mot)\n");
            }

            // Désignation
            product.designation = desc;

            // Quantité
            product.quantite = parseFrenchNumber(qteStr);

            // Prix unitaire
            if (!isFree)
            {
                // Article normal : récupérer le prix du groupe 4
                std::string prixStr = match[4].str();
                product.prixHT = parseFrenchNumber(prixStr);
            }
            else
            {
                // Article GRATUIT : prix = 0.00
                product.prixHT = 0.0;
            }

            std::string debugMsg = "[Pompac] Ligne article trouvee: Ref=" + product.reference +
                " | Qte=" + std::to_string(product.quantite) +
                " | Prix=" + std::to_string(product.prixHT) +
                (isFree ? " (GRATUIT)" : "") + "\n";
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

                    std::string debugExtra = "[Pompac] Ligne complementaire ajoutee: " + nextLine + "\n";
                    OutputDebugStringA(debugExtra.c_str());
                }
            }

            // Ajouter le produit à la liste
            lines.push_back(product);
            extractedCount++;

            std::string productMsg = "Pompac produit #" + std::to_string(extractedCount) + ": " +
                product.reference + " | " + product.designation + " | " +
                std::to_string(product.quantite) + " | " + std::to_string(product.prixHT) + "\n";
            OutputDebugStringA(productMsg.c_str());
        }
    }
    catch (const std::regex_error& e)
    {
        std::string errorMsg = "[Pompac] ERREUR REGEX: " + std::string(e.what()) + "\n";
        OutputDebugStringA(errorMsg.c_str());
    }
    catch (const std::exception& e)
    {
        std::string errorMsg = "[Pompac] EXCEPTION: " + std::string(e.what()) + "\n";
        OutputDebugStringA(errorMsg.c_str());
    }
    catch (...)
    {
        OutputDebugStringA("[Pompac] EXCEPTION INCONNUE\n");
    }

    // Log final
    std::string finalMsg = "=== RESUME PARSING POMPAC ===\n";
    finalMsg += "Produits extraits : " + std::to_string(extractedCount) + "\n";
    finalMsg += "==============================\n";
    OutputDebugStringA(finalMsg.c_str());

    return lines;
}

std::vector<PdfLine> PompacPdfParser::parse(const std::string& filePath)
{
    std::string text = extractText(filePath);
    return parseTextContent(text);
}
