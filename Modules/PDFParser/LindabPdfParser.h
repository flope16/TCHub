#pragma once
#include <string>
#include <vector>

struct PdfLine
{
    std::string reference;
    std::string designation;
    double quantite = 0.0;
    double prixHT = 0.0;
    double montantHT = 0.0;
};

class LindabPdfParser
{
public:
    std::vector<PdfLine> parse(const std::string& filePath);
};
