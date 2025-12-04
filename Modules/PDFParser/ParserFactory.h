#pragma once
#include "IPdfParser.h"
#include <memory>
#include <string>
#include <vector>

// Énumération des fournisseurs supportés
enum class Supplier
{
    Lindab,
    Fischer,
    Siehr,
    // Ajouter d'autres fournisseurs ici dans le futur
    // Atlantic,
    // Daikin,
    // etc.
};

// Factory pour créer les parseurs appropriés
class ParserFactory
{
public:
    // Créer un parseur pour un fournisseur spécifique
    static std::unique_ptr<IPdfParser> createParser(Supplier supplier);

    // Obtenir la liste des fournisseurs supportés
    static std::vector<std::string> getSupportedSuppliers();

    // Convertir un nom de fournisseur en enum
    static Supplier supplierFromString(const std::string& name);
};
