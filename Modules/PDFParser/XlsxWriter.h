#pragma once
#include "IPdfParser.h"
#include <string>
#include <vector>

// Classe pour écrire les données dans un fichier XLSX
class XlsxWriter
{
public:
    // Écrit les données dans un fichier XLSX
    // Retourne true si succès, false si erreur
    static bool writeToXlsx(
        const std::string& outputPath,
        const std::vector<PdfLine>& lines
    );

private:
    // Génère le contenu XML pour le fichier XLSX
    static std::string generateXlsxContent(const std::vector<PdfLine>& lines);

    // Échappe les caractères spéciaux XML
    static std::string escapeXml(const std::string& str);
};
