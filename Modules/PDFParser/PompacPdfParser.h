#pragma once
#include "IPdfParser.h"

class PompacPdfParser : public IPdfParser
{
public:
    std::vector<PdfLine> parse(const std::string& filePath) override;
    std::string getSupplierName() const override { return "Pompac"; }

private:
    // Méthodes privées pour le parsing spécifique Pompac
    std::string extractText(const std::string& filePath);
    std::vector<PdfLine> parseTextContent(const std::string& text);
};
