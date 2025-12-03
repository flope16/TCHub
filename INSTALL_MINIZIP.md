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
C:\Dev\vcpkg\installed\x64-windows\include\minizip\zip.h
C:\Dev\vcpkg\installed\x64-windows\include\minizip\unzip.h
C:\Dev\vcpkg\installed\x64-windows\lib\minizip.lib
C:\Dev\vcpkg\installed\x64-windows\bin\minizip.dll
```

Pour vérifier que minizip est bien installé :
```powershell
cd C:\Dev\vcpkg
.\vcpkg list | findstr minizip
```

Vous devriez voir quelque chose comme :
```
minizip:x64-windows          x.x.x    A library for manipulating zip files
```

## Configuration du projet

Le projet `TCHub.vcxproj` a été configuré pour :
- Ajouter `zlib.lib` aux dépendances (minizip fait partie de zlib)
- Ajouter `USE_MINIZIP` aux définitions du préprocesseur
- Copier automatiquement `minizip.dll` vers le dossier de sortie

⚠️ **IMPORTANT** : Après installation de minizip, vous devez faire un **Rebuild complet** du projet dans Visual Studio.

## Avantages

- ✅ Création ZIP native en C++
- ✅ Pas de dépendance à PowerShell
- ✅ Fonctionne avec tous les noms de fichiers
- ✅ Plus rapide
- ✅ Pas de fenêtres console
- ✅ Fichiers XLSX véritables garantis
