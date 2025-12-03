# 🚀 Instructions de mise en route - TCHub

## ❌ Erreur actuelle : "minizip.dll est introuvable"

Cette erreur signifie que la bibliothèque minizip n'est pas installée ou que la DLL n'est pas copiée.

## ✅ Solution en 3 étapes

### Étape 1 : Installer minizip via vcpkg

Ouvrez un terminal PowerShell et exécutez :

```powershell
cd C:\Dev\vcpkg
.\vcpkg install minizip:x64-windows
```

**Vérification :**
```powershell
.\vcpkg list | findstr minizip
```

Vous devriez voir :
```
minizip:x64-windows          x.x.x    A library for manipulating zip files
```

### Étape 2 : Recompiler complètement le projet

⚠️ **IMPORTANT** : Vous devez faire un **Rebuild**, pas juste un Build !

1. Ouvrez Visual Studio 2022
2. Ouvrez `TCHub.sln`
3. Sélectionnez **Debug x64** ou **Release x64**
4. Menu : **Build → Rebuild Solution** (Ctrl+Alt+F7)

### Étape 3 : Vérifier que minizip.dll est copiée

Après le rebuild, vérifiez que le fichier existe :
```
x64\Debug\minizip.dll         (pour Debug)
x64\Release\minizip.dll       (pour Release)
```

Si le fichier n'est pas là, vérifiez que le chemin vcpkg est correct dans le projet :
```
C:\Dev\vcpkg\installed\x64-windows\bin\minizip.dll
```

## 🎯 Résultat attendu

Après ces étapes :
- ✅ L'application démarre sans erreur
- ✅ Les fichiers XLSX sont créés avec minizip (natif, rapide, fiable)
- ✅ Plus de dépendance à PowerShell pour la création de ZIP

## 📊 Vérification dans les logs

Quand vous convertissez un PDF Lindab, vous devriez voir :
```
[XlsxWriter] Tentative de création ZIP avec minizip...
[XlsxWriter] Fichier XLSX créé avec succès via minizip
```

Au lieu de :
```
[XlsxWriter] Tentative avec PowerShell...
[XlsxWriter] Résultat ZIP : 1
[XlsxWriter] ERREUR: Fichier ZIP non créé, utilisation du fallback SpreadsheetML
```

## ❓ Problèmes courants

### minizip n'est pas installé
```powershell
# Réinstallez minizip
cd C:\Dev\vcpkg
.\vcpkg remove minizip:x64-windows
.\vcpkg install minizip:x64-windows
```

### Le chemin vcpkg est différent
Si vcpkg est installé ailleurs que `C:\Dev\vcpkg`, modifiez les chemins dans `TCHub.vcxproj` :
- Ligne 107 : `AdditionalIncludeDirectories`
- Ligne 112 : `AdditionalLibraryDirectories`
- Lignes 117-141 : Commandes `xcopy` dans `PostBuildEvent`

### La DLL n'est toujours pas trouvée
Vérifiez que le fichier existe :
```powershell
dir C:\Dev\vcpkg\installed\x64-windows\bin\minizip.dll
```

Si le fichier n'existe pas, minizip n'est pas correctement installé.
