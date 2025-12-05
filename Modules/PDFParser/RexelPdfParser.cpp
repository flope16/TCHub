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

    // Heuristique quantité :
    // 1) ligne composée uniquement de chiffres/espaces juste après la référence
    // 2) sinon, choisir le plus grand token numérique (>=2 chiffres) avant "P"
    for (size_t j = i; j < blockEnd; ++j) {
      std::string candidate = textLines[j];
      bool onlyDigitsAndSpaces =
          std::all_of(candidate.begin(), candidate.end(), [](unsigned char c) {
            return std::isspace(c) || (c >= '0' && c <= '9');
          });

      if (!onlyDigitsAndSpaces)
        continue;

      // retirer tous les espaces
      candidate.erase(
          std::remove_if(candidate.begin(), candidate.end(),
                         [](unsigned char c) { return std::isspace(c); }),
          candidate.end());
      if (candidate.empty())
        continue;

      if (j > i) {
        try {
          product.quantite = std::stod(candidate);
          OutputDebugStringA(
              ("[Rexel] Qte extraite (ligne numérique): " + candidate + "\n")
                  .c_str());
          break;
        } catch (...) {
        }
      }
    }

    if (product.quantite <= 0) {
      size_t pIndex = tokens.size();
      for (size_t t = refTokenIndex + 1; t < tokens.size(); ++t) {
        if (tokens[t] == "P") {
          pIndex = t;
          break;
        }
      }

      std::string bestNumeric;
      for (size_t t = refTokenIndex + 1; t < pIndex; ++t) {
        bool allDigits = std::all_of(
            tokens[t].begin(), tokens[t].end(), [](unsigned char c) {
              return std::isdigit(static_cast<unsigned char>(c));
            });
        if (!allDigits)
          continue;

        if (tokens[t].size() >= 2) {
          if (tokens[t].size() > bestNumeric.size())
            bestNumeric = tokens[t];
        } else if (bestNumeric.empty()) {
          bestNumeric = tokens[t];
        }
      }

      if (!bestNumeric.empty()) {
        try {
          product.quantite = std::stod(bestNumeric);
          OutputDebugStringA(
              ("[Rexel] Qte extraite (tokens): " + bestNumeric + "\n").c_str());
        } catch (...) {
          OutputDebugStringA("[Rexel] Erreur conversion qte (tokens)\n");
        }
      }

      // Si toujours rien, chercher un nombre au format "1 200" avant le P
      if (product.quantite <= 0 && pIndex > refTokenIndex + 1) {
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
            product.quantite = std::stod(compact);
            OutputDebugStringA(
                ("[Rexel] Qte extraite (spaced): " + compact + "\n").c_str());
          } catch (...) {
            OutputDebugStringA("[Rexel] Erreur conversion qte (spaced)\n");
          }
        }
      }

      if (product.quantite <= 0) {
        OutputDebugStringA("[Rexel] Quantité non trouvée\n");
      }
    }

    // Rechercher le prix APRÈS la quantité et le marqueur "P"
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
      for (size_t t =
               (pIndex == tokens.size() ? refTokenIndex + 1 : pIndex + 1);
           t < tokens.size(); ++t) {
        // Combiner les fragments (ex: "00" suivi de ",68000")
        if (tokens[t].find(',') != std::string::npos) {
          if (!priceCandidate.empty())
            priceCandidate += tokens[t];
          else if (t > 0 &&
                   std::all_of(tokens[t - 1].begin(), tokens[t - 1].end(),
                               [](unsigned char c) { return std::isdigit(c); }))
            priceCandidate = tokens[t - 1] + tokens[t];
          else
            priceCandidate = tokens[t];
          break;
        }
      }

      if (!priceCandidate.empty()) {
        product.prixHT = parseFrenchNumber(priceCandidate);
        product.prixHT =
            std::round(product.prixHT * 100.0) / 100.0; // forcer deux décimales
        std::string debugPrix = "[Rexel] Prix extrait: " + priceCandidate +
                                " => " + std::to_string(product.prixHT) + "\n";
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
