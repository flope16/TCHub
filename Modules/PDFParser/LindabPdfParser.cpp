#include "LindabPdfParser.h"

// STUB TEMPORAIRE : ne fait encore aucun parsing.
// On branchera poppler plus tard, une fois l'appli Win32 stable.

std::vector<PdfLine> LindabPdfParser::parse(const std::string& filePath)
{
    (void)filePath; // éviter le warning "unused parameter"
    std::vector<PdfLine> result;

    // TODO : implémenter le parsing avec poppler ici plus tard.

    return result;
}
