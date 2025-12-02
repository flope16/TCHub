#pragma once
#include "IPdfParser.h"
#include <string>
#include <vector>

// Classe pour écrire les données dans un vrai fichier XLSX (format Office Open XML)
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
    // Génère le contenu sheet1.xml (feuille de calcul principale)
    static std::string generateSheetXml(const std::vector<PdfLine>& lines);

    // Génère le contenu workbook.xml
    static std::string generateWorkbookXml();

    // Génère le contenu styles.xml
    static std::string generateStylesXml();

    // Génère le contenu [Content_Types].xml
    static std::string generateContentTypesXml();

    // Génère le contenu _rels/.rels
    static std::string generateRelsXml();

    // Génère le contenu xl/_rels/workbook.xml.rels
    static std::string generateWorkbookRelsXml();

    // Génère le contenu sharedStrings.xml
    static std::string generateSharedStringsXml(const std::vector<PdfLine>& lines);

    // Génère un fichier SpreadsheetML (fallback si ZIP échoue)
    static std::string generateSpreadsheetML(const std::vector<PdfLine>& lines);

    // Échappe les caractères spéciaux XML
    static std::string escapeXml(const std::string& str);
};
