#include "LindabPdfParser.h"
#include "PopplerPdfExtractor.h"
#include <fstream>
#include <sstream>
#include <regex>
#include <algorithm>
#include <filesystem>
#include <windows.h>

// Version avec intégration Poppler pour extraction native du texte PDF

std::string LindabPdfParser::extractText(const std::string& filePath)
{
    // Utiliser PopplerPdfExtractor qui gère automatiquement :
    // 1. Extraction native avec Poppler si disponible
    // 2. Fallback vers fichier .txt
    // 3. Fallback vers pdftotext en ligne de commande
    std::string text = PopplerPdfExtractor::extractTextFromPdf(filePath);

    // Debug: sauvegarder le texte extrait dans un fichier pour inspection
    try
    {
        std::filesystem::path pdfPathObj(filePath);
        std::filesystem::path debugPath = pdfPathObj.parent_path() / (pdfPathObj.stem().string() + "_extracted.txt");
        std::ofstream debugFile(debugPath);
        if (debugFile.is_open())
        {
            debugFile << text;
            debugFile.close();
            std::string debugMsg = "Texte extrait sauvegarde dans: " + debugPath.string() + "\n";
            OutputDebugStringA(debugMsg.c_str());
        }
    }
    catch (...) {}

    // Debug: afficher les 1000 premiers caractères dans la console
    OutputDebugStringA("=== DEBUG EXTRACTION PDF ===\n");
    std::string debug = "Poppler disponible: " + std::string(PopplerPdfExtractor::isPopplerAvailable() ? "OUI" : "NON") + "\n";
    debug += "Texte extrait (" + std::to_string(text.length()) + " caracteres):\n";
    size_t previewLength = text.length() < 1000 ? text.length() : 1000;
    debug += text.substr(0, previewLength) + "\n";
    debug += "=== FIN DEBUG ===\n";
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
    return std::stod(cleaned);
}

std::vector<PdfLine> LindabPdfParser::parseTextContent(const std::string& text)
{
    std::vector<PdfLine> lines;

    // Vérifier si c'est un message d'erreur
    if (text.find("ERREUR:") == 0)
    {
        return lines;
    }

    // Parser multi-lignes adapté à la structure réelle Lindab
    // Format observé :
    //   Ligne 1 : "1         224931             SR               200 3000 GALV"
    //   Lignes suivantes : désignation multi-lignes
    //   Ligne PCE : "10,00  PCE                                             18,90                    188,99"
    //   Ligne fin : "Date de réception                  14/11/2025"

    std::istringstream stream(text);
    std::string line;
    int totalLineCount = 0;
    int pceLineCount = 0;

    // État du parsing
    bool inProduct = false;
    std::string currentRef;
    std::string currentDesig;

    // Regex pour détecter l'en-tête d'un article : "1 224931 SR 200 3000 GALV"
    // Format : <numéro ligne> <référence (6 chiffres)> <reste>
    std::regex headerPattern(R"(^\s*(\d+)\s+(\d{6})\b)");

    // Regex pour extraire les données de la ligne PCE
    // Format : "10,00  PCE                 18,90                    188,99"
    std::regex pcePattern(R"((-?\d+,\d{2})\s*PCE\s+(-?\d+,\d{2})\s+(-?\d+,\d{2}))");

    while (std::getline(stream, line))
    {
        totalLineCount++;

        // Nettoyer les espaces en début/fin
        std::string stripped = line;
        stripped.erase(0, stripped.find_first_not_of(" \t\r\n"));
        stripped.erase(stripped.find_last_not_of(" \t\r\n") + 1);

        if (stripped.empty())
            continue;

        // 1. Détecter le début d'un nouvel article
        std::smatch headerMatch;
        if (std::regex_search(stripped, headerMatch, headerPattern))
        {
            // Si on était dans un produit précédent, on le finalise (rare mais possible)
            if (inProduct && !currentRef.empty())
            {
                // Produit incomplet sans ligne PCE, on l'ignore
                OutputDebugStringA(("ATTENTION: Article " + currentRef + " sans ligne PCE\n").c_str());
            }

            // Nouveau produit
            currentRef = headerMatch[2].str();
            inProduct = true;
            currentDesig.clear();

            // La première ligne contient aussi le début de la désignation (après la référence)
            size_t refPos = stripped.find(currentRef);
            if (refPos != std::string::npos)
            {
                std::string remainder = stripped.substr(refPos + currentRef.length());
                remainder.erase(0, remainder.find_first_not_of(" \t"));
                if (!remainder.empty())
                {
                    currentDesig = remainder;
                }
            }

            continue;
        }

        // 2. Si on est dans un produit, chercher la ligne PCE
        if (inProduct)
        {
            // Fin de bloc produit ?
            if (stripped.find("Date de réception") != std::string::npos ||
                stripped.find("Date de r") != std::string::npos)
            {
                // Fin du bloc produit (sans ligne PCE trouvée avant)
                inProduct = false;
                currentRef.clear();
                currentDesig.clear();
                continue;
            }

            // Ligne PCE ?
            std::smatch pceMatch;
            if (std::regex_search(stripped, pceMatch, pcePattern))
            {
                pceLineCount++;

                // Debug: afficher les 5 premières lignes avec PCE
                if (pceLineCount <= 5)
                {
                    std::string debugLine = "Ligne avec PCE #" + std::to_string(pceLineCount) + ": " + stripped + "\n";
                    OutputDebugStringA(debugLine.c_str());
                }

                try
                {
                    double quantite = parseFrenchNumber(pceMatch[1].str());
                    double prixHT = parseFrenchNumber(pceMatch[2].str());
                    double montantHT = parseFrenchNumber(pceMatch[3].str());

                    // Ignorer les lignes avec quantité négative ou nulle (remises)
                    if (quantite > 0)
                    {
                        PdfLine pdfLine;
                        pdfLine.reference = currentRef;
                        pdfLine.designation = currentDesig;
                        pdfLine.quantite = quantite;
                        pdfLine.prixHT = prixHT;
                        pdfLine.montantHT = montantHT;

                        // Nettoyer la désignation
                        pdfLine.designation.erase(0, pdfLine.designation.find_first_not_of(" \t"));
                        pdfLine.designation.erase(pdfLine.designation.find_last_not_of(" \t") + 1);

                        if (!pdfLine.reference.empty())
                        {
                            lines.push_back(pdfLine);
                        }
                    }
                    else
                    {
                        OutputDebugStringA(("Ignoré: Article " + currentRef + " avec quantité négative/nulle\n").c_str());
                    }
                }
                catch (const std::exception& e)
                {
                    OutputDebugStringA(("ERREUR parsing PCE: " + std::string(e.what()) + "\n").c_str());
                }

                // Reset après avoir trouvé la ligne PCE
                inProduct = false;
                currentRef.clear();
                currentDesig.clear();
                continue;
            }

            // Sinon, c'est une ligne de désignation à accumuler
            if (!stripped.empty())
            {
                if (!currentDesig.empty())
                    currentDesig += " ";
                currentDesig += stripped;
            }
        }
    }

    // Debug final
    std::string debugSummary = "=== RESUME PARSING LINDAB ===\n";
    debugSummary += "Lignes totales texte : " + std::to_string(totalLineCount) + "\n";
    debugSummary += "Lignes avec PCE : " + std::to_string(pceLineCount) + "\n";
    debugSummary += "Produits extraits : " + std::to_string(lines.size()) + "\n";
    debugSummary += "==============================\n";
    OutputDebugStringA(debugSummary.c_str());

    return lines;
}

std::vector<PdfLine> LindabPdfParser::parse(const std::string& filePath)
{
    // Extraire le texte du PDF
    std::string text = extractText(filePath);

    // Parser le contenu texte
    return parseTextContent(text);
}
