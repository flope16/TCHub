#pragma once
#include <string>

// Classe pour extraire le texte d'un PDF avec Poppler
class PopplerPdfExtractor
{
public:
    // Extrait le texte d'un PDF en utilisant Poppler
    // Retourne le texte extrait ou une chaîne vide en cas d'erreur
    static std::string extractTextFromPdf(const std::string& pdfPath);

    // Vérifie si Poppler est disponible
    static bool isPopplerAvailable();

private:
    // Charge et lit un PDF avec Poppler
    static std::string readWithPoppler(const std::string& pdfPath);
};
