# TC Hub

Centre de modules C++ avec interface Qt moderne pour diverses tâches d'automatisation.

## Interface utilisateur

TC Hub utilise **Qt 6** pour offrir une interface moderne, élégante et intuitive :
- Design moderne avec couleurs professionnelles
- Interface responsive et agréable à utiliser
- Messages de statut en temps réel
- Barre de progression pour le suivi des opérations
- Emojis pour une meilleure lisibilité

## Modules disponibles

### PDF Parser

Module de parsing de fichiers PDF provenant de différents fournisseurs vers des fichiers Excel (.xlsx) structurés.

#### Fonctionnalités

- **Support multi-fournisseurs** : Architecture modulaire permettant d'ajouter facilement de nouveaux parseurs
- **Fournisseurs supportés** :
  - Lindab (conduits et accessoires de ventilation)
  - *D'autres fournisseurs peuvent être ajoutés facilement*
- **Interface moderne** : Interface Qt avec retour visuel en temps réel
- **Barre de progression** : Suivi visuel du processus de parsing

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
5. Cliquer sur "🚀 Parser le PDF"
6. Le fichier .xml (compatible Excel) sera créé dans le même dossier que le PDF source

## Architecture technique

### Technologies

- **C++17** : Langage de programmation
- **Qt 6.10.1** : Framework d'interface graphique moderne
- **Visual Studio 2022** : Environnement de développement
- **Architecture Pattern** : Strategy Pattern pour les parseurs

### Structure du projet

```
TCHub/
├── main.cpp                          # Point d'entrée Qt
├── MainWindow.h/cpp                  # Fenêtre principale moderne
├── Modules/
│   └── PDFParser/
│       ├── IPdfParser.h              # Interface abstraite pour les parseurs
│       ├── ParserFactory.h/cpp       # Factory pour créer les parseurs
│       ├── LindabPdfParser.h/cpp     # Parseur spécifique Lindab
│       ├── XlsxWriter.h/cpp          # Générateur de fichiers XLSX
│       └── PDFParserWindow.h/cpp     # Interface Qt du module
├── Resources/                        # Ressources (images, icônes, logos)
└── Resources.qrc                     # Fichier de ressources Qt
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
- Qt 6.10.1 (MSVC 2022 64-bit)
- Windows SDK 10.0
- C++17

### Installation de Qt

1. Télécharger Qt 6.10.1 depuis https://www.qt.io/download
2. Installer Qt avec les composants MSVC 2022 64-bit
3. Le chemin par défaut est `C:\Qt\6.10.1\msvc2022_64`

### Build

1. Ouvrir `TCHub.sln` dans Visual Studio
2. Vérifier que Qt est correctement configuré
3. Sélectionner la configuration (Debug/Release x64)
4. Build → Build Solution (Ctrl+Shift+B)

### Déploiement

Pour déployer l'application, utiliser l'outil Qt `windeployqt` :

```bash
cd C:\Dev\TCHub\x64\Release
C:\Qt\6.10.1\msvc2022_64\bin\windeployqt.exe TCHub.exe
```

Cela copiera automatiquement toutes les DLL Qt nécessaires.

## Caractéristiques de l'interface Qt

### Fenêtre principale
- Design épuré avec logo TC Hub
- Liste des modules avec descriptions
- Boutons stylisés avec effets hover
- Centrage automatique de la fenêtre

### Fenêtre PDF Parser
- **Groupe Configuration** : Sélection du fournisseur et du fichier
- **Bouton d'action** : Grand bouton vert avec emoji pour parser
- **Barre de progression** : Affichage visuel de l'avancement
- **Zone de résultat** : Console avec timestamps et emojis pour les messages
- **Feedback visuel** : Couleurs différentes pour succès/erreur/warning

### Style visuel
- Palette de couleurs professionnelle :
  - Bleu (#3498db) pour les actions principales
  - Vert (#27ae60) pour le bouton de parsing
  - Rouge (#e74c3c) pour les erreurs
  - Gris élégant (#2c3e50, #95a5a6) pour le texte
- Bordures arrondies
- Effets hover sur les boutons
- Police Segoe UI pour un look moderne

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

- [x] Interface Qt moderne et élégante
- [x] Architecture modulaire avec Pattern Strategy
- [x] Support du fournisseur Lindab
- [ ] Intégration d'une bibliothèque PDF pour extraction automatique
- [ ] Support du format XLSX natif (avec compression)
- [ ] Ajout de parseurs pour d'autres fournisseurs
- [ ] Mode sombre/clair
- [ ] Module de gestion des stocks
- [ ] Module de génération de devis
- [ ] Export vers d'autres formats (CSV, JSON)
- [ ] Prévisualisation des données avant export

## Licence

Projet privé - Tous droits réservés
