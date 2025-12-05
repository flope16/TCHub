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

  int extractedCount = 0;

  // Nouveau parsing : on traite bloc par bloc entre deux références "NDX"
  std::regex refPattern(R"(NDX\d+)");

  for (size_t i = 0; i < textLines.size(); ++i) {
    std::smatch refMatch;
    if (!std::regex_search(textLines[i], refMatch, refPattern))
      continue;

    PdfLine product;
    product.reference = trim(refMatch.str());

    // Déterminer la fin du bloc (prochaine référence ou fin du fichier)
    size_t blockEnd = i + 1;
    for (; blockEnd < textLines.size(); ++blockEnd) {
      if (std::regex_search(textLines[blockEnd], refMatch, refPattern))
        break;

      // Stopper avant de rentrer dans les CGV/sections textuelles
      std::string lower = textLines[blockEnd];
      std::transform(lower.begin(), lower.end(), lower.begin(),
                     [](unsigned char c) { return std::tolower(c); });
      if (lower.find("conditions generales") != std::string::npos ||
          lower.find("conditions générales") != std::string::npos ||
          lower.find("cgv") != std::string::npos) {
        break;
      }
    }

    // Les lignes d'un produit sont compactes ; couper à un maximum de 12 lignes
    size_t maxBlockEnd = std::min(textLines.size(), i + 12);
    blockEnd = std::min(blockEnd, maxBlockEnd);

    // Agréger les lignes du bloc pour faciliter le parsing
    std::string blockText;
    for (size_t j = i; j < blockEnd; ++j) {
      if (!blockText.empty())
        blockText += ' ';
      blockText += trim(textLines[j]);
    }

    std::string debugMsg = "[Rexel] ✓ MATCH REF à ligne " + std::to_string(i) +
                           ": \"" + product.reference +
                           "\" | Bloc=" + blockText.substr(0, 150) + "...\n";
    OutputDebugStringA(debugMsg.c_str());

    // Trouver la désignation : on prend la ligne avec lettres la plus longue
    // puis on nettoie les éventuels tokens de quantité/délai à droite
    size_t bestDescLine = SIZE_MAX;
    for (size_t j = i; j < blockEnd; ++j) {
      std::string candidate = trim(textLines[j]);
      bool hasAlpha =
          std::any_of(candidate.begin(), candidate.end(),
                      [](unsigned char c) { return std::isalpha(c); });
      if (hasAlpha && candidate.find("NDX") == std::string::npos) {
        if (product.designation.size() < candidate.size()) {
          product.designation = cleanDesignation(candidate);
          bestDescLine = j;
        }
      }
    }
    if (!product.designation.empty()) {
      std::string debugDesc = "[Rexel] Desc trouvée (ligne " +
                              std::to_string(bestDescLine) + "): \"" +
                              product.designation + "\"\n";
      OutputDebugStringA(debugDesc.c_str());
    }

    // Collapser le bloc (suppression des espaces) pour reconstruire
    // quantité/prix éclatés
    std::string collapsed;
    collapsed.reserve(blockText.size());
    for (char c : blockText) {
      if (!std::isspace(static_cast<unsigned char>(c)))
        collapsed.push_back(c);
    }

    // Chercher quantité et prix en utilisant les tokens du bloc (préserve
    // l'ordre et les ruptures de lignes)
    std::vector<std::string> tokens;
    {
      std::istringstream iss(blockText);
      std::string tok;
      while (iss >> tok) {
        tokens.push_back(tok);
      }
    }

    // Trouver l'index de la référence dans les tokens afin de ne lire que ce
    // qui suit
    size_t refTokenIndex = 0;
    for (; refTokenIndex < tokens.size(); ++refTokenIndex) {
      if (tokens[refTokenIndex].find(product.reference) != std::string::npos)
        break;
    }

    // Heuristique quantité : on calcule plusieurs candidats et on retient le
    // plus grand afin d'éviter qu'un fragment tronqué (ex: "200") ne masque un
    // millier éclaté (ex: "1" + "200").
    double bestQuantity = 0.0;
    auto registerQty = [&](double value, const std::string &label) {
      if (value > bestQuantity) {
        bestQuantity = value;
        OutputDebugStringA(("[Rexel] Qte candidate (" + label + "): " +
                            std::to_string(value) + "\n")
                               .c_str());
      }
    };

    // 1) ligne composée uniquement de chiffres/espaces juste après la référence
    for (size_t j = i; j < blockEnd; ++j) {
      std::string candidate = textLines[j];
      bool onlyDigitsAndSpaces =
          std::all_of(candidate.begin(), candidate.end(), [](unsigned char c) {
            return std::isspace(c) || (c >= '0' && c <= '9');
          });

      if (!onlyDigitsAndSpaces)
        continue;

      candidate.erase(
          std::remove_if(candidate.begin(), candidate.end(),
                         [](unsigned char c) { return std::isspace(c); }),
          candidate.end());
      if (candidate.empty())
        continue;

      if (j > i) {
        try {
          registerQty(std::stod(candidate), "ligne numérique");
        } catch (...) {
        }
      }
    }

    size_t pIndex = tokens.size();
    for (size_t t = refTokenIndex + 1; t < tokens.size(); ++t) {
      if (tokens[t] == "P") {
        pIndex = t;
        break;
      }
    }

    // 2) tokens numériques avant "P" (on prend la plus grande valeur)
    for (size_t t = refTokenIndex + 1; t < pIndex; ++t) {
      bool allDigits = std::all_of(tokens[t].begin(), tokens[t].end(),
                                   [](unsigned char c) {
                                     return std::isdigit(
                                         static_cast<unsigned char>(c));
                                   });
      if (!allDigits || tokens[t].size() < 2)
        continue;

      try {
        registerQty(std::stod(tokens[t]), "token");
      } catch (...) {
        OutputDebugStringA("[Rexel] Erreur conversion qte (tokens)\n");
      }
    }

    // 3) nombre au format "1 200" avant le P
    if (pIndex > refTokenIndex + 1) {
      std::string preP;
      for (size_t t = refTokenIndex + 1; t < pIndex; ++t) {
        if (!preP.empty())
          preP += ' ';
        preP += tokens[t];
      }

      std::regex spacedNumber(R"((\d{1,3}(?:\s\d{3})+))");
      std::smatch spacedMatch;
      std::string bestSpaced;
      std::string search = preP;
      while (std::regex_search(search, spacedMatch, spacedNumber)) {
        if (spacedMatch.str().size() > bestSpaced.size())
          bestSpaced = spacedMatch.str();
        search = spacedMatch.suffix().str();
      }

      if (!bestSpaced.empty()) {
        std::string compact = bestSpaced;
        compact.erase(std::remove_if(compact.begin(), compact.end(),
                                     [](unsigned char c) { return c == ' '; }),
                      compact.end());
        try {
          registerQty(std::stod(compact), "spaced");
        } catch (...) {
          OutputDebugStringA("[Rexel] Erreur conversion qte (spaced)\n");
        }
      }
    }

    // 4) reconstruction simple d'un millier "1" suivi d'un bloc à 3 chiffres
    // (ex: tokens "1" "200")
    for (size_t t = refTokenIndex + 1; t + 1 < tokens.size(); ++t) {
      if (tokens[t] == "1" && tokens[t + 1].size() == 3 &&
          std::all_of(tokens[t + 1].begin(), tokens[t + 1].end(),
                      [](unsigned char c) { return std::isdigit(c); })) {
        std::string merged = tokens[t] + tokens[t + 1];
        try {
          registerQty(std::stod(merged), "millier reconstruit");
        } catch (...) {
          OutputDebugStringA("[Rexel] Erreur conversion qte (millier)\n");
        }
      }
    }

    if (bestQuantity > 0.0) {
      product.quantite = bestQuantity;
    } else {
      OutputDebugStringA("[Rexel] Quantité non trouvée\n");
    }

    // Rechercher le prix (priorité aux valeurs avec virgule juste avant "P",
    // puis après)
    if (product.quantite > 0) {
      // trouver la position du token "P"
      size_t pIndex = tokens.size();
      for (size_t t = refTokenIndex + 1; t < tokens.size(); ++t) {
        if (tokens[t] == "P") {
          pIndex = t;
          break;
        }
      }

      std::string priceCandidate;
      double bestPrice = 0.0;

      auto pushCandidate = [&](const std::string &raw) -> bool {
        if (raw.empty())
          return false;

        double value = parseFrenchNumber(raw);

        // Cas particulier : certains fragments arrivent sous la forme
        // ",0818" (pas de partie entière, 4 chiffres). Dans ce cas la valeur
        // obtenue est 0.0818, on la re-projette sur deux décimales => 8.18.
        if (value > 0.0 && value < 1.0 && raw.size() == 5 && raw[0] == ',' &&
            std::all_of(raw.begin() + 1, raw.end(), [](unsigned char c) {
              return std::isdigit(c);
            })) {
          value *= 100.0;
        }

        if (value > 0.0) {
          product.prixHT = std::round(value * 100.0) / 100.0;
          priceCandidate = raw;
          bestPrice = product.prixHT;
          return true;
        }
        return false;
      };

      auto tryTokenAt = [&](size_t idx) -> bool {
        const std::string &tok = tokens[idx];
        if (tok.find(',') != std::string::npos) {
          if (pushCandidate(tok))
            return true;

          if (idx > 0) {
            const std::string &prev = tokens[idx - 1];
            bool prevNumeric =
                std::all_of(prev.begin(), prev.end(), [](unsigned char c) {
                  return std::isdigit(c);
                });
            if (prevNumeric) {
              if (pushCandidate(prev + tok))
                return true;
            }
          }
        }
        return false;
      };

      // 1) parcourir en arrière avant "P" (le prix unitaire se situe
      // généralement juste avant la colonne quantité)
      size_t backStart = (pIndex == tokens.size() ? tokens.size() : pIndex);
      while (backStart > refTokenIndex + 1) {
        --backStart;
        if (tryTokenAt(backStart))
          break;
      }

      // 2) si rien trouvé, continuer après "P"
      if (priceCandidate.empty() && pIndex < tokens.size()) {
        for (size_t t = pIndex + 1; t < tokens.size(); ++t) {
          if (tryTokenAt(t))
            break;
        }
      }

      if (!priceCandidate.empty()) {
        std::string debugPrix = "[Rexel] Prix extrait: " + priceCandidate +
                                " => " + std::to_string(bestPrice) + "\n";
        OutputDebugStringA(debugPrix.c_str());
      } else {
        OutputDebugStringA("[Rexel] Prix non trouvé\n");
      }
    }

    if (!product.reference.empty() && product.quantite > 0) {
      extractedCount++;
      lines.push_back(product);

      std::string productMsg = "[Rexel] ✓✓✓ Produit #" +
                               std::to_string(extractedCount) +
                               " AJOUTE: " + product.reference +
                               " | Qte=" + std::to_string(product.quantite) +
                               " | PU=" + std::to_string(product.prixHT) +
                               " | Desc=\"" + product.designation + "\"\n";
      OutputDebugStringA(productMsg.c_str());
    } else {
      std::string debugFail =
          "[Rexel] ✗ Produit NON ajouté (ref=\"" + product.reference +
          "\", qte=" + std::to_string(product.quantite) + ")\n";
      OutputDebugStringA(debugFail.c_str());
    }

    // Continuer après le bloc déjà traité
    i = (blockEnd == 0) ? i : blockEnd - 1;
  }

  // Log final
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
