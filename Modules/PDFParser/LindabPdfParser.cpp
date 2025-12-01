#include "LindabPdfParser.h"
#include <fstream>
#include <sstream>
#include <regex>
#include <algorithm>
#include <filesystem>
#include <windows.h>

// NOTE: Cette version utilise pdftotext (poppler) ou cherche un fichier .txt correspondant
// Pour une intégration complète, il faudrait intégrer directement une bibliothèque PDF

std::string LindabPdfParser::extractText(const std::string& filePath)
{
    std::filesystem::path pdfPath(filePath);
    std::filesystem::path txtPath = pdfPath.parent_path() / (pdfPath.stem().string() + ".txt");

    // Méthode 1: Chercher un fichier .txt déjà extrait
    if (std::filesystem::exists(txtPath))
    {
        std::ifstream file(txtPath);
        if (file.is_open())
        {
            std::stringstream buffer;
            buffer << file.rdbuf();
            return buffer.str();
        }
    }

    // Méthode 2: Essayer d'utiliser pdftotext (si installé)
    std::string tempTxtPath = pdfPath.parent_path().string() + "\\temp_extract.txt";
    std::string command = "pdftotext \"" + filePath + "\" \"" + tempTxtPath + "\" 2>nul";

    int result = system(command.c_str());

    if (result == 0 && std::filesystem::exists(tempTxtPath))
    {
        std::ifstream file(tempTxtPath);
        if (file.is_open())
        {
            std::stringstream buffer;
            buffer << file.rdbuf();
            file.close();

            // Supprimer le fichier temporaire
            std::filesystem::remove(tempTxtPath);

            return buffer.str();
        }
    }

    // Méthode 3: Lire directement (ne fonctionnera pas pour un vrai PDF, mais utile pour les tests)
    std::ifstream file(filePath, std::ios::binary);
    if (!file.is_open())
        return "";

    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string content = buffer.str();

    // Si le contenu commence par %PDF, c'est un vrai PDF et on ne peut pas le lire comme du texte
    if (content.find("%PDF") == 0)
    {
        // Retourner un message d'erreur plutôt qu'un contenu vide
        return "ERREUR: PDF binaire détecté. Veuillez:\n1. Installer pdftotext (poppler-utils)\n2. Ou créer un fichier .txt à côté du PDF avec le même nom";
    }

    return content;
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

    while (std::getline(stream, line))
    {
        // Debug: chercher les lignes qui ont PCE (indicateur de ligne de produit)
        if (line.find("PCE") == std::string::npos)
            continue;

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

    return lines;
}

std::vector<PdfLine> LindabPdfParser::parse(const std::string& filePath)
{
    // Extraire le texte du PDF
    std::string text = extractText(filePath);

    // Parser le contenu texte
    return parseTextContent(text);
}
