#include "ParserFactory.h"
#include "LindabPdfParser.h"
#include "FischerPdfParser.h"
#include "SiehrPdfParser.h"
#include <algorithm>

std::unique_ptr<IPdfParser> ParserFactory::createParser(Supplier supplier)
{
    switch (supplier)
    {
    case Supplier::Lindab:
        return std::make_unique<LindabPdfParser>();

    case Supplier::Fischer:
        return std::make_unique<FischerPdfParser>();

    case Supplier::Siehr:
        return std::make_unique<SiehrPdfParser>();

    // Ajouter d'autres cas ici pour de nouveaux fournisseurs
    // case Supplier::Atlantic:
    //     return std::make_unique<AtlanticPdfParser>();

    default:
        return nullptr;
    }
}

std::vector<std::string> ParserFactory::getSupportedSuppliers()
{
    return {
        "Lindab",
        "Fischer",
        "Siehr"
        // Ajouter d'autres fournisseurs ici
    };
}

Supplier ParserFactory::supplierFromString(const std::string& name)
{
    std::string lowerName = name;
    std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::tolower);

    if (lowerName == "lindab")
        return Supplier::Lindab;

    if (lowerName == "fischer")
        return Supplier::Fischer;

    if (lowerName == "siehr")
        return Supplier::Siehr;

    // Par défaut, retourner Lindab
    return Supplier::Lindab;
}
