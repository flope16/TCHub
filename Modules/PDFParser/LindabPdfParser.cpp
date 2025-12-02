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

    // Debug: afficher les 500 premiers caractères dans la console
    OutputDebugStringA("=== DEBUG EXTRACTION PDF ===\n");
    std::string debug = "Poppler disponible: " + std::string(PopplerPdfExtractor::isPopplerAvailable() ? "OUI" : "NON") + "\n";
    debug += "Texte extrait (" + std::to_string(text.length()) + " caracteres):\n";
    debug += text.substr(0, std::min(size_t(500), text.length())) + "\n";
    debug += "=== FIN DEBUG ===\n";
    OutputDebugStringA(debug.c_str());

    return text;
}

std::vector<PdfLine> LindabPdfParser::parseTextContent(const std::string& text)
{
    std::vector<PdfLine> lines;

    // Vérifier si c'est un message d'erreur
    if (text.find("ERREUR:") == 0)
    {
        // Retourner une liste vide, l'erreur sera gérée par l'appelant
        return lines;
    }

    // Expression régulière pour capturer les lignes de produits Lindab
    // Format: Ligne N° Désignation Qté PCE Prix Montant
    // Exemple: "1 224931 SR 200 3000 GALV Conduit... 10,00 PCE 18,90 188,99"

    std::istringstream stream(text);
    std::string line;
    int pceLineCount = 0;
    int totalLineCount = 0;

    while (std::getline(stream, line))
    {
        totalLineCount++;

        // Debug: chercher les lignes qui ont PCE (indicateur de ligne de produit)
        if (line.find("PCE") == std::string::npos)
            continue;

        pceLineCount++;

        // Debug: afficher les 5 premières lignes avec PCE
        if (pceLineCount <= 5)
        {
            std::string debugLine = "Ligne avec PCE #" + std::to_string(pceLineCount) + ": " + line + "\n";
            OutputDebugStringA(debugLine.c_str());
        }

        // Pattern pour extraire: Référence, Qté, Prix, Montant
        // Format: numéro_ligne référence ... qté PCE prix montant
        std::regex fullPattern(R"(\s*(\d+)\s+(\d{6})\s+(.*?)\s+([\d,]+)\s+PCE\s+([\d,]+)\s+([\d,]+)\s*$)");
        std::smatch match;

        if (std::regex_search(line, match, fullPattern) && match.size() >= 7)
        {
            PdfLine pdfLine;

            // Référence (groupe 2)
            pdfLine.reference = match[2].str();

            // Désignation (groupe 3) - nettoyer
            std::string rawDesig = match[3].str();

            // Extraire juste la partie texte descriptive avant les dimensions
            std::regex desigPattern(R"((.*?)\s+\d+\s+\d+\s+\w+\s*)");
            std::smatch desigMatch;
            if (std::regex_search(rawDesig, desigMatch, desigPattern))
            {
                pdfLine.designation = desigMatch[1].str();
            }
            else
            {
                pdfLine.designation = rawDesig;
            }

            // Nettoyer les espaces
            pdfLine.designation.erase(0, pdfLine.designation.find_first_not_of(" \t"));
            pdfLine.designation.erase(pdfLine.designation.find_last_not_of(" \t") + 1);

            // Quantité (groupe 4)
            std::string qteStr = match[4].str();
            std::replace(qteStr.begin(), qteStr.end(), ',', '.');
            pdfLine.quantite = std::stod(qteStr);

            // Prix HT (groupe 5)
            std::string prixStr = match[5].str();
            std::replace(prixStr.begin(), prixStr.end(), ',', '.');
            pdfLine.prixHT = std::stod(prixStr);

            // Montant HT (groupe 6)
            std::string montantStr = match[6].str();
            std::replace(montantStr.begin(), montantStr.end(), ',', '.');
            pdfLine.montantHT = std::stod(montantStr);

            // Ajouter si valide
            if (!pdfLine.reference.empty() && pdfLine.quantite > 0)
            {
                lines.push_back(pdfLine);
            }
        }
        else
        {
            // Essayer un pattern simplifié pour les lignes problématiques
            std::regex simplePattern(R"((\d{6})\s+.+\s+([\d,]+)\s+PCE\s+([\d,]+)\s+([\d,]+))");
            std::smatch simpleMatch;

            if (std::regex_search(line, simpleMatch, simplePattern) && simpleMatch.size() >= 5)
            {
                PdfLine pdfLine;
                pdfLine.reference = simpleMatch[1].str();
                pdfLine.designation = "Article " + pdfLine.reference;

                std::string qteStr = simpleMatch[2].str();
                std::replace(qteStr.begin(), qteStr.end(), ',', '.');
                pdfLine.quantite = std::stod(qteStr);

                std::string prixStr = simpleMatch[3].str();
                std::replace(prixStr.begin(), prixStr.end(), ',', '.');
                pdfLine.prixHT = std::stod(prixStr);

                std::string montantStr = simpleMatch[4].str();
                std::replace(montantStr.begin(), montantStr.end(), ',', '.');
                pdfLine.montantHT = std::stod(montantStr);

                lines.push_back(pdfLine);
            }
        }
    }

    // Debug final
    std::string debugSummary = "=== RESUME PARSING ===\n";
    debugSummary += "Lignes totales: " + std::to_string(totalLineCount) + "\n";
    debugSummary += "Lignes avec PCE: " + std::to_string(pceLineCount) + "\n";
    debugSummary += "Produits extraits: " + std::to_string(lines.size()) + "\n";
    debugSummary += "===================\n";
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
