#include "FischerPdfParser.h"
#include "PopplerPdfExtractor.h"
#include <fstream>
#include <sstream>
#include <regex>
#include <algorithm>
#include <filesystem>
#include <windows.h>

std::string FischerPdfParser::extractText(const std::string& filePath)
{
    // Utiliser PopplerPdfExtractor
    std::string text = PopplerPdfExtractor::extractTextFromPdf(filePath);

    // Debug: sauvegarder le texte extrait
    try
    {
        std::filesystem::path pdfPathObj(filePath);
        std::filesystem::path debugPath = pdfPathObj.parent_path() / (pdfPathObj.stem().string() + "_fischer_extracted.txt");
        std::ofstream debugFile(debugPath);
        if (debugFile.is_open())
        {
            debugFile << text;
            debugFile.close();
            std::string debugMsg = "[Fischer] Texte extrait sauvegardé dans: " + debugPath.string() + "\n";
            OutputDebugStringA(debugMsg.c_str());
        }
    }
    catch (...) {}

    // Debug: afficher dans la console
    OutputDebugStringA("=== DEBUG EXTRACTION PDF FISCHER ===\n");
    std::string debug = "Poppler disponible: " + std::string(PopplerPdfExtractor::isPopplerAvailable() ? "OUI" : "NON") + "\n";
    debug += "Texte extrait (" + std::to_string(text.length()) + " caractères):\n";
    size_t previewLength = text.length() < 1500 ? text.length() : 1500;
    debug += text.substr(0, previewLength) + "\n";
    debug += "=== FIN DEBUG FISCHER ===\n";
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

std::vector<PdfLine> FischerPdfParser::parseTextContent(const std::string& text)
{
    std::vector<PdfLine> lines;

    OutputDebugStringA("=== DEBUT PARSING FISCHER ===\n");

    // Vérifier si c'est un message d'erreur
    if (text.find("ERREUR:") == 0)
    {
        OutputDebugStringA("[Fischer] Texte contient ERREUR, abandon\n");
        return lines;
    }

    if (text.empty())
    {
        OutputDebugStringA("[Fischer] Texte vide, abandon\n");
        return lines;
    }

    // Parser ligne par ligne adapté à la structure Fischer
    // Format observé sur 3 lignes :
    //   Ligne 1 : "10         Collier FRSR 20-25 M8/M10 -100/bte           6   BTE    0,01    1   BTE    145,14"
    //   Ligne 2 : "4048962230956               534135                      Éco-contribution    0,01    0,06"
    //   Ligne 3 : "Total éco-contribution comprise                         22,99    137,94"

    int extractedCount = 0;

    try
    {
        OutputDebugStringA("[Fischer] Creation des regex ligne par ligne...\n");

        // Regex pour chaque ligne (approche GPT - plus robuste)
        // Ligne 1 : Position + Désignation + Quantité + Unité
        std::regex line1Regex(R"(^\s*\d+\s+(.+?)\s+(\d+)\s+(BTE|PCE)\b)");

        // Ligne 2 : GTIN + Référence (le 2ème nombre après GTIN)
        std::regex line2Regex(R"(^\s*\d{10,}\s+(\d{5,})\b)");

        // Ligne 3 : Prix (format large pour capturer xx,xx avec optionnellement € ou EUR)
        // On cherche simplement un montant au format français
        std::regex line3Regex(R"((\d+,\d{2})\s*(?:€|EUR)?)", std::regex::ECMAScript);

        OutputDebugStringA("[Fischer] Regex crees, debut parsing ligne par ligne...\n");

        // Séparer le texte en lignes
        std::istringstream stream(text);
        std::string line;
        std::vector<std::string> textLines;

        while (std::getline(stream, line))
        {
            textLines.push_back(line);
        }

        std::string lineCountMsg = "[Fischer] Nombre de lignes: " + std::to_string(textLines.size()) + "\n";
        OutputDebugStringA(lineCountMsg.c_str());

        // State machine à 3 états pour parser les blocs de 3 lignes
        // Avec compteur pour permettre de sauter des lignes (coupures de pages)
        enum class State { WAITING_LINE1, WAITING_LINE2, WAITING_LINE3 };
        State state = State::WAITING_LINE1;
        PdfLine currentProduct;
        int skipLinesCounter = 0;
        const int MAX_SKIP_LINES = 10;  // Permettre de sauter jusqu'à 10 lignes (en-têtes, pieds de page)

        for (size_t i = 0; i < textLines.size(); ++i)
        {
            const std::string& currentLine = textLines[i];
            std::smatch match;

            switch (state)
            {
            case State::WAITING_LINE1:
                if (std::regex_search(currentLine, match, line1Regex))
                {
                    currentProduct = PdfLine();
                    currentProduct.designation = match[1].str();
                    currentProduct.quantite = parseFrenchNumber(match[2].str());
                    // Unité dans match[3] si besoin

                    // Nettoyer la désignation
                    currentProduct.designation.erase(0, currentProduct.designation.find_first_not_of(" \t"));
                    currentProduct.designation.erase(currentProduct.designation.find_last_not_of(" \t") + 1);

                    std::string debugMsg = "[Fischer] Ligne1 trouvee: " + currentProduct.designation +
                        " | Qte: " + std::to_string(currentProduct.quantite) + "\n";
                    OutputDebugStringA(debugMsg.c_str());

                    state = State::WAITING_LINE2;
                    skipLinesCounter = 0;  // Réinitialiser le compteur
                }
                break;

            case State::WAITING_LINE2:
                if (std::regex_search(currentLine, match, line2Regex))
                {
                    currentProduct.reference = match[1].str();

                    std::string debugMsg = "[Fischer] Ligne2 trouvee: Ref=" + currentProduct.reference +
                        " (apres " + std::to_string(skipLinesCounter) + " lignes sautees)\n";
                    OutputDebugStringA(debugMsg.c_str());

                    state = State::WAITING_LINE3;
                    skipLinesCounter = 0;  // Réinitialiser le compteur
                }
                else
                {
                    skipLinesCounter++;
                    if (skipLinesCounter > MAX_SKIP_LINES)
                    {
                        // Trop de lignes sautées, abandon du produit
                        std::string debugMsg = "[Fischer] Ligne2 non trouvee apres " +
                            std::to_string(MAX_SKIP_LINES) + " lignes, abandon produit\n";
                        OutputDebugStringA(debugMsg.c_str());
                        state = State::WAITING_LINE1;
                    }
                    // Sinon, continuer à chercher ligne 2 sur les lignes suivantes
                }
                break;

            case State::WAITING_LINE3:
                if (std::regex_search(currentLine, match, line3Regex))
                {
                    // On a trouvé un montant au format français
                    currentProduct.prixHT = parseFrenchNumber(match[1].str());

                    std::string debugMsg = "[Fischer] Ligne3 trouvee: Prix=" + std::to_string(currentProduct.prixHT) +
                        " (apres " + std::to_string(skipLinesCounter) + " lignes sautees)\n";
                    OutputDebugStringA(debugMsg.c_str());

                    // Produit complet, l'ajouter à la liste
                    lines.push_back(currentProduct);
                    extractedCount++;

                    std::string productMsg = "Fischer produit #" + std::to_string(extractedCount) + ": " +
                        currentProduct.reference + " | " + currentProduct.designation + " | " +
                        std::to_string(currentProduct.quantite) + " | " + std::to_string(currentProduct.prixHT) + "\n";
                    OutputDebugStringA(productMsg.c_str());

                    state = State::WAITING_LINE1;
                    skipLinesCounter = 0;
                }
                else
                {
                    skipLinesCounter++;
                    if (skipLinesCounter > MAX_SKIP_LINES)
                    {
                        // Trop de lignes sautées, ajouter le produit avec prix=0.0
                        std::string debugMsg = "[Fischer][WARN] Prix non trouve apres " +
                            std::to_string(MAX_SKIP_LINES) + " lignes, ajout avec prix=0.0\n";
                        OutputDebugStringA(debugMsg.c_str());

                        currentProduct.prixHT = 0.0;
                        lines.push_back(currentProduct);
                        extractedCount++;

                        std::string productMsg = "Fischer produit #" + std::to_string(extractedCount) + " (SANS PRIX): " +
                            currentProduct.reference + " | " + currentProduct.designation + " | " +
                            std::to_string(currentProduct.quantite) + " | " + std::to_string(currentProduct.prixHT) + "\n";
                        OutputDebugStringA(productMsg.c_str());

                        state = State::WAITING_LINE1;
                        skipLinesCounter = 0;
                    }
                    // Sinon, continuer à chercher ligne 3 sur les lignes suivantes
                }
                break;
            }
        }
    }
    catch (const std::regex_error& e)
    {
        std::string errorMsg = "[Fischer] ERREUR REGEX: " + std::string(e.what()) + "\n";
        OutputDebugStringA(errorMsg.c_str());
    }
    catch (const std::exception& e)
    {
        std::string errorMsg = "[Fischer] EXCEPTION: " + std::string(e.what()) + "\n";
        OutputDebugStringA(errorMsg.c_str());
    }
    catch (...)
    {
        OutputDebugStringA("[Fischer] EXCEPTION INCONNUE\n");
    }

    // Log final
    std::string finalMsg = "=== RESUME PARSING FISCHER ===\n";
    finalMsg += "Produits extraits : " + std::to_string(extractedCount) + "\n";
    finalMsg += "==============================\n";
    OutputDebugStringA(finalMsg.c_str());

    return lines;
}

std::vector<PdfLine> FischerPdfParser::parse(const std::string& filePath)
{
    std::string text = extractText(filePath);
    return parseTextContent(text);
}
