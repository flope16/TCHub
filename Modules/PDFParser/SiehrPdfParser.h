#pragma once
#include "IPdfParser.h"

class SiehrPdfParser : public IPdfParser
{
public:
    std::vector<PdfLine> parse(const std::string& filePath) override;
    std::string getSupplierName() const override { return "Siehr"; }

private:
    // Méthodes privées pour le parsing spécifique Siehr
    std::string extractText(const std::string& filePath);
    std::vector<PdfLine> parseTextContent(const std::string& text);
};
