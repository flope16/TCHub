#ifndef NOMINMAX
#define NOMINMAX
#endif

#include "RexelPdfParser.h"
#include "PopplerPdfExtractor.h"
#include <algorithm>
#include <cctype>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <regex>
#include <sstream>
#include <windows.h>

// ===== FONCTIONS HELPERS =====

// Fonction helper pour normaliser un nombre français (virgule → point)
// Ex: "0,68000" → 0.68 ou "544,00" → 544.00
static double normalizeNumber(std::string str) {
  // Enlever les espaces
  str.erase(std::remove_if(str.begin(), str.end(),
                           [](unsigned char c) { return std::isspace(c); }),
            str.end());

  // Remplacer virgule par point
  std::replace(str.begin(), str.end(), ',', '.');

  try {
    return std::stod(str);
  } catch (...) {
    return 0.0;
  }
}

// Fonction helper pour trim les espaces
static std::string trim(const std::string &s) {
  auto start = s.find_first_not_of(" \t\r\n");
  auto end = s.find_last_not_of(" \t\r\n");
  if (start == std::string::npos)
    return std::string();
  return s.substr(start, end - start + 1);
}

// Helper pour nettoyer une désignation : on supprime les tokens situés après
// le dernier token contenant une lettre (afin d'éliminer les quantités/délais
// alignés à droite sur la même ligne)
static std::string cleanDesignation(const std::string &line) {
  std::istringstream iss(line);
  std::vector<std::string> tokens;
  std::string tok;
  while (iss >> tok) {
    tokens.push_back(tok);
  }

  if (tokens.empty())
    return trim(line);

  size_t lastAlphaIndex = SIZE_MAX;
  for (size_t i = 0; i < tokens.size(); ++i) {
    if (std::any_of(tokens[i].begin(), tokens[i].end(), [](unsigned char c) {
          return std::isalpha(c);
        })) {
      lastAlphaIndex = i;
    }
  }

  if (lastAlphaIndex == SIZE_MAX)
    return trim(line);

  std::string result;
  for (size_t i = 0; i <= lastAlphaIndex; ++i) {
    if (!result.empty())
      result += ' ';
    result += tokens[i];
  }
  return result;
}

// ===== METHODES DE LA CLASSE =====

std::string RexelPdfParser::extractText(const std::string &filePath) {
  // Utiliser PopplerPdfExtractor AVEC -layout pour préserver la structure en
  // colonnes
  std::string text = PopplerPdfExtractor::extractTextFromPdf(filePath, true);

  // Debug: sauvegarder le texte extrait
  try {
    std::filesystem::path pdfPathObj(filePath);
    std::filesystem::path debugPath =
        pdfPathObj.parent_path() /
        (pdfPathObj.stem().string() + "_rexel_extracted.txt");
    std::ofstream debugFile(debugPath);
    if (debugFile.is_open()) {
      debugFile << text;
      debugFile.close();
      std::string debugMsg =
          "[Rexel] Texte extrait sauvegardé dans: " + debugPath.string() + "\n";
      OutputDebugStringA(debugMsg.c_str());
    }
  } catch (...) {
  }

  // Debug: afficher dans la console
  OutputDebugStringA("=== DEBUG EXTRACTION PDF REXEL ===\n");
  std::string debug =
      "Poppler disponible: " +
      std::string(PopplerPdfExtractor::isPopplerAvailable() ? "OUI" : "NON") +
      "\n";
  debug +=
      "Texte extrait (" + std::to_string(text.length()) + " caractères):\n";
  size_t previewLength = text.length() < 1500 ? text.length() : 1500;
  debug += text.substr(0, previewLength) + "\n";
  debug += "=== FIN DEBUG REXEL ===\n";
  OutputDebugStringA(debug.c_str());

  return text;
}

// Fonction helper pour parser un nombre français avec virgule
static double parseFrenchNumber(const std::string &s) {
  std::string cleaned;
  cleaned.reserve(s.size());

  for (char c : s) {
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
  } catch (...) {
    return 0.0;
  }
}

std::vector<PdfLine> RexelPdfParser::parseTextContent(const std::string &text) {
  std::vector<PdfLine> lines;

  OutputDebugStringA("=== DEBUT PARSING REXEL (format avec -layout) ===\n");

  // Vérifier si c'est un message d'erreur
  if (text.find("ERREUR:") == 0) {
    OutputDebugStringA("[Rexel] Texte contient ERREUR, abandon\n");
    return lines;
  }

  if (text.empty()) {
    OutputDebugStringA("[Rexel] Texte vide, abandon\n");
    return lines;
  }

  // Séparer le texte en lignes
  std::istringstream stream(text);
  std::string line;
  std::vector<std::string> textLines;

  while (std::getline(stream, line)) {
    textLines.push_back(line);
  }

  std::string lineCountMsg =
      "[Rexel] Nombre de lignes: " + std::to_string(textLines.size()) + "\n";
  OutputDebugStringA(lineCountMsg.c_str());

  // LOG: afficher les 100 premières lignes pour debug
  OutputDebugStringA(
      "[Rexel] === AFFICHAGE DES 100 PREMIERES LIGNES (avec -layout) ===\n");
  for (size_t i = 0; i < textLines.size() && i < 100; ++i) {
    std::string logLine =
        "[Rexel] Ligne " + std::to_string(i) + ": \"" + textLines[i] + "\"\n";
    OutputDebugStringA(logLine.c_str());
  }
  OutputDebugStringA("[Rexel] === FIN AFFICHAGE ===\n");

  // Détection de la zone du tableau : on cherche la ligne contenant
  // "Référence / Désignation" puis on s'arrête à "NC = Nous consulter" ou
  // équivalent.
  auto toLower = [](std::string s) {
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    return s;
  };

  size_t startIdx = 0;
  for (; startIdx < textLines.size(); ++startIdx) {
    std::string lower = toLower(textLines[startIdx]);
    if (lower.find("référence / désignation") != std::string::npos ||
        lower.find("reference / designation") != std::string::npos ||
        lower.find("référence / designation") != std::string::npos) {
      ++startIdx;
      break;
    }
  }

  if (startIdx >= textLines.size()) {
    OutputDebugStringA("[Rexel] Zone de tableau non trouvée\n");
    return lines;
  }

  size_t endIdx = textLines.size();
  for (size_t i = startIdx; i < textLines.size(); ++i) {
    std::string lower = toLower(textLines[i]);
    if (lower.find("nc = nous consulter") != std::string::npos ||
        lower.find("nc = nous consuler") != std::string::npos ||
        lower.find("nc=") != std::string::npos) {
      endIdx = i;
      break;
    }
  }

  int extractedCount = 0;
  std::regex refPattern(R"(NDX\d+)");

  for (size_t i = startIdx; i < endIdx; ++i) {
    std::smatch refMatch;
    if (!std::regex_search(textLines[i], refMatch, refPattern))
      continue;

    PdfLine product;
    product.reference = trim(refMatch.str());

    // Déterminer la fin du bloc
    size_t blockEnd = i + 1;
    for (; blockEnd < endIdx; ++blockEnd) {
      if (std::regex_search(textLines[blockEnd], refMatch, refPattern))
        break;
    }

    // Agréger le bloc
    std::string blockText;
    for (size_t j = i; j < blockEnd; ++j) {
      std::string trimmed = trim(textLines[j]);
      if (!trimmed.empty()) {
        if (!blockText.empty())
          blockText += ' ';
        blockText += trimmed;
      }
    }

    std::string debugMsg = "[Rexel] Bloc ref ligne " + std::to_string(i) +
                           " => " + blockText.substr(0, 150) + "...\n";
    OutputDebugStringA(debugMsg.c_str());

    // Tokenisation du bloc
    std::vector<std::string> tokens;
    {
      std::istringstream iss(blockText);
      std::string tok;
      while (iss >> tok) {
        tokens.push_back(tok);
      }
    }

    // Index de la référence dans les tokens
    size_t refTokenIndex = 0;
    for (; refTokenIndex < tokens.size(); ++refTokenIndex) {
      if (tokens[refTokenIndex].find(product.reference) != std::string::npos)
        break;
    }

    // Désignation : ligne avec lettres la plus longue ne contenant pas la ref
    size_t bestDescLine = SIZE_MAX;
    for (size_t j = i; j < blockEnd; ++j) {
      std::string cand = trim(textLines[j]);
      if (cand.find(product.reference) != std::string::npos)
        continue;

      bool hasAlpha = std::any_of(cand.begin(), cand.end(), [](unsigned char c) {
        return std::isalpha(c);
      });
      if (!hasAlpha)
        continue;

      if (product.designation.size() < cand.size()) {
        product.designation = cleanDesignation(cand);
        bestDescLine = j;
      }
    }
    if (!product.designation.empty()) {
      std::string debugDesc = "[Rexel] Désignation (ligne " +
                              std::to_string(bestDescLine) + "): " +
                              product.designation + "\n";
      OutputDebugStringA(debugDesc.c_str());
    }

    // Recherche du marqueur "P"
    size_t pIndex = tokens.size();
    for (size_t t = refTokenIndex + 1; t < tokens.size(); ++t) {
      if (tokens[t] == "P") {
        pIndex = t;
        break;
      }
    }

    // Quantité : analyser les tokens entre la référence et "P"
    double bestQuantity = 0.0;
    auto tryQty = [&](const std::string &raw, const char *label) {
      if (raw.empty())
        return;
      try {
        double v = std::stod(raw);
        if (v > bestQuantity) {
          bestQuantity = v;
          OutputDebugStringA(("[Rexel] Qte candidate (" + std::string(label) +
                              "): " + std::to_string(v) + "\n")
                                 .c_str());
        }
      } catch (...) {
      }
    };

    // tokens numériques simples
    for (size_t t = refTokenIndex + 1; t < pIndex; ++t) {
      if (std::all_of(tokens[t].begin(), tokens[t].end(), [](unsigned char c) {
            return std::isdigit(c);
          })) {
        tryQty(tokens[t], "token");
      }
    }

    // combinaisons de deux tokens numériques consécutifs
    for (size_t t = refTokenIndex + 1; t + 1 < pIndex; ++t) {
      bool digits1 = std::all_of(tokens[t].begin(), tokens[t].end(),
                                 [](unsigned char c) { return std::isdigit(c); });
      bool digits2 = std::all_of(tokens[t + 1].begin(), tokens[t + 1].end(),
                                 [](unsigned char c) { return std::isdigit(c); });
      if (digits1 && digits2) {
        tryQty(tokens[t] + tokens[t + 1], "fusion2");
      }
    }

    // motif type "1 200" reconstruit à partir du texte avant P
    if (pIndex > refTokenIndex + 1) {
      std::string preP;
      for (size_t t = refTokenIndex + 1; t < pIndex; ++t) {
        if (!preP.empty())
          preP += ' ';
        preP += tokens[t];
      }
      std::regex spaced(R"((\d{1,3}(?:\s\d{3})+))");
      std::smatch m;
      if (std::regex_search(preP, m, spaced)) {
        std::string compact = m.str();
        compact.erase(std::remove(compact.begin(), compact.end(), ' '),
                      compact.end());
        tryQty(compact, "spaced");
      }
    }

    if (bestQuantity > 0.0)
      product.quantite = bestQuantity;

    // Prix unitaire : premier nombre avec virgule après P
    if (product.quantite > 0 && pIndex < tokens.size()) {
      auto parsePriceToken = [&](size_t idx) -> bool {
        const std::string &tok = tokens[idx];
        if (tok.find(',') == std::string::npos)
          return false;

        double val = parseFrenchNumber(tok);
        if (val <= 0.0) {
          // Cas ",0818" : rattacher le token précédent s'il est numérique
          if (idx > 0 && std::all_of(tokens[idx - 1].begin(),
                                     tokens[idx - 1].end(), [](unsigned char c) {
                                       return std::isdigit(c);
                                     })) {
            val = parseFrenchNumber(tokens[idx - 1] + tok);
          }
        }

        if (val > 0.0) {
          product.prixHT = std::round(val * 100.0) / 100.0;
          OutputDebugStringA(("[Rexel] Prix extrait: " + tok + " => " +
                              std::to_string(product.prixHT) + "\n")
                                 .c_str());
          return true;
        }
        return false;
      };

      for (size_t t = pIndex + 1; t < tokens.size(); ++t) {
        if (parsePriceToken(t))
          break;
      }
    }

    if (!product.reference.empty() && product.quantite > 0) {
      extractedCount++;
      lines.push_back(product);

      std::string productMsg = "[Rexel] Produit #" + std::to_string(extractedCount) +
                               " => Ref=" + product.reference +
                               " Qte=" + std::to_string(product.quantite) +
                               " PU=" + std::to_string(product.prixHT) +
                               " Desc=\"" + product.designation + "\"\n";
      OutputDebugStringA(productMsg.c_str());
    } else {
      OutputDebugStringA("[Rexel] Produit ignoré (réf/quantité manquante)\n");
    }

    i = blockEnd - 1;
  }

  std::string finalMsg = "=== RESUME PARSING REXEL ===\n";
  finalMsg += "Produits extraits : " + std::to_string(extractedCount) + "\n";
  finalMsg += "==============================\n";
  OutputDebugStringA(finalMsg.c_str());

  return lines;
}

std::vector<PdfLine> RexelPdfParser::parse(const std::string &filePath) {
  std::string text = extractText(filePath);
  return parseTextContent(text);
}
