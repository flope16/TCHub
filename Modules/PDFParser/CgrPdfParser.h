#pragma once
#include "IPdfParser.h"

class CgrPdfParser : public IPdfParser
{
public:
    std::vector<PdfLine> parse(const std::string& filePath) override;
    std::string getSupplierName() const override { return "CGR"; }

private:
    // Méthodes privées pour le parsing spécifique CGR
    std::string extractText(const std::string& filePath);
    std::vector<PdfLine> parseTextContent(const std::string& text);
};
