#include "CgrPdfParser.h"
#include "PopplerPdfExtractor.h"
#include <fstream>
#include <sstream>
#include <regex>
#include <algorithm>
#include <filesystem>
#include <cctype>
#include <windows.h>

// ===== FONCTIONS HELPERS (définies avant extractText) =====

// Fonction helper pour normaliser un nombre français avec espaces intercalés
// Ex: "1 9 ,0 2" → 19.02
//     "7 4 1 7 ,8 0" → 7417.80
static double normalizeNumber(std::string str)
{
    // Enlever TOUS les espaces (y compris tabs, espaces insécables U+00A0)
    str.erase(std::remove_if(str.begin(), str.end(),
        [](unsigned char c) { return std::isspace(c) || c == '\xA0'; }),
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

// Post-traite le texte pour enlever les espaces intercalés (causés par -layout)
// Stratégie : les espaces multiples (2+) = vrais séparateurs de colonnes
//             les espaces simples = faux espaces entre caractères
static std::string removeInterleavedSpaces(const std::string& text)
{
    std::string result;
    result.reserve(text.length());

    const std::string MARKER = "\x01\x02\x03";  // Marqueur temporaire

    // Traiter ligne par ligne pour préserver les newlines
    std::istringstream stream(text);
    std::string line;
    bool first = true;

    while (std::getline(stream, line))
    {
        if (!first) result += '\n';
        first = false;

        std::string processed;

        // 1. Remplacer les espaces multiples (2+) par le marqueur
        size_t i = 0;
        while (i < line.length())
        {
            if (line[i] == ' ')
            {
                size_t spaceCount = 0;
                size_t j = i;
                while (j < line.length() && line[j] == ' ')
                {
                    spaceCount++;
                    j++;
                }

                if (spaceCount >= 2)
                {
                    // Espaces multiples = vrai séparateur
                    processed += MARKER;
                }
                // Sinon on ignore (espace simple = faux espace)

                i = j;
            }
            else
            {
                processed += line[i];
                i++;
            }
        }

        // 2. Remplacer le marqueur par un espace unique
        size_t pos = 0;
        while ((pos = processed.find(MARKER, pos)) != std::string::npos)
        {
            processed.replace(pos, MARKER.length(), " ");
            pos += 1;
        }

        result += processed;
    }

    return result;
}

// Fonction pour nettoyer une référence en enlevant les espaces intercalés
// Ex: "R SA U 5 0" → "RSAU50"
static std::string cleanReference(std::string str)
{
    str.erase(std::remove_if(str.begin(), str.end(),
        [](unsigned char c) { return std::isspace(c); }),
        str.end());
    return str;
}

// ===== METHODES DE LA CLASSE =====

std::string CgrPdfParser::extractText(const std::string& filePath)
{
    // Utiliser PopplerPdfExtractor AVEC -layout pour avoir tout sur une ligne
    // Puis on post-traitera pour enlever les espaces intercalés
    std::string text = PopplerPdfExtractor::extractTextFromPdf(filePath, true);

    // Post-traiter pour enlever les espaces intercalés
    text = removeInterleavedSpaces(text);

    OutputDebugStringA("[CGR] Post-traitement des espaces intercales effectue\n");

    // Debug: sauvegarder le texte extrait après post-traitement
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

// Regex SIMPLE uniquement pour la fin de ligne CGR : Qte Prix € Total €
// Ex après post-traitement: " 390 19,02€ 7417,80€"  (plus d'espaces intercalés)
static const std::regex RE_CGR_TAIL(
    R"(\s(\d+)\s+([\d,]+)\s*€\s+([\d,]+)\s*€)",
    std::regex::ECMAScript
);

// Parsing d'une ligne CGR sans gros regex (évite error_complexity)
static bool parseLigneCGR(const std::string& lineIn, PdfLine& out)
{
    std::string line = lineIn;

    // Filtrage rapide : doit contenir € (chaîne UTF-8, pas char) et virgule
    // Note : € en UTF-8 = 3 bytes (0xE2 0x82 0xAC)
    // Mais si lu comme Windows-1252, ça donne "â‚¬"
    bool hasComma = (line.find(',') != std::string::npos);

    // DEBUG: Log des lignes avec virgule pour trouver les articles
    static int debugCommaLineCount = 0;
    if (hasComma && debugCommaLineCount < 10) {
        std::string debugMsg = "[CGR DEBUG] Ligne avec virgule #" + std::to_string(debugCommaLineCount) + ": " +
            line.substr(0, std::min<size_t>(150, line.length())) + "\n";
        OutputDebugStringA(debugMsg.c_str());

        // Afficher les bytes du symbole € si présent
        size_t pos = line.find("\xE2\x82\xAC");
        if (pos != std::string::npos) {
            OutputDebugStringA("[CGR DEBUG] Trouve € (UTF-8) dans la ligne !\n");
        }

        // Chercher "â‚¬" (affichage Windows-1252 de € UTF-8)
        if (line.find("â‚¬") != std::string::npos ||
            line.find("\xC3\xA2\xE2\x80\x9A\xC2\xAC") != std::string::npos) {
            OutputDebugStringA("[CGR DEBUG] Trouve sequence 'â‚¬' dans la ligne !\n");
        }

        debugCommaLineCount++;
    }

    bool hasEuro = (line.find("\xE2\x82\xAC") != std::string::npos) ||  // € en UTF-8
                   (line.find("€") != std::string::npos);                // Essai direct

    if (!hasEuro || !hasComma)
        return false;

    // On ne garde que la fin de la ligne pour le regex (évite error_complexity)
    const size_t MAX_TAIL = 120;
    size_t tailStart = 0;
    std::string tail;

    if (line.size() > MAX_TAIL) {
        tailStart = line.size() - MAX_TAIL;
        tail = line.substr(tailStart);
    } else {
        tail = line;
    }

    // 1) Extraction Qte / Prix / Total sur la queue avec petit regex
    std::smatch m;
    try {
        if (!std::regex_search(tail, m, RE_CGR_TAIL)) {
            return false; // pas une ligne article CGR
        }
    }
    catch (const std::regex_error& e) {
        std::string errorMsg = "[CGR] ERREUR regex tail: " + std::string(e.what()) + "\n";
        OutputDebugStringA(errorMsg.c_str());
        return false;
    }

    std::string qteStr = m[1].str();   // ex: "390"
    std::string prixStr = m[2].str();  // ex: "1 9 ,0 2 "
    std::string totalStr = m[3].str(); // ex: "7 4 1 7 ,8 0 "

    // Position du match dans la ligne complète
    size_t matchOffsetInTail = static_cast<size_t>(m.position(0));
    size_t headEnd = tailStart + matchOffsetInTail;

    // 2) Partie gauche : index + ref + désignation
    std::string head = line.substr(0, headEnd);
    head = trim(head);

    // On saute les espaces + l'index numérique au début
    size_t p = 0;
    while (p < head.size() && std::isspace(static_cast<unsigned char>(head[p]))) ++p;
    while (p < head.size() && std::isdigit(static_cast<unsigned char>(head[p]))) ++p;
    while (p < head.size() && std::isspace(static_cast<unsigned char>(head[p]))) ++p;

    // Début de la référence
    size_t refStart = p;
    size_t refEnd = head.size();

    // On cherche la première occurrence de "double espace" pour séparer Réf / Désignation
    for (size_t i = refStart; i + 1 < head.size(); ++i) {
        if (head[i] == ' ' && head[i + 1] == ' ') {
            refEnd = i;
            break;
        }
    }

    std::string refRaw = trim(head.substr(refStart, refEnd - refStart));
    std::string desc = (refEnd < head.size()) ? trim(head.substr(refEnd)) : "";

    // On "recolle" la référence (supprimer les espaces) : "R SA U 5 0" -> "RSAU50"
    std::string ref = cleanReference(refRaw);

    // 3) Conversion des nombres
    double qte = 0.0;
    double prix = 0.0;
    double total = 0.0;

    try {
        qte = std::stod(trim(qteStr));          // int simple (390)
        prix = normalizeNumber(prixStr);        // "1 9 ,0 2" -> 19.02
        total = normalizeNumber(totalStr);      // "7 4 1 7 ,8 0" -> 7417.80
    }
    catch (const std::exception& e) {
        std::string errorMsg = "[CGR] ERREUR conversion nombre: " + std::string(e.what()) +
            " | qte='" + qteStr + "' prix='" + prixStr + "' total='" + totalStr + "'\n";
        OutputDebugStringA(errorMsg.c_str());
        return false;
    }

    // 4) Remplissage de la struct
    out.reference = ref;
    out.designation = desc;
    out.quantite = qte;
    out.prixHT = prix;

    return true;
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

    OutputDebugStringA("[CGR] Parsing ligne par ligne avec approche simplifiee (sans gros regex)...\n");

    // Séparer le texte en lignes
    std::istringstream stream(text);
    std::string line;
    std::vector<std::string> textLines;

    while (std::getline(stream, line))
    {
        textLines.push_back(line);
    }

    std::string lineCountMsg = "[CGR] Nombre de lignes: " + std::to_string(textLines.size()) + "\n";
    OutputDebugStringA(lineCountMsg.c_str());

    int extractedCount = 0;

    // Parser chaque ligne avec la fonction simplifiée
    for (size_t i = 0; i < textLines.size(); ++i)
    {
        PdfLine product;

        if (parseLigneCGR(textLines[i], product))
        {
            lines.push_back(product);
            extractedCount++;

            std::string debugMsg = "[CGR] Ligne article #" + std::to_string(extractedCount) +
                ": Ref=" + product.reference +
                " | Desc=" + product.designation +
                " | Qte=" + std::to_string(product.quantite) +
                " | Prix=" + std::to_string(product.prixHT) + "\n";
            OutputDebugStringA(debugMsg.c_str());
        }
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
