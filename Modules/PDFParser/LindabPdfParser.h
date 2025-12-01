#pragma once
#include "IPdfParser.h"

class LindabPdfParser : public IPdfParser
{
public:
    std::vector<PdfLine> parse(const std::string& filePath) override;
    std::string getSupplierName() const override { return "Lindab"; }

private:
    // Méthodes privées pour le parsing spécifique Lindab
    std::string extractText(const std::string& filePath);
    std::vector<PdfLine> parseTextContent(const std::string& text);
};
