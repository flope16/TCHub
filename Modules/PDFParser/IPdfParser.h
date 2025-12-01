#pragma once
#include <string>
#include <vector>

// Structure représentant une ligne de produit
struct PdfLine
{
    std::string reference;      // N° article
    std::string designation;    // Désignation du produit
    double quantite = 0.0;      // Quantité
    double prixHT = 0.0;        // Prix HT unitaire
    double montantHT = 0.0;     // Montant total HT (optionnel)
};

// Interface abstraite pour les parseurs PDF
class IPdfParser
{
public:
    virtual ~IPdfParser() = default;

    // Parse un fichier PDF et retourne les lignes extraites
    virtual std::vector<PdfLine> parse(const std::string& filePath) = 0;

    // Retourne le nom du fournisseur
    virtual std::string getSupplierName() const = 0;
};
