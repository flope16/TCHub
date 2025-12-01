# TC Hub

Centre de modules C++ pour diverses tâches d'automatisation.

## Modules disponibles

### PDF Parser

Module de parsing de fichiers PDF provenant de différents fournisseurs vers des fichiers Excel (.xlsx) structurés.

#### Fonctionnalités

- **Support multi-fournisseurs** : Architecture modulaire permettant d'ajouter facilement de nouveaux parseurs
- **Fournisseurs supportés** :
  - Lindab (conduits et accessoires de ventilation)
  - *D'autres fournisseurs peuvent être ajoutés facilement*

#### Format de sortie

Le fichier Excel généré contient les colonnes suivantes :
- **Référence** : Numéro d'article du produit
- **Désignation** : Description du produit
- **Quantité** : Quantité commandée
- **Prix HT** : Prix unitaire HT

#### Utilisation

1. Lancer TCHub.exe
2. Cliquer sur "Ouvrir" dans la section PDF Parser
3. Sélectionner le fournisseur dans la liste déroulante
4. Parcourir et sélectionner le fichier PDF à parser
5. Cliquer sur "Parser le PDF"
6. Le fichier .xml (compatible Excel) sera créé dans le même dossier que le PDF source

## Architecture technique

### Structure du projet

```
TCHub/
├── main.cpp                          # Point d'entrée de l'application
├── TCHub.h/cpp                       # Fenêtre principale
├── Modules/
│   └── PDFParser/
│       ├── IPdfParser.h              # Interface abstraite pour les parseurs
│       ├── ParserFactory.h/cpp       # Factory pour créer les parseurs
│       ├── LindabPdfParser.h/cpp     # Parseur spécifique Lindab
│       ├── XlsxWriter.h/cpp          # Générateur de fichiers XLSX
│       └── PDFParserWindow.h/cpp     # Interface utilisateur
└── Resources/                        # Ressources (images, icônes)
```

### Ajout d'un nouveau parseur

Pour ajouter un parseur pour un nouveau fournisseur :

1. **Créer la classe de parseur** héritant de `IPdfParser` :
   ```cpp
   class MonFournisseurParser : public IPdfParser
   {
   public:
       std::vector<PdfLine> parse(const std::string& filePath) override;
       std::string getSupplierName() const override { return "MonFournisseur"; }
   };
   ```

2. **Ajouter l'enum dans ParserFactory.h** :
   ```cpp
   enum class Supplier
   {
       Lindab,
       MonFournisseur  // Nouveau
   };
   ```

3. **Mettre à jour ParserFactory.cpp** :
   ```cpp
   case Supplier::MonFournisseur:
       return std::make_unique<MonFournisseurParser>();
   ```

4. **Implémenter la logique de parsing** spécifique au format du fournisseur

## Compilation

### Prérequis

- Visual Studio 2022
- Windows SDK 10.0
- C++17

### Build

1. Ouvrir `TCHub.sln` dans Visual Studio
2. Sélectionner la configuration (Debug/Release)
3. Build → Build Solution (Ctrl+Shift+B)

## Notes techniques

### Extraction de texte PDF

La version actuelle suppose que le texte est déjà extrait du PDF. Pour une intégration complète, il est recommandé d'intégrer une bibliothèque PDF comme :
- **Poppler** : Open source, très complet
- **MuPDF** : Léger et rapide
- **PDFium** : De Google, robuste

### Format XLSX

Le fichier généré utilise le format SpreadsheetML (XML), qui est compatible avec :
- Microsoft Excel 2003+
- LibreOffice Calc
- Google Sheets (après import)

Pour un format XLSX natif (.xlsx avec compression ZIP), considérer l'utilisation de :
- libxlsxwriter
- OpenXLSX
- xlnt

## Roadmap

- [ ] Intégration d'une bibliothèque PDF pour extraction automatique du texte
- [ ] Support du format XLSX natif (avec compression)
- [ ] Ajout de parseurs pour d'autres fournisseurs
- [ ] Module de gestion des stocks
- [ ] Module de génération de devis
- [ ] Export vers d'autres formats (CSV, JSON)

## Licence

Projet privé - Tous droits réservés
