#pragma once
#include "IPdfParser.h"

class FischerPdfParser : public IPdfParser
{
public:
    std::vector<PdfLine> parse(const std::string& filePath) override;
    std::string getSupplierName() const override { return "Fischer"; }

private:
    // Méthodes privées pour le parsing spécifique Fischer
    std::string extractText(const std::string& filePath);
    std::vector<PdfLine> parseTextContent(const std::string& text);
};
