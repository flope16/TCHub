#include "LindabPdfParser.h"
#include <fstream>
#include <sstream>
#include <regex>
#include <algorithm>

// NOTE: Pour l'instant, cette implémentation suppose que le PDF a été
// converti en texte. Pour une intégration complète, il faudra utiliser
// une bibliothèque comme Poppler ou MuPDF pour extraire le texte directement.

std::string LindabPdfParser::extractText(const std::string& filePath)
{
    // Pour l'instant, on lit le fichier comme s'il était déjà en texte
    // TODO: Intégrer une bibliothèque PDF (Poppler, MuPDF, ou PDFium)

    std::ifstream file(filePath);
    if (!file.is_open())
        return "";

    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

std::vector<PdfLine> LindabPdfParser::parseTextContent(const std::string& text)
{
    std::vector<PdfLine> lines;

    // Expression régulière pour capturer les lignes de produits Lindab
    // Format attendu : Numéro Article Désignation... Quantité PCE Prix Montant
    // Exemple: "1 224931 SR 200 3000 GALV ... 10,00 PCE 18,90 188,99"

    std::istringstream stream(text);
    std::string line;

    while (std::getline(stream, line))
    {
        // Chercher les lignes qui commencent par un numéro (ligne de produit)
        std::regex linePattern(R"(^\s*\d+\s+(\d+)\s+)");
        std::smatch match;

        if (std::regex_search(line, match, linePattern))
        {
            PdfLine pdfLine;

            // Extraire le numéro d'article (premier groupe de chiffres après le numéro de ligne)
            std::regex articlePattern(R"(^\s*\d+\s+(\d+))");
            std::smatch articleMatch;
            if (std::regex_search(line, articleMatch, articlePattern))
            {
                pdfLine.reference = articleMatch[1];
            }

            // Extraire la quantité et le prix (chercher pattern: "nombre,nombre PCE nombre,nombre nombre,nombre")
            std::regex pricePattern(R"((\d+,\d+)\s+PCE\s+(\d+,\d+)\s+(\d+,\d+))");
            std::smatch priceMatch;
            if (std::regex_search(line, priceMatch, pricePattern))
            {
                // Convertir les nombres avec virgule en double
                std::string qteStr = priceMatch[1];
                std::string prixStr = priceMatch[2];
                std::string montantStr = priceMatch[3];

                std::replace(qteStr.begin(), qteStr.end(), ',', '.');
                std::replace(prixStr.begin(), prixStr.end(), ',', '.');
                std::replace(montantStr.begin(), montantStr.end(), ',', '.');

                pdfLine.quantite = std::stod(qteStr);
                pdfLine.prixHT = std::stod(prixStr);
                pdfLine.montantHT = std::stod(montantStr);
            }

            // Extraire la désignation (entre le numéro d'article et la quantité)
            // La désignation commence après le numéro d'article et se termine avant "PCE"
            size_t refPos = line.find(pdfLine.reference);
            size_t pcePos = line.find("PCE");

            if (refPos != std::string::npos && pcePos != std::string::npos)
            {
                size_t startPos = refPos + pdfLine.reference.length();
                size_t length = pcePos - startPos;

                pdfLine.designation = line.substr(startPos, length);

                // Nettoyer la désignation (enlever espaces superflus et chiffres de dimensions)
                // Garder le texte principal avant les dimensions
                std::regex desigPattern(R"(^\s*(.*?)\s+\d+\s+\d+\s+\w+)");
                std::smatch desigMatch;
                if (std::regex_search(pdfLine.designation, desigMatch, desigPattern))
                {
                    pdfLine.designation = desigMatch[1];
                }

                // Trim
                pdfLine.designation.erase(0, pdfLine.designation.find_first_not_of(" \t\n\r"));
                pdfLine.designation.erase(pdfLine.designation.find_last_not_of(" \t\n\r") + 1);
            }

            // Ajouter la ligne si elle contient des données valides
            if (!pdfLine.reference.empty() && pdfLine.quantite > 0)
            {
                lines.push_back(pdfLine);
            }
        }
    }

    return lines;
}

std::vector<PdfLine> LindabPdfParser::parse(const std::string& filePath)
{
    // Extraire le texte du PDF
    std::string text = extractText(filePath);

    // Parser le contenu texte
    return parseTextContent(text);
}
