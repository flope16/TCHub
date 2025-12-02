# Installation de Poppler pour TCHub

## 🎯 Pourquoi Poppler ?

Poppler permet d'extraire directement le texte des fichiers PDF sans avoir besoin de créer des fichiers .txt manuellement.

## 📦 Installation via vcpkg (Recommandé)

### Étape 1 : Installer Poppler

Ouvrez PowerShell **en tant qu'administrateur** et exécutez :

```powershell
cd C:\Dev\vcpkg
.\vcpkg install poppler:x64-windows
```

Ou utilisez le script fourni :
```powershell
.\install_poppler.ps1
```

Cela va télécharger et compiler Poppler (peut prendre 10-15 minutes).

### Étape 2 : Rebuild TCHub

1. Ouvrez TCHub dans Visual Studio
2. Build → Rebuild Solution (Ctrl+Shift+B)
3. Le projet va automatiquement détecter Poppler via `USE_POPPLER`

## ✅ Vérification

Une fois compilé avec Poppler, TCHub pourra :
- ✅ Lire directement les fichiers PDF sans conversion
- ✅ Extraire le texte automatiquement
- ✅ Parser les données Lindab en un seul clic

## 🔄 Fallback sans Poppler

Si Poppler n'est pas installé, TCHub utilisera automatiquement :
1. Fichier .txt avec le même nom (si existant)
2. Commande `pdftotext` (si installée)
3. Message d'erreur avec instructions

## 📋 Structure des fichiers

Après installation de Poppler via vcpkg :
```
C:\Dev\vcpkg\
├── installed\
│   └── x64-windows\
│       ├── include\
│       │   └── poppler\
│       │       └── cpp\            # Headers Poppler C++
│       ├── lib\
│       │   └── poppler-cpp.lib     # Bibliothèque statique
│       └── bin\
│           └── poppler-*.dll       # DLL runtime
```

## 🔧 Configuration du projet

Le fichier `TCHub.vcxproj` inclut déjà :
- Chemins d'inclusion : `C:\Dev\vcpkg\installed\x64-windows\include`
- Bibliothèques : `poppler-cpp.lib`
- Définition : `USE_POPPLER` (active le code Poppler)

## ⚠️ Problèmes courants

### "Poppler headers not found"
- Vérifiez que vcpkg est installé dans `C:\Dev\vcpkg`
- Si vcpkg est ailleurs, modifiez les chemins dans TCHub.vcxproj

### "poppler-cpp.lib not found"
- Vérifiez l'installation : `.\vcpkg list | findstr poppler`
- Réinstallez si nécessaire : `.\vcpkg remove poppler:x64-windows` puis réinstallez

### Temps de compilation long
- Premier build avec Poppler prend du temps (téléchargement + compilation)
- Les builds suivants sont rapides

## 🚀 Test

1. Build le projet
2. Lancez TCHub
3. Sélectionnez un PDF Lindab
4. Cliquez "Parser le PDF"
5. Le texte devrait être extrait automatiquement ! ✨
