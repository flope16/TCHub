# Excel Cracker

Module C++ pour casser/forcer des fichiers Excel protégés, porté depuis les scripts Python originaux.

## Fonctionnalités

### 1. Suppression de Protection des Feuilles Excel

Supprime la protection des feuilles dans un fichier Excel (.xlsx) **non crypté**.

**Comment ça marche:**
- Décompresse le fichier .xlsx (qui est un fichier ZIP)
- Parse les fichiers XML des feuilles de calcul
- Supprime les balises `<sheetProtection>` dans `xl/worksheets/sheet*.xml`
- Recompresse le tout dans un nouveau fichier

**Usage:**
```cpp
#include "ExcelProtectionRemover.h"

bool success = ExcelProtectionRemover::removeProtection("fichier.xlsx");
// Crée automatiquement "fichier_unprotected.xlsx"

// Ou spécifier le chemin de sortie:
bool success = ExcelProtectionRemover::removeProtection(
    "fichier.xlsx",
    "fichier_sans_protection.xlsx"
);

if (!success) {
    std::string error = ExcelProtectionRemover::getLastError();
    // Gérer l'erreur
}
```

### 2. Brute-Force de Mot de Passe Excel

Force le mot de passe d'un fichier Excel **crypté** en testant toutes les combinaisons possibles.

**⚠️ ATTENTION:**
- Le brute-force peut prendre **BEAUCOUP** de temps
- Pour un mot de passe de 4 caractères minuscules: ~456,976 tentatives
- Pour un mot de passe de 5 caractères minuscules: ~11,881,376 tentatives
- **Recommandé uniquement pour des mots de passe courts (1-4 caractères)**

**Comment ça marche:**
- Utilise COM Automation (Excel.Application) pour tester les mots de passe
- Nécessite **Microsoft Excel installé** sur le système
- Génère toutes les combinaisons possibles et teste chacune

**Usage:**
```cpp
#include "ExcelBruteForce.h"

ExcelBruteForce::Config config;
config.minLength = 1;
config.maxLength = 4;
config.charset = "abcdefghijklmnopqrstuvwxyz"; // Lettres minuscules

std::string password = ExcelBruteForce::bruteForce(
    "fichier_crypte.xlsx",
    config,
    [](int attempts, const std::string& current) {
        // Callback pour rapporter les progrès
        std::cout << "Tentatives: " << attempts
                  << ", Mot de passe testé: " << current << std::endl;
    }
);

if (!password.empty()) {
    std::cout << "Mot de passe trouvé: " << password << std::endl;
} else {
    std::string error = ExcelBruteForce::getLastError();
    std::cout << "Échec: " << error << std::endl;
}
```

**Jeux de caractères disponibles:**
```cpp
config.charset = "abcdefghijklmnopqrstuvwxyz";              // Minuscules
config.charset = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";              // Majuscules
config.charset = "0123456789";                               // Chiffres
config.charset = "abcdefghijklmnopqrstuvwxyz0123456789";     // Alphanumérique
config.charset = "abcABC123!@#$%";                           // Personnalisé
```

## Prérequis

### Pour la Suppression de Protection:
- ✅ Compilateur C++17 ou supérieur
- ✅ Bibliothèque **minizip** (pour ZIP)
- ✅ Définir `USE_MINIZIP` lors de la compilation

### Pour le Brute-Force:
- ✅ Tout ce qui est requis pour la suppression de protection
- ✅ **Microsoft Excel installé** (utilise COM Automation)
- ✅ Windows (utilise l'API COM)

## Compilation

### Avec Visual Studio:

1. Ajouter les fichiers au projet:
   - `ExcelProtectionRemover.h` / `.cpp`
   - `ExcelBruteForce.h` / `.cpp`
   - `ExcelCrackerWindow.h` / `.cpp`

2. Définir les macros de préprocesseur:
   - `USE_MINIZIP`

3. Ajouter les dépendances:
   - `minizip` (voir [INSTALL_MINIZIP.md](../../INSTALL_MINIZIP.md))
   - Qt5 ou Qt6 (pour l'interface graphique)

4. Compiler en mode Release pour de meilleures performances

### Dépendances minizip:

Voir le fichier [INSTALL_MINIZIP.md](../../INSTALL_MINIZIP.md) à la racine du projet pour les instructions d'installation de minizip.

## Interface Graphique Qt

Le module inclut une fenêtre Qt (`ExcelCrackerWindow`) avec:

### Mode 1: Suppression de Protection
1. Sélectionner un fichier Excel
2. Cliquer sur "Traiter"
3. Le fichier `*_unprotected.xlsx` est créé automatiquement

### Mode 2: Brute-Force
1. Sélectionner un fichier Excel crypté
2. Configurer:
   - Longueur minimale/maximale du mot de passe
   - Jeu de caractères (a-z, A-Z, 0-9, caractères spéciaux)
3. Cliquer sur "Traiter"
4. Attendre (peut être très long!)
5. Le mot de passe s'affiche si trouvé

## Différences avec les Scripts Python

### Scripts Python originaux:
- `remove_excel_protection.py`: Utilisait `lxml` pour parser le XML
- `excel_bruteforce.py`: Utilisait `msoffcrypto-tool` pour le déchiffrement

### Version C++:
- **Suppression de protection**:
  - ✅ Fonctionnalité équivalente
  - ✅ Plus rapide (natif C++)
  - ✅ Pas de dépendance Python

- **Brute-force**:
  - ⚠️ Utilise COM Automation au lieu de msoffcrypto-tool
  - ⚠️ Nécessite Excel installé
  - ⚠️ Plus lent que msoffcrypto-tool pour certains cas
  - ✅ Pas de dépendance Python

## Exemples d'Utilisation

### Supprimer la protection d'un fichier:

```bash
# Le fichier original reste intact, un backup est créé
Input:  fichier.xlsx
Output: fichier_unprotected.xlsx
Backup: fichier.xlsx.backup
```

### Forcer un mot de passe:

```bash
# Configuration recommandée pour débuter:
Longueur min: 1
Longueur max: 4
Caractères: a-z (26 caractères)

# Nombre total de combinaisons:
# Longueur 1: 26
# Longueur 2: 676
# Longueur 3: 17,576
# Longueur 4: 456,976
# TOTAL: 475,254 tentatives
```

## Limitations

### Suppression de Protection:
- ❌ Ne fonctionne **PAS** sur les fichiers Excel cryptés
- ❌ Ne supprime que la protection des feuilles, pas du classeur
- ✅ Fonctionne sur tous les fichiers .xlsx non cryptés

### Brute-Force:
- ❌ Nécessite Excel installé (COM Automation)
- ❌ Très lent pour des mots de passe > 4 caractères
- ❌ Seulement Windows (API COM)
- ⚠️ Peut déclencher des alertes antivirus (comportement suspect)

## Sécurité et Avertissements

⚠️ **IMPORTANT:**
- Ce module est destiné à un **usage légal uniquement**
- Utilisez-le uniquement sur vos propres fichiers ou avec autorisation
- Le brute-force peut être détecté comme un comportement malveillant
- Certains antivirus peuvent bloquer l'exécution

## Support

Pour des questions ou problèmes:
1. Vérifier que minizip est bien installé et `USE_MINIZIP` est défini
2. Vérifier que Excel est installé (pour le brute-force)
3. Consulter les logs de débogage (OutputDebugString sur Windows)

## Licence

Ce module fait partie du projet TCHub.
