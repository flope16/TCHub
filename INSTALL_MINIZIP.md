# Installation de minizip pour la génération XLSX

## Problème
PowerShell Compress-Archive échoue avec des noms de fichiers contenant des espaces ou caractères spéciaux.

## Solution
Utiliser **minizip** (bibliothèque C++) pour créer les fichiers ZIP nativement.

## Installation

### Via vcpkg (RECOMMANDÉ)

```powershell
cd C:\Dev\vcpkg
.\vcpkg install minizip:x64-windows
```

Cela installe :
- `minizip` : bibliothèque pour créer/extraire des ZIP
- Automatiquement lié à `zlib`

### Vérification

Après installation, vous devriez avoir :
```
C:\Dev\vcpkg\installed\x64-windows\include\minizip\
C:\Dev\vcpkg\installed\x64-windows\lib\minizip.lib
```

## Configuration du projet

Le projet `TCHub.vcxproj` sera mis à jour pour :
- Ajouter `minizip.lib` aux dépendances
- Inclure les headers minizip

## Avantages

- ✅ Création ZIP native en C++
- ✅ Pas de dépendance à PowerShell
- ✅ Fonctionne avec tous les noms de fichiers
- ✅ Plus rapide
- ✅ Pas de fenêtres console
- ✅ Fichiers XLSX véritables garantis
